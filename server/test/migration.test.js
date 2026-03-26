process.env.NODE_ENV = 'test';

const test = require('node:test');
const assert = require('node:assert/strict');

const {
  getMigrationEntries,
  splitSqlStatements
} = require('../src/config/migrationService');

test('迁移文件按版本顺序加载', () => {
  const migrationEntries = getMigrationEntries();

  assert.deepEqual(
    migrationEntries.map((entry) => entry.version),
    ['0001_init', '0002_admin_columns']
  );
});

test('SQL 语句拆分会跳过注释和空语句', () => {
  const statements = splitSqlStatements(`
    -- comment
    CREATE TABLE demo (id INT);

    INSERT INTO demo (id) VALUES (1);
  `);

  assert.deepEqual(statements, [
    'CREATE TABLE demo (id INT)',
    'INSERT INTO demo (id) VALUES (1)'
  ]);
});
