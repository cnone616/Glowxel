<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">WiFi Setup</span>
      <h1 class="glx-page-shell__title">热点配网</h1>
      <p class="glx-page-shell__desc">
        Web 端这里提供正式配网引导。浏览器侧不引入新的 BLE 依赖，继续沿用设备热点与本地门户的正式链路。
      </p>
    </section>

    <section class="glx-grid glx-grid--three">
      <article class="glx-section-card glx-section-card--stack">
        <strong class="glx-section-title">1. 连接设备热点</strong>
        <p class="glx-page-shell__desc">在电脑或手机 WiFi 列表里连接设备热点，只使用 `2.4GHz WiFi` 完成首次配网。</p>
      </article>
      <article class="glx-section-card glx-section-card--stack">
        <strong class="glx-section-title">2. 打开本地门户</strong>
        <p class="glx-page-shell__desc">浏览器打开 <code>http://192.168.4.1</code>，进入设备配网页。</p>
      </article>
      <article class="glx-section-card glx-section-card--stack">
        <strong class="glx-section-title">3. 写入 2.4GHz WiFi</strong>
        <p class="glx-page-shell__desc">选择家庭网络并保存，设备重启后回到局域网。</p>
      </article>
    </section>

    <section class="glx-grid glx-grid--two">
      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">正式主链</h2>
          <span class="glx-section-meta">固定步骤</span>
        </div>
        <div class="glx-stack">
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">先连设备热点</strong>
              <span class="glx-list-card__desc">不要跳过热点连接直接猜测门户地址，主路径固定从设备热点开始。</span>
            </div>
          </div>
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">固定门户地址</strong>
              <span class="glx-list-card__desc">门户地址固定为 `192.168.4.1`，当前页面不引入新的浏览器 BLE 依赖。</span>
            </div>
          </div>
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">完成后回到设备控制</strong>
              <span class="glx-list-card__desc">设备重启进入局域网后，回到设备控制页重新连接并刷新状态。</span>
            </div>
          </div>
        </div>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">检查项</h2>
          <span class="glx-section-meta">配网前后</span>
        </div>
        <div class="glx-stack">
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">配网前</strong>
              <span class="glx-list-card__desc">确认系统网络已经切到设备热点，否则 `192.168.4.1` 不会打开正确门户。</span>
            </div>
          </div>
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">配网后</strong>
              <span class="glx-list-card__desc">返回设备控制页并刷新状态，确认设备地址和模式回显已经恢复。</span>
            </div>
          </div>
        </div>
      </article>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-inline-actions">
        <a href="http://192.168.4.1" target="_blank" rel="noreferrer" class="glx-button glx-button--primary">打开 192.168.4.1</a>
        <router-link to="/device-control" class="glx-button glx-button--ghost">回到设备控制</router-link>
      </div>
      <p class="glx-page-shell__desc">
        如果当前电脑没有自动切到设备热点，请先确认系统网络已经连上设备热点，再打开本地门户。
      </p>
    </section>
  </div>
</template>
