<template>
  <div class="glx-page-shell ble-config-page">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">WiFi Setup</span>
      <h1 class="glx-page-shell__title">热点配网</h1>
      <p class="glx-page-shell__desc">
        网站端这里复刻小程序热点配网页的正式引导，不引入新的浏览器 BLE 依赖。设备首次联网和重置网络后的回连，都走设备热点加本地门户这条主链。
      </p>
      <div class="glx-inline-actions">
        <a
          href="http://192.168.4.1"
          target="_blank"
          rel="noreferrer"
          class="glx-button glx-button--primary"
        >
          打开 192.168.4.1
        </a>
        <router-link to="/device-control" class="glx-button glx-button--ghost">回到设备控制</router-link>
      </div>
    </section>

    <section class="glx-grid glx-grid--three">
      <DeviceSetupStep
        step="1"
        title="连接设备热点"
        description="在系统 WiFi 列表里选择 Glowxel PixelBoard-设备序列号，先把当前网络切到设备热点。"
      />
      <DeviceSetupStep
        step="2"
        title="打开 192.168.4.1"
        description="进入配网页后可任选一种写法，把家庭 2.4GHz WiFi 信息交给设备。"
        :options="portalOptions"
      />
      <DeviceSetupStep
        step="3"
        title="输入密码并保存"
        description="只支持 2.4GHz，保存后设备会自动重启。"
        note="成功后看屏幕上的新 IP；失败会回到热点模式。"
      />
    </section>

    <section class="glx-grid glx-grid--two">
      <DeviceParamsSection
        title="配网前检查"
        meta="进入门户前"
        description="先确认链路站在正确位置，再打开本地门户。这样能避免把浏览器错误页误认成设备门户。"
      >
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
              <span class="glx-list-card__desc">门户地址固定为 <code>192.168.4.1</code>，当前页面不额外接入 BLE。</span>
            </div>
          </div>
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">只支持 2.4GHz</strong>
              <span class="glx-list-card__desc">如果家里路由器当前是 5GHz-only，设备不会在这一步完成联网。</span>
            </div>
          </div>
        </div>
      </DeviceParamsSection>

      <DeviceParamsSection
        title="配网后判断"
        meta="保存并重启后"
        description="保存后不要停在旧热点页面，按屏幕结果判断设备是否真的回到局域网。"
      >
        <div class="glx-stack">
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">成功标志</strong>
              <span class="glx-list-card__desc">看屏幕上的新 IP，说明设备已经回到局域网，可以回设备控制页重新连接。</span>
            </div>
          </div>
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">失败标志</strong>
              <span class="glx-list-card__desc">如果联网失败，设备会回到热点模式，这时继续连热点并重新打开门户即可。</span>
            </div>
          </div>
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">后续动作</strong>
              <span class="glx-list-card__desc">回到设备控制页并刷新状态，确认设备地址和模式回显已经恢复。</span>
            </div>
          </div>
        </div>
      </DeviceParamsSection>
    </section>

    <DeviceParamsSection
      title="正式主链"
      meta="固定步骤"
      description="这页只负责把小程序正式配网文案和节奏搬到网站端，不单独造另一套浏览器配网流程。"
    >
      <div class="ble-config-page__chain">
        <div class="ble-config-page__chain-item">
          <span class="ble-config-page__chain-index">A</span>
          <div class="ble-config-page__chain-copy">
            <strong>系统先切到设备热点</strong>
            <span>当前电脑或手机如果没连上设备热点，<code>192.168.4.1</code> 不会打开正确门户。</span>
          </div>
        </div>
        <div class="ble-config-page__chain-item">
          <span class="ble-config-page__chain-index">B</span>
          <div class="ble-config-page__chain-copy">
            <strong>门户里写入家庭 WiFi</strong>
            <span>可以扫描后直接选择，也可以手动输入 WiFi 名称，最终都要输入密码并保存。</span>
          </div>
        </div>
        <div class="ble-config-page__chain-item">
          <span class="ble-config-page__chain-index">C</span>
          <div class="ble-config-page__chain-copy">
            <strong>重启后回设备控制页</strong>
            <span>设备拿到新 IP 后，再回网站端设备控制页重新连接，不在这页做额外状态猜测。</span>
          </div>
        </div>
      </div>
    </DeviceParamsSection>
  </div>
</template>

<script setup>
import DeviceParamsSection from "@/components/device/params/DeviceParamsSection.vue";
import DeviceSetupStep from "@/components/device/params/DeviceSetupStep.vue";

const portalOptions = [
  "扫描后直接选择 WiFi",
  "手动输入 WiFi 名称",
];
</script>

<style scoped>
.ble-config-page {
  gap: 20px;
}

.ble-config-page code {
  padding: 2px 6px;
  border: var(--glx-shell-border);
  background: #ffffff;
  color: var(--nb-ink);
  font-size: 0.92em;
  font-weight: 800;
}

.ble-config-page__chain {
  display: grid;
  gap: 12px;
}

.ble-config-page__chain-item {
  display: grid;
  grid-template-columns: 42px minmax(0, 1fr);
  gap: 14px;
  align-items: flex-start;
  padding: 16px;
  border: var(--glx-shell-border);
  background: #ffffff;
}

.ble-config-page__chain-index {
  width: 42px;
  height: 42px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: var(--glx-shell-border);
  background: var(--nb-yellow);
  color: var(--nb-ink);
  font-size: 18px;
  font-weight: 900;
}

.ble-config-page__chain-copy {
  display: grid;
  gap: 4px;
}

.ble-config-page__chain-copy strong {
  color: var(--nb-ink);
  font-size: 15px;
  font-weight: 900;
}

.ble-config-page__chain-copy span {
  color: var(--nb-text-secondary);
  font-size: 13px;
  line-height: 1.7;
}

@media (max-width: 1080px) {
  .ble-config-page :deep(.glx-grid--three) {
    grid-template-columns: minmax(0, 1fr);
  }
}

@media (max-width: 920px) {
  .ble-config-page :deep(.glx-grid--two) {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
