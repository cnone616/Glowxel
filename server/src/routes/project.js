const router = require('express').Router();
const { auth } = require('../middleware/auth');
const projectService = require('../services/projectService');

// 获取云端项目列表
router.get('/list', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const data = await projectService.listProjects(req.user.id, page, limit);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 上传/同步项目
router.post('/sync', auth, async (req, res) => {
  try {
    const { project, pixels } = req.body;
    const data = await projectService.syncProject(req.user.id, project, pixels);
    res.json({ code: 0, data });
  } catch (err) {
    console.error('同步失败:', err);
    res.json({ code: 500, message: '同步失败' });
  }
});

// 获取同步状态
router.get('/sync-status', auth, async (req, res) => {
  try {
    const data = await projectService.getSyncStatus(req.user.id);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 获取项目详情+像素数据
router.get('/:id', auth, async (req, res) => {
  try {
    const data = await projectService.getProjectDetail(req.user.id, req.params.id);
    if (!data) return res.json({ code: 404, message: '项目不存在' });
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 删除云端项目
router.delete('/:id', auth, async (req, res) => {
  try {
    const data = await projectService.removeProject(req.user.id, req.params.id);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '删除失败' });
  }
});

// 批量同步
router.post('/batch-sync', auth, async (req, res) => {
  try {
    const { projects } = req.body;
    const data = await projectService.batchSyncProjects(req.user.id, projects);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '批量同步失败' });
  }
});

module.exports = router;
