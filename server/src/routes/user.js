const router = require('express').Router();
const jwt = require('jsonwebtoken');
const axios = require('axios');
const crypto = require('crypto');
const db = require('../config/db');
const { auth } = require('../middleware/auth');
const { contentFilter } = require('../middleware/contentFilter');

// 工具：sha256 哈希密码
const hashPassword = (pwd) => crypto.createHash('sha256').update(pwd + (process.env.PWD_SALT || 'glowxel')).digest('hex');

// 管理员账号密码登录
router.post('/admin-login', async (req, res) => {
  try {
    const { username, password } = req.body;
    if (!username || !password) return res.json({ code: 400, message: '请输入用户名和密码' });

    const [rows] = await db.query(
      "SELECT id, name, avatar, role, admin_username, admin_password FROM users WHERE admin_username = ? AND role = 'admin'",
      [username]
    );
    if (!rows.length) return res.json({ code: 401, message: '用户名或密码错误' });

    const user = rows[0];
    if (user.admin_password !== hashPassword(password)) {
      return res.json({ code: 401, message: '用户名或密码错误' });
    }

    const token = jwt.sign({ id: user.id, role: 'admin' }, process.env.JWT_SECRET, { expiresIn: '7d' });
    res.json({ code: 0, data: { token, user: { id: user.id, name: user.name, avatar: user.avatar, role: user.role } } });
  } catch (err) {
    console.error('管理员登录失败:', err);
    res.json({ code: 500, message: '登录失败' });
  }
});

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
router.put('/profile', auth, contentFilter(['name', 'bio']), async (req, res) => {
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

// 用户统计
router.get('/stats', auth, async (req, res) => {
  try {
    const [[u]] = await db.query('SELECT works_count, followers_count, following_count, total_likes FROM users WHERE id = ?', [req.user.id]);
    const [[{ collects }]] = await db.query('SELECT COUNT(*) as collects FROM collections WHERE user_id = ?', [req.user.id]);
    res.json({ code: 0, data: { ...u, collects } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 推荐用户
router.get('/recommended', async (req, res) => {
  try {
    const limit = parseInt(req.query.limit) || 8;
    const [list] = await db.query(
      'SELECT id, name, avatar, bio, works_count, followers_count FROM users WHERE status = "active" ORDER BY followers_count DESC LIMIT ?', [limit]
    );
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 搜索用户
router.get('/search', async (req, res) => {
  try {
    const keyword = req.query.keyword ? `%${req.query.keyword}%` : '%';
    const [list] = await db.query(
      'SELECT id, name, avatar, bio, works_count, followers_count FROM users WHERE name LIKE ? AND status = "active" LIMIT 20', [keyword]
    );
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '搜索失败' }); }
});

// 用户设置（必须在 /:id 前面，否则会被 /:id 匹配到）
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

// 获取用户详情（/:id 必须放在所有静态路由之后）
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

module.exports = router;