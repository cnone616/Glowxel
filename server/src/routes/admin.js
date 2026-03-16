const router = require('express').Router();
const db = require('../config/db');
const { adminAuth } = require('../middleware/auth');

// 所有 admin 路由都需要管理员权限
router.use(adminAuth);

// ─── 仪表盘统计 ───────────────────────────────────────────────
router.get('/stats', async (req, res) => {
  try {
    const [[{ userCount }]] = await db.query('SELECT COUNT(*) as userCount FROM users');
    const [[{ artworkCount }]] = await db.query('SELECT COUNT(*) as artworkCount FROM artworks');
    const [[{ templateCount }]] = await db.query('SELECT COUNT(*) as templateCount FROM templates');
    const [[{ commentCount }]] = await db.query('SELECT COUNT(*) as commentCount FROM comments');
    const [[{ challengeCount }]] = await db.query('SELECT COUNT(*) as challengeCount FROM challenges');
    const today = new Date().toISOString().slice(0, 10);
    const [[{ todayActive }]] = await db.query('SELECT COUNT(*) as todayActive FROM users WHERE DATE(created_at) = ?', [today]);
    res.json({ code: 0, data: { userCount, artworkCount, templateCount, commentCount, challengeCount, todayActive } });
  } catch (err) {
    console.error(err);
    res.json({ code: 500, message: '获取统计失败' });
  }
});

