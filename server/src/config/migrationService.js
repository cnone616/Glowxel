require('dotenv').config();

const fs = require('fs');
const path = require('path');
const mysql = require('mysql2/promise');

const MIGRATIONS_DIR = path.join(__dirname, 'migrations');
const INIT_SQL_PATH = path.join(__dirname, 'init.sql');

const TEST_DB_CONFIG = {
  host: '127.0.0.1',
  port: 3306,
  user: 'test',
  password: '',
  database: 'test'
};

const getDatabaseConfig = () => {
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
};

const getConnectionOptions = () => {
  const databaseConfig = getDatabaseConfig();
  return {
    host: databaseConfig.host,
    port: databaseConfig.port,
    user: databaseConfig.user,
    password: databaseConfig.password,
    charset: 'utf8mb4',
    multipleStatements: true
  };
};

const getDatabaseName = () => {
  return getDatabaseConfig().database;
};

const splitSqlStatements = (sql) => {
  const normalizedSql = sql
    .split('\n')
    .filter((line) => !line.trim().startsWith('--'))
    .join('\n');

  return normalizedSql
    .split(';')
    .map((statement) => statement.trim())
    .filter((statement) => statement.length > 0);
};

const ensureDatabaseExists = async () => {
  const connection = await mysql.createConnection(getConnectionOptions());
  const databaseName = getDatabaseName();

  await connection.query(
    `CREATE DATABASE IF NOT EXISTS \`${databaseName}\` DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci`
  );

  await connection.end();
};

const createDatabaseConnection = async () => {
  return mysql.createConnection({
    ...getConnectionOptions(),
    database: getDatabaseName()
  });
};

const executeSqlFile = async (connection, filePath) => {
  const sql = fs.readFileSync(filePath, 'utf8');
  const statements = splitSqlStatements(sql);

  for (const statement of statements) {
    await connection.query(statement);
  }
};

const getMigrationEntries = () => {
  return fs
    .readdirSync(MIGRATIONS_DIR)
    .filter((fileName) => fileName.endsWith('.js'))
    .sort()
    .map((fileName) => {
      return {
        fileName,
        filePath: path.join(MIGRATIONS_DIR, fileName),
        version: fileName.replace(/\.js$/, '')
      };
    });
};

const runMigrations = async (options = {}) => {
  const logger = options.logger || (() => {});

  await ensureDatabaseExists();
  const connection = await createDatabaseConnection();

  try {
    await connection.query(`
      CREATE TABLE IF NOT EXISTS schema_migrations (
        version VARCHAR(100) PRIMARY KEY,
        created_at DATETIME DEFAULT NOW()
      ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    `);

    const [rows] = await connection.query(
      'SELECT version FROM schema_migrations ORDER BY version ASC'
    );
    const appliedVersions = new Set(rows.map((row) => row.version));
    const migrationEntries = getMigrationEntries();
    const executedVersions = [];

    for (const migrationEntry of migrationEntries) {
      if (appliedVersions.has(migrationEntry.version)) {
        continue;
      }

      logger(`执行迁移 ${migrationEntry.version}`);
      const migration = require(migrationEntry.filePath);
      await migration.up({
        connection,
        executeSqlFile,
        initSqlPath: INIT_SQL_PATH
      });
      await connection.query(
        'INSERT INTO schema_migrations (version) VALUES (?)',
        [migrationEntry.version]
      );
      executedVersions.push(migrationEntry.version);
    }

    return { executedVersions };
  } finally {
    await connection.end();
  }
};

module.exports = {
  executeSqlFile,
  getMigrationEntries,
  runMigrations,
  splitSqlStatements
};
