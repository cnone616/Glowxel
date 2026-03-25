const router = require('express').Router();
const db = require('../config/db');
const { adminAuth } = require('../middleware/auth');
const artworkService = require('../services/artworkService');
const commentService = require('../services/commentService');
const userService = require('../services/userService');

// 所有 admin 路由都需要管理员权限
router.use(adminAuth);

// ─── 仪表盘统计 ───────────────────────────────────────────────
router.get('/stats', async (req, res) => {
  try {
    const [[{ userCount }]] = await db.query('SELECT COUNT(*) as userCount FROM users');
    const [[{ artworkCount }]] = await db.query('SELECT COUNT(*) as artworkCount FROM artworks');
    const [[{ templateCount }]] = await db.query('SELECT COUNT(*) as templateCount FROM templates');
    const [[{ commentCount }]] = await db.query('SELECT COUNT(*) as commentCount FROM comments');
    const [[{ challengeCount }]] = await db.query('SELECT COUNT(*) as challengeCount FROM challenges');
    const today = new Date().toISOString().slice(0, 10);
    const [[{ todayActive }]] = await db.query('SELECT COUNT(*) as todayActive FROM users WHERE DATE(created_at) = ?', [today]);
    res.json({ code: 0, data: { userCount, artworkCount, templateCount, commentCount, challengeCount, todayActive } });
  } catch (err) {
    console.error(err);
    res.json({ code: 500, message: '获取统计失败' });
  }
});

