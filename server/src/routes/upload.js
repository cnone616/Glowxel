const router = require('express').Router();
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const { auth } = require('../middleware/auth');

// 存储配置
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    const type = req.query.type || 'misc'; // avatar / cover / misc
    const dir = path.join(__dirname, '../../uploads', type);
    if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
    cb(null, dir);
  },
  filename: (req, file, cb) => {
    const ext = path.extname(file.originalname) || '.jpg';
    cb(null, `${Date.now()}_${Math.random().toString(36).slice(2)}${ext}`);
  }
});

const upload = multer({
  storage,
  limits: { fileSize: 5 * 1024 * 1024 }, // 5MB
  fileFilter: (req, file, cb) => {
    const allowed = ['image/jpeg', 'image/png', 'image/gif', 'image/webp'];
    if (allowed.includes(file.mimetype)) cb(null, true);
    else cb(new Error('只支持 jpg/png/gif/webp 格式'));
  }
});

// 上传图片
router.post('/', auth, upload.single('file'), (req, res) => {
  if (!req.file) return res.json({ code: 400, message: '未收到文件' });
  const type = req.query.type || 'misc';
  const url = `https://glowxel.com/uploads/${type}/${req.file.filename}`;
  res.json({ code: 0, data: { url } });
});

// multer 错误处理
router.use((err, req, res, next) => {
  res.json({ code: 400, message: err.message || '上传失败' });
});

module.exports = router;

