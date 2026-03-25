const router = require('express').Router();
const db = require('../config/db');
const { auth, optionalAuth } = require('../middleware/auth');

// 挑战列表
router.get('/list', async (req, res) => {
  try {
    const { status } = req.query;
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    let sql = 'SELECT id, title, banner_url, description, start_date, end_date, status, participants, submissions, prize, difficulty FROM challenges WHERE 1=1';
    const params = [];
    if (status) { sql += ' AND status = ?'; params.push(status); }
    sql += ' ORDER BY created_at DESC LIMIT ? OFFSET ?';
    params.push(limit, offset);
    const [list] = await db.query(sql, params);
    let countSql = 'SELECT COUNT(*) as total FROM challenges WHERE 1=1';
    const countParams = [];
    if (status) { countSql += ' AND status = ?'; countParams.push(status); }
    const [[{ total }]] = await db.query(countSql, countParams);
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 热门挑战
router.get('/popular', async (req, res) => {
  try {
    const limit = parseInt(req.query.limit) || 10;
    const [list] = await db.query(
      'SELECT id, title, banner_url, status, participants, submissions, prize, difficulty FROM challenges ORDER BY participants DESC LIMIT ?', [limit]
    );
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 参加挑战
router.post('/:id/join', auth, async (req, res) => {
  try {
    const [result] = await db.query(
      'INSERT IGNORE INTO challenge_participants (challenge_id, user_id) VALUES (?, ?)',
      [req.params.id, req.user.id]
    );
    if (result.affectedRows > 0) {
      await db.query('UPDATE challenges SET participants = participants + 1 WHERE id = ?', [req.params.id]);
    }
    res.json({ code: 0, data: { success: true, joined: true, changed: result.affectedRows > 0 } });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

// 提交作品
router.post('/:id/submit', auth, async (req, res) => {
  try {
    const { artworkId } = req.body;
    if (!artworkId) return res.json({ code: 400, message: '缺少作品 ID' });

    const [[participant]] = await db.query(
      'SELECT artwork_id FROM challenge_participants WHERE challenge_id = ? AND user_id = ?',
      [req.params.id, req.user.id]
    );
    if (!participant) return res.json({ code: 400, message: '请先参与挑战' });

    const [[artwork]] = await db.query(
      'SELECT id FROM artworks WHERE id = ? AND user_id = ? AND status = "published"',
      [artworkId, req.user.id]
    );
    if (!artwork) return res.json({ code: 400, message: '作品不存在或无权提交' });

    await db.query(
      'UPDATE challenge_participants SET artwork_id = ?, submitted_at = NOW() WHERE challenge_id = ? AND user_id = ?',
      [artworkId, req.params.id, req.user.id]
    );

    if (!participant.artwork_id) {
      await db.query('UPDATE challenges SET submissions = submissions + 1 WHERE id = ?', [req.params.id]);
    }

    res.json({ code: 0, data: { success: true, artworkId } });
  } catch (err) { res.json({ code: 500, message: '提交失败' }); }
});

// 挑战作品列表
router.get('/:id/submissions', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT a.id, a.title, a.cover_url, a.likes, u.id as author_id, u.name as author_name, u.avatar as author_avatar
       FROM challenge_participants cp
       JOIN artworks a ON cp.artwork_id = a.id
       JOIN users u ON cp.user_id = u.id
       WHERE cp.challenge_id = ? AND cp.artwork_id IS NOT NULL
       ORDER BY cp.submitted_at DESC LIMIT ? OFFSET ?`,
      [req.params.id, limit, offset]
    );
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 挑战详情 (/:id 必须在所有静态路由之后)
router.get('/:id', optionalAuth, async (req, res) => {
  try {
    const [rows] = await db.query('SELECT * FROM challenges WHERE id = ?', [req.params.id]);
    if (rows.length === 0) return res.json({ code: 404, message: '挑战不存在' });
    const challenge = rows[0];
    if (req.user) {
      const [[participant]] = await db.query(
        'SELECT artwork_id FROM challenge_participants WHERE challenge_id = ? AND user_id = ?',
        [req.params.id, req.user.id]
      );
      challenge.joined = !!participant;
      challenge.submitted_artwork_id = participant?.artwork_id || null;
    }
    res.json({ code: 0, data: { challenge } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

module.exports = router;
