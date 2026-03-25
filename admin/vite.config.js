import { defineConfig, loadEnv } from 'vite'
import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '')

  return {
    plugins: [vue()],
    resolve: {
      alias: { '@': resolve(__dirname, 'src') }
    },
    server: {
      port: 3001,
      proxy: {
        '/api': {
          target: env.VITE_API_PROXY_TARGET || 'http://175.178.153.146:3000',
          changeOrigin: true
        }
      }
    }
  }
})
