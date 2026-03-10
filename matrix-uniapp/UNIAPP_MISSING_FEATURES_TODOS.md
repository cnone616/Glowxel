# MATRIX UniApp 缺失功能 TODOs

## 📊 项目现状分析

**当前状态：** 基础框架完整，UI组件齐全，核心编辑功能完善  
**缺失状态：** 详情页面、用户交互功能、后端集成  
**技术栈：** uni-app + 微信小程序 + Pinia + Mock数据

---

## 🚨 高优先级缺失功能（立即需要）

### 1. **作品详情页** ✅ `ArtworkDetailPage`
**文件位置：** `pages/artwork-detail/artwork-detail.vue`  
**当前状态：** ✅ 已完成  
**影响范围：** ✅ 已修复 gallery.vue, community.vue, library.vue 中的 `handleArtworkClick`

**已实现的功能：**
- ✅ 作品大图展示（支持缩放）
- ✅ 作品基本信息（标题、作者、创建时间、尺寸、颜色数）
- ✅ 作者信息卡片（头像、昵称、关注按钮）
- ✅ 交互按钮（点赞、收藏、评论、分享）
- ✅ 评论列表展示
- ✅ 评论输入和发表
- ✅ 相关作品推荐
- ✅ 举报功能
- ✅ 下载/使用作品功能

**路由配置：** ✅ 已添加到 pages.json

### 2. **评论功能系统** ✅ `CommentSystem`
**当前状态：** ✅ 已完成基础功能  
**影响范围：** ✅ 已集成到作品详情页

**已实现：**
- ✅ 评论发表功能
- ✅ 评论列表展示
- ✅ 评论点赞功能
- ✅ 评论时间格式化
- ✅ 评论组件优化

**需要完善的API：**
```javascript
// api/comment.js - 已在Mock API中实现
- addComment(artworkId, content, parentId?) ✅
- getComments(artworkId, page, limit) ✅
- deleteComment(commentId) ✅
- likeComment(commentId) ✅
```

### 3. **用户详情页** ✅ `UserDetailPage`
**文件位置：** `pages/user-detail/user-detail.vue`  
**当前状态：** ✅ 已完成  
**影响范围：** ✅ 已修复 community.vue 中的 `goToUserProfile`

**已实现：**
- ✅ 用户基本信息展示
- ✅ 用户作品列表
- ✅ 关注/取消关注功能
- ✅ 用户统计数据（作品数、粉丝数、关注数）
- ✅ 用户作品分类（全部、最新、最热）
- ✅ 渐变背景设计
- ✅ 操作按钮（关注、私信、分享）

### 4. **用户列表页** ✅ `UserListPage`
**文件位置：** `pages/user-list/user-list.vue`  
**当前状态：** ✅ 已完成  
**影响范围：** 关注列表、粉丝列表、推荐用户

**已实现：**
- ✅ 关注列表展示
- ✅ 粉丝列表展示
- ✅ 推荐用户列表
- ✅ 用户搜索功能
- ✅ 批量关注功能
- ✅ 分页加载

---

## 🟡 中优先级缺失功能（近期需要）

### 5. **模板使用功能** ❌ `TemplateUsage`
**当前状态：** 只有展示，无使用功能  
**影响范围：** gallery.vue, library.vue 中的 `handleTemplateClick`

**需要实现：**
- 模板预览功能
- 使用模板创建画布
- 模板参数配置（尺寸调整等）
- 跳转到编辑器并加载模板数据

### 6. **关注系统** ❌ `FollowSystem`
**当前状态：** UI存在，功能缺失  
**影响范围：** community.vue 中的 `handleUserFollow`

**需要实现：**
- 关注/取消关注用户
- 关注列表页面
- 粉丝列表页面
- 关注动态流

### 7. **用户列表页** ❌ `UserListPage`
**文件位置：** `pages/user-list/user-list.vue`  
**当前状态：** 完全缺失  
**影响范围：** community.vue 中的 `goToUserList`

**需要实现：**
- 推荐用户列表
- 用户搜索功能
- 用户分类（推荐、最新、最活跃）
- 批量关注功能

### 8. **点赞收藏系统** ❌ `LikeCollectSystem`
**当前状态：** 前端UI完整，后端逻辑缺失  
**影响范围：** 所有作品卡片的点赞收藏功能

**需要实现：**
- 点赞状态持久化
- 收藏状态持久化
- 我的点赞列表页面
- 我的收藏列表页面
- 点赞收藏统计

---

## 🟢 低优先级缺失功能（长期规划）

### 9. **搜索结果页** ❌ `SearchResultPage`
**文件位置：** `pages/search-result/search-result.vue`  
**当前状态：** 搜索UI存在，结果页缺失

**需要实现：**
- 搜索结果展示页面
- 搜索历史记录
- 搜索建议
- 高级搜索筛选

