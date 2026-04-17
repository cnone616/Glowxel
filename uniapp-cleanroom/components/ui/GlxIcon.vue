<template>
  <text class="glx-icon iconfont" :class="'icon-' + resolvedName" :style="iconStyle"></text>
</template>

<script>
const ICON_ALIASES = {
  delete: "ashbin",
  trash: "ashbin",
  home: "browse",
  profile: "user",
  device: "mobile-phone",
  create: "edit",
};

export default {
  name: "GlxIcon",
  props: {
    name: {
      type: String,
      required: true,
    },
    size: {
      type: Number,
      default: 32,
    },
    color: {
      type: String,
      default: "currentColor",
    },
  },
  computed: {
    resolvedName() {
      return ICON_ALIASES[this.name] || this.name;
    },
    iconStyle() {
      return {
        fontSize: `${this.size}rpx`,
        color: this.color,
      };
    },
  },
};
</script>

<style scoped lang="scss">
@import "../../static/iconfont/iconfont.css";

.glx-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  line-height: 1;
}
</style>
