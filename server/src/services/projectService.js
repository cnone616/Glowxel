const db = require('../config/db');

function hasOwnProperty(target, key) {
  return Object.prototype.hasOwnProperty.call(target, key);
}

function buildProjectRecord(project) {
  return {
    id: project.id,
    name: project.name,
    width: project.width,
    height: project.height,
    paddedWidth: hasOwnProperty(project, 'paddedWidth') ? project.paddedWidth : null,
    paddedHeight: hasOwnProperty(project, 'paddedHeight') ? project.paddedHeight : null,
    palette: JSON.stringify(project.palette),
    thumbnail: project.thumbnail,
    progress: project.progress,
    status: project.status,
    description: hasOwnProperty(project, 'description') ? project.description : null,
    tags: hasOwnProperty(project, 'tags') ? JSON.stringify(project.tags) : null,
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
  const record = buildProjectRecord(project);
  const pixelBuffer = pixels === null ? null : Buffer.from(JSON.stringify(pixels));

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
      record.id,
      userId,
      record.name,
      record.width,
      record.height,
      record.paddedWidth,
      record.paddedHeight,
      record.palette,
      record.thumbnail,
      record.progress,
      record.status,
      record.description,
      record.tags,
      pixelBuffer,
    ]
  );

  return { projectId: record.id };
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
