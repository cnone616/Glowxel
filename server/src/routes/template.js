const router = require('express').Router();
const db = require('../config/db');

// 模板列表
router.get('/list', async (req, res) => {
  try {
    const { category, difficulty } = req.query;
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 20;
    const offset = (page - 1) * limit;
    let sql = 'SELECT id, name, image_url, category, size, color_count, difficulty, usage_count, description FROM templates WHERE 1=1';
    const params = [];
    if (category) { sql += ' AND category = ?'; params.push(category); }
    if (difficulty) { sql += ' AND difficulty = ?'; params.push(difficulty); }
    sql += ' ORDER BY usage_count DESC LIMIT ? OFFSET ?';
    params.push(limit, offset);
    const [list] = await db.query(sql, params);
    let countSql = 'SELECT COUNT(*) as total FROM templates WHERE 1=1';
    const countParams = [];
    if (category) { countSql += ' AND category = ?'; countParams.push(category); }
    if (difficulty) { countSql += ' AND difficulty = ?'; countParams.push(difficulty); }
    const [[{ total }]] = await db.query(countSql, countParams);
    res.json({ code: 0, data: { list, total } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 模板分类列表
router.get('/categories', async (req, res) => {
  try {
    const [rows] = await db.query('SELECT DISTINCT category FROM templates');
    res.json({ code: 0, data: { categories: rows.map(r => r.category) } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 热门模板
router.get('/popular', async (req, res) => {
  try {
    const limit = parseInt(req.query.limit) || 10;
    const [list] = await db.query('SELECT id, name, image_url, category, size, difficulty, usage_count FROM templates ORDER BY usage_count DESC LIMIT ?', [limit]);
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

// 搜索模板
router.get('/search', async (req, res) => {
  try {
    const keyword = req.query.keyword ? `%${req.query.keyword}%` : '%';
    const [list] = await db.query(
      'SELECT id, name, image_url, category, size, difficulty, usage_count FROM templates WHERE name LIKE ? OR category LIKE ? ORDER BY usage_count DESC LIMIT 20',
      [keyword, keyword]
    );
    res.json({ code: 0, data: { list } });
  } catch (err) { res.json({ code: 500, message: '搜索失败' }); }
});

// 使用模板
router.post('/:id/use', async (req, res) => {
  try {
    await db.query('UPDATE templates SET usage_count = usage_count + 1 WHERE id = ?', [req.params.id]);
    res.json({ code: 0, data: { success: true } });
  } catch (err) { res.json({ code: 500, message: '操作失败' }); }
});

// 模板详情 (/:id 必须在所有静态路由之后)
router.get('/:id', async (req, res) => {
  try {
    const [rows] = await db.query('SELECT * FROM templates WHERE id = ?', [req.params.id]);
    if (rows.length === 0) return res.json({ code: 404, message: '模板不存在' });
    const template = rows[0];
    const pixels = template.pixel_data ? JSON.parse(template.pixel_data.toString()) : null;
    delete template.pixel_data;
    res.json({ code: 0, data: { template, pixels } });
  } catch (err) { res.json({ code: 500, message: '获取失败' }); }
});

module.exports = router;
