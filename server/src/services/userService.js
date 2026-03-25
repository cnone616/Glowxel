const db = require('../config/db');

function buildInClause(values = []) {
  return values.map(() => '?').join(',');
}

async function recomputeFollowCounts(conn, userIds = []) {
  if (!userIds.length) return;
  const placeholders = buildInClause(userIds);
  await conn.query(
    `UPDATE users
     SET followers_count = (
       SELECT COUNT(*) FROM follows WHERE following_id = users.id
     ),
     following_count = (
       SELECT COUNT(*) FROM follows WHERE follower_id = users.id
     )
     WHERE id IN (${placeholders})`,
    userIds
  );
}

async function removeUserById(targetUserId, actorUserId) {
  const conn = await db.getConnection();
  try {
    await conn.beginTransaction();

    const [[targetUser]] = await conn.query(
      'SELECT id, role FROM users WHERE id = ?',
      [targetUserId]
    );
    if (!targetUser) {
      await conn.rollback();
      return { success: false, notFound: true };
    }

    if (String(targetUserId) === String(actorUserId)) {
      await conn.rollback();
      return { success: false, cannotDeleteSelf: true };
    }

    if (targetUser.role === 'admin') {
      const [[{ adminCount }]] = await conn.query(
        'SELECT COUNT(*) as adminCount FROM users WHERE role = "admin"',
      );
      if (adminCount <= 1) {
        await conn.rollback();
        return { success: false, cannotDeleteLastAdmin: true };
      }
    }

    const [ownedArtworks] = await conn.query(
      'SELECT id FROM artworks WHERE user_id = ?',
      [targetUserId]
    );
    const ownedArtworkIds = ownedArtworks.map((item) => item.id);
    const ownedArtworkPlaceholders = buildInClause(ownedArtworkIds);

    const [commentLikeRows] = await conn.query(
      `SELECT artwork_id, COUNT(*) as count
       FROM comments
       WHERE user_id = ? AND artwork_id NOT IN (
         SELECT id FROM artworks WHERE user_id = ?
       )
       GROUP BY artwork_id`,
      [targetUserId, targetUserId]
    );
    for (const row of commentLikeRows) {
      await conn.query(
        'UPDATE artworks SET comments_count = GREATEST(comments_count - ?, 0) WHERE id = ?',
        [row.count, row.artwork_id]
      );
    }

    const [likeRows] = await conn.query(
      `SELECT artwork_id, COUNT(*) as count
       FROM likes
       WHERE user_id = ? AND artwork_id NOT IN (
         SELECT id FROM artworks WHERE user_id = ?
       )
       GROUP BY artwork_id`,
      [targetUserId, targetUserId]
    );
    for (const row of likeRows) {
      await conn.query(
        'UPDATE artworks SET likes = GREATEST(likes - ?, 0) WHERE id = ?',
        [row.count, row.artwork_id]
      );
    }

    const [collectRows] = await conn.query(
      `SELECT artwork_id, COUNT(*) as count
       FROM collections
       WHERE user_id = ? AND artwork_id NOT IN (
         SELECT id FROM artworks WHERE user_id = ?
       )
       GROUP BY artwork_id`,
      [targetUserId, targetUserId]
    );
    for (const row of collectRows) {
      await conn.query(
        'UPDATE artworks SET collects = GREATEST(collects - ?, 0) WHERE id = ?',
        [row.count, row.artwork_id]
      );
    }

    const [followRows] = await conn.query(
      `SELECT follower_id, following_id
       FROM follows
       WHERE follower_id = ? OR following_id = ?`,
      [targetUserId, targetUserId]
    );
    const affectedFollowUserIds = [
      ...new Set(
        followRows.flatMap((row) => [row.follower_id, row.following_id]).filter((id) => String(id) !== String(targetUserId))
      ),
    ];

    const [participantRows] = await conn.query(
      `SELECT challenge_id, COUNT(*) as participant_count,
              SUM(CASE WHEN artwork_id IS NOT NULL THEN 1 ELSE 0 END) as submission_count
       FROM challenge_participants
       WHERE user_id = ?
       GROUP BY challenge_id`,
      [targetUserId]
    );
    for (const row of participantRows) {
      await conn.query(
        `UPDATE challenges
         SET participants = GREATEST(participants - ?, 0),
             submissions = GREATEST(submissions - ?, 0)
         WHERE id = ?`,
        [row.participant_count, row.submission_count || 0, row.challenge_id]
      );
    }

    if (ownedArtworkIds.length > 0) {
      const [submissionRows] = await conn.query(
        `SELECT challenge_id, COUNT(*) as count
         FROM challenge_participants
         WHERE artwork_id IN (${ownedArtworkPlaceholders})
         GROUP BY challenge_id`,
        ownedArtworkIds
      );
      for (const row of submissionRows) {
        await conn.query(
          'UPDATE challenges SET submissions = GREATEST(submissions - ?, 0) WHERE id = ?',
          [row.count, row.challenge_id]
        );
      }

      await conn.query(
        `UPDATE challenge_participants
         SET artwork_id = NULL, submitted_at = NULL
         WHERE artwork_id IN (${ownedArtworkPlaceholders})`,
        ownedArtworkIds
      );

      await conn.query(
        `DELETE FROM artwork_tags WHERE artwork_id IN (${ownedArtworkPlaceholders})`,
        ownedArtworkIds
      );
      await conn.query(
        `DELETE FROM likes WHERE artwork_id IN (${ownedArtworkPlaceholders})`,
        ownedArtworkIds
      );
      await conn.query(
        `DELETE FROM collections WHERE artwork_id IN (${ownedArtworkPlaceholders})`,
        ownedArtworkIds
      );
      await conn.query(
        `DELETE FROM comments WHERE artwork_id IN (${ownedArtworkPlaceholders})`,
        ownedArtworkIds
      );
      await conn.query(
        `DELETE FROM notifications
         WHERE artwork_id IN (${ownedArtworkPlaceholders})
            OR comment_id IN (
              SELECT id FROM comments WHERE artwork_id IN (${ownedArtworkPlaceholders})
            )`,
        [...ownedArtworkIds, ...ownedArtworkIds]
      );
      await conn.query(
        `DELETE FROM artworks WHERE id IN (${ownedArtworkPlaceholders})`,
        ownedArtworkIds
      );
    }

    await conn.query('DELETE FROM comments WHERE user_id = ?', [targetUserId]);
    await conn.query('DELETE FROM likes WHERE user_id = ?', [targetUserId]);
    await conn.query('DELETE FROM collections WHERE user_id = ?', [targetUserId]);
    await conn.query('DELETE FROM notifications WHERE user_id = ? OR actor_id = ?', [targetUserId, targetUserId]);
    await conn.query('DELETE FROM follows WHERE follower_id = ? OR following_id = ?', [targetUserId, targetUserId]);
    await conn.query('DELETE FROM challenge_participants WHERE user_id = ?', [targetUserId]);
    await conn.query('DELETE FROM projects WHERE user_id = ?', [targetUserId]);
    await conn.query('DELETE FROM user_devices WHERE user_id = ?', [targetUserId]);
    await conn.query('UPDATE activities SET author_id = NULL WHERE author_id = ?', [targetUserId]);
    await conn.query('DELETE FROM users WHERE id = ?', [targetUserId]);

    await recomputeFollowCounts(conn, affectedFollowUserIds);
    await conn.commit();
    return { success: true };
  } catch (error) {
    await conn.rollback();
    throw error;
  } finally {
    conn.release();
  }
}

module.exports = {
  removeUserById,
};
