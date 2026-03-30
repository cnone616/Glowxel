<template>
  <div class="create">
    <div class="container">
      <div class="page-header">
        <h1 class="page-title">开始创作</h1>
        <p class="page-desc">选择一种方式开始你的像素创作</p>
      </div>

      <div class="create-grid">
        <router-link to="/editor" class="create-card">
          <div class="card-icon">
            <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M12 19l7-7 3 3-7 7-3-3z"/><path d="M18 13l-1.5-7.5L2 2l3.5 14.5L13 18l5-5z"/><path d="M2 2l7.586 7.586"/><circle cx="11" cy="11" r="2"/></svg>
          </div>
          <h3>像素绘制</h3>
          <p>在画布上自由绘制像素图案，支持多种工具</p>
          <span class="card-arrow">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="5" y1="12" x2="19" y2="12"/><polyline points="12 5 19 12 12 19"/></svg>
          </span>
        </router-link>

        <router-link to="/editor?mode=image" class="create-card">
          <div class="card-icon">
            <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/><polyline points="21 15 16 10 5 21"/></svg>
          </div>
          <h3>图片导入</h3>
          <p>上传图片，智能转换为 Artkal 拼豆像素图案</p>
          <span class="card-arrow">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="5" y1="12" x2="19" y2="12"/><polyline points="12 5 19 12 12 19"/></svg>
          </span>
        </router-link>

        <router-link to="/clock-editor" class="create-card">
          <div class="card-icon">
            <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
          </div>
          <h3>时钟编辑器</h3>
          <p>连接 RenLight 设备，配置字体、时间样式并一键下发</p>
          <span class="card-arrow">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="5" y1="12" x2="19" y2="12"/><polyline points="12 5 19 12 12 19"/></svg>
          </span>
        </router-link>
      </div>

      <div class="section-divider">
        <span>或者从模板开始</span>
      </div>

      <div class="quick-links">
        <router-link to="/templates" class="quick-link">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/><rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/></svg>
          浏览模板库
        </router-link>
        <router-link to="/challenges" class="quick-link">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z"/></svg>
          参与挑战
        </router-link>
      </div>
    </div>
  </div>
</template>

<script setup>
</script>

<style scoped>
.create { padding: 60px 0 80px; }
.page-header { text-align: center; margin-bottom: 48px; }
.page-title { font-size: 32px; font-weight: 700; color: var(--color-text-primary); }
.page-desc { color: var(--color-text-muted); font-size: 15px; margin-top: 8px; }

.create-grid {
  display: grid; grid-template-columns: repeat(3, 1fr); gap: 20px; margin-bottom: 48px;
}

.create-card {
  position: relative;
  background: #fff; border: 1px solid var(--color-border);
  border-radius: var(--radius-lg); padding: 32px 28px;
  text-decoration: none; color: inherit;
  transition: all 0.2s; display: block;
}
.create-card:hover { border-color: var(--color-primary); box-shadow: var(--shadow-md); transform: translateY(-2px); }

.card-icon {
  width: 52px; height: 52px; background: var(--color-bg-muted);
  border-radius: var(--radius-md); display: flex; align-items: center;
  justify-content: center; margin-bottom: 20px; color: var(--color-text-primary);
}
.create-card:hover .card-icon { background: var(--color-primary); color: #fff; }

.create-card h3 { font-size: 17px; font-weight: 600; color: var(--color-text-primary); margin-bottom: 8px; }
.create-card p { font-size: 13px; color: var(--color-text-muted); line-height: 1.6; }

.card-arrow {
  position: absolute; bottom: 28px; right: 28px;
  color: var(--color-text-muted); opacity: 0; transition: opacity 0.2s;
}
.create-card:hover .card-arrow { opacity: 1; color: var(--color-primary); }

.section-divider {
  text-align: center; position: relative; margin: 0 0 32px;
}
.section-divider::before {
  content: ''; position: absolute; top: 50%; left: 0; right: 0;
  height: 1px; background: var(--color-border);
}
.section-divider span {
  position: relative; background: #fff;
  padding: 0 16px; color: var(--color-text-muted); font-size: 13px;
}

.quick-links { display: flex; gap: 12px; justify-content: center; }
.quick-link {
  display: inline-flex; align-items: center; gap: 6px;
  padding: 10px 20px; border: 1px solid var(--color-border);
  border-radius: var(--radius-sm); text-decoration: none;
  color: var(--color-text-secondary); font-size: 14px; transition: all 0.15s;
}
.quick-link:hover { border-color: var(--color-primary); color: var(--color-primary); }

@media (max-width: 768px) {
  .create-grid { grid-template-columns: 1fr; }
  .page-title { font-size: 24px; }
  .create { padding: 40px 0 60px; }
}
</style>
