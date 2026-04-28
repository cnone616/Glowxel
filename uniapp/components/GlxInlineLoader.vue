<template>
  <view class="glx-inline-loader">
    <GlxLogoLoader
      :variant="resolvedVariant"
      :cell-size="loaderConfig.cellSize"
      :gap="loaderConfig.gap"
      :padding="loaderConfig.padding"
      :border-width="loaderConfig.borderWidth"
      :shadow-offset="loaderConfig.shadowOffset"
      :surface="surface"
    />
  </view>
</template>

<script>
import GlxLogoLoader from "./GlxLogoLoader.vue";

const SIZE_CONFIG = {
  xs: {
    cellSize: 6,
    gap: 2,
    padding: 0,
    borderWidth: 2,
    shadowOffset: 0,
  },
  sm: {
    cellSize: 8,
    gap: 3,
    padding: 0,
    borderWidth: 2,
    shadowOffset: 0,
  },
  md: {
    cellSize: 10,
    gap: 4,
    padding: 0,
    borderWidth: 2,
    shadowOffset: 0,
  },
  lg: {
    cellSize: 20,
    gap: 6,
    padding: 0,
    borderWidth: 2,
    shadowOffset: 0,
  },
};

export default {
  name: "GlxInlineLoader",

  components: {
    GlxLogoLoader,
  },

  props: {
    variant: {
      type: String,
      default: "flip",
    },
    size: {
      type: String,
      default: "sm",
    },
    surface: {
      type: String,
      default: "plain",
    },
  },

  computed: {
    loaderConfig() {
      if (Object.prototype.hasOwnProperty.call(SIZE_CONFIG, this.size)) {
        return SIZE_CONFIG[this.size];
      }
      return SIZE_CONFIG.sm;
    },

    resolvedVariant() {
      if (typeof this.variant === "string" && this.variant) {
        return this.variant;
      }
      return "flip";
    },
  },
};
</script>

<style scoped>
.glx-inline-loader {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  line-height: 1;
}
</style>
