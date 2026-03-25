const router = require('express').Router();
const { auth } = require('../middleware/auth');
const socialService = require('../services/socialService');

// 点赞
router.post('/:artworkId', auth, async (req, res) => {
  try {
    const data = await socialService.likeArtwork(req.user.id, req.params.artworkId);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 取消点赞
router.delete('/:artworkId', auth, async (req, res) => {
  try {
    const data = await socialService.unlikeArtwork(req.user.id, req.params.artworkId);
    res.json({ code: 0, data });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

// 检查是否点赞
router.get('/:artworkId/check', auth, async (req, res) => {
  try {
    const data = await socialService.hasLikedArtwork(req.user.id, req.params.artworkId);
    res.json({ code: 0, data });
  } catch (err) { res.json({ code: 500, message: '查询失败' }); }
});

module.exports = router;
