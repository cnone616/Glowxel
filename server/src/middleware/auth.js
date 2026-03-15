const jwt = require('jsonwebtoken');
const db = require('../config/db');

// JWT 鉴权中间件
const auth = (req, res, next) => {
  const token = req.headers.authorization?.replace('Bearer ', '');
  if (!token) {
    return res.status(401).json({ code: 401, message: '未登录' });
  }
  try {
    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    req.user = decoded;
    next();
  } catch (err) {
    return res.status(401).json({ code: 401, message: 'token 无效或已过期' });
  }
};

// 可选鉴权（有 token 就解析，没有也放行）
const optionalAuth = (req, res, next) => {
  const token = req.headers.authorization?.replace('Bearer ', '');
  if (token) {
    try {
      req.user = jwt.verify(token, process.env.JWT_SECRET);
    } catch (err) {}
  }
  next();
};

// 管理员鉴权
const adminAuth = async (req, res, next) => {
  const token = req.headers.authorization?.replace('Bearer ', '');
  if (!token) {
    return res.status(401).json({ code: 401, message: '未登录' });
  }
  try {
    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    req.user = decoded;
    // 检查是否是管理员
    const [rows] = await db.query('SELECT role FROM users WHERE id = ?', [decoded.id]);
    if (!rows.length || rows[0].role !== 'admin') {
      return res.status(403).json({ code: 403, message: '无管理员权限' });
    }
    next();
  } catch (err) {
    return res.status(401).json({ code: 401, message: 'token 无效或已过期' });
  }
};

// 设备认证（ESP32 用 API Key）
const deviceAuth = (req, res, next) => {
  const apiKey = req.headers['x-device-key'] || req.query.device_key;
  if (!apiKey || apiKey !== process.env.DEVICE_API_KEY) {
    return res.status(401).json({ code: 401, message: '设备认证失败' });
  }
  next();
};

// 请求频率限制
const rateLimitStore = {};
const rateLimit = (maxRequests = 60, windowMs = 60000) => {
  return (req, res, next) => {
    const ip = req.ip || req.connection.remoteAddress;
    const now = Date.now();

    if (!rateLimitStore[ip]) {
      rateLimitStore[ip] = { count: 1, resetTime: now + windowMs };
    } else if (now > rateLimitStore[ip].resetTime) {
      rateLimitStore[ip] = { count: 1, resetTime: now + windowMs };
    } else {
      rateLimitStore[ip].count++;
    }

    if (rateLimitStore[ip].count > maxRequests) {
      return res.status(429).json({ code: 429, message: '请求过于频繁，请稍后再试' });
    }

    res.set('X-RateLimit-Limit', maxRequests);
    res.set('X-RateLimit-Remaining', Math.max(0, maxRequests - rateLimitStore[ip].count));
    next();
  };
};

// 定期清理过期的限流记录
setInterval(() => {
  const now = Date.now();
  for (const ip in rateLimitStore) {
    if (now > rateLimitStore[ip].resetTime) {
      delete rateLimitStore[ip];
    }
  }
}, 300000); // 5分钟清理一次

module.exports = { auth, optionalAuth, adminAuth, deviceAuth, rateLimit };

