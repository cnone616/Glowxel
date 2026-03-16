const router = require('express').Router();
const db = require('../config/db');
const { auth } = require('../middleware/auth');

// 收藏
router.post('/:artworkId', auth, async (req, res) => {
  try {
    await db.query('INSERT IGNORE INTO collections (user_id, artwork_id) VALUES (?, ?)', [req.user.id, req.params.artworkId]);
    await db.query('UPDATE artworks SET collects = collects + 1 WHERE id = ?', [req.params.artworkId]);
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 取消收藏
router.delete('/:artworkId', auth, async (req, res) => {
  try {
    const [result] = await db.query('DELETE FROM collections WHERE user_id = ? AND artwork_id = ?', [req.user.id, req.params.artworkId]);
    if (result.affectedRows > 0) {
      await db.query('UPDATE artworks SET collects = GREATEST(collects - 1, 0) WHERE id = ?', [req.params.artworkId]);
    }
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 我的收藏列表
router.get('/list', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT a.id, a.title, a.cover_url, a.likes, a.views, a.created_at,
       u.id as author_id, u.name as author_name, u.avatar as author_avatar
       FROM collections c JOIN artworks a ON c.artwork_id = a.id JOIN users u ON a.user_id = u.id
       WHERE c.user_id = ? AND a.status = 'published' ORDER BY c.created_at DESC LIMIT ? OFFSET ?`,
      [req.user.id, limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM collections WHERE user_id = ?', [req.user.id]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 检查是否收藏
router.get('/:artworkId/check', auth, async (req, res) => {
  try {
    const [rows] = await db.query('SELECT 1 FROM collections WHERE user_id = ? AND artwork_id = ?', [req.user.id, req.params.artworkId]);
    res.json({ code: 0, data: rows.length > 0 });
  } catch (err) { res.json({ code: 500, message: '查询失败' }); }
});

module.exports = router;
