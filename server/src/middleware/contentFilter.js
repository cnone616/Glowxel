/**
 * 内容过滤中间件
 * - 拦截脏话/敏感词
 * - 拦截长串数字（防止电话/QQ/微信号引流）
 * - 拦截常见联系方式格式
 */

// 敏感词列表（可持续扩充）
const BANNED_WORDS = [
  // 脏话
  '傻逼', '操你', '妈的', '草泥马', 'sb', 'cnm', 'nmsl', '滚蛋', '废物', '垃圾人',
  '死妈', '你妈', '狗日', '狗逼', '臭逼', '贱人', '婊子', '妓女', '王八蛋',
  // 引流词
  '加我', '私聊', '联系我', '加微信', '加qq', '加QQ', '扫码', '二维码',
  '微信号', 'wx号', 'WX号', '威信', 'V信',
];

// 长串数字检测（8位以上连续数字，可能是手机/QQ/微信号）
const LONG_NUMBER_RE = /\d[\d\s\-]{7,}\d/;

// 联系方式格式
const CONTACT_RE = /(\+?86[\s\-]?)?(1[3-9]\d{9})|([Vv][Xx]?[:：\s]\S+)|(qq[:：\s]\d+)/i;

// 检测内容是否违规，返回原因或 null
function detectViolation(text) {
  if (!text || typeof text !== 'string') return null;
  const lower = text.toLowerCase();

  for (const word of BANNED_WORDS) {
    if (lower.includes(word.toLowerCase())) {
      return '内容包含违禁词汇';
    }
  }

  if (LONG_NUMBER_RE.test(text)) {
    return '内容包含疑似联系方式（长串数字）';
  }

  if (CONTACT_RE.test(text)) {
    return '内容包含联系方式，禁止引流';
  }

  return null;
}

/**
 * 过滤指定字段的中间件工厂
 * @param {string[]} fields - 要检测的 body 字段名，默认 ['content']
 */
function contentFilter(fields = ['content']) {
  return (req, res, next) => {
    for (const field of fields) {
      const value = req.body?.[field];
      if (!value) continue;
      const reason = detectViolation(value);
      if (reason) {
        return res.json({ code: 400, message: reason });
      }
    }
    next();
  };
}

module.exports = { contentFilter, detectViolation };

