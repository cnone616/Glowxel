<template>
  <view
    class="glx-switch"
    :class="{ 'is-disabled': disabled }"
    @click.stop="handleToggle"
  >
    <view class="glx-switch-shell" :class="{ active: checked }">
      <view class="glx-switch-thumb"></view>
    </view>
  </view>
</template>

<script>
export default {
  name: "GlxSwitch",
  props: {
    checked: {
      type: Boolean,
      default: false,
    },
    disabled: {
      type: Boolean,
      default: false,
    },
  },
  emits: ["change"],
  methods: {
    handleToggle() {
      if (this.disabled) {
        return;
      }
      this.$emit("change", {
        detail: {
          value: !this.checked,
        },
      });
    },
  },
};
</script>

<style scoped>
.glx-switch {
  display: inline-flex;
  align-items: center;
  justify-content: flex-end;
}

.glx-switch.is-disabled {
  opacity: 0.45;
}
</style>
