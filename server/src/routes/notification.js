const router = require('express').Router();
const db = require('../config/db');
const { auth } = require('../middleware/auth');

// 获取通知列表
router.get('/list', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT n.id, n.type, n.is_read, n.artwork_id, n.comment_id, n.created_at,
              u.id as actor_id, u.name as actor_name, u.avatar as actor_avatar,
              a.title as artwork_title, a.cover_url as artwork_cover
       FROM notifications n
       JOIN users u ON n.actor_id = u.id
       LEFT JOIN artworks a ON n.artwork_id = a.id
       WHERE n.user_id = ?
       ORDER BY n.created_at DESC LIMIT ? OFFSET ?`,
      [req.user.id, limit, offset]
    );
    const [[{ total }]] = await db.query(
      'SELECT COUNT(*) as total FROM notifications WHERE user_id = ?', [req.user.id]
    );
    const [[{ unread }]] = await db.query(
      'SELECT COUNT(*) as unread FROM notifications WHERE user_id = ? AND is_read = 0', [req.user.id]
    );
    res.json({ code: 0, data: { list, total, unread } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 标记全部已读
router.put('/read-all', auth, async (req, res) => {
  try {
    await db.query('UPDATE notifications SET is_read = 1 WHERE user_id = ?', [req.user.id]);
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 标记单条已读
router.put('/:id/read', auth, async (req, res) => {
  try {
    await db.query('UPDATE notifications SET is_read = 1 WHERE id = ? AND user_id = ?', [req.params.id, req.user.id]);
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 未读数量
router.get('/unread-count', auth, async (req, res) => {
  try {
    const [[{ count }]] = await db.query(
      'SELECT COUNT(*) as count FROM notifications WHERE user_id = ? AND is_read = 0', [req.user.id]
    );
    res.json({ code: 0, data: { count } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

module.exports = router;