// ─── 用户管理 ─────────────────────────────────────────────────
router.get('/users', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 10;
    const offset = (page - 1) * limit;
    const keyword = req.query.keyword ? `%${req.query.keyword}%` : '%';
    const [list] = await db.query(
      'SELECT id, name, avatar, bio, level, status, role, works_count, followers_count, created_at FROM users WHERE name LIKE ? OR id LIKE ? ORDER BY created_at DESC LIMIT ? OFFSET ?',
      [keyword, keyword, limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM users WHERE name LIKE ?', [keyword]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

router.put('/users/:id/status', async (req, res) => {
  try {
    const { status } = req.body;
    if (!['active', 'banned'].includes(status)) return res.json({ code: 400, message: '状态值无效' });
    await db.query('UPDATE users SET status = ? WHERE id = ?', [status, req.params.id]);
    res.json({ code: 0, message: '操作成功' });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

router.put('/users/:id/role', async (req, res) => {
  try {
    const { role } = req.body;
    if (!['user', 'admin'].includes(role)) return res.json({ code: 400, message: '角色值无效' });
    await db.query('UPDATE users SET role = ? WHERE id = ?', [role, req.params.id]);
    res.json({ code: 0, message: '操作成功' });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

router.delete('/users/:id', async (req, res) => {
  try {
    await db.query('DELETE FROM users WHERE id = ?', [req.params.id]);
    res.json({ code: 0, message: '已删除' });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

// ─── 作品管理 ─────────────────────────────────────────────────
router.get('/artworks', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 10;
    const offset = (page - 1) * limit;
    const keyword = req.query.keyword ? `%${req.query.keyword}%` : '%';
    const status = req.query.status || null;
    const statusCond = status ? 'AND a.status = ?' : '';
    const params = status ? [keyword, keyword, status, limit, offset] : [keyword, keyword, limit, offset];
    const [list] = await db.query(
      `SELECT a.id, a.title, a.cover_url, a.status, a.likes, a.views, a.created_at, u.name as author
       FROM artworks a LEFT JOIN users u ON a.user_id = u.id
       WHERE (a.title LIKE ? OR u.name LIKE ?) ${statusCond}
       ORDER BY a.created_at DESC LIMIT ? OFFSET ?`, params
    );
    const [[{ total }]] = await db.query(`SELECT COUNT(*) as total FROM artworks a LEFT JOIN users u ON a.user_id = u.id WHERE (a.title LIKE ? OR u.name LIKE ?) ${statusCond}`,
      status ? [keyword, keyword, status] : [keyword, keyword]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

router.put('/artworks/:id/status', async (req, res) => {
  try {
    const { status } = req.body;
    if (!['published', 'hidden', 'pending'].includes(status)) return res.json({ code: 400, message: '状态值无效' });
    await db.query('UPDATE artworks SET status = ? WHERE id = ?', [status, req.params.id]);
    res.json({ code: 0, message: '操作成功' });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

router.delete('/artworks/:id', async (req, res) => {
  try {
    await db.query('DELETE FROM artworks WHERE id = ?', [req.params.id]);
    res.json({ code: 0, message: '已删除' });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

// ─── 评论管理 ─────────────────────────────────────────────────
router.get('/comments', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT c.id, c.content, c.created_at, u.name as author, a.title as artwork_title
       FROM comments c
       LEFT JOIN users u ON c.user_id = u.id
       LEFT JOIN artworks a ON c.artwork_id = a.id
       ORDER BY c.created_at DESC LIMIT ? OFFSET ?`, [limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM comments');
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

router.delete('/comments/:id', async (req, res) => {
  try {
    await db.query('DELETE FROM comments WHERE id = ?', [req.params.id]);
    res.json({ code: 0, message: '已删除' });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

// ─── 模板管理 ─────────────────────────────────────────────────
router.get('/templates', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 10;
    const offset = (page - 1) * limit;
    const [list] = await db.query('SELECT * FROM templates ORDER BY created_at DESC LIMIT ? OFFSET ?', [limit, offset]);
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM templates');
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

router.post('/templates', async (req, res) => {
  try {
    const { name, category, description, image_url, size, color_count, difficulty } = req.body;
    if (!name) return res.json({ code: 400, message: '名称不能为空' });
    const [result] = await db.query(
      'INSERT INTO templates (name, category, description, image_url, size, color_count, difficulty) VALUES (?,?,?,?,?,?,?)',
      [name, category || '', description || '', image_url || '', size || '', color_count || 0, difficulty || '简单']
    );
    res.json({ code: 0, data: { id: result.insertId } });
  } catch (err) { res.json({ code: 500, message: '创建失败' }); }
});

router.put('/templates/:id', async (req, res) => {
  try {
    const { name, category, description, image_url, size, color_count, difficulty } = req.body;
    await db.query(
      'UPDATE templates SET name=COALESCE(?,name), category=COALESCE(?,category), description=COALESCE(?,description), image_url=COALESCE(?,image_url), size=COALESCE(?,size), color_count=COALESCE(?,color_count), difficulty=COALESCE(?,difficulty) WHERE id=?',
      [name, category, description, image_url, size, color_count, difficulty, req.params.id]
    );
    res.json({ code: 0, message: '更新成功' });
  } catch (err) { res.json({ code: 500, message: '更新失败' }); }
});

router.delete('/templates/:id', async (req, res) => {
  try {
    await db.query('DELETE FROM templates WHERE id = ?', [req.params.id]);
    res.json({ code: 0, message: '已删除' });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

// ─── 挑战管理 ─────────────────────────────────────────────────
router.get('/challenges', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 10;
    const offset = (page - 1) * limit;
    const [list] = await db.query('SELECT * FROM challenges ORDER BY created_at DESC LIMIT ? OFFSET ?', [limit, offset]);
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM challenges');
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

router.post('/challenges', async (req, res) => {
  try {
    const { title, description, status, start_date, end_date, prize, difficulty, banner_url } = req.body;
    if (!title) return res.json({ code: 400, message: '标题不能为空' });
    const [result] = await db.query(
      'INSERT INTO challenges (title, description, status, start_date, end_date, prize, difficulty, banner_url) VALUES (?,?,?,?,?,?,?,?)',
      [title, description || '', status || 'upcoming', start_date || null, end_date || null, prize || '', difficulty || '简单', banner_url || '']
    );
    res.json({ code: 0, data: { id: result.insertId } });
  } catch (err) { res.json({ code: 500, message: '创建失败' }); }
});

router.put('/challenges/:id', async (req, res) => {
  try {
    const { title, description, status, start_date, end_date, prize, difficulty, banner_url } = req.body;
    await db.query(
      'UPDATE challenges SET title=COALESCE(?,title), description=COALESCE(?,description), status=COALESCE(?,status), start_date=COALESCE(?,start_date), end_date=COALESCE(?,end_date), prize=COALESCE(?,prize), difficulty=COALESCE(?,difficulty), banner_url=COALESCE(?,banner_url) WHERE id=?',
      [title, description, status, start_date, end_date, prize, difficulty, banner_url, req.params.id]
    );
    res.json({ code: 0, message: '更新成功' });
  } catch (err) { res.json({ code: 500, message: '更新失败' }); }
});

router.delete('/challenges/:id', async (req, res) => {
  try {
    await db.query('DELETE FROM challenges WHERE id = ?', [req.params.id]);
    res.json({ code: 0, message: '已删除' });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

module.exports = router;

