/**
 * 数据库初始化 + 种子数据脚本（合并 init.sql + seed.sql）
 * 用法: node server/src/config/seed.js
 * 幂等：可重复执行，不会重复插入
 */
require("dotenv").config({
  path: require("path").join(__dirname, "../../.env"),
});
const db = require("./db");
const fs = require("fs");
const path = require("path");
const crypto = require("crypto");

const hashPassword = (pwd) =>
  crypto
    .createHash("sha256")
    .update(pwd + (process.env.PWD_SALT || "glowxel"))
    .digest("hex");

async function seed() {
  console.log("=== RenLight 数据库初始化 ===\n");

  // 1. 执行 init.sql 建表
  console.log("[1/6] 执行建表脚本...");
  const initSql = fs.readFileSync(path.join(__dirname, "init.sql"), "utf8");
  const statements = initSql
    .split(";")
    .map((s) => s.trim())
    .filter((s) => s.length > 0 && !s.startsWith("--"));
  for (const stmt of statements) {
    try {
      await db.query(stmt);
    } catch (e) {
      if (
        !e.message.includes("already exists") &&
        !e.message.includes("Duplicate")
      ) {
        console.log("  ! SQL错误:", e.message.substring(0, 80));
      }
    }
  }
  console.log("  ✓ 表结构就绪");

  // 2. 补充字段
  console.log("[2/6] 检查字段更新...");
  const alterColumns = [
    [
      "ALTER TABLE users ADD COLUMN role ENUM('user','admin') DEFAULT 'user' AFTER status",
      "role",
    ],
    [
      "ALTER TABLE users ADD COLUMN admin_username VARCHAR(50) NULL AFTER role",
      "admin_username",
    ],
    [
      "ALTER TABLE users ADD COLUMN admin_password VARCHAR(64) NULL AFTER admin_username",
      "admin_password",
    ],
  ];
  for (const [sql, col] of alterColumns) {
    try {
      await db.query(sql);
      console.log(`  ✓ users 表添加 ${col} 字段`);
    } catch (e) {
      if (e.code === "ER_DUP_FIELDNAME") console.log(`  - ${col} 字段已存在`);
      else console.log("  !", e.message);
    }
  }

  // 3. 插入用户
  console.log("[3/6] 插入用户数据...");
  const users = [
    [
      1,
      "seed_user_001",
      "像素小王子",
      "https://picsum.photos/seed/u1/200",
      "热爱像素艺术的创作者",
      5,
      128,
      12,
      356,
    ],
    [
      2,
      "seed_user_002",
      "拼豆达人",
      "https://picsum.photos/seed/u2/200",
      "每天一幅像素画",
      3,
      56,
      8,
      120,
    ],
    [
      3,
      "seed_user_003",
      "PixelMaster",
      "https://picsum.photos/seed/u3/200",
      "专注游戏角色像素化",
      7,
      320,
      25,
      890,
    ],
    [
      4,
      "seed_user_004",
      "彩虹工坊",
      "https://picsum.photos/seed/u4/200",
      "色彩搭配爱好者",
      4,
      89,
      15,
      210,
    ],
  ];
  for (const u of users) {
    await db.query(
      `INSERT INTO users (id, openid, name, avatar, bio, level, followers_count, works_count, total_likes)
       VALUES (?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE name=VALUES(name)`,
      u,
    );
  }
  // 设第一个为管理员，写入账号密码
  await db.query("UPDATE users SET role='admin' WHERE id=1");
  await db.query(
    "UPDATE users SET admin_username=?, admin_password=? WHERE id=1",
    ["along", hashPassword("along.0408")],
  );
  console.log("  ✓ " + users.length + " 个用户（#1 为管理员，账号: along）");

  // 4. 插入作品
  console.log("[4/6] 插入作品数据...");
  const artworks = [
    [
      1,
      1,
      "超级马里奥",
      "经典游戏角色像素还原",
      "https://picsum.photos/seed/art1/200",
      32,
      32,
      12,
      1,
      "中等",
      86,
      12,
      320,
      45,
    ],
    [
      2,
      2,
      "皮卡丘",
      "宝可梦系列-皮卡丘",
      "https://picsum.photos/seed/art2/200",
      32,
      32,
      8,
      1,
      "简单",
      152,
      28,
      580,
      78,
    ],
    [
      3,
      3,
      "星之卡比",
      "粉色小可爱",
      "https://picsum.photos/seed/art3/200",
      16,
      16,
      6,
      1,
      "简单",
      67,
      5,
      210,
      32,
    ],
    [
      4,
      1,
      "林克",
      "塞尔达传说主角",
      "https://picsum.photos/seed/art4/200",
      32,
      32,
      15,
      1,
      "困难",
      203,
      35,
      890,
      120,
    ],
    [
      5,
      4,
      "向日葵",
      "阳光下的向日葵",
      "https://picsum.photos/seed/art5/200",
      32,
      32,
      10,
      1,
      "中等",
      45,
      8,
      156,
      22,
    ],
    [
      6,
      3,
      "吃豆人",
      "经典街机游戏",
      "https://picsum.photos/seed/art6/200",
      16,
      16,
      4,
      1,
      "简单",
      98,
      15,
      420,
      55,
    ],
    [
      7,
      2,
      "独角兽",
      "梦幻独角兽",
      "https://picsum.photos/seed/art7/200",
      32,
      32,
      12,
      1,
      "中等",
      134,
      20,
      510,
      68,
    ],
    [
      8,
      4,
      "樱花树",
      "春天的樱花",
      "https://picsum.photos/seed/art8/200",
      32,
      32,
      8,
      1,
      "中等",
      76,
      10,
      280,
      38,
    ],
  ];
  for (const a of artworks) {
    await db.query(
      `INSERT INTO artworks (id, user_id, title, description, cover_url, width, height, color_count, board_count, difficulty, likes, comments_count, views, collects)
       VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE title=VALUES(title)`,
      a,
    );
  }
  console.log("  ✓ " + artworks.length + " 个作品");

  // 5. 插入模板
  console.log("[5/6] 插入模板数据...");
  const templates = [
    [
      1,
      "爱心",
      "https://picsum.photos/seed/tpl1/200",
      "基础图形",
      "16x16",
      2,
      "简单",
      1520,
      "简单的爱心图案，适合入门",
    ],
    [
      2,
      "星星",
      "https://picsum.photos/seed/tpl2/200",
      "基础图形",
      "16x16",
      3,
      "简单",
      980,
      "五角星图案",
    ],
    [
      3,
      "小猫咪",
      "https://picsum.photos/seed/tpl3/200",
      "动物",
      "32x32",
      8,
      "中等",
      2340,
      "可爱的小猫咪头像",
    ],
    [
      4,
      "小狗",
      "https://picsum.photos/seed/tpl4/200",
      "动物",
      "32x32",
      7,
      "中等",
      1870,
      "柴犬像素画",
    ],
    [
      5,
      "蘑菇",
      "https://picsum.photos/seed/tpl5/200",
      "游戏",
      "16x16",
      5,
      "简单",
      3200,
      "超级马里奥蘑菇",
    ],
    [
      6,
      "宝剑",
      "https://picsum.photos/seed/tpl6/200",
      "游戏",
      "32x32",
      6,
      "中等",
      1450,
      "像素风宝剑",
    ],
    [
      7,
      "圣诞树",
      "https://picsum.photos/seed/tpl7/200",
      "节日",
      "32x32",
      10,
      "困难",
      890,
      "圣诞树装饰",
    ],
    [
      8,
      "南瓜灯",
      "https://picsum.photos/seed/tpl8/200",
      "节日",
      "32x32",
      6,
      "中等",
      760,
      "万圣节南瓜灯",
    ],
  ];
  for (const t of templates) {
    await db.query(
      `INSERT INTO templates (id, name, image_url, category, size, color_count, difficulty, usage_count, description)
       VALUES (?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE name=VALUES(name)`,
      t,
    );
  }
  console.log("  ✓ " + templates.length + " 个模板");

  // 6. 插入挑战 + 固件版本
  console.log("[6/6] 插入挑战和固件数据...");
  const challenges = [
    [
      1,
      "春日花语挑战",
      "https://picsum.photos/seed/ch1/400/200",
      "用像素画出你心中的春天花朵",
      "2026-03-01",
      "2026-03-31",
      "active",
      56,
      23,
      "限定头像框",
      "简单",
    ],
    [
      2,
      "游戏角色大赛",
      "https://picsum.photos/seed/ch2/400/200",
      "还原你最喜欢的游戏角色",
      "2026-03-10",
      "2026-04-10",
      "active",
      128,
      45,
      "实体拼豆套装",
      "中等",
    ],
    [
      3,
      "像素美食节",
      "https://picsum.photos/seed/ch3/400/200",
      "画出让人垂涎的美食像素画",
      "2026-04-01",
      "2026-04-30",
      "upcoming",
      12,
      0,
      "美食礼盒",
      "简单",
    ],
  ];
  for (const c of challenges) {
    await db.query(
      `INSERT INTO challenges (id, title, banner_url, description, start_date, end_date, status, participants, submissions, prize, difficulty)
       VALUES (?,?,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE title=VALUES(title)`,
      c,
    );
  }
  console.log("  ✓ " + challenges.length + " 个挑战");

  await db.query(
    `INSERT INTO firmware_versions (version, device_type, file_url, file_size, changelog)
     VALUES ('1.0.0', 'renlight-64', 'uploads/firmware/initial.bin', 0, '初始版本')
     ON DUPLICATE KEY UPDATE version=VALUES(version)`,
  );
  console.log("  ✓ 固件版本 v1.0.0");

  console.log("\n=== 数据库初始化完成 ===");
  process.exit(0);
}

seed().catch((e) => {
  console.error("初始化失败:", e);
  process.exit(1);
});
