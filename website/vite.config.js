import { defineConfig, loadEnv } from 'vite'
import vue from '@vitejs/plugin-vue'
import { fileURLToPath, URL } from 'node:url'

export default defineConfig(({ command, mode }) => {
  const env = loadEnv(mode, process.cwd(), '')
  const server = {
    port: 3000,
    open: true
  }

  if (command === 'serve') {
    if (!env.VITE_API_PROXY_TARGET) {
      throw new Error('缺少 VITE_API_PROXY_TARGET，请在环境变量中显式配置 API 代理地址')
    }

    server.proxy = {
      '/api': {
        target: env.VITE_API_PROXY_TARGET,
        changeOrigin: true
      }
    }
  }

  return {
    plugins: [vue()],
    resolve: {
      alias: {
        '@': fileURLToPath(new URL('./src', import.meta.url))
      }
    },
    server
  }
})
