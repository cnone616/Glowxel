const router = require('express').Router();
const { auth, optionalAuth } = require('../middleware/auth');
const socialService = require('../services/socialService');

// 关注用户
router.post('/:userId', auth, async (req, res) => {
  try {
    const targetId = parseInt(req.params.userId);
    const data = await socialService.followUser(req.user.id, targetId);
    if (data.invalidSelf) return res.json({ code: 400, message: '不能关注自己' });
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 取消关注
router.delete('/:userId', auth, async (req, res) => {
  try {
    const targetId = parseInt(req.params.userId);
    await socialService.unfollowUser(req.user.id, targetId);
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 我的关注列表
router.get('/following', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const data = await socialService.listFollowers(req.user.id, page, limit, 'following', true, req.user.id);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 我的粉丝列表
router.get('/followers', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const data = await socialService.listFollowers(req.user.id, page, limit, 'followers', true, req.user.id);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 他人关注列表
router.get('/:userId/following', optionalAuth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const data = await socialService.listFollowers(req.params.userId, page, limit, 'following', false, req.user?.id || null);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 他人粉丝列表
router.get('/:userId/followers', optionalAuth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const data = await socialService.listFollowers(req.params.userId, page, limit, 'followers', false, req.user?.id || null);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 切换关注（toggle）
router.post('/:userId/toggle', auth, async (req, res) => {
  try {
    const targetId = parseInt(req.params.userId);
    const data = await socialService.toggleFollow(req.user.id, targetId);
    if (data.invalidSelf) return res.json({ code: 400, message: '不能关注自己' });
    res.json({ code: 0, data: { followed: data.followed } });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

module.exports = router;
