const db = require('../config/db');

function normalizeProjectPayload(project = {}) {
  return {
    id: project.id,
    name: project.name || '未命名项目',
    width: project.width || 0,
    height: project.height || 0,
    paddedWidth: project.paddedWidth || project.width || 0,
    paddedHeight: project.paddedHeight || project.height || 0,
    palette: JSON.stringify(project.palette || []),
    thumbnail: project.thumbnail || '',
    progress: project.progress || 0,
    status: project.status || 'draft',
    description: project.description || '',
    tags: JSON.stringify(project.tags || []),
  };
}

async function listProjects(userId, page = 1, limit = 20) {
  const offset = (page - 1) * limit;
  const [list] = await db.query(
    `SELECT id, name, width, height, thumbnail_url, progress, status, updated_at
     FROM projects
     WHERE user_id = ? AND deleted_at IS NULL
     ORDER BY updated_at DESC
     LIMIT ? OFFSET ?`,
    [userId, limit, offset]
  );
  const [[{ total }]] = await db.query(
    'SELECT COUNT(*) as total FROM projects WHERE user_id = ? AND deleted_at IS NULL',
    [userId]
  );
  return { list, total };
}

async function syncProject(userId, project, pixels) {
  const normalized = normalizeProjectPayload(project);
  const pixelBuffer = pixels ? Buffer.from(JSON.stringify(pixels)) : null;

  await db.query(
    `INSERT INTO projects (
      id, user_id, name, width, height, padded_width, padded_height,
      palette, thumbnail_url, progress, status, description, tags, pixel_data
    )
     VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
     ON DUPLICATE KEY UPDATE
       name = VALUES(name),
       width = VALUES(width),
       height = VALUES(height),
       padded_width = VALUES(padded_width),
       padded_height = VALUES(padded_height),
       palette = VALUES(palette),
       thumbnail_url = VALUES(thumbnail_url),
       progress = VALUES(progress),
       status = VALUES(status),
       description = VALUES(description),
       tags = VALUES(tags),
       pixel_data = VALUES(pixel_data),
       deleted_at = NULL,
       updated_at = NOW()`,
    [
      normalized.id,
      userId,
      normalized.name,
      normalized.width,
      normalized.height,
      normalized.paddedWidth,
      normalized.paddedHeight,
      normalized.palette,
      normalized.thumbnail,
      normalized.progress,
      normalized.status,
      normalized.description,
      normalized.tags,
      pixelBuffer,
    ]
  );

  return { projectId: normalized.id };
}

async function getProjectDetail(userId, projectId) {
  const [rows] = await db.query(
    'SELECT * FROM projects WHERE id = ? AND user_id = ? AND deleted_at IS NULL',
    [projectId, userId]
  );
  if (rows.length === 0) return null;

  const project = rows[0];
  const pixels = project.pixel_data ? JSON.parse(project.pixel_data.toString()) : null;
  delete project.pixel_data;
  return { project, pixels };
}

async function removeProject(userId, projectId) {
  await db.query(
    'UPDATE projects SET deleted_at = NOW() WHERE id = ? AND user_id = ?',
    [projectId, userId]
  );
  return { success: true };
}

async function batchSyncProjects(userId, projects = []) {
  const results = [];
  for (const item of projects) {
    await syncProject(userId, item.project, item.pixels);
    results.push({ id: item.project.id, success: true });
  }
  return { results };
}

async function getSyncStatus(userId) {
  const [[{ count }]] = await db.query(
    'SELECT COUNT(*) as count FROM projects WHERE user_id = ? AND deleted_at IS NULL',
    [userId]
  );
  const [[{ lastSync }]] = await db.query(
    'SELECT MAX(updated_at) as lastSync FROM projects WHERE user_id = ?',
    [userId]
  );
  return { lastSync, count, dataSize: 0 };
}

async function updateProjectStatus(userId, projectId, status) {
  await db.query(
    'UPDATE projects SET status = ?, updated_at = NOW() WHERE id = ? AND user_id = ?',
    [status, projectId, userId]
  );
}

module.exports = {
  listProjects,
  syncProject,
  getProjectDetail,
  removeProject,
  batchSyncProjects,
  getSyncStatus,
  updateProjectStatus,
};
