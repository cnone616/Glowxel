process.env.NODE_ENV = 'test';
process.env.JWT_SECRET = 'test-secret';

const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('fs');
const path = require('path');
const jwt = require('jsonwebtoken');
const request = require('supertest');

const db = require('../src/config/db');
const app = require('../src/app');
const serverLogger = require('../src/services/serverLogger');
const adminAuditService = require('../src/services/adminAuditService');
const {
  hashAdminPassword,
  hashLegacyAdminPassword
} = require('../src/services/adminPasswordService');

const originalQuery = db.query.bind(db);
const originalLogInfo = serverLogger.info;
const originalLogWarn = serverLogger.warn;
const originalLogError = serverLogger.error;
const originalWriteAdminAuditLog = adminAuditService.writeAdminAuditLog;

test.beforeEach(() => {
  db.query = async (sql) => {
    throw new Error(`Unexpected SQL: ${sql}`);
  };
  serverLogger.info = originalLogInfo;
  serverLogger.warn = originalLogWarn;
  serverLogger.error = originalLogError;
  adminAuditService.writeAdminAuditLog = originalWriteAdminAuditLog;
});

test.after(() => {
  db.query = originalQuery;
  serverLogger.info = originalLogInfo;
  serverLogger.warn = originalLogWarn;
  serverLogger.error = originalLogError;
  adminAuditService.writeAdminAuditLog = originalWriteAdminAuditLog;
});

test('GET /api/health 返回健康状态', async () => {
  const response = await request(app).get('/api/health');

  assert.equal(response.status, 200);
  assert.equal(response.body.code, 0);
  assert.equal(response.body.message, 'ok');
  assert.ok(response.body.time);
});

test('POST /api/user/admin-login 支持 bcrypt 管理员登录', async () => {
  const bcryptHash = await hashAdminPassword('along.0408');

  db.query = async (sql, params) => {
    assert.equal(
      sql,
      "SELECT id, name, avatar, role, admin_username, admin_password FROM users WHERE admin_username = ? AND role = 'admin'"
    );
    assert.deepEqual(params, ['along']);
    return [[{
      id: 1,
      name: 'Admin',
      avatar: '',
      role: 'admin',
      admin_username: 'along',
      admin_password: bcryptHash
    }]];
  };

  const response = await request(app)
    .post('/api/user/admin-login')
    .send({ username: 'along', password: 'along.0408' });

  assert.equal(response.status, 200);
  assert.equal(response.body.code, 0);
  assert.ok(response.body.data.token);
  assert.equal(response.body.data.user.role, 'admin');
});

test('POST /api/user/admin-login 错误密码会被拒绝', async () => {
  const bcryptHash = await hashAdminPassword('along.0408');
  const warnEvents = [];

  serverLogger.warn = (event, payload) => {
    warnEvents.push({ event, payload });
  };

  db.query = async () => {
    return [[{
      id: 1,
      name: 'Admin',
      avatar: '',
      role: 'admin',
      admin_username: 'along',
      admin_password: bcryptHash
    }]];
  };

  const response = await request(app)
    .post('/api/user/admin-login')
    .send({ username: 'along', password: 'wrong-password' });

  assert.equal(response.status, 200);
  assert.equal(response.body.code, 401);
  assert.ok(warnEvents.some((item) => item.event === 'admin_login_failed'));
  assert.ok(
    warnEvents.some(
      (item) =>
        item.event === 'admin_login_failed' &&
        item.payload.reason === 'password_mismatch'
    )
  );
});

test('POST /api/user/admin-login 遇到旧 sha256 哈希会自动升级', async () => {
  const legacyHash = hashLegacyAdminPassword('along.0408');
  let updatedPasswordHash = '';
  let updatedUserId = 0;

  db.query = async (sql, params) => {
    if (sql === "SELECT id, name, avatar, role, admin_username, admin_password FROM users WHERE admin_username = ? AND role = 'admin'") {
      return [[{
        id: 1,
        name: 'Admin',
        avatar: '',
        role: 'admin',
        admin_username: 'along',
        admin_password: legacyHash
      }]];
    }

    if (sql === 'UPDATE users SET admin_password = ? WHERE id = ?') {
      updatedPasswordHash = params[0];
      updatedUserId = params[1];
      return [{ affectedRows: 1 }];
    }

    throw new Error(`Unexpected SQL: ${sql}`);
  };

  const response = await request(app)
    .post('/api/user/admin-login')
    .send({ username: 'along', password: 'along.0408' });

  assert.equal(response.status, 200);
  assert.equal(response.body.code, 0);
  assert.equal(updatedUserId, 1);
  assert.match(updatedPasswordHash, /^\$2[aby]\$\d{2}\$/);
});

