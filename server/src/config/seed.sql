-- Matrix 种子数据
USE matrix;

-- 插入示例用户
INSERT INTO users (id, openid, name, avatar, bio, level, followers_count, works_count, total_likes) VALUES
(1, 'seed_user_001', '像素小王子', 'https://picsum.photos/seed/u1/200', '热爱像素艺术的创作者', 5, 128, 12, 356),
(2, 'seed_user_002', '拼豆达人', 'https://picsum.photos/seed/u2/200', '每天一幅像素画', 3, 56, 8, 120),
(3, 'seed_user_003', 'PixelMaster', 'https://picsum.photos/seed/u3/200', '专注游戏角色像素化', 7, 320, 25, 890),
(4, 'seed_user_004', '彩虹工坊', 'https://picsum.photos/seed/u4/200', '色彩搭配爱好者', 4, 89, 15, 210)
ON DUPLICATE KEY UPDATE name=VALUES(name);

-- 插入示例作品
INSERT INTO artworks (id, user_id, title, description, cover_url, width, height, color_count, board_count, difficulty, likes, comments_count, views, collects, status) VALUES
(1, 1, '超级马里奥', '经典游戏角色像素还原', 'https://picsum.photos/seed/art1/200', 32, 32, 12, 1, '中等', 86, 12, 320, 45, 'published'),
(2, 2, '皮卡丘', '宝可梦系列-皮卡丘', 'https://picsum.photos/seed/art2/200', 32, 32, 8, 1, '简单', 152, 28, 580, 78, 'published'),
(3, 3, '星之卡比', '粉色小可爱', 'https://picsum.photos/seed/art3/200', 16, 16, 6, 1, '简单', 67, 5, 210, 32, 'published'),
(4, 1, '林克', '塞尔达传说主角', 'https://picsum.photos/seed/art4/200', 32, 32, 15, 1, '困难', 203, 35, 890, 120, 'published'),
(5, 4, '向日葵', '阳光下的向日葵', 'https://picsum.photos/seed/art5/200', 32, 32, 10, 1, '中等', 45, 8, 156, 22, 'published'),
(6, 3, '吃豆人', '经典街机游戏', 'https://picsum.photos/seed/art6/200', 16, 16, 4, 1, '简单', 98, 15, 420, 55, 'published'),
(7, 2, '独角兽', '梦幻独角兽', 'https://picsum.photos/seed/art7/200', 32, 32, 12, 1, '中等', 134, 20, 510, 68, 'published'),
(8, 4, '樱花树', '春天的樱花', 'https://picsum.photos/seed/art8/200', 32, 32, 8, 1, '中等', 76, 10, 280, 38, 'published')
ON DUPLICATE KEY UPDATE title=VALUES(title);

-- 插入示例模板
INSERT INTO templates (id, name, image_url, category, size, color_count, difficulty, usage_count, description) VALUES
(1, '爱心', 'https://picsum.photos/seed/tpl1/200', '基础图形', '16x16', 2, '简单', 1520, '简单的爱心图案，适合入门'),
(2, '星星', 'https://picsum.photos/seed/tpl2/200', '基础图形', '16x16', 3, '简单', 980, '五角星图案'),
(3, '小猫咪', 'https://picsum.photos/seed/tpl3/200', '动物', '32x32', 8, '中等', 2340, '可爱的小猫咪头像'),
(4, '小狗', 'https://picsum.photos/seed/tpl4/200', '动物', '32x32', 7, '中等', 1870, '柴犬像素画'),
(5, '蘑菇', 'https://picsum.photos/seed/tpl5/200', '游戏', '16x16', 5, '简单', 3200, '超级马里奥蘑菇'),
(6, '宝剑', 'https://picsum.photos/seed/tpl6/200', '游戏', '32x32', 6, '中等', 1450, '像素风宝剑'),
(7, '圣诞树', 'https://picsum.photos/seed/tpl7/200', '节日', '32x32', 10, '困难', 890, '圣诞树装饰'),
(8, '南瓜灯', 'https://picsum.photos/seed/tpl8/200', '节日', '32x32', 6, '中等', 760, '万圣节南瓜灯')
ON DUPLICATE KEY UPDATE name=VALUES(name);

-- 插入示例挑战
INSERT INTO challenges (id, title, banner_url, description, start_date, end_date, status, participants, submissions, prize, difficulty) VALUES
(1, '春日花语挑战', 'https://picsum.photos/seed/ch1/400/200', '用像素画出你心中的春天花朵', '2026-03-01', '2026-03-31', 'active', 56, 23, '限定头像框', '简单'),
(2, '游戏角色大赛', 'https://picsum.photos/seed/ch2/400/200', '还原你最喜欢的游戏角色', '2026-03-10', '2026-04-10', 'active', 128, 45, '实体拼豆套装', '中等'),
(3, '像素美食节', 'https://picsum.photos/seed/ch3/400/200', '画出让人垂涎的美食像素画', '2026-04-01', '2026-04-30', 'upcoming', 12, 0, '美食礼盒', '简单')
ON DUPLICATE KEY UPDATE title=VALUES(title);

