import { defineConfig, loadEnv } from 'vite'
import vue from '@vitejs/plugin-vue'
import Components from 'unplugin-vue-components/vite'
import { AntDesignVueResolver } from 'unplugin-vue-components/resolvers'
import { resolve } from 'path'

export default defineConfig(({ command, mode }) => {
  const env = loadEnv(mode, process.cwd(), '')
  const server = {
    port: 3001
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
    plugins: [
      vue(),
      Components({
        dts: false,
        resolvers: [AntDesignVueResolver({ importStyle: false })]
      })
    ],
    resolve: {
      alias: { '@': resolve(__dirname, 'src') }
    },
    build: {
      rollupOptions: {
        output: {
          manualChunks(id) {
            if (!id.includes('node_modules')) {
              return
            }

            if (id.includes('@antv')) {
              return 'antv'
            }

            if (
              id.includes('/node_modules/vue/') ||
              id.includes('/node_modules/@vue/') ||
              id.includes('/node_modules/vue-router/') ||
              id.includes('/node_modules/pinia/')
            ) {
              return 'vue-vendor'
            }

            if (id.includes('axios')) {
              return 'axios'
            }

            return
          }
        }
      }
    },
    server
  }
})
