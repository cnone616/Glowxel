const db = require('../config/db');
const projectService = require('./projectService');

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

  const pixelBuffer = pixelData ? Buffer.from(JSON.stringify(pixelData)) : null;
  const [result] = await db.query(
    `INSERT INTO artworks (
      user_id, project_id, title, description, cover_url,
      width, height, color_count, board_count, difficulty, pixel_data
    )
     VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
    [
      userId,
      projectId || null,
      title,
      description || '',
      thumbnail || '',
      width || 0,
      height || 0,
      colorCount || 0,
      boardCount || 1,
      difficulty || '中等',
      pixelBuffer,
    ]
  );

  if (tags && tags.length > 0) {
    const tagValues = tags.map((tag) => [result.insertId, tag]);
    await db.query('INSERT INTO artwork_tags (artwork_id, tag) VALUES ?', [tagValues]);
  }

  if (projectId) {
    await projectService.updateProjectStatus(userId, projectId, 'published');
  }

  await db.query('UPDATE users SET works_count = works_count + 1 WHERE id = ?', [userId]);
  return { artworkId: result.insertId, projectId: projectId || null };
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
