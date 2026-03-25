<template>
  <div class="clock-editor">
    <div class="container">
      <div class="hero">
        <h1>时钟设计</h1>
        <p>Web 端时钟编辑器还在整理中，当前建议使用 uni-app 里的设备控制和时钟编辑页面完成配置。</p>
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
.hero h1 { font-size: 34px; font-weight: 700; color: var(--color-text-primary); margin-bottom: 12px; }
.hero p { max-width: 560px; margin: 0 auto; font-size: 15px; line-height: 1.7; color: var(--color-text-secondary); }
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
}
</style>
