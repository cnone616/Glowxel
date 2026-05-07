<template>
  <ClockStandardEditor
    mode="clock"
    eyebrow="Clock"
    title="静态时钟"
    description="按 uniapp `clock-editor` 页面族复刻静态时钟设置页，保留时间、字体、图片、日期、星期五段式编辑路径。"
    preview-title="时钟预览"
  />
</template>

<script setup>
import ClockStandardEditor from "@/components/device/clock/ClockStandardEditor.vue";
</script>
