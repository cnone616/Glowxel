-- Matrix 数据库初始化脚本
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
  tag           VARCHAR(30) NOT NULL,
  PRIMARY KEY (artwork_id, tag),
  INDEX idx_tag (tag)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

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

-- 模板表
CREATE TABLE IF NOT EXISTS templates (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  name          VARCHAR(100),
  image_url     VARCHAR(500),
  category      VARCHAR(30),
  size          VARCHAR(10),
  color_count   INT,
  difficulty    ENUM('简单','中等','困难'),
  usage_count   INT DEFAULT 0,
  description   TEXT,
  tags          JSON,
  pixel_data    LONGBLOB,
  created_at    DATETIME DEFAULT NOW()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 挑战表
CREATE TABLE IF NOT EXISTS challenges (
  id            BIGINT PRIMARY KEY AUTO_INCREMENT,
  title         VARCHAR(100),
  banner_url    VARCHAR(500),
  description   TEXT,
  start_date    DATE,
  end_date      DATE,
  status        ENUM('active','upcoming','ended') DEFAULT 'upcoming',
  participants  INT DEFAULT 0,
  submissions   INT DEFAULT 0,
  prize         VARCHAR(200),
  difficulty    ENUM('简单','中等','困难'),
  tags          JSON,
  rules         JSON,
  judges_criteria JSON,
  created_at    DATETIME DEFAULT NOW()
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

-- 设备绑定表
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
