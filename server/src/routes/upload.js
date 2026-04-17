const router = require('express').Router();
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const { auth } = require('../middleware/auth');

const ALLOWED_UPLOAD_TYPES = new Set(['avatar', 'cover', 'misc']);

const getRequestBaseUrl = (req) => {
  const forwardedProto = req.get('x-forwarded-proto');
  const host = req.get('host');

  if (forwardedProto) {
    return `${forwardedProto}://${host}`;
  }

  return `${req.protocol}://${host}`;
};

const getUploadType = (req) => {
  const type = typeof req.query.type === 'string' ? req.query.type : '';
  if (!ALLOWED_UPLOAD_TYPES.has(type)) {
    return null;
  }
  return type;
};

// 存储配置
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    const type = getUploadType(req); // avatar / cover / misc
    if (!type) {
      cb(new Error('上传类型无效'));
      return;
    }
    const dir = path.join(__dirname, '../../uploads', type);
    if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
    cb(null, dir);
  },
  filename: (req, file, cb) => {
    const ext = path.extname(file.originalname);
    if (!ext) {
      cb(new Error('文件扩展名无效'));
      return;
    }
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
  const type = getUploadType(req);
  if (!type) return res.json({ code: 400, message: '上传类型无效' });
  if (!req.file) return res.json({ code: 400, message: '未收到文件' });
  const url = `${getRequestBaseUrl(req)}/uploads/${type}/${req.file.filename}`;
  res.json({ code: 0, data: { url } });
});

// multer 错误处理
router.use((err, req, res, next) => {
  res.json({ code: 400, message: err.message || '上传失败' });
});

module.exports = router;