test('GET /api/user/settings 返回当前用户设置', async () => {
  const token = jwt.sign({ id: 7 }, process.env.JWT_SECRET);

  db.query = async (sql, params) => {
    assert.equal(sql, 'SELECT settings FROM users WHERE id = ?');
    assert.deepEqual(params, [7]);
    return [[{ settings: { showSeconds: true, timeFormat: 24 } }]];
  };

  const response = await request(app)
    .get('/api/user/settings')
    .set('Authorization', `Bearer ${token}`);

  assert.equal(response.status, 200);
  assert.equal(response.body.code, 0);
  assert.deepEqual(response.body.data.settings, { showSeconds: true, timeFormat: 24 });
});

test('PUT /api/user/settings 保存当前用户设置', async () => {
  const token = jwt.sign({ id: 7 }, process.env.JWT_SECRET);
  const payload = { showSeconds: false, timeFormat: 12 };

  db.query = async (sql, params) => {
    assert.equal(sql, 'UPDATE users SET settings = ? WHERE id = ?');
    assert.deepEqual(params, [JSON.stringify(payload), 7]);
    return [{ affectedRows: 1 }];
  };

  const response = await request(app)
    .put('/api/user/settings')
    .set('Authorization', `Bearer ${token}`)
    .send(payload);

  assert.equal(response.status, 200);
  assert.equal(response.body.code, 0);
  assert.deepEqual(response.body.data.settings, payload);
});

test('GET /api/firmware/list 返回固件列表', async () => {
  db.query = async (sql) => {
    assert.equal(
      sql,
      `SELECT version, device_type, file_size, changelog, is_force, created_at 
       FROM firmware_versions WHERE is_active = 1 
       ORDER BY id DESC LIMIT 20`
    );
    return [[{
      version: '1.0.0',
      device_type: 'renlight-64',
      file_size: 1024,
      changelog: '初始版本',
      is_force: 0,
      created_at: '2026-03-26 00:00:00'
    }]];
  };

  const response = await request(app).get('/api/firmware/list');

  assert.equal(response.status, 200);
  assert.equal(response.body.code, 0);
  assert.equal(response.body.data.versions.length, 1);
  assert.equal(response.body.data.versions[0].version, '1.0.0');
});

test('PUT /api/admin/users/:id/status 会写入管理员审计日志', async () => {
  const token = jwt.sign({ id: 9, role: 'admin' }, process.env.JWT_SECRET);
  let auditPayload = null;

  adminAuditService.writeAdminAuditLog = (payload) => {
    auditPayload = payload;
  };

  db.query = async (sql, params) => {
    if (sql === 'SELECT role FROM users WHERE id = ?') {
      assert.deepEqual(params, [9]);
      return [[{ role: 'admin' }]];
    }

    if (sql === 'UPDATE users SET status = ? WHERE id = ?') {
      assert.deepEqual(params, ['banned', '12']);
      return [{ affectedRows: 1 }];
    }

    throw new Error(`Unexpected SQL: ${sql}`);
  };

  const response = await request(app)
    .put('/api/admin/users/12/status')
    .set('Authorization', `Bearer ${token}`)
    .send({ status: 'banned' });

  assert.equal(response.status, 200);
  assert.equal(response.body.code, 0);
  assert.equal(auditPayload.actorId, 9);
  assert.equal(auditPayload.method, 'PUT');
  assert.equal(auditPayload.resource, 'users');
  assert.equal(auditPayload.targetId, '12');
  assert.deepEqual(auditPayload.body, { status: 'banned' });
});

test('GET /api/admin/stats 失败时会输出 error 级日志', async () => {
  const token = jwt.sign({ id: 3, role: 'admin' }, process.env.JWT_SECRET);
  let errorPayload = null;

  serverLogger.error = (event, payload) => {
    errorPayload = { event, payload };
  };

  db.query = async (sql, params) => {
    if (sql === 'SELECT role FROM users WHERE id = ?') {
      assert.deepEqual(params, [3]);
      return [[{ role: 'admin' }]];
    }

    throw new Error('db unavailable');
  };

  const response = await request(app)
    .get('/api/admin/stats')
    .set('Authorization', `Bearer ${token}`);

  assert.equal(response.status, 200);
  assert.equal(response.body.code, 500);
  assert.equal(errorPayload.event, 'api_response_failed');
  assert.equal(errorPayload.payload.path, '/api/admin/stats');
  assert.equal(errorPayload.payload.responseCode, 500);
});

test('POST /api/upload 按当前请求地址返回上传文件 URL', async () => {
  const token = jwt.sign({ id: 7 }, process.env.JWT_SECRET);

  const response = await request(app)
    .post('/api/upload?type=avatar')
    .set('Authorization', `Bearer ${token}`)
    .attach('file', Buffer.from('avatar-binary'), 'avatar.png');

  assert.equal(response.status, 200);
  assert.equal(response.body.code, 0);
  assert.match(response.body.data.url, /^http:\/\/127\.0\.0\.1:\d+\/uploads\/avatar\//);

  const fileName = response.body.data.url.split('/').pop();
  const filePath = path.join(__dirname, '../uploads/avatar', fileName);

  if (fs.existsSync(filePath)) {
    fs.unlinkSync(filePath);
  }
});
