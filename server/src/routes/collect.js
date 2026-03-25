const router = require('express').Router();
const { auth } = require('../middleware/auth');
const socialService = require('../services/socialService');

// 收藏
router.post('/:artworkId', auth, async (req, res) => {
  try {
    const data = await socialService.collectArtwork(req.user.id, req.params.artworkId);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 取消收藏
router.delete('/:artworkId', auth, async (req, res) => {
  try {
    const data = await socialService.uncollectArtwork(req.user.id, req.params.artworkId);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 我的收藏列表
router.get('/list', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const data = await socialService.listCollections(req.user.id, page, limit);
    res.json({ code: 0, data });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 检查是否收藏
router.get('/:artworkId/check', auth, async (req, res) => {
  try {
    const data = await socialService.hasCollectedArtwork(req.user.id, req.params.artworkId);
    res.json({ code: 0, data });
  } catch (err) { res.json({ code: 500, message: '查询失败' }); }
});

module.exports = router;
