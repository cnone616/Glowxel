const mysql = require('mysql2/promise');
require('dotenv').config();

const TEST_DB_CONFIG = {
  host: '127.0.0.1',
  port: 3306,
  user: 'test',
  password: '',
  database: 'test'
};

function getDatabaseConfig() {
  if (process.env.NODE_ENV === 'test') {
    return TEST_DB_CONFIG;
  }

  const requiredKeys = ['DB_HOST', 'DB_PORT', 'DB_USER', 'DB_NAME'];
  const missingKeys = requiredKeys.filter((key) => {
    return typeof process.env[key] !== 'string' || process.env[key].trim() === '';
  });

  if (missingKeys.length > 0) {
    throw new Error(`缺少数据库环境变量: ${missingKeys.join(', ')}`);
  }

  const port = Number(process.env.DB_PORT);
  if (!Number.isInteger(port) || port <= 0) {
    throw new Error('数据库环境变量 DB_PORT 无效');
  }

  return {
    host: process.env.DB_HOST,
    port,
    user: process.env.DB_USER,
    password: process.env.DB_PASSWORD,
    database: process.env.DB_NAME
  };
}

const databaseConfig = getDatabaseConfig();
const pool = mysql.createPool({
  host: databaseConfig.host,
  port: databaseConfig.port,
  user: databaseConfig.user,
  password: databaseConfig.password,
  database: databaseConfig.database,
  waitForConnections: true,
  connectionLimit: 10,
  queueLimit: 0,
  charset: 'utf8mb4'
});

if (process.env.NODE_ENV !== 'test') {
  // 测试连接
  pool.getConnection()
    .then(conn => {
      console.log('MySQL 连接成功');
      conn.release();
    })
    .catch(err => {
      console.error('MySQL 连接失败:', err.message);
    });
}

module.exports = pool;
