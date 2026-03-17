const router = require('express').Router();
const db = require('../config/db');
const { auth, optionalAuth } = require('../middleware/auth');

// 发布作品
router.post('/publish', auth, async (req, res) => {
  try {
    const { title, description, tags, pixelData, thumbnail, width, height, colorCount, boardCount, difficulty } = req.body;
    const pixelBuffer = pixelData ? Buffer.from(JSON.stringify(pixelData)) : null;
    const [result] = await db.query(
      `INSERT INTO artworks (user_id, title, description, cover_url, width, height, color_count, board_count, difficulty, pixel_data)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      [req.user.id, title, description, thumbnail, width, height, colorCount || 0, boardCount || 1, difficulty || '中等', pixelBuffer]
    );
    // 插入标签
    if (tags && tags.length > 0) {
      const tagValues = tags.map(t => [result.insertId, t]);
      await db.query('INSERT INTO artwork_tags (artwork_id, tag) VALUES ?', [tagValues]);
    }
    // 更新用户作品数
    await db.query('UPDATE users SET works_count = works_count + 1 WHERE id = ?', [req.user.id]);
    res.json({ code: 0, data: { artworkId: result.insertId } });
  } catch (err) {
    console.error('发布失败:', err);
    res.json({ code: 500, message: '发布失败' });
  }
});

// 作品列表
router.get('/list', optionalAuth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const tag = req.query.tag;
    let sql = `SELECT a.id, a.title, a.cover_url, a.likes, a.views, a.comments_count, a.collects, a.width, a.height, a.difficulty, a.created_at,
               u.id as author_id, u.name as author_name, u.avatar as author_avatar
               FROM artworks a JOIN users u ON a.user_id = u.id WHERE a.status = 'published'`;
    const params = [];
    if (tag) {
      sql += ' AND a.id IN (SELECT artwork_id FROM artwork_tags WHERE tag = ?)';
      params.push(tag);
    }
    sql += ' ORDER BY a.created_at DESC LIMIT ? OFFSET ?';
    params.push(limit, offset);
    const [list] = await db.query(sql, params);
    // total 也必须带上 tag 过滤条件，否则分页总数不准
    let countSql = 'SELECT COUNT(*) as total FROM artworks WHERE status = "published"';
    const countParams = [];
    if (tag) {
      countSql += ' AND id IN (SELECT artwork_id FROM artwork_tags WHERE tag = ?)';
      countParams.push(tag);
    }
    const [[{ total }]] = await db.query(countSql, countParams);
    res.json({ code: 0, data: { list, total } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 热门作品
router.get('/popular', optionalAuth, async (req, res) => {
  try {
    const limit = parseInt(req.query.limit) || 20;
    const [list] = await db.query(
      `SELECT a.id, a.title, a.cover_url, a.likes, a.views, a.created_at,
       u.id as author_id, u.name as author_name, u.avatar as author_avatar
       FROM artworks a JOIN users u ON a.user_id = u.id
       WHERE a.status = 'published' ORDER BY a.likes DESC LIMIT ?`, [limit]
    );
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 最新作品
router.get('/latest', optionalAuth, async (req, res) => {
  try {
    const limit = parseInt(req.query.limit) || 20;
    const [list] = await db.query(
      `SELECT a.id, a.title, a.cover_url, a.likes, a.views, a.created_at,
       u.id as author_id, u.name as author_name, u.avatar as author_avatar
       FROM artworks a JOIN users u ON a.user_id = u.id
       WHERE a.status = 'published' ORDER BY a.created_at DESC LIMIT ?`, [limit]
    );
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 搜索作品
router.get('/search', optionalAuth, async (req, res) => {
  try {
    const { keyword, tag } = req.query;
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    let sql = `SELECT a.id, a.title, a.cover_url, a.likes, a.views, a.created_at,
               u.id as author_id, u.name as author_name, u.avatar as author_avatar
               FROM artworks a JOIN users u ON a.user_id = u.id WHERE a.status = 'published'`;
    const params = [];
    if (keyword) { sql += ' AND a.title LIKE ?'; params.push(`%${keyword}%`); }
    if (tag) { sql += ' AND a.id IN (SELECT artwork_id FROM artwork_tags WHERE tag = ?)'; params.push(tag); }
    sql += ' ORDER BY a.created_at DESC LIMIT ? OFFSET ?';
    params.push(limit, offset);
    const [list] = await db.query(sql, params);
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '搜索失败' }); }
});

// 我发布的作品
router.get('/my', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      'SELECT id, title, cover_url, likes, views, status, created_at FROM artworks WHERE user_id = ? AND status != "deleted" ORDER BY created_at DESC LIMIT ? OFFSET ?',
      [req.user.id, limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM artworks WHERE user_id = ? AND status != "deleted"', [req.user.id]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 关注的人的作品
router.get('/following', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT a.id, a.title, a.cover_url, a.likes, a.views, a.created_at,
       u.id as author_id, u.name as author_name, u.avatar as author_avatar
       FROM artworks a JOIN users u ON a.user_id = u.id
       WHERE a.status = 'published' AND a.user_id IN (
         SELECT following_id FROM follows WHERE follower_id = ?
       ) ORDER BY a.created_at DESC LIMIT ? OFFSET ?`,
      [req.user.id, limit, offset]
    );
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 作品详情 (/:id 必须在所有静态路由之后)
router.get('/:id', optionalAuth, async (req, res) => {
  try {
    const [rows] = await db.query(
      `SELECT a.*, u.id as author_id, u.name as author_name, u.avatar as author_avatar, u.works_count as author_works_count
       FROM artworks a JOIN users u ON a.user_id = u.id WHERE a.id = ?`, [req.params.id]
    );
    if (rows.length === 0) return res.json({ code: 404, message: '作品不存在' });
    await db.query('UPDATE artworks SET views = views + 1 WHERE id = ?', [req.params.id]);
    const artwork = rows[0];
    delete artwork.pixel_data;
    if (req.user) {
      const [liked] = await db.query('SELECT 1 FROM likes WHERE user_id = ? AND artwork_id = ?', [req.user.id, req.params.id]);
      const [collected] = await db.query('SELECT 1 FROM collections WHERE user_id = ? AND artwork_id = ?', [req.user.id, req.params.id]);
      artwork.isLiked = liked.length > 0;
      artwork.isCollected = collected.length > 0;
    }
    res.json({ code: 0, data: { artwork } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 删除作品
router.delete('/:id', auth, async (req, res) => {
  try {
    await db.query('UPDATE artworks SET status = "deleted" WHERE id = ? AND user_id = ?', [req.params.id, req.user.id]);
    await db.query('UPDATE users SET works_count = GREATEST(works_count - 1, 0) WHERE id = ?', [req.user.id]);
    res.json({ code: 0, data: { success: true } });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

// 获取作品像素数据
router.get('/:id/pixels', auth, async (req, res) => {
  try {
    const [rows] = await db.query('SELECT pixel_data, width, height FROM artworks WHERE id = ?', [req.params.id]);
    if (rows.length === 0) return res.json({ code: 404, message: '作品不存在' });
    const pixels = rows[0].pixel_data ? JSON.parse(rows[0].pixel_data.toString()) : null;
    res.json({ code: 0, data: { pixels, width: rows[0].width, height: rows[0].height } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 相关作品
router.get('/:id/related', optionalAuth, async (req, res) => {
  try {
    const [rows] = await db.query('SELECT user_id FROM artworks WHERE id = ?', [req.params.id]);
    if (!rows.length) return res.json({ code: 0, data: { list: [] } });
    const [list] = await db.query(
      `SELECT a.id, a.title, a.cover_url, a.likes, a.views, a.created_at,
       u.id as author_id, u.name as author_name, u.avatar as author_avatar
       FROM artworks a JOIN users u ON a.user_id = u.id
       WHERE a.status = 'published' AND a.id != ? AND a.user_id = ? LIMIT 8`,
      [req.params.id, rows[0].user_id]
    );
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

module.exports = router;
