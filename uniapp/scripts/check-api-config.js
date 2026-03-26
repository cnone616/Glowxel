const fs = require('fs');
const path = require('path');

const projectRoot = path.resolve(__dirname, '..');
const apiConfigPath = path.join(projectRoot, 'config', 'api.js');
const requestPath = path.join(projectRoot, 'api', 'request.js');
const editProfilePath = path.join(projectRoot, 'pages', 'edit-profile', 'edit-profile.vue');

function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function readFile(filePath) {
  return fs.readFileSync(filePath, 'utf8');
}

function main() {
  const apiConfigContent = readFile(apiConfigPath);
  const requestContent = readFile(requestPath);
  const editProfileContent = readFile(editProfilePath);

  assert(apiConfigContent.includes('export const API_BASE_URL ='), '缺少统一 API_BASE_URL 配置');
  assert(apiConfigContent.includes('export const getUploadUrl ='), '缺少统一上传地址生成函数');
  assert(requestContent.includes("import { API_BASE_URL } from '../config/api'"), 'request.js 未从统一配置入口读取 API_BASE_URL');
  assert(editProfileContent.includes('getUploadUrl'), '编辑资料页未使用统一上传地址入口');

  const scanTargets = [];
  collectFiles(path.join(projectRoot, 'api'), scanTargets);
  collectFiles(path.join(projectRoot, 'pages'), scanTargets);
  collectFiles(path.join(projectRoot, 'utils'), scanTargets);

  const hardcodedHits = [];
  for (const filePath of scanTargets) {
    const content = readFile(filePath);
    if (content.includes('https://glowxel.com/api/upload')) {
      hardcodedHits.push(path.relative(projectRoot, filePath));
    }
  }

  assert(hardcodedHits.length === 0, `发现散落的上传硬编码: ${hardcodedHits.join(', ')}`);
  console.log('uniapp API 配置检查通过');
}

function collectFiles(dirPath, target) {
  if (!fs.existsSync(dirPath)) {
    return;
  }

  for (const entry of fs.readdirSync(dirPath, { withFileTypes: true })) {
    const fullPath = path.join(dirPath, entry.name);
    if (entry.isDirectory()) {
      collectFiles(fullPath, target);
      continue;
    }

    if (entry.isFile() && (entry.name.endsWith('.js') || entry.name.endsWith('.vue'))) {
      target.push(fullPath);
    }
  }
}

try {
  main();
} catch (error) {
  console.error(error.message);
  process.exit(1);
}
