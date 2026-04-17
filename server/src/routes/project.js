const router = require('express').Router();
const { auth } = require('../middleware/auth');
const projectService = require('../services/projectService');

function hasOwnProperty(target, key) {
  return Object.prototype.hasOwnProperty.call(target, key);
}

function validateProject(project) {
  if (!project || typeof project !== 'object' || Array.isArray(project)) {
    return '项目数据无效';
  }

  const requiredStringFields = ['id', 'name', 'thumbnail', 'status'];
  for (const field of requiredStringFields) {
    if (!hasOwnProperty(project, field) || typeof project[field] !== 'string') {
      return `缺少项目字段 ${field}`;
    }
  }

  const requiredNumberFields = ['width', 'height', 'progress'];
  for (const field of requiredNumberFields) {
    if (!hasOwnProperty(project, field) || !Number.isFinite(project[field])) {
      return `缺少项目字段 ${field}`;
    }
  }

  if (!hasOwnProperty(project, 'palette') || !Array.isArray(project.palette)) {
    return '缺少项目字段 palette';
  }

  if (hasOwnProperty(project, 'paddedWidth') && !Number.isFinite(project.paddedWidth)) {
    return '项目字段 paddedWidth 无效';
  }

  if (hasOwnProperty(project, 'paddedHeight') && !Number.isFinite(project.paddedHeight)) {
    return '项目字段 paddedHeight 无效';
  }

  if (hasOwnProperty(project, 'description') && typeof project.description !== 'string') {
    return '项目字段 description 无效';
  }

  if (hasOwnProperty(project, 'tags') && !Array.isArray(project.tags)) {
    return '项目字段 tags 无效';
  }

  return null;
}

function validateProjectSyncPayload(project, payload, pixelsRequiredMessage) {
  const projectError = validateProject(project);
  if (projectError) {
    return projectError;
  }

  if (typeof payload === 'undefined') {
    return pixelsRequiredMessage;
  }

  return null;
}

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
  const { project, pixels } = req.body;
  const validationError = validateProjectSyncPayload(project, pixels, '缺少 pixels 字段');
  if (validationError) {
    return res.json({ code: 400, message: validationError });
  }

  try {
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
  const { projects } = req.body;
  if (!Array.isArray(projects)) {
    return res.json({ code: 400, message: 'projects 必须是数组' });
  }

  for (const item of projects) {
    if (!item || typeof item !== 'object' || Array.isArray(item)) {
      return res.json({ code: 400, message: '批量同步项目数据无效' });
    }

    if (!hasOwnProperty(item, 'project')) {
      return res.json({ code: 400, message: '缺少 project 字段' });
    }

    if (!hasOwnProperty(item, 'pixels')) {
      return res.json({ code: 400, message: '缺少 pixels 字段' });
    }

    const validationError = validateProjectSyncPayload(
      item.project,
      item.pixels,
      '缺少 pixels 字段'
    );
    if (validationError) {
      return res.json({ code: 400, message: validationError });
    }
  }

  try {
    const data = await projectService.batchSyncProjects(req.user.id, projects);
    res.json({ code: 0, data });
  } catch (err) {
    res.json({ code: 500, message: '批量同步失败' });
  }
});

module.exports = router;
