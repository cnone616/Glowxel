<template>
  <text
    class="iconfont"
    :class="'icon-' + resolvedName"
    :style="iconStyle"
  ></text>
</template>

<script>
// 先把仓库里历史遗留的图标名收敛到当前可渲染的 iconfont 词表，避免页面出现空白图标。
const FONT_ICON_ALIASES = {
  award: "work",
  bell: "notification",
  bookmark: "favorite",
  check: "check-item",
  "check-circle": "check-item",
  "check-circle-fill": "check-item-filling",
  circle: "success",
  "cloud-check": "success-filling",
  "cloud-off": "stop",
  compass: "navigation",
  "column-3": "3column",
  crown: "top",
  database: "file-common",
  drink: "notification",
  fireworks: "success-filling",
  gallery: "picture",
  grid: "column-4",
  heart: "favorite",
  image: "picture",
  info: "help",
  list: "menu",
  moon: "time",
  "more-horizontal": "more",
  palette: "adjust",
  plus: "add",
  shield: "security",
  star: "favorite",
  target: "map",
  "thumbs-up": "good",
  timer: "time",
  trash: "ashbin",
  trophy: "success-filling",
  users: "user",
  weather: "prompt",
  wifi: "link",
  "x-circle": "close-bold",
  alert: "warning",
};

function normalizeIconName(name) {
  if (typeof name !== "string") {
    return "help";
  }
  const normalized = name.trim();
  if (!normalized) {
    return "help";
  }
  return FONT_ICON_ALIASES[normalized] || normalized;
}

export default {
  name: "Icon",
  props: {
    name: {
      type: String,
      required: true,
    },
    size: {
      type: Number,
      default: 48,
    },
    color: {
      type: String,
      default: "currentColor",
    },
  },
  computed: {
    resolvedName() {
      return normalizeIconName(this.name);
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

<style>
@import url("../static/iconfont/iconfont.css");
</style>
