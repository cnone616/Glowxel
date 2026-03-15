const router = require('express').Router();
const db = require('../config/db');
const { auth } = require('../middleware/auth');

// 关注用户
router.post('/:userId', auth, async (req, res) => {
  try {
    const targetId = parseInt(req.params.userId);
    if (targetId === req.user.id) return res.json({ code: 400, message: '不能关注自己' });

    await db.query('INSERT IGNORE INTO follows (follower_id, following_id) VALUES (?, ?)', [req.user.id, targetId]);
    await db.query('UPDATE users SET following_count = following_count + 1 WHERE id = ?', [req.user.id]);
    await db.query('UPDATE users SET followers_count = followers_count + 1 WHERE id = ?', [targetId]);
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 取消关注
router.delete('/:userId', auth, async (req, res) => {
  try {
    const targetId = parseInt(req.params.userId);
    const [result] = await db.query('DELETE FROM follows WHERE follower_id = ? AND following_id = ?', [req.user.id, targetId]);
    if (result.affectedRows > 0) {
      await db.query('UPDATE users SET following_count = GREATEST(following_count - 1, 0) WHERE id = ?', [req.user.id]);
      await db.query('UPDATE users SET followers_count = GREATEST(followers_count - 1, 0) WHERE id = ?', [targetId]);
    }
    res.json({ code: 0, data: { success: true } });
  } catch (err) {
    res.json({ code: 500, message: '操作失败' });
  }
});

// 我的关注列表
router.get('/following', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT u.id, u.name, u.avatar, u.bio, u.works_count, u.followers_count
       FROM follows f JOIN users u ON f.following_id = u.id
       WHERE f.follower_id = ? ORDER BY f.created_at DESC LIMIT ? OFFSET ?`,
      [req.user.id, limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM follows WHERE follower_id = ?', [req.user.id]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 我的粉丝列表
router.get('/followers', auth, async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT u.id, u.name, u.avatar, u.bio, u.works_count, u.followers_count
       FROM follows f JOIN users u ON f.follower_id = u.id
       WHERE f.following_id = ? ORDER BY f.created_at DESC LIMIT ? OFFSET ?`,
      [req.user.id, limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM follows WHERE following_id = ?', [req.user.id]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 他人关注列表
router.get('/:userId/following', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT u.id, u.name, u.avatar, u.bio FROM follows f JOIN users u ON f.following_id = u.id
       WHERE f.follower_id = ? ORDER BY f.created_at DESC LIMIT ? OFFSET ?`,
      [req.params.userId, limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM follows WHERE follower_id = ?', [req.params.userId]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 他人粉丝列表
router.get('/:userId/followers', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT u.id, u.name, u.avatar, u.bio FROM follows f JOIN users u ON f.follower_id = u.id
       WHERE f.following_id = ? ORDER BY f.created_at DESC LIMIT ? OFFSET ?`,
      [req.params.userId, limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM follows WHERE following_id = ?', [req.params.userId]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

module.exports = router;