### 10. **消息通知系统** ❌ `NotificationSystem`
**需要实现：**
- 消息中心页面
- 点赞通知
- 评论通知
- 关注通知
- 系统通知

### 11. **分享功能** ❌ `ShareSystem`
**需要实现：**
- 微信分享
- 朋友圈分享
- 复制链接分享
- 生成分享海报

---

## 📋 具体实施计划

### Phase 1: 核心详情页面（3天）

#### Day 1: 作品详情页
- [ ] 创建 `pages/artwork-detail/artwork-detail.vue`
- [ ] 实现作品大图展示
- [ ] 实现作品信息展示
- [ ] 实现基础交互按钮
- [ ] 修复 gallery.vue 等页面的跳转

#### Day 2: 评论功能
- [ ] 完善 Comment 组件功能
- [ ] 实现评论发表功能
- [ ] 实现评论列表展示
- [ ] 集成到作品详情页

#### Day 3: 用户详情页
- [ ] 创建 `pages/user-detail/user-detail.vue`
- [ ] 实现用户信息展示
- [ ] 实现用户作品列表
- [ ] 修复 community.vue 的跳转

### Phase 2: 交互功能完善（2天）

#### Day 4: 关注和点赞系统
- [ ] 实现关注/取消关注功能
- [ ] 完善点赞收藏的持久化
- [ ] 创建相关列表页面

#### Day 5: 模板和挑战功能
- [ ] 实现模板使用功能
- [ ] 创建挑战详情页
- [ ] 完善相关跳转逻辑

### Phase 3: 扩展功能（按需）
- [ ] 搜索结果页面
- [ ] 消息通知系统
- [ ] 分享功能
- [ ] 其他优化功能

---

## 🔧 技术实现要点

### 路由配置更新
需要在 `pages.json` 中添加以下页面：
```json
{
  "pages": [
    // 现有页面...
    {
      "path": "pages/artwork-detail/artwork-detail",
      "style": { "navigationStyle": "custom" }
    },
    {
      "path": "pages/user-detail/user-detail", 
      "style": { "navigationStyle": "custom" }
    },
    {
      "path": "pages/challenge-detail/challenge-detail",
      "style": { "navigationStyle": "custom" }
    },
    {
      "path": "pages/user-list/user-list",
      "style": { "navigationStyle": "custom" }
    },
    {
      "path": "pages/search-result/search-result",
      "style": { "navigationStyle": "custom" }
    }
  ]
}
```

### API 接口设计
需要创建以下 API 模块：
- `api/artwork.js` - 作品相关接口
- `api/user.js` - 用户相关接口  
- `api/comment.js` - 评论相关接口
- `api/follow.js` - 关注相关接口
- `api/like.js` - 点赞收藏接口
- `api/challenge.js` - 挑战相关接口

### 状态管理扩展
需要扩展 Pinia Store：
- `store/artwork.js` - 作品状态管理
- `store/comment.js` - 评论状态管理
- `store/follow.js` - 关注状态管理
- `store/notification.js` - 通知状态管理

---

## 📊 完成度统计

| 功能模块 | 总数 | 已完成 | 缺失 | 完成度 |
|---------|------|--------|------|--------|
| 核心页面 | 13 | 10 | 3 | 77% |
| 详情页面 | 4 | 3 | 1 | 75% |
| 交互功能 | 6 | 4 | 2 | 67% |
| 用户功能 | 5 | 4 | 1 | 80% |
| **总计** | **28** | **21** | **7** | **75%** |

---

## 🎯 关键修复点

### 立即需要修复的 TODO 注释：
1. ✅ `gallery.vue:428` - handleArtworkClick 跳转作品详情页
2. ✅ `gallery.vue:450` - handleArtworkComment 评论功能（已集成到详情页）
3. `gallery.vue:484` - handleTemplateClick 使用模板功能
4. `gallery.vue:492` - handleChallengeClick 跳转挑战详情页
5. ✅ `community.vue:334` - handleArtworkClick 跳转作品详情页
6. ✅ `community.vue:357` - handleArtworkComment 评论功能（已集成到详情页）
7. ✅ `community.vue:383` - goToUserProfile 跳转用户详情页
8. `community.vue:391` - goToUserList 跳转用户列表页（如存在）
9. ✅ `library.vue:380` - handleArtworkClick 跳转作品详情页
10. `library.vue:392` - handleTemplateClick 使用模板功能
11. `library.vue:400` - handleChallengeClick 跳转挑战详情页

### 后端集成点：
1. `gallery.vue:327` - 获取关注用户的作品
2. `community.vue:352` - 发送点赞请求到服务器
3. `community.vue:373` - 发送关注请求到服务器

---

**文档创建时间：** 2026-03-10  
**预计完成时间：** 5-7天  
**当前状态：** 待开始实施