const router = require('express').Router();
const db = require('../config/db');
const { auth } = require('../middleware/auth');

// 获取云端项目列表
router.get('/list', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      'SELECT id, name, width, height, thumbnail_url, progress, status, updated_at FROM projects WHERE user_id = ? AND deleted_at IS NULL ORDER BY updated_at DESC LIMIT ? OFFSET ?',
      [req.user.id, limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM projects WHERE user_id = ? AND deleted_at IS NULL', [req.user.id]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 上传/同步项目
router.post('/sync', auth, async (req, res) => {
  try {
    const { project, pixels } = req.body;
    const pixelBuffer = pixels ? Buffer.from(JSON.stringify(pixels)) : null;

    await db.query(
      `INSERT INTO projects (id, user_id, name, width, height, padded_width, padded_height, palette, thumbnail_url, progress, status, description, tags, pixel_data)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
       ON DUPLICATE KEY UPDATE name=VALUES(name), width=VALUES(width), height=VALUES(height),
       palette=VALUES(palette), thumbnail_url=VALUES(thumbnail_url), progress=VALUES(progress),
       description=VALUES(description), tags=VALUES(tags), pixel_data=VALUES(pixel_data), updated_at=NOW()`,
      [project.id, req.user.id, project.name, project.width, project.height,
       project.paddedWidth, project.paddedHeight, JSON.stringify(project.palette),
       project.thumbnail, project.progress || 0, project.status || 'draft',
       project.description, JSON.stringify(project.tags || []), pixelBuffer]
    );
    res.json({ code: 0, data: { projectId: project.id } });
  } catch (err) {
    console.error('同步失败:', err);
    res.json({ code: 500, message: '同步失败' });
  }
});

// 获取项目详情+像素数据
router.get('/:id', auth, async (req, res) => {
  try {
    const [rows] = await db.query('SELECT * FROM projects WHERE id = ? AND user_id = ? AND deleted_at IS NULL', [req.params.id, req.user.id]);
    if (rows.length === 0) return res.json({ code: 404, message: '项目不存在' });
    const project = rows[0];
    const pixels = project.pixel_data ? JSON.parse(project.pixel_data.toString()) : null;
    delete project.pixel_data;
    res.json({ code: 0, data: { project, pixels } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 删除云端项目
router.delete('/:id', auth, async (req, res) => {
  try {
    await db.query('UPDATE projects SET deleted_at = NOW() WHERE id = ? AND user_id = ?', [req.params.id, req.user.id]);
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '删除失败' });
  }
});

// 批量同步
router.post('/batch-sync', auth, async (req, res) => {
  try {
    const { projects } = req.body;
    const results = [];
    for (const item of projects) {
      const pixelBuffer = item.pixels ? Buffer.from(JSON.stringify(item.pixels)) : null;
      await db.query(
        `INSERT INTO projects (id, user_id, name, width, height, palette, thumbnail_url, progress, pixel_data)
         VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
         ON DUPLICATE KEY UPDATE name=VALUES(name), palette=VALUES(palette),
         thumbnail_url=VALUES(thumbnail_url), progress=VALUES(progress), pixel_data=VALUES(pixel_data), updated_at=NOW()`,
        [item.project.id, req.user.id, item.project.name, item.project.width, item.project.height,
         JSON.stringify(item.project.palette), item.project.thumbnail, item.project.progress || 0, pixelBuffer]
      );
      results.push({ id: item.project.id, success: true });
    }
    res.json({ code: 0, data: { results } });
  } catch (err) {
    res.json({ code: 500, message: '批量同步失败' });
  }
});

// 获取同步状态
router.get('/sync-status', auth, async (req, res) => {
  try {
    const [[{ count }]] = await db.query('SELECT COUNT(*) as count FROM projects WHERE user_id = ? AND deleted_at IS NULL', [req.user.id]);
    const [[{ lastSync }]] = await db.query('SELECT MAX(updated_at) as lastSync FROM projects WHERE user_id = ?', [req.user.id]);
    res.json({ code: 0, data: { lastSync, count, dataSize: 0 } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

module.exports = router;
