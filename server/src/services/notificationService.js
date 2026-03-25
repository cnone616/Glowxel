const db = require('../config/db');

async function createNotification(payload) {
  const {
    userId,
    actorId,
    type,
    artworkId = null,
    commentId = null,
    ignoreDuplicate = false,
  } = payload;

  if (!userId || !actorId || !type || userId === actorId) {
    return { success: false };
  }

  const verb = ignoreDuplicate ? 'INSERT IGNORE' : 'INSERT';
  await db.query(
    `${verb} INTO notifications (user_id, actor_id, type, artwork_id, comment_id) VALUES (?,?,?,?,?)`,
    [userId, actorId, type, artworkId, commentId]
  );
  return { success: true };
}

async function listNotifications(userId, page = 1, limit = 20) {
  const offset = (page - 1) * limit;
  const [list] = await db.query(
    `SELECT n.id, n.type, n.is_read, n.artwork_id, n.comment_id, n.created_at,
            u.id as actor_id, u.name as actor_name, u.avatar as actor_avatar,
            a.title as artwork_title, a.cover_url as artwork_cover
     FROM notifications n
     JOIN users u ON n.actor_id = u.id
     LEFT JOIN artworks a ON n.artwork_id = a.id
     WHERE n.user_id = ?
     ORDER BY n.created_at DESC LIMIT ? OFFSET ?`,
    [userId, limit, offset]
  );
  const [[{ total }]] = await db.query(
    'SELECT COUNT(*) as total FROM notifications WHERE user_id = ?',
    [userId]
  );
  const [[{ unread }]] = await db.query(
    'SELECT COUNT(*) as unread FROM notifications WHERE user_id = ? AND is_read = 0',
    [userId]
  );
  return { list, total, unread };
}

async function markAllRead(userId) {
  await db.query('UPDATE notifications SET is_read = 1 WHERE user_id = ?', [userId]);
  return { success: true };
}

async function markRead(userId, notificationId) {
  await db.query('UPDATE notifications SET is_read = 1 WHERE id = ? AND user_id = ?', [notificationId, userId]);
  return { success: true };
}

async function getUnreadCount(userId) {
  const [[{ count }]] = await db.query(
    'SELECT COUNT(*) as count FROM notifications WHERE user_id = ? AND is_read = 0',
    [userId]
  );
  return { count };
}

module.exports = {
  createNotification,
  listNotifications,
  markAllRead,
  markRead,
  getUnreadCount,
};
