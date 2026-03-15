const router = require('express').Router();
const db = require('../config/db');
const { auth } = require('../middleware/auth');

// 点赞
router.post('/:artworkId', auth, async (req, res) => {
  try {
    await db.query('INSERT IGNORE INTO likes (user_id, artwork_id) VALUES (?, ?)', [req.user.id, req.params.artworkId]);
    await db.query('UPDATE artworks SET likes = likes + 1 WHERE id = ?', [req.params.artworkId]);
    const [[{ likes }]] = await db.query('SELECT likes FROM artworks WHERE id = ?', [req.params.artworkId]);
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
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

module.exports = router;
