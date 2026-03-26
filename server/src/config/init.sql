-- Glowxel 数据库初始化脚本
CREATE DATABASE IF NOT EXISTS matrix DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE matrix;

-- 用户表
CREATE TABLE IF NOT EXISTS users (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  openid        VARCHAR(64) UNIQUE NOT NULL,
  union_id      VARCHAR(64),
  name          VARCHAR(50) NOT NULL DEFAULT '微信用户',
  avatar        VARCHAR(500),
  bio           VARCHAR(200),
  level         INT DEFAULT 1,
  followers_count  INT DEFAULT 0,
  following_count  INT DEFAULT 0,
  works_count   INT DEFAULT 0,
  total_likes   INT DEFAULT 0,
  status        ENUM('active','banned') DEFAULT 'active',
  role          ENUM('user','admin') DEFAULT 'user',
  admin_username VARCHAR(50) NULL,         -- 管理员登录用户名
  admin_password VARCHAR(128) NULL,        -- 管理员登录密码(bcrypt)
  settings      JSON,
  created_at    DATETIME DEFAULT NOW(),
  updated_at    DATETIME DEFAULT NOW() ON UPDATE NOW()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 项目表（云同步）
CREATE TABLE IF NOT EXISTS projects (
  id            VARCHAR(36) PRIMARY KEY,
  user_id       BIGINT NOT NULL,
  name          VARCHAR(100),
  width         INT,
  height        INT,
  padded_width  INT,
  padded_height INT,
  palette       JSON,
  thumbnail_url VARCHAR(500),
  progress      INT DEFAULT 0,
  status        ENUM('draft','reviewing','published','rejected') DEFAULT 'draft',
  description   TEXT,
  tags          JSON,
  pixel_data    LONGBLOB,
  created_at    DATETIME DEFAULT NOW(),
  updated_at    DATETIME DEFAULT NOW() ON UPDATE NOW(),
  deleted_at    DATETIME NULL,
  INDEX idx_user (user_id),
  INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 作品表（社区发布）
CREATE TABLE IF NOT EXISTS artworks (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  user_id       BIGINT NOT NULL,
  project_id    VARCHAR(36),
  title         VARCHAR(100) NOT NULL,
  description   TEXT,
  cover_url     VARCHAR(500),
  preview_url   VARCHAR(500),
  width         INT,
  height        INT,
  color_count   INT,
  board_count   INT DEFAULT 1,
  difficulty    ENUM('简单','中等','困难'),
  pixel_data    LONGBLOB,
  likes         INT DEFAULT 0,
  comments_count INT DEFAULT 0,
  views         INT DEFAULT 0,
  collects      INT DEFAULT 0,
  status        ENUM('published','hidden','deleted') DEFAULT 'published',
  created_at    DATETIME DEFAULT NOW(),
  INDEX idx_user (user_id),
  INDEX idx_created (created_at DESC),
  INDEX idx_likes (likes DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 作品标签关联表
CREATE TABLE IF NOT EXISTS artwork_tags (
  artwork_id    BIGINT NOT NULL,
  tag           VARCHAR(50) NOT NULL,       -- 标签名（与路由代码一致）
  PRIMARY KEY (artwork_id, tag),
  INDEX idx_tag (tag)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 标签分类表（管理员维护）
CREATE TABLE IF NOT EXISTS tag_categories (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  name          VARCHAR(50) NOT NULL UNIQUE, -- 分类名，如：风格、主题、难度
  sort_order    INT DEFAULT 0,              -- 排序权重，越大越靠前
  is_active     TINYINT(1) DEFAULT 1,       -- 是否启用
  created_at    DATETIME DEFAULT NOW(),
  INDEX idx_sort (sort_order DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 标签表（管理员维护，用户只能选不能自定义）
CREATE TABLE IF NOT EXISTS tags (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  name          VARCHAR(50) NOT NULL,       -- 标签名，如：像素风、动物、简单
  category_id   BIGINT NOT NULL,            -- 所属分类
  sort_order    INT DEFAULT 0,              -- 分类内排序权重
  is_active     TINYINT(1) DEFAULT 1,       -- 是否启用（下架不影响已有数据）
  usage_count   INT DEFAULT 0,             -- 使用次数（冗余字段，方便排序）
  created_at    DATETIME DEFAULT NOW(),
  UNIQUE KEY uk_name_cat (name, category_id),
  INDEX idx_category (category_id),
  INDEX idx_sort (sort_order DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 预置分类数据
INSERT IGNORE INTO tag_categories (name, sort_order) VALUES
  ('风格', 100), ('主题', 90), ('颜色', 80), ('尺寸', 70), ('难度', 60);

-- 预置标签数据
INSERT IGNORE INTO tags (name, category_id, sort_order) VALUES
  ('像素风', 1, 100), ('极简', 1, 90), ('复古', 1, 80), ('赛博朋克', 1, 70),
  ('动物', 2, 100), ('人物', 2, 90), ('风景', 2, 80), ('食物', 2, 70), ('建筑', 2, 60),
  ('彩色', 3, 100), ('黑白', 3, 90), ('渐变', 3, 80),
  ('16x16', 4, 100), ('32x32', 4, 90), ('64x64', 4, 80),
  ('入门', 5, 100), ('进阶', 5, 90), ('高难', 5, 80);

-- 关注关系表
CREATE TABLE IF NOT EXISTS follows (
  follower_id   BIGINT NOT NULL,
  following_id  BIGINT NOT NULL,
  created_at    DATETIME DEFAULT NOW(),
  PRIMARY KEY (follower_id, following_id),
  INDEX idx_following (following_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 点赞表
CREATE TABLE IF NOT EXISTS likes (
  user_id       BIGINT NOT NULL,
  artwork_id    BIGINT NOT NULL,
  created_at    DATETIME DEFAULT NOW(),
  PRIMARY KEY (user_id, artwork_id),
  INDEX idx_artwork (artwork_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 收藏表
CREATE TABLE IF NOT EXISTS collections (
  user_id       BIGINT NOT NULL,
  artwork_id    BIGINT NOT NULL,
  created_at    DATETIME DEFAULT NOW(),
  PRIMARY KEY (user_id, artwork_id),
  INDEX idx_user (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 评论表
CREATE TABLE IF NOT EXISTS comments (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  artwork_id    BIGINT NOT NULL,
  user_id       BIGINT NOT NULL,
  content       TEXT NOT NULL,
  reply_to      BIGINT NULL,
  likes         INT DEFAULT 0,
  created_at    DATETIME DEFAULT NOW(),
  INDEX idx_artwork (artwork_id, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 挑战表
CREATE TABLE IF NOT EXISTS challenges (
  id              BIGINT PRIMARY KEY AUTO_INCREMENT,
  title           VARCHAR(100) NOT NULL,
  cover_url       VARCHAR(500),           -- 列表封面缩略图
  banner_url      VARCHAR(500),           -- 详情页横幅大图
  description     TEXT,
  rules           JSON,                   -- 参赛规则（数组）
  judges_criteria JSON,                   -- 评分标准
  start_date      DATE,
  end_date        DATE,
  status          ENUM('active','upcoming','ended') DEFAULT 'upcoming',
  participants    INT DEFAULT 0,
  submissions     INT DEFAULT 0,
  max_participants INT DEFAULT 0,         -- 0=不限
  prize           VARCHAR(200),
  reward_type     ENUM('实物','虚拟','荣誉') DEFAULT '荣誉',
  difficulty      ENUM('简单','中等','困难'),
  tags            JSON,
  sort_order      INT DEFAULT 0,          -- 排序权重，越大越靠前
  created_at      DATETIME DEFAULT NOW(),
  updated_at      DATETIME DEFAULT NOW() ON UPDATE NOW()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 挑战参与表
CREATE TABLE IF NOT EXISTS challenge_participants (
  challenge_id  BIGINT NOT NULL,
  user_id       BIGINT NOT NULL,
  artwork_id    BIGINT NULL,
  joined_at     DATETIME DEFAULT NOW(),
  submitted_at  DATETIME NULL,
  PRIMARY KEY (challenge_id, user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 模板表
CREATE TABLE IF NOT EXISTS templates (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  name          VARCHAR(100) NOT NULL,
  image_url     VARCHAR(500),             -- 主预览图
  category      VARCHAR(30),
  size          VARCHAR(10),
  color_count   INT DEFAULT 0,
  difficulty    ENUM('简单','中等','困难') DEFAULT '简单',
  usage_count   INT DEFAULT 0,
  description   TEXT,
  tags          JSON,
  is_featured   TINYINT(1) DEFAULT 0,    -- 是否推荐
  sort_order    INT DEFAULT 0,           -- 排序权重
  pixel_data    LONGBLOB,
  created_at    DATETIME DEFAULT NOW(),
  updated_at    DATETIME DEFAULT NOW() ON UPDATE NOW()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 活动表（公告/活动/教程）
CREATE TABLE IF NOT EXISTS activities (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  title         VARCHAR(200) NOT NULL,   -- 标题
  subtitle      VARCHAR(300),            -- 副标题/摘要
  cover_url     VARCHAR(500),            -- 封面图
  content       LONGTEXT,               -- 正文（富文本/Markdown）
  type          ENUM('activity','announcement','tutorial') DEFAULT 'activity',
                                         -- activity=活动 announcement=公告 tutorial=教程
  status        ENUM('draft','published','archived') DEFAULT 'draft',
                                         -- draft=草稿 published=已发布 archived=已归档
  is_pinned     TINYINT(1) DEFAULT 0,   -- 是否置顶
  sort_order    INT DEFAULT 0,           -- 排序权重，越大越靠前
  start_time    DATETIME NULL,           -- 活动开始时间（可选）
  end_time      DATETIME NULL,           -- 活动结束时间（可选）
  link_url      VARCHAR(500),            -- 外链（可选，点击跳转）
  views         INT DEFAULT 0,           -- 浏览次数
  author_id     BIGINT NULL,             -- 发布人（管理员）
  created_at    DATETIME DEFAULT NOW(),
  updated_at    DATETIME DEFAULT NOW() ON UPDATE NOW(),
  published_at  DATETIME NULL,           -- 发布时间
  INDEX idx_type_status (type, status),
  INDEX idx_pinned (is_pinned DESC, sort_order DESC),
  INDEX idx_published (published_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS user_devices (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  user_id       BIGINT NOT NULL,
  name          VARCHAR(50),
  mac_address   VARCHAR(20),
  type          VARCHAR(10),
  firmware_ver  VARCHAR(20),
  created_at    DATETIME DEFAULT NOW(),
  INDEX idx_user (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 固件版本表（OTA升级）
CREATE TABLE IF NOT EXISTS firmware_versions (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  version       VARCHAR(20) NOT NULL UNIQUE,
  device_type   VARCHAR(30) DEFAULT 'renlight-64',
  file_url      VARCHAR(500) NOT NULL,
  file_size     INT DEFAULT 0,
  md5_hash      VARCHAR(32),
  changelog     TEXT,
  is_force      TINYINT(1) DEFAULT 0,
  is_active     TINYINT(1) DEFAULT 1,
  created_at    DATETIME DEFAULT NOW(),
  INDEX idx_device_active (device_type, is_active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;


-- 通知表
CREATE TABLE IF NOT EXISTS notifications (
  id          BIGINT PRIMARY KEY AUTO_INCREMENT,
  user_id     BIGINT NOT NULL,           -- 接收通知的用户
  actor_id    BIGINT NOT NULL,           -- 触发通知的用户
  type        ENUM('like','comment','follow') NOT NULL,
  artwork_id  BIGINT,                    -- 相关作品（like/comment 时有值）
  comment_id  BIGINT,                    -- 相关评论（comment 时有值）
  is_read     TINYINT(1) DEFAULT 0,
  created_at  DATETIME DEFAULT NOW(),
  INDEX idx_user_read (user_id, is_read),
  INDEX idx_user_created (user_id, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
