const db = require('../config/db');
const notificationService = require('./notificationService');

async function listComments(artworkId, page = 1, limit = 20) {
  const offset = (page - 1) * limit;
  const [list] = await db.query(
    `SELECT c.id, c.content, c.likes, c.reply_to, c.created_at,
            u.id as user_id, u.name as user_name, u.avatar as user_avatar
     FROM comments c
     JOIN users u ON c.user_id = u.id
     WHERE c.artwork_id = ?
     ORDER BY c.created_at DESC
     LIMIT ? OFFSET ?`,
    [artworkId, limit, offset]
  );
  const [[{ total }]] = await db.query(
    'SELECT COUNT(*) as total FROM comments WHERE artwork_id = ?',
    [artworkId]
  );
  return { list, total };
}

async function createComment(artworkId, userId, payload) {
  const { content, replyTo = null } = payload;
  if (!content) {
    return { invalidContent: true };
  }

  const [result] = await db.query(
    'INSERT INTO comments (artwork_id, user_id, content, reply_to) VALUES (?, ?, ?, ?)',
    [artworkId, userId, content, replyTo]
  );

  await db.query(
    'UPDATE artworks SET comments_count = comments_count + 1 WHERE id = ?',
    [artworkId]
  );

  const [[artwork]] = await db.query('SELECT user_id FROM artworks WHERE id = ?', [artworkId]);
  if (artwork) {
    await notificationService.createNotification({
      userId: artwork.user_id,
      actorId: userId,
      type: 'comment',
      artworkId,
      commentId: result.insertId,
    });
  }

  const [[comment]] = await db.query(
    `SELECT c.id, c.content, c.likes, c.reply_to, c.created_at,
            u.id as user_id, u.name as user_name, u.avatar as user_avatar
     FROM comments c
     JOIN users u ON c.user_id = u.id
     WHERE c.id = ?`,
    [result.insertId]
  );

  return { comment };
}

async function removeComment(commentId, userId) {
  const [rows] = await db.query(
    'SELECT artwork_id FROM comments WHERE id = ? AND user_id = ?',
    [commentId, userId]
  );
  if (rows.length === 0) {
    return { forbidden: true };
  }

  await db.query('DELETE FROM comments WHERE id = ?', [commentId]);
  await db.query(
    'UPDATE artworks SET comments_count = GREATEST(comments_count - 1, 0) WHERE id = ?',
    [rows[0].artwork_id]
  );

  return { success: true };
}

async function removeCommentById(commentId) {
  const [rows] = await db.query(
    'SELECT artwork_id FROM comments WHERE id = ?',
    [commentId]
  );
  if (rows.length === 0) {
    return { success: false, notFound: true };
  }

  await db.query('DELETE FROM comments WHERE id = ?', [commentId]);
  await db.query(
    'UPDATE artworks SET comments_count = GREATEST(comments_count - 1, 0) WHERE id = ?',
    [rows[0].artwork_id]
  );

  return { success: true, artworkId: rows[0].artwork_id };
}

async function likeComment(commentId) {
  await db.query('UPDATE comments SET likes = likes + 1 WHERE id = ?', [commentId]);
  return { success: true };
}

module.exports = {
  listComments,
  createComment,
  removeComment,
  removeCommentById,
  likeComment,
};
