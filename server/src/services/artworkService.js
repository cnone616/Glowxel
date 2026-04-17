const db = require('../config/db');
const projectService = require('./projectService');

function hasOwnProperty(target, key) {
  return Object.prototype.hasOwnProperty.call(target, key);
}

async function publishArtwork(userId, payload) {
  const {
    title,
    description,
    tags,
    pixelData,
    thumbnail,
    width,
    height,
    colorCount,
    boardCount,
    difficulty,
    projectId,
  } = payload;

  const pixelBuffer = Buffer.from(JSON.stringify(pixelData));
  const fields = {
    user_id: userId,
    title,
    cover_url: thumbnail,
    width,
    height,
    pixel_data: pixelBuffer,
  };

  if (hasOwnProperty(payload, 'projectId')) {
    fields.project_id = projectId;
  }

  if (hasOwnProperty(payload, 'description')) {
    fields.description = description;
  }

  if (hasOwnProperty(payload, 'colorCount')) {
    fields.color_count = colorCount;
  }

  if (hasOwnProperty(payload, 'boardCount')) {
    fields.board_count = boardCount;
  }

  if (hasOwnProperty(payload, 'difficulty')) {
    fields.difficulty = difficulty;
  }

  const columns = Object.keys(fields);
  const placeholders = columns.map(() => '?').join(', ');
  const values = columns.map((key) => fields[key]);
  const [result] = await db.query(
    `INSERT INTO artworks (${columns.join(', ')}) VALUES (${placeholders})`,
    values
  );

  if (hasOwnProperty(payload, 'tags') && tags.length > 0) {
    const tagValues = tags.map((tag) => [result.insertId, tag]);
    await db.query('INSERT INTO artwork_tags (artwork_id, tag) VALUES ?', [tagValues]);
  }

  if (hasOwnProperty(payload, 'projectId') && projectId !== null) {
    await projectService.updateProjectStatus(userId, projectId, 'published');
  }

  await db.query('UPDATE users SET works_count = works_count + 1 WHERE id = ?', [userId]);
  const resultProjectId = hasOwnProperty(payload, 'projectId') ? projectId : null;
  return { artworkId: result.insertId, projectId: resultProjectId };
}

async function removeArtwork(userId, artworkId) {
  const [rows] = await db.query(
    'SELECT id, project_id, status FROM artworks WHERE id = ? AND user_id = ?',
    [artworkId, userId]
  );

  if (!rows.length) {
    return { success: false, notFound: true };
  }

  const artwork = rows[0];
  if (artwork.status === 'deleted') {
    return { success: true };
  }

  await db.query('UPDATE artworks SET status = "deleted" WHERE id = ? AND user_id = ?', [artworkId, userId]);
  await db.query('UPDATE users SET works_count = GREATEST(works_count - 1, 0) WHERE id = ?', [userId]);

  if (artwork.project_id) {
    await projectService.updateProjectStatus(userId, artwork.project_id, 'draft');
  }

  return { success: true, projectId: artwork.project_id || null };
}

async function removeArtworkById(artworkId) {
  const [rows] = await db.query(
    'SELECT id, user_id, project_id, status FROM artworks WHERE id = ?',
    [artworkId]
  );

  if (!rows.length) {
    return { success: false, notFound: true };
  }

  const artwork = rows[0];
  if (artwork.status === 'deleted') {
    return { success: true, userId: artwork.user_id, projectId: artwork.project_id || null };
  }

  await db.query('UPDATE artworks SET status = "deleted" WHERE id = ?', [artworkId]);
  await db.query('UPDATE users SET works_count = GREATEST(works_count - 1, 0) WHERE id = ?', [artwork.user_id]);

  if (artwork.project_id) {
    await projectService.updateProjectStatus(artwork.user_id, artwork.project_id, 'draft');
  }

  return { success: true, userId: artwork.user_id, projectId: artwork.project_id || null };
}

module.exports = {
  publishArtwork,
  removeArtwork,
  removeArtworkById,
};
