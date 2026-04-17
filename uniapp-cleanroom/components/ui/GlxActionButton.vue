<template>
  <button class="glx-action-button" :class="toneClass" @click="$emit('click')">
    <GlxIcon v-if="icon" :name="icon" :size="iconSize" />
    <text class="glx-action-button__label">{{ label }}</text>
  </button>
</template>

<script>
import GlxIcon from "./GlxIcon.vue";

export default {
  name: "GlxActionButton",
  components: {
    GlxIcon,
  },
  props: {
    label: {
      type: String,
      required: true,
    },
    icon: {
      type: String,
      default: "",
    },
    tone: {
      type: String,
      default: "primary",
    },
    iconSize: {
      type: Number,
      default: 24,
    },
  },
  computed: {
    toneClass() {
      return `glx-action-button--${this.tone}`;
    },
  },
};
</script>

<style scoped lang="scss">
.glx-action-button {
  min-height: 76rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 12rpx;
  padding: 0 26rpx;
  border-radius: 24rpx;
  border: 2rpx solid transparent;
  transition: background-color 180ms ease, border-color 180ms ease, color 180ms ease;
}

.glx-action-button__label {
  color: inherit;
  font-size: 24rpx;
  font-weight: 700;
  line-height: 1;
}

.glx-action-button--primary {
  background: var(--glx-mint);
  border-color: rgba(46, 170, 150, 0.24);
  color: var(--glx-text);
}

.glx-action-button--secondary {
  background: var(--glx-surface);
  border-color: var(--glx-border);
  color: var(--glx-text);
}

.glx-action-button--tool {
  background: rgba(74, 134, 239, 0.1);
  border-color: rgba(74, 134, 239, 0.18);
  color: var(--glx-blue-deep);
}

.glx-action-button--edit {
  background: var(--glx-blue);
  border-color: rgba(47, 107, 218, 0.28);
  color: #ffffff;
}

.glx-action-button--danger {
  background: var(--glx-danger);
  border-color: rgba(207, 73, 84, 0.32);
  color: #ffffff;
}
</style>