// ─── 用户管理 ─────────────────────────────────────────────────
router.get('/users', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 10;
    const offset = (page - 1) * limit;
    const keyword = req.query.keyword ? `%${req.query.keyword}%` : '%';
    const [list] = await db.query(
      'SELECT id, name, avatar, bio, level, status, role, works_count, followers_count, created_at FROM users WHERE name LIKE ? OR id LIKE ? ORDER BY created_at DESC LIMIT ? OFFSET ?',
      [keyword, keyword, limit, offset]
    );
    const [[{ total }]] = await db.query(
      'SELECT COUNT(*) as total FROM users WHERE name LIKE ? OR id LIKE ?',
      [keyword, keyword]
    );
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

router.put('/users/:id/status', async (req, res) => {
  try {
    const { status } = req.body;
    if (!['active', 'banned'].includes(status)) return res.json({ code: 400, message: '状态值无效' });
    await db.query('UPDATE users SET status = ? WHERE id = ?', [status, req.params.id]);
    res.json({ code: 0, message: '操作成功' });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

router.put('/users/:id/role', async (req, res) => {
  try {
    const { role } = req.body;
    if (!['user', 'admin'].includes(role)) return res.json({ code: 400, message: '角色值无效' });
    await db.query('UPDATE users SET role = ? WHERE id = ?', [role, req.params.id]);
    res.json({ code: 0, message: '操作成功' });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

router.delete('/users/:id', async (req, res) => {
  try {
    const data = await userService.removeUserById(req.params.id, req.user.id);
    if (data.notFound) return res.json({ code: 404, message: '用户不存在' });
    if (data.cannotDeleteSelf) return res.json({ code: 400, message: '不能删除当前登录管理员' });
    if (data.cannotDeleteLastAdmin) return res.json({ code: 400, message: '不能删除最后一个管理员账号' });
    res.json({ code: 0, message: '已删除', data });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

// ─── 作品管理 ─────────────────────────────────────────────────
router.get('/artworks', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 10;
    const offset = (page - 1) * limit;
    const keyword = req.query.keyword ? `%${req.query.keyword}%` : '%';
    const status = req.query.status || null;
    const statusCond = status ? 'AND a.status = ?' : '';
    const params = status ? [keyword, keyword, status, limit, offset] : [keyword, keyword, limit, offset];
    const [list] = await db.query(
      `SELECT a.id, a.title, a.cover_url, a.status, a.likes, a.views, a.created_at, u.name as author
       , a.comments_count
       FROM artworks a LEFT JOIN users u ON a.user_id = u.id
       WHERE (a.title LIKE ? OR u.name LIKE ?) ${statusCond}
       ORDER BY a.created_at DESC LIMIT ? OFFSET ?`, params
    );
    const [[{ total }]] = await db.query(`SELECT COUNT(*) as total FROM artworks a LEFT JOIN users u ON a.user_id = u.id WHERE (a.title LIKE ? OR u.name LIKE ?) ${statusCond}`,
      status ? [keyword, keyword, status] : [keyword, keyword]);
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

router.put('/artworks/:id/status', async (req, res) => {
  try {
    const { status } = req.body;
    if (!['published', 'hidden', 'deleted'].includes(status)) return res.json({ code: 400, message: '状态值无效' });
    await db.query('UPDATE artworks SET status = ? WHERE id = ?', [status, req.params.id]);
    res.json({ code: 0, message: '操作成功' });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

router.delete('/artworks/:id', async (req, res) => {
  try {
    const data = await artworkService.removeArtworkById(req.params.id);
    if (data.notFound) return res.json({ code: 404, message: '作品不存在' });
    res.json({ code: 0, message: '已删除', data });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

// ─── 评论管理 ─────────────────────────────────────────────────
router.get('/comments', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    const [list] = await db.query(
      `SELECT c.id, c.content, c.created_at, u.name as author, a.title as artwork_title
       FROM comments c
       LEFT JOIN users u ON c.user_id = u.id
       LEFT JOIN artworks a ON c.artwork_id = a.id
       ORDER BY c.created_at DESC LIMIT ? OFFSET ?`, [limit, offset]
    );
    const [[{ total }]] = await db.query('SELECT COUNT(*) as total FROM comments');
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

router.delete('/comments/:id', async (req, res) => {
  try {
    const data = await commentService.removeCommentById(req.params.id);
    if (data.notFound) return res.json({ code: 404, message: '评论不存在' });
    res.json({ code: 0, message: '已删除', data });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

// ─── 模板管理 ─────────────────────────────────────────────────
router.get('/templates', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 10;
    const offset = (page - 1) * limit;
    const keyword = req.query.keyword ? `%${req.query.keyword}%` : '%';
    const [list] = await db.query(
      'SELECT * FROM templates WHERE name LIKE ? ORDER BY sort_order DESC, created_at DESC LIMIT ? OFFSET ?',
      [keyword, limit, offset]
    );
    const [[{ total }]] = await db.query(
      'SELECT COUNT(*) as total FROM templates WHERE name LIKE ?',
      [keyword]
    );
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

router.post('/templates', async (req, res) => {
  try {
    const { name, category, description, image_url, size, color_count, difficulty, is_featured, sort_order } = req.body;
    if (!name) return res.json({ code: 400, message: '名称不能为空' });
    const [result] = await db.query(
      `INSERT INTO templates
       (name, category, description, image_url, size, color_count, difficulty, is_featured, sort_order)
       VALUES (?,?,?,?,?,?,?,?,?)`,
      [
        name,
        category || '',
        description || '',
        image_url || '',
        size || '',
        color_count || 0,
        difficulty || '简单',
        is_featured ? 1 : 0,
        sort_order || 0,
      ]
    );
    res.json({ code: 0, data: { id: result.insertId } });
  } catch (err) { res.json({ code: 500, message: '创建失败' }); }
});

router.put('/templates/:id', async (req, res) => {
  try {
    const { name, category, description, image_url, size, color_count, difficulty, is_featured, sort_order } = req.body;
    await db.query(
      `UPDATE templates SET
        name = COALESCE(?, name),
        category = COALESCE(?, category),
        description = COALESCE(?, description),
        image_url = COALESCE(?, image_url),
        size = COALESCE(?, size),
        color_count = COALESCE(?, color_count),
        difficulty = COALESCE(?, difficulty),
        is_featured = COALESCE(?, is_featured),
        sort_order = COALESCE(?, sort_order)
       WHERE id = ?`,
      [
        name,
        category,
        description,
        image_url,
        size,
        color_count,
        difficulty,
        is_featured !== undefined ? (is_featured ? 1 : 0) : null,
        sort_order,
        req.params.id,
      ]
    );
    res.json({ code: 0, message: '更新成功' });
  } catch (err) { res.json({ code: 500, message: '更新失败' }); }
});

router.delete('/templates/:id', async (req, res) => {
  try {
    await db.query('DELETE FROM templates WHERE id = ?', [req.params.id]);
    res.json({ code: 0, message: '已删除' });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

// ─── 挑战管理 ─────────────────────────────────────────────────
router.get('/challenges', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 10;
    const offset = (page - 1) * limit;
    const keyword = req.query.keyword ? `%${req.query.keyword}%` : '%';
    const [list] = await db.query(
      'SELECT * FROM challenges WHERE title LIKE ? ORDER BY sort_order DESC, created_at DESC LIMIT ? OFFSET ?',
      [keyword, limit, offset]
    );
    const [[{ total }]] = await db.query(
      'SELECT COUNT(*) as total FROM challenges WHERE title LIKE ?',
      [keyword]
    );
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

router.post('/challenges', async (req, res) => {
  try {
    const {
      title, description, status, start_date, end_date, prize, difficulty, banner_url,
      cover_url, reward_type, max_participants, sort_order, rules
    } = req.body;
    if (!title) return res.json({ code: 400, message: '标题不能为空' });
    const [result] = await db.query(
      `INSERT INTO challenges
       (title, description, status, start_date, end_date, prize, difficulty, banner_url, cover_url,
        reward_type, max_participants, sort_order, rules)
       VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)`,
      [
        title,
        description || '',
        status || 'upcoming',
        start_date || null,
        end_date || null,
        prize || '',
        difficulty || '简单',
        banner_url || '',
        cover_url || '',
        reward_type || '荣誉',
        max_participants || 0,
        sort_order || 0,
        rules ? JSON.stringify(rules) : null,
      ]
    );
    res.json({ code: 0, data: { id: result.insertId } });
  } catch (err) { res.json({ code: 500, message: '创建失败' }); }
});

router.put('/challenges/:id', async (req, res) => {
  try {
    const {
      title, description, status, start_date, end_date, prize, difficulty, banner_url,
      cover_url, reward_type, max_participants, sort_order, rules
    } = req.body;
    await db.query(
      `UPDATE challenges SET
        title = COALESCE(?, title),
        description = COALESCE(?, description),
        status = COALESCE(?, status),
        start_date = COALESCE(?, start_date),
        end_date = COALESCE(?, end_date),
        prize = COALESCE(?, prize),
        difficulty = COALESCE(?, difficulty),
        banner_url = COALESCE(?, banner_url),
        cover_url = COALESCE(?, cover_url),
        reward_type = COALESCE(?, reward_type),
        max_participants = COALESCE(?, max_participants),
        sort_order = COALESCE(?, sort_order),
        rules = COALESCE(?, rules)
       WHERE id = ?`,
      [
        title,
        description,
        status,
        start_date,
        end_date,
        prize,
        difficulty,
        banner_url,
        cover_url,
        reward_type,
        max_participants,
        sort_order,
        rules ? JSON.stringify(rules) : null,
        req.params.id,
      ]
    );
    res.json({ code: 0, message: '更新成功' });
  } catch (err) { res.json({ code: 500, message: '更新失败' }); }
});

router.delete('/challenges/:id', async (req, res) => {
  try {
    await db.query('DELETE FROM challenges WHERE id = ?', [req.params.id]);
    res.json({ code: 0, message: '已删除' });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

// ─── 标签分类管理 ──────────────────────────────────────────────
router.get('/tag-categories', async (req, res) => {
  try {
    const [list] = await db.query('SELECT * FROM tag_categories ORDER BY sort_order DESC, id ASC')
    res.json({ code: 0, data: { list } })
  } catch (err) { res.json({ code: 500, message: '获取失败' }) }
})

router.post('/tag-categories', async (req, res) => {
  try {
    const { name, sort_order } = req.body
    if (!name) return res.json({ code: 400, message: '分类名不能为空' })
    const [r] = await db.query('INSERT INTO tag_categories (name, sort_order) VALUES (?,?)', [name, sort_order || 0])
    res.json({ code: 0, data: { id: r.insertId } })
  } catch (err) { res.json({ code: 500, message: err.code === 'ER_DUP_ENTRY' ? '分类名已存在' : '创建失败' }) }
})

router.put('/tag-categories/:id', async (req, res) => {
  try {
    const { name, sort_order, is_active } = req.body
    await db.query(
      'UPDATE tag_categories SET name=COALESCE(?,name), sort_order=COALESCE(?,sort_order), is_active=COALESCE(?,is_active) WHERE id=?',
      [name, sort_order, is_active !== undefined ? (is_active ? 1 : 0) : null, req.params.id]
    )
    res.json({ code: 0, message: '更新成功' })
  } catch (err) { res.json({ code: 500, message: '更新失败' }) }
})

router.delete('/tag-categories/:id', async (req, res) => {
  try {
    const [[{ cnt }]] = await db.query('SELECT COUNT(*) as cnt FROM tags WHERE category_id=?', [req.params.id])
    if (cnt > 0) return res.json({ code: 400, message: `该分类下还有 ${cnt} 个标签，请先删除` })
    await db.query('DELETE FROM tag_categories WHERE id=?', [req.params.id])
    res.json({ code: 0, message: '已删除' })
  } catch (err) { res.json({ code: 500, message: '删除失败' }) }
})

// ─── 标签管理 ──────────────────────────────────────────────────
router.get('/tags', async (req, res) => {
  try {
    const category_id = req.query.category_id || null
    const keyword = req.query.keyword ? `%${req.query.keyword}%` : '%'
    const catCond = category_id ? 'AND t.category_id=?' : ''
    const params = category_id ? [keyword, category_id] : [keyword]
    const [list] = await db.query(
      `SELECT t.*, c.name as category_name
       FROM tags t LEFT JOIN tag_categories c ON t.category_id = c.id
       WHERE t.name LIKE ? ${catCond}
       ORDER BY t.category_id ASC, t.sort_order DESC, t.id ASC`, params
    )
    res.json({ code: 0, data: { list } })
  } catch (err) { res.json({ code: 500, message: '获取失败' }) }
})

router.post('/tags', async (req, res) => {
  try {
    const { name, category_id, sort_order } = req.body
    if (!name || !category_id) return res.json({ code: 400, message: '标签名和分类不能为空' })
    const [r] = await db.query(
      'INSERT INTO tags (name, category_id, sort_order) VALUES (?,?,?)',
      [name, category_id, sort_order || 0]
    )
    res.json({ code: 0, data: { id: r.insertId } })
  } catch (err) { res.json({ code: 500, message: err.code === 'ER_DUP_ENTRY' ? '该分类下标签名已存在' : '创建失败' }) }
})

router.put('/tags/:id', async (req, res) => {
  try {
    const { name, category_id, sort_order, is_active } = req.body
    await db.query(
      `UPDATE tags SET name=COALESCE(?,name), category_id=COALESCE(?,category_id),
       sort_order=COALESCE(?,sort_order), is_active=COALESCE(?,is_active) WHERE id=?`,
      [name, category_id, sort_order, is_active !== undefined ? (is_active ? 1 : 0) : null, req.params.id]
    )
    res.json({ code: 0, message: '更新成功' })
  } catch (err) { res.json({ code: 500, message: '更新失败' }) }
})

router.delete('/tags/:id', async (req, res) => {
  try {
    const [[tagRow]] = await db.query('SELECT name FROM tags WHERE id=?', [req.params.id])
    if (!tagRow) return res.json({ code: 404, message: '标签不存在' })
    await db.query('DELETE FROM artwork_tags WHERE tag=?', [tagRow.name])
    await db.query('DELETE FROM tags WHERE id=?', [req.params.id])
    res.json({ code: 0, message: '已删除' })
  } catch (err) { res.json({ code: 500, message: '删除失败' }) }
})

// ─── 活动管理 ─────────────────────────────────────────────────
router.get('/activities', async (req, res) => {
  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 10;
    const offset = (page - 1) * limit;
    const keyword = req.query.keyword ? `%${req.query.keyword}%` : '%';
    const type = req.query.type || null;
    const status = req.query.status || null;
    const typeCond = type ? 'AND type = ?' : '';
    const statusCond = status ? 'AND status = ?' : '';
    const params = [keyword, ...(type ? [type] : []), ...(status ? [status] : []), limit, offset];
    const [list] = await db.query(
      `SELECT id, title, subtitle, cover_url, type, status, is_pinned, sort_order,
              start_time, end_time, views, created_at, published_at
       FROM activities
       WHERE title LIKE ? ${typeCond} ${statusCond}
       ORDER BY is_pinned DESC, sort_order DESC, created_at DESC
       LIMIT ? OFFSET ?`, params
    );
    const countParams = [keyword, ...(type ? [type] : []), ...(status ? [status] : [])];
    const [[{ total }]] = await db.query(
      `SELECT COUNT(*) as total FROM activities WHERE title LIKE ? ${typeCond} ${statusCond}`, countParams
    );
    res.json({ code: 0, data: { list, total } });
  } catch (err) { console.error(err); res.json({ code: 500, message: '获取失败' }); }
});

router.post('/activities', async (req, res) => {
  try {
    const { title, subtitle, cover_url, content, type, status, is_pinned,
            sort_order, start_time, end_time, link_url, published_at } = req.body;
    if (!title) return res.json({ code: 400, message: '标题不能为空' });
    const [result] = await db.query(
      `INSERT INTO activities (title, subtitle, cover_url, content, type, status,
        is_pinned, sort_order, start_time, end_time, link_url, author_id, published_at)
       VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)`,
      [title, subtitle || '', cover_url || '', content || '',
       type || 'activity', status || 'draft', is_pinned ? 1 : 0,
       sort_order || 0, start_time || null, end_time || null,
       link_url || '', req.user?.id || null,
       status === 'published' ? (published_at || new Date()) : null]
    );
    res.json({ code: 0, data: { id: result.insertId } });
  } catch (err) { console.error(err); res.json({ code: 500, message: '创建失败' }); }
});

router.put('/activities/:id', async (req, res) => {
  try {
    const { title, subtitle, cover_url, content, type, status, is_pinned,
            sort_order, start_time, end_time, link_url, published_at } = req.body;
    await db.query(
      `UPDATE activities SET
        title = COALESCE(?, title), subtitle = COALESCE(?, subtitle),
        cover_url = COALESCE(?, cover_url), content = COALESCE(?, content),
        type = COALESCE(?, type), status = COALESCE(?, status),
        is_pinned = COALESCE(?, is_pinned), sort_order = COALESCE(?, sort_order),
        start_time = ?, end_time = ?, link_url = COALESCE(?, link_url),
        published_at = COALESCE(?, published_at)
       WHERE id = ?`,
      [title, subtitle, cover_url, content, type, status,
       is_pinned !== undefined ? (is_pinned ? 1 : 0) : null,
       sort_order, start_time, end_time, link_url, published_at, req.params.id]
    );
    res.json({ code: 0, message: '更新成功' });
  } catch (err) { console.error(err); res.json({ code: 500, message: '更新失败' }); }
});

router.delete('/activities/:id', async (req, res) => {
  try {
    await db.query('DELETE FROM activities WHERE id = ?', [req.params.id]);
    res.json({ code: 0, message: '已删除' });
  } catch (err) { res.json({ code: 500, message: '删除失败' }); }
});

module.exports = router;
