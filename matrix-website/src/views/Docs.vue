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
  font-size: 42px;
  margin-bottom: 40px;
  text-align: center;
  background: linear-gradient(135deg, #00f3ff 0%, #c864ff 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.docs-content {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 12px;
  padding: 40px;
}

.doc-section {
  margin-bottom: 50px;
}

.doc-section:last-child {
  margin-bottom: 0;
}

.doc-section h2 {
  color: #00f3ff;
  font-size: 28px;
  margin-bottom: 20px;
  padding-bottom: 10px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}

.doc-section h3 {
  color: #fff;
  font-size: 20px;
  margin: 25px 0 15px;
}

.doc-section p {
  color: rgba(255, 255, 255, 0.7);
  line-height: 1.8;
  margin-bottom: 15px;
}

.doc-section ul {
  color: rgba(255, 255, 255, 0.7);
  line-height: 1.8;
  margin-left: 20px;
  margin-bottom: 15px;
}

.doc-section li {
  margin-bottom: 8px;
}

pre {
  background: #0a0a0a;
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  padding: 20px;
  overflow-x: auto;
  margin: 20px 0;
}

code {
  color: #00f3ff;
  font-family: 'Courier New', monospace;
  font-size: 14px;
  line-height: 1.6;
}

@media (max-width: 768px) {
  .docs-content {
    padding: 24px;
  }
}
</style>
