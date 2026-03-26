<template>
  <div class="clock-editor">
    <div class="container">
      <div class="hero">
        <span class="eyebrow">Web 端说明</span>
        <h1>时钟配置当前仅在小程序提供</h1>
        <p>
          官网当前不提供设备时钟的完整编辑、字体切换、板载同步和实时预览。
          如果你要配置像素时钟，请使用 uni-app / 微信小程序内的时钟编辑器与设备控制页面。
        </p>
        <div class="capabilities">
          <div class="capability">
            <strong>Web 端当前适合</strong>
            <span>像素创作、图片导入、模板浏览、社区分享</span>
          </div>
          <div class="capability">
            <strong>请在小程序完成</strong>
            <span>时钟字体、秒钟显示、12/24 小时制、样式调整、设备同步</span>
          </div>
        </div>
        <div class="steps">
          <div class="step">1. 在小程序连接设备</div>
          <div class="step">2. 进入时钟编辑器配置样式</div>
          <div class="step">3. 保存并同步到板载程序</div>
        </div>
        <div class="actions">
          <router-link to="/workspace" class="btn-secondary">返回工作台</router-link>
          <router-link to="/editor" class="btn-primary">继续像素创作</router-link>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.clock-editor { min-height: calc(100vh - 60px); display: flex; align-items: center; }
.container { max-width: 960px; margin: 0 auto; padding: 0 20px; width: 100%; }
.hero {
  background: linear-gradient(135deg, #fff7ed 0%, #ffffff 55%, #eff6ff 100%);
  border: 1px solid var(--color-border);
  border-radius: 24px;
  padding: 56px 40px;
  text-align: center;
  box-shadow: var(--shadow-md);
}
.eyebrow {
  display: inline-block;
  padding: 6px 10px;
  border-radius: 999px;
  background: rgba(249, 115, 22, 0.08);
  color: #c2410c;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.04em;
  margin-bottom: 14px;
}
.hero h1 { font-size: 34px; font-weight: 700; color: var(--color-text-primary); margin-bottom: 12px; }
.hero p { max-width: 560px; margin: 0 auto; font-size: 15px; line-height: 1.7; color: var(--color-text-secondary); }
.capabilities {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
  margin-top: 24px;
  text-align: left;
}
.capability {
  background: rgba(255, 255, 255, 0.85);
  border: 1px solid var(--color-border);
  border-radius: 16px;
  padding: 18px;
}
.capability strong {
  display: block;
  font-size: 14px;
  color: var(--color-text-primary);
  margin-bottom: 8px;
}
.capability span {
  font-size: 13px;
  line-height: 1.7;
  color: var(--color-text-secondary);
}
.steps {
  display: flex;
  justify-content: center;
  gap: 10px;
  flex-wrap: wrap;
  margin-top: 18px;
}
.step {
  padding: 10px 14px;
  border-radius: 999px;
  background: #fff;
  border: 1px solid var(--color-border);
  font-size: 13px;
  color: var(--color-text-secondary);
}
.actions { display: flex; justify-content: center; gap: 12px; margin-top: 28px; flex-wrap: wrap; }
.btn-primary,
.btn-secondary {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 140px;
  padding: 10px 18px;
  border-radius: var(--radius-sm);
  text-decoration: none;
  font-size: 14px;
  font-weight: 600;
}
.btn-primary { background: var(--color-primary); color: #fff; }
.btn-secondary { border: 1px solid var(--color-border); color: var(--color-text-primary); background: #fff; }
@media (max-width: 768px) {
  .hero { padding: 40px 24px; }
  .hero h1 { font-size: 28px; }
  .capabilities { grid-template-columns: 1fr; }
}
</style>
