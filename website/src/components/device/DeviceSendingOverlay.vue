<template>
  <div v-if="visible" class="device-sending-overlay">
    <div class="device-sending-overlay__preview">
      <slot />
    </div>

    <div class="device-sending-overlay__panel">
      <div class="device-sending-overlay__loader">
        <GlxInlineLoader size="sm" />
      </div>
      <strong class="device-sending-overlay__title">{{ title }}</strong>
      <p class="device-sending-overlay__desc">{{ description }}</p>
    </div>
  </div>
</template>

<script setup>
import GlxInlineLoader from "@/components/glx/GlxInlineLoader.vue";

defineProps({
  visible: {
    type: Boolean,
    default: false,
  },
  title: {
    type: String,
    required: true,
  },
  description: {
    type: String,
    required: true,
  },
});
</script>

<style scoped>
.device-sending-overlay {
  position: absolute;
  inset: 0;
  z-index: 2;
  display: grid;
  align-content: center;
  gap: 14px;
  padding: 18px;
  background: rgba(8, 10, 14, 0.9);
}

.device-sending-overlay__preview {
  border: 2px solid #000000;
  background: #000000;
  overflow: hidden;
}

.device-sending-overlay__panel {
  display: grid;
  gap: 8px;
  padding: 14px 16px;
  border: 2px solid #000000;
  background: #ffffff;
  box-shadow: 4px 4px 0 #000000;
}

.device-sending-overlay__loader {
  display: inline-flex;
  justify-content: flex-start;
}

.device-sending-overlay__title {
  color: #000000;
  font-size: 14px;
  font-weight: 900;
}

.device-sending-overlay__desc {
  margin: 0;
  color: #444444;
  font-size: 12px;
  line-height: 1.6;
}
</style>
