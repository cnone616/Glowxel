const router = require('express').Router();
const db = require('../config/db');
const { auth } = require('../middleware/auth');

// 获取评论列表
router.get('/:artworkId', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT c.id, c.content, c.likes, c.reply_to, c.created_at,
       u.id as user_id, u.name as user_name, u.avatar as user_avatar
       FROM comments c JOIN users u ON c.user_id = u.id
       WHERE c.artwork_id = ? ORDER BY c.created_at DESC LIMIT ? OFFSET ?`,
      [req.params.artworkId, limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM comments WHERE artwork_id = ?', [req.params.artworkId]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 发表评论
router.post('/:artworkId', auth, async (req, res) => {
  try {
    const { content, replyTo } = req.body;
    if (!content) return res.json({ code: 400, message: '评论内容不能为空' });
    const [result] = await db.query(
      'INSERT INTO comments (artwork_id, user_id, content, reply_to) VALUES (?, ?, ?, ?)',
      [req.params.artworkId, req.user.id, content, replyTo || null]
    );
    await db.query('UPDATE artworks SET comments_count = comments_count + 1 WHERE id = ?', [req.params.artworkId]);
    const [rows] = await db.query(
      `SELECT c.id, c.content, c.likes, c.reply_to, c.created_at,
       u.id as user_id, u.name as user_name, u.avatar as user_avatar
       FROM comments c JOIN users u ON c.user_id = u.id WHERE c.id = ?`, [result.insertId]
    );
    res.json({ code: 0, data: { comment: rows[0] } });
  } catch (err) {
    res.json({ code: 500, message: '评论失败' });
  }
});

// 删除评论
router.delete('/:id', auth, async (req, res) => {
  try {
    const [rows] = await db.query('SELECT artwork_id FROM comments WHERE id = ? AND user_id = ?', [req.params.id, req.user.id]);
    if (rows.length === 0) return res.json({ code: 403, message: '无权删除' });
    await db.query('DELETE FROM comments WHERE id = ?', [req.params.id]);
    await db.query('UPDATE artworks SET comments_count = GREATEST(comments_count - 1, 0) WHERE id = ?', [rows[0].artwork_id]);
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '删除失败' });
  }
});

// 评论点赞
router.post('/:id/like', auth, async (req, res) => {
  try {
    await db.query('UPDATE comments SET likes = likes + 1 WHERE id = ?', [req.params.id]);
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

module.exports = router;
