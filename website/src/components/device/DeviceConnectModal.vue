<template>
  <div v-if="visible" class="device-connect-modal">
    <button
      type="button"
      class="device-connect-modal__scrim"
      aria-label="关闭连接弹窗"
      @click="handleCancel"
    ></button>

    <section class="device-connect-modal__card">
      <span class="glx-page-shell__eyebrow">Connect Device</span>
      <h2 class="device-connect-modal__title">连接设备</h2>
      <p class="device-connect-modal__desc">
        输入 Glowxel PixelBoard 的局域网 IP 地址，网站会直接通过 WebSocket 建立连接。
      </p>

      <label class="glx-field">
        <span class="glx-field__label">设备 IP 地址</span>
        <input
          v-model="draftValue"
          class="glx-input"
          placeholder="例如 192.168.31.138"
          :disabled="loading"
          @keyup.enter="handleConfirm"
        />
      </label>

      <p v-if="errorMessage.length > 0" class="device-connect-modal__error">
        {{ errorMessage }}
      </p>

      <div class="glx-inline-actions">
        <button
          type="button"
          class="glx-button glx-button--primary"
          :disabled="loading || draftValue.trim().length === 0"
          @click="handleConfirm"
        >
          {{ loading ? "连接中..." : "确认连接" }}
        </button>
        <button
          type="button"
          class="glx-button glx-button--ghost"
          :disabled="loading"
          @click="handleCancel"
        >
          取消
        </button>
      </div>
    </section>
  </div>
</template>

<script setup>
import { ref, watch } from "vue";

const props = defineProps({
  visible: {
    type: Boolean,
    default: false,
  },
  defaultValue: {
    type: String,
    default: "",
  },
  loading: {
    type: Boolean,
    default: false,
  },
  errorMessage: {
    type: String,
    default: "",
  },
});

const emit = defineEmits(["confirm", "cancel", "update:visible"]);

const draftValue = ref("");

watch(
  () => props.visible,
  (visible) => {
    if (visible) {
      draftValue.value = props.defaultValue;
    }
  },
  { immediate: true },
);

watch(
  () => props.defaultValue,
  (value) => {
    if (!props.visible) {
      draftValue.value = value;
    }
  },
);

function handleCancel() {
  if (props.loading) {
    return;
  }
  emit("cancel");
  emit("update:visible", false);
}

function handleConfirm() {
  emit("confirm", draftValue.value);
}
</script>

<style scoped>
.device-connect-modal {
  position: fixed;
  inset: 0;
  z-index: 1500;
  display: grid;
  place-items: center;
  padding: 24px;
}

.device-connect-modal__scrim {
  position: absolute;
  inset: 0;
  border: 0;
  background: rgba(17, 17, 17, 0.54);
  cursor: pointer;
}

.device-connect-modal__card {
  position: relative;
  z-index: 1;
  width: min(100%, 480px);
  padding: 24px;
  display: grid;
  gap: 16px;
  border: var(--glx-shell-border-strong);
  background: #ffffff;
  box-shadow: var(--glx-shadow-strong);
}

.device-connect-modal__title {
  color: var(--nb-ink);
  font-size: 28px;
  line-height: 1.05;
  font-weight: 900;
}

.device-connect-modal__desc {
  color: var(--nb-text-secondary);
  font-size: 14px;
  line-height: 1.75;
}

.device-connect-modal__error {
  padding: 10px 12px;
  border: var(--glx-shell-border);
  background: #ffe2e2;
  color: var(--nb-ink);
  font-size: 13px;
  font-weight: 800;
  line-height: 1.6;
}

@media (max-width: 640px) {
  .device-connect-modal {
    padding: 16px;
  }

  .device-connect-modal__card {
    padding: 20px;
  }
}
</style>
