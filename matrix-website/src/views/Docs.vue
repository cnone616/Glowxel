<template>
  <div class="docs">
    <div class="container">
      <h1 class="page-title">使用文档</h1>
      
      <div class="docs-content">
        <section class="doc-section">
          <h2>快速开始</h2>
          <p>欢迎使用像素矩阵创作平台！这里是您创作 LED 矩阵作品的最佳工具。</p>
          
          <h3>1. 时钟设计器</h3>
          <p>自定义您的 LED 时钟显示样式：</p>
          <ul>
            <li>调整时间、日期、星期的位置和大小</li>
            <li>选择 9 种预设颜色</li>
            <li>实时预览效果</li>
            <li>保存和导出配置</li>
          </ul>

          <h3>2. 图案编辑器</h3>
          <p>创作您的像素艺术作品：</p>
          <ul>
            <li>64x64 像素画布</li>
            <li>多种绘制工具（画笔、橡皮擦、填充）</li>
            <li>图片导入和转换</li>
            <li>快速操作（镜像、反转等）</li>
          </ul>

          <h3>3. 作品展示</h3>
          <p>浏览社区创作，获取灵感：</p>
          <ul>
            <li>按分类筛选作品</li>
            <li>搜索感兴趣的内容</li>
            <li>点赞和收藏</li>
          </ul>
        </section>

        <section class="doc-section">
          <h2>配置说明</h2>
          
          <h3>时钟配置格式</h3>
          <pre><code>{
  "time": {
    "fontSize": 1,
    "x": 17,
    "y": 18,
    "r": 100, "g": 200, "b": 255
  },
  "date": {
    "show": true,
    "fontSize": 1,
    "x": 3,
    "y": 30,
    "r": 120, "g": 120, "b": 120
  },
  "week": {
    "show": true,
    "x": 23,
    "y": 44,
    "r": 100, "g": 100, "b": 100
  }
}</code></pre>
        </section>

        <section class="doc-section">
          <h2>常见问题</h2>
          
          <h3>如何导出配置？</h3>
          <p>在时钟设计器中点击导出按钮，配置将保存为 JSON 文件。</p>

          <h3>配置如何应用到设备？</h3>
          <p>将导出的配置文件通过 WebSocket 发送到您的 ESP32 设备。</p>

          <h3>支持哪些设备？</h3>
          <p>支持基于 ESP32 的 64x64 LED 矩阵显示屏。</p>
        </section>
      </div>
    </div>
  </div>
</template>

<style scoped>
.docs {
  padding: 40px 0 80px;
  min-height: calc(100vh - 200px);
}

.container {
  max-width: 900px;
  margin: 0 auto;
  padding: 0 20px;
}

.page-title {
  font-size: 48px;
  font-weight: 800;
  margin-bottom: 48px;
  text-align: center;
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.docs-content {
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 24px;
  padding: 48px;
  box-shadow: 0 8px 32px rgba(255, 107, 107, 0.12);
}

.doc-section {
  margin-bottom: 56px;
}

.doc-section:last-child {
  margin-bottom: 0;
}

.doc-section h2 {
  color: #ff6b6b;
  font-size: 32px;
  font-weight: 700;
  margin-bottom: 24px;
  padding-bottom: 12px;
  border-bottom: 3px solid rgba(255, 107, 107, 0.2);
}

.doc-section h3 {
  color: #2d3748;
  font-size: 22px;
  font-weight: 700;
  margin: 28px 0 16px;
}

.doc-section p {
  color: #4a5568;
  line-height: 1.8;
  margin-bottom: 16px;
  font-size: 15px;
}

.doc-section ul {
  color: #4a5568;
  line-height: 1.8;
  margin-left: 24px;
  margin-bottom: 16px;
}

.doc-section li {
  margin-bottom: 10px;
}

pre {
  background: #2d3748;
  border: 2px solid rgba(255, 107, 107, 0.2);
  border-radius: 12px;
  padding: 24px;
  overflow-x: auto;
  margin: 24px 0;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.1);
}

code {
  color: #ffa500;
  font-family: 'SF Mono', 'Monaco', 'Courier New', monospace;
  font-size: 14px;
  line-height: 1.6;
}

@media (max-width: 768px) {
  .docs {
    padding: 24px 0 60px;
  }
  
  .page-title {
    font-size: 36px;
    margin-bottom: 32px;
  }
  
  .docs-content {
    padding: 32px 24px;
    border-radius: 20px;
  }
  
  .doc-section {
    margin-bottom: 40px;
  }
  
  .doc-section h2 {
    font-size: 26px;
    margin-bottom: 20px;
  }
  
  .doc-section h3 {
    font-size: 20px;
    margin: 24px 0 14px;
  }
  
  .doc-section p,
  .doc-section ul {
    font-size: 14px;
  }
  
  pre {
    padding: 20px;
    margin: 20px 0;
    overflow-x: auto;
  }
  
  code {
    font-size: 13px;
  }
}
</style>
