/**
 * 统一 API 导出
 * 页面统一从这里导入，后端就绪后只需修改各模块内部实现
 */
export * as userAPI from './user.js'
export * as artworkAPI from './artwork.js'
export * as followAPI from './follow.js'
export * as collectionAPI from './collection.js'
export * as likeAPI from './like.js'
export * as commentAPI from './comment.js'
export * as templateAPI from './template.js'
export * as challengeAPI from './challenge.js'
export { isMockMode } from './request.js'

