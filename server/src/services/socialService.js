const db = require('../config/db');
const notificationService = require('./notificationService');

async function followUser(followerId, followingId) {
  if (followerId === followingId) {
    return { invalidSelf: true };
  }

  const [result] = await db.query(
    'INSERT IGNORE INTO follows (follower_id, following_id) VALUES (?, ?)',
    [followerId, followingId]
  );

  if (result.affectedRows > 0) {
    await db.query('UPDATE users SET following_count = following_count + 1 WHERE id = ?', [followerId]);
    await db.query('UPDATE users SET followers_count = followers_count + 1 WHERE id = ?', [followingId]);
    await notificationService.createNotification({
      userId: followingId,
      actorId: followerId,
      type: 'follow',
    });
  }

  return { success: true, followed: true, changed: result.affectedRows > 0 };
}

async function unfollowUser(followerId, followingId) {
  const [result] = await db.query(
    'DELETE FROM follows WHERE follower_id = ? AND following_id = ?',
    [followerId, followingId]
  );

  if (result.affectedRows > 0) {
    await db.query('UPDATE users SET following_count = GREATEST(following_count - 1, 0) WHERE id = ?', [followerId]);
    await db.query('UPDATE users SET followers_count = GREATEST(followers_count - 1, 0) WHERE id = ?', [followingId]);
  }

  return { success: true, followed: false, changed: result.affectedRows > 0 };
}

async function toggleFollow(followerId, followingId) {
  const [rows] = await db.query(
    'SELECT 1 FROM follows WHERE follower_id = ? AND following_id = ?',
    [followerId, followingId]
  );
  if (rows.length > 0) {
    return unfollowUser(followerId, followingId);
  }
  return followUser(followerId, followingId);
}

async function listFollowers(userId, page = 1, limit = 20, mode = 'followers', includeStats = false, viewerId = null) {
  const offset = (page - 1) * limit;
  const isFollowers = mode === 'followers';
  const joinField = isFollowers ? 'f.follower_id' : 'f.following_id';
  const whereField = isFollowers ? 'f.following_id' : 'f.follower_id';
  const selectStats = includeStats ? ', u.works_count, u.followers_count' : '';
  const viewerJoin = viewerId
    ? 'LEFT JOIN follows vf ON vf.follower_id = ? AND vf.following_id = u.id'
    : '';
  const viewerSelect = viewerId
    ? ', CASE WHEN vf.follower_id IS NULL THEN 0 ELSE 1 END as is_following'
    : '';
  const params = viewerId ? [viewerId, userId, limit, offset] : [userId, limit, offset];

  const [list] = await db.query(
    `SELECT u.id, u.name, u.avatar, u.bio${selectStats}${viewerSelect}
     FROM follows f JOIN users u ON ${joinField} = u.id
     ${viewerJoin}
     WHERE ${whereField} = ?
     ORDER BY f.created_at DESC
     LIMIT ? OFFSET ?`,
    params
  );

  const [[{ total }]] = await db.query(
    `SELECT COUNT(*) as total FROM follows WHERE ${whereField} = ?`,
    [userId]
  );

  return { list, total };
}

async function likeArtwork(userId, artworkId) {
  const [result] = await db.query(
    'INSERT IGNORE INTO likes (user_id, artwork_id) VALUES (?, ?)',
    [userId, artworkId]
  );

  if (result.affectedRows > 0) {
    await db.query('UPDATE artworks SET likes = likes + 1 WHERE id = ?', [artworkId]);
    const [[artwork]] = await db.query('SELECT user_id FROM artworks WHERE id = ?', [artworkId]);
    if (artwork) {
      await notificationService.createNotification({
        userId: artwork.user_id,
        actorId: userId,
        type: 'like',
        artworkId,
        ignoreDuplicate: true,
      });
    }
  }

  const [[{ likes }]] = await db.query('SELECT likes FROM artworks WHERE id = ?', [artworkId]);
  return { success: true, likes, changed: result.affectedRows > 0 };
}

async function unlikeArtwork(userId, artworkId) {
  const [result] = await db.query(
    'DELETE FROM likes WHERE user_id = ? AND artwork_id = ?',
    [userId, artworkId]
  );

  if (result.affectedRows > 0) {
    await db.query('UPDATE artworks SET likes = GREATEST(likes - 1, 0) WHERE id = ?', [artworkId]);
  }

  const [[{ likes }]] = await db.query('SELECT likes FROM artworks WHERE id = ?', [artworkId]);
  return { success: true, likes, changed: result.affectedRows > 0 };
}

async function hasLikedArtwork(userId, artworkId) {
  const [rows] = await db.query(
    'SELECT 1 FROM likes WHERE user_id = ? AND artwork_id = ?',
    [userId, artworkId]
  );
  return rows.length > 0;
}

async function collectArtwork(userId, artworkId) {
  const [result] = await db.query(
    'INSERT IGNORE INTO collections (user_id, artwork_id) VALUES (?, ?)',
    [userId, artworkId]
  );

  if (result.affectedRows > 0) {
    await db.query('UPDATE artworks SET collects = collects + 1 WHERE id = ?', [artworkId]);
  }

  return { success: true, changed: result.affectedRows > 0 };
}

async function uncollectArtwork(userId, artworkId) {
  const [result] = await db.query(
    'DELETE FROM collections WHERE user_id = ? AND artwork_id = ?',
    [userId, artworkId]
  );

  if (result.affectedRows > 0) {
    await db.query('UPDATE artworks SET collects = GREATEST(collects - 1, 0) WHERE id = ?', [artworkId]);
  }

  return { success: true, changed: result.affectedRows > 0 };
}

async function hasCollectedArtwork(userId, artworkId) {
  const [rows] = await db.query(
    'SELECT 1 FROM collections WHERE user_id = ? AND artwork_id = ?',
    [userId, artworkId]
  );
  return rows.length > 0;
}

async function hasFollowedUser(followerId, followingId) {
  const [rows] = await db.query(
    'SELECT 1 FROM follows WHERE follower_id = ? AND following_id = ?',
    [followerId, followingId]
  );
  return rows.length > 0;
}

async function listCollections(userId, page = 1, limit = 20) {
  const offset = (page - 1) * limit;
  const [list] = await db.query(
    `SELECT a.id, a.title, a.cover_url, a.likes, a.views, a.created_at,
            u.id as author_id, u.name as author_name, u.avatar as author_avatar
     FROM collections c
     JOIN artworks a ON c.artwork_id = a.id
     JOIN users u ON a.user_id = u.id
     WHERE c.user_id = ? AND a.status = 'published'
     ORDER BY c.created_at DESC
     LIMIT ? OFFSET ?`,
    [userId, limit, offset]
  );
  const [[{ total }]] = await db.query(
    `SELECT COUNT(*) as total
     FROM collections c
     JOIN artworks a ON c.artwork_id = a.id
     WHERE c.user_id = ? AND a.status = 'published'`,
    [userId]
  );
  return { list, total };
}

module.exports = {
  followUser,
  unfollowUser,
  toggleFollow,
  listFollowers,
  likeArtwork,
  unlikeArtwork,
  hasLikedArtwork,
  collectArtwork,
  uncollectArtwork,
  hasCollectedArtwork,
  hasFollowedUser,
  listCollections,
};
