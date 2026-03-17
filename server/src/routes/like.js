const router = require('express').Router();
const db = require('../config/db');
const { auth } = require('../middleware/auth');

// 点赞
router.post('/:artworkId', auth, async (req, res) => {
  try {
    await db.query('INSERT IGNORE INTO likes (user_id, artwork_id) VALUES (?, ?)', [req.user.id, req.params.artworkId]);
    await db.query('UPDATE artworks SET likes = likes + 1 WHERE id = ?', [req.params.artworkId]);
    const [[{ likes }]] = await db.query('SELECT likes FROM artworks WHERE id = ?', [req.params.artworkId]);
    // 写入通知（不通知自己）
    const [[artwork]] = await db.query('SELECT user_id FROM artworks WHERE id = ?', [req.params.artworkId]);
    if (artwork && artwork.user_id !== req.user.id) {
      await db.query(
        'INSERT IGNORE INTO notifications (user_id, actor_id, type, artwork_id) VALUES (?,?,?,?)',
        [artwork.user_id, req.user.id, 'like', req.params.artworkId]
      );
    }
    res.json({ code: 0, data: { success: true, likes } });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 取消点赞
router.delete('/:artworkId', auth, async (req, res) => {
  try {
    const [result] = await db.query('DELETE FROM likes WHERE user_id = ? AND artwork_id = ?', [req.user.id, req.params.artworkId]);
    if (result.affectedRows > 0) {
      await db.query('UPDATE artworks SET likes = GREATEST(likes - 1, 0) WHERE id = ?', [req.params.artworkId]);
    }
    const [[{ likes }]] = await db.query('SELECT likes FROM artworks WHERE id = ?', [req.params.artworkId]);
    res.json({ code: 0, data: { success: true, likes } });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

// 检查是否点赞
router.get('/:artworkId/check', auth, async (req, res) => {
  try {
    const [rows] = await db.query('SELECT 1 FROM likes WHERE user_id = ? AND artwork_id = ?', [req.user.id, req.params.artworkId]);
    res.json({ code: 0, data: rows.length > 0 });
  } catch (err) { res.json({ code: 500, message: '查询失败' }); }
});

module.exports = router;
