/**
 * 项目云同步 API
 */
import { get, post, del } from './request.js'

// 获取云端项目列表
export function getCloudProjects(page = 1, limit = 20) {
  return get('/api/project/list', { page, limit })
}

// 同步单个项目（上传/更新）
export function syncProject(project, pixels) {
  return post('/api/project/sync', { project, pixels })
}

// 批量同步项目
export function batchSyncProjects(projects) {
  return post('/api/project/batch-sync', { projects })
}

// 获取项目详情+像素数据
export function getCloudProject(projectId) {
  return get(`/api/project/${projectId}`)
}

// 删除云端项目
export function deleteCloudProject(projectId) {
  return del(`/api/project/${projectId}`)
}

// 获取同步状态（云端项目数量、最后同步时间）
export function getSyncStatus() {
  return get('/api/project/sync-status')
}
