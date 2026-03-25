const router = require('express').Router();
const { auth } = require('../middleware/auth');
const notificationService = require('../services/notificationService');

// 获取通知列表
router.get('/list', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const data = await notificationService.listNotifications(req.user.id, page, limit);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 标记全部已读
router.put('/read-all', auth, async (req, res) => {
  try {
    const data = await notificationService.markAllRead(req.user.id);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 标记单条已读
router.put('/:id/read', auth, async (req, res) => {
  try {
    const data = await notificationService.markRead(req.user.id, req.params.id);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 未读数量
router.get('/unread-count', auth, async (req, res) => {
  try {
    const data = await notificationService.getUnreadCount(req.user.id);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

module.exports = router;
