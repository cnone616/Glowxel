const router = require('express').Router();
const jwt = require('jsonwebtoken');
const axios = require('axios');
const db = require('../config/db');
const { auth } = require('../middleware/auth');

// 微信登录
router.post('/login', async (req, res) => {
  try {
    const { code } = req.body;
    if (!code) return res.json({ code: 400, message: '缺少 code' });

    // 用 code 换取 openid
    const wxRes = await axios.get('https://api.weixin.qq.com/sns/jscode2session', {
      params: {
        appid: process.env.WX_APPID,
        secret: process.env.WX_SECRET,
        js_code: code,
        grant_type: 'authorization_code'
      }
    });

    const { openid, session_key } = wxRes.data;
    if (!openid) return res.json({ code: 400, message: '微信登录失败' });

    // 查找或创建用户
    let [rows] = await db.query('SELECT * FROM users WHERE openid = ?', [openid]);
    let user;
    if (rows.length === 0) {
      const [result] = await db.query(
        'INSERT INTO users (openid, name) VALUES (?, ?)',
        [openid, '微信用户']
      );
      user = { id: result.insertId, openid, name: '微信用户', avatar: '', bio: '', level: 1 };
    } else {
      user = rows[0];
    }

    const token = jwt.sign({ id: user.id, openid }, process.env.JWT_SECRET, { expiresIn: '30d' });
    res.json({ code: 0, data: { token, user } });
  } catch (err) {
    console.error('登录失败:', err);
    res.json({ code: 500, message: '登录失败' });
  }
});

// 获取当前用户信息
router.get('/profile', auth, async (req, res) => {
  try {
    const [rows] = await db.query('SELECT id, name, avatar, bio, level, followers_count, following_count, works_count, total_likes, settings, created_at FROM users WHERE id = ?', [req.user.id]);
    if (rows.length === 0) return res.json({ code: 404, message: '用户不存在' });
    res.json({ code: 0, data: { user: rows[0] } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 更新个人资料
router.put('/profile', auth, async (req, res) => {
  try {
    const { name, avatar, bio } = req.body;
    await db.query('UPDATE users SET name = COALESCE(?, name), avatar = COALESCE(?, avatar), bio = COALESCE(?, bio) WHERE id = ?',
      [name, avatar, bio, req.user.id]);
    const [rows] = await db.query('SELECT id, name, avatar, bio, level FROM users WHERE id = ?', [req.user.id]);
    res.json({ code: 0, data: { user: rows[0] } });
  } catch (err) {
    res.json({ code: 500, message: '更新失败' });
  }
});

// 获取他人主页
router.get('/:id', async (req, res) => {
  try {
    const [rows] = await db.query('SELECT id, name, avatar, bio, level, followers_count, following_count, works_count, total_likes, created_at FROM users WHERE id = ?', [req.params.id]);
    if (rows.length === 0) return res.json({ code: 404, message: '用户不存在' });
    const [artworks] = await db.query('SELECT id, title, cover_url, likes, views, created_at FROM artworks WHERE user_id = ? AND status = "published" ORDER BY created_at DESC LIMIT 20', [req.params.id]);
    res.json({ code: 0, data: { user: rows[0], artworks } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 用户作品列表
router.get('/:id/artworks', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query('SELECT id, title, cover_url, likes, views, created_at FROM artworks WHERE user_id = ? AND status = "published" ORDER BY created_at DESC LIMIT ? OFFSET ?', [req.params.id, limit, offset]);
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM artworks WHERE user_id = ? AND status = "published"', [req.params.id]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

// 用户设置
router.get('/settings', auth, async (req, res) => {
  try {
    const [rows] = await db.query('SELECT settings FROM users WHERE id = ?', [req.user.id]);
    res.json({ code: 0, data: { settings: rows[0]?.settings || {} } });
  } catch (err) {
    res.json({ code: 500, message: '获取失败' });
  }
});

router.put('/settings', auth, async (req, res) => {
  try {
    await db.query('UPDATE users SET settings = ? WHERE id = ?', [JSON.stringify(req.body), req.user.id]);
    res.json({ code: 0, data: { settings: req.body } });
  } catch (err) {
    res.json({ code: 500, message: '更新失败' });
  }
});

module.exports = router;