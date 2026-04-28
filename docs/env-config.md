# Glowxel 环境配置约定

说明：本文中的 `Glowxel` 指仓库 / 工程代号。对外命名统一为：官网 / 网站端 / 小程序 `光格像素工坊`，设备 `Glowxel PixelBoard`，公司 `仁光数字科技有限公司`（英文名 `RenLight`）。

## 目标

统一 `server`、`website`、`admin`、`uniapp` 的接口地址配置入口，避免页面内散落硬编码。

## server

配置来源：

- `server/.env`
- `server/.env.example`

当前约定：

- `DB_HOST`
- `DB_PORT`
- `DB_USER`
- `DB_PASSWORD`
- `DB_NAME`
- `JWT_SECRET`
- `WX_APPID`
- `WX_SECRET`
- `DEVICE_API_KEY`

说明：

- 上传接口返回地址不再写死域名，改为按当前请求的协议和 Host 动态生成
- 数据库初始化不再直接依赖 `init.sql` 手动执行，统一走 `npm run migrate`

## website

配置来源：

- `website/.env.development`
- `website/.env.production`
- `website/.env.example`

当前约定：

- `VITE_API_BASE_URL`
- `VITE_API_PROXY_TARGET`

规则：

- 本地开发：`VITE_API_BASE_URL=/api`，通过 `VITE_API_PROXY_TARGET` 代理到本地后端
- 生产构建：`VITE_API_BASE_URL=/api`

## admin

配置来源：

- `admin/.env.development`
- `admin/.env.production`
- `admin/.env.example`

当前约定：

- `VITE_API_BASE_URL`
- `VITE_API_PROXY_TARGET`

规则：

- 本地开发：`VITE_API_BASE_URL=/api`，通过 `VITE_API_PROXY_TARGET` 代理到本地后端
- 生产构建：`VITE_API_BASE_URL=/api`

## uniapp

配置来源：

- `uniapp/config/api.js`

当前约定：

- `API_BASE_URL`

规则：

- 当前仓库先收口为单一配置入口，避免页面内继续硬编码域名
- 切换本地、测试、生产环境时，只修改 `uniapp/config/api.js`
- 因当前本机缺少 `uni` CLI，尚未接入统一的编译期环境变量方案；后续可在 HBuilderX / uni-cli 环境补齐

## 切换顺序

1. `server` 先确认 `.env` 正确
2. `website` 与 `admin` 按目标环境切换 `.env.*`
3. `uniapp` 按目标环境修改 `uniapp/config/api.js`
4. 构建前执行对应测试或构建命令

## 当前验证状态

- `server` 测试通过
- `website` 构建通过
- `admin` 构建通过
- `uniapp` API 配置检查通过
- `uniapp` 尚未完成本机编译验证
