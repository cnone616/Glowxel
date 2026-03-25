const router = require('express').Router();
const { contentFilter } = require('../middleware/contentFilter');
const { auth } = require('../middleware/auth');
const commentService = require('../services/commentService');

// 获取评论列表
router.get('/:artworkId', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const data = await commentService.listComments(req.params.artworkId, page, limit);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 发表评论
router.post('/:artworkId', auth, contentFilter(['content']), async (req, res) => {
  try {
    const data = await commentService.createComment(req.params.artworkId, req.user.id, req.body);
    if (data.invalidContent) return res.json({ code: 400, message: '评论内容不能为空' });
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '评论失败' });
  }
});

// 删除评论
router.delete('/:id', auth, async (req, res) => {
  try {
    const data = await commentService.removeComment(req.params.id, req.user.id);
    if (data.forbidden) return res.json({ code: 403, message: '无权删除' });
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '删除失败' });
  }
});

// 评论点赞
router.post('/:id/like', auth, async (req, res) => {
  try {
    const data = await commentService.likeComment(req.params.id);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

module.exports = router;
