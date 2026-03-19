const router = require("express").Router();
const db = require("../config/db");
const { auth } = require("../middleware/auth");
const { adminAuth, rateLimit } = require("../middleware/auth");
const multer = require("multer");
const path = require("path");
const fs = require("fs");
const crypto = require("crypto");

// 固件文件存储
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    const dir = path.join(__dirname, "../../uploads/firmware");
    if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
    cb(null, dir);
  },
  filename: (req, file, cb) => {
    cb(null, `firmware_${Date.now()}.bin`);
  },
});
const upload = multer({ storage, limits: { fileSize: 4 * 1024 * 1024 } }); // 4MB限制

// ESP32 检查更新 (设备直接调用，加限流)
router.get("/check", rateLimit(30, 60000), async (req, res) => {
  try {
    const { version, device_type } = req.query;
    const type = device_type || "renlight-64";

    const [rows] = await db.query(
      `SELECT version, file_url, file_size, md5_hash, changelog, is_force 
       FROM firmware_versions 
       WHERE device_type = ? AND is_active = 1 
       ORDER BY id DESC LIMIT 1`,
      [type],
    );

    if (rows.length === 0) {
      return res.json({ code: 0, data: { has_update: false } });
    }

    const latest = rows[0];
    const hasUpdate = version ? latest.version !== version : false;

    res.json({
      code: 0,
      data: {
        has_update: hasUpdate,
        version: latest.version,
        file_url: latest.file_url,
        file_size: latest.file_size,
        md5: latest.md5_hash,
        changelog: latest.changelog,
        is_force: !!latest.is_force,
      },
    });
  } catch (err) {
    console.error("检查更新失败:", err);
    res.json({ code: 500, message: "检查更新失败" });
  }
});

// 下载固件 (ESP32 直接下载 bin 文件)
router.get("/download/:version", async (req, res) => {
  try {
    const [rows] = await db.query(
      "SELECT file_url FROM firmware_versions WHERE version = ? AND is_active = 1",
      [req.params.version],
    );
    if (rows.length === 0) return res.status(404).send("版本不存在");

    const filePath = path.join(__dirname, "../../", rows[0].file_url);
    if (!fs.existsSync(filePath)) return res.status(404).send("固件文件不存在");

    res.download(filePath);
  } catch (err) {
    res.status(500).send("下载失败");
  }
});

// 上传新固件版本 (需要管理员权限)
router.post(
  "/upload",
  adminAuth,
  upload.single("firmware"),
  async (req, res) => {
    try {
      const { version, changelog, device_type, is_force } = req.body;
      if (!version || !req.file) {
        return res.json({ code: 400, message: "缺少版本号或固件文件" });
      }

      // 计算 MD5
      const fileBuffer = fs.readFileSync(req.file.path);
      const md5 = crypto.createHash("md5").update(fileBuffer).digest("hex");

      const fileUrl = `uploads/firmware/${req.file.filename}`;

      await db.query(
        `INSERT INTO firmware_versions (version, device_type, file_url, file_size, md5_hash, changelog, is_force)
       VALUES (?, ?, ?, ?, ?, ?, ?)`,
        [
          version,
          device_type || "renlight-64",
          fileUrl,
          req.file.size,
          md5,
          changelog || "",
          is_force ? 1 : 0,
        ],
      );

      res.json({ code: 0, data: { version, file_size: req.file.size, md5 } });
    } catch (err) {
      console.error("上传固件失败:", err);
      res.json({ code: 500, message: "上传失败" });
    }
  },
);

// 获取版本列表 (小程序用)
router.get("/list", async (req, res) => {
  try {
    const [rows] = await db.query(
      `SELECT version, device_type, file_size, changelog, is_force, created_at 
       FROM firmware_versions WHERE is_active = 1 
       ORDER BY id DESC LIMIT 20`,
    );
    res.json({ code: 0, data: { versions: rows } });
  } catch (err) {
    res.json({ code: 500, message: "获取失败" });
  }
});

module.exports = router;
