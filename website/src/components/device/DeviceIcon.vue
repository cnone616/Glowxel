<template>
  <svg
    :width="size"
    :height="size"
    viewBox="0 0 24 24"
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
    :style="{ color }"
    aria-hidden="true"
  >
    <template v-if="name === 'wifi'">
      <path d="M3 9.5C8.4 5.2 15.6 5.2 21 9.5" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M6.5 13C10.1 10.3 13.9 10.3 17.5 13" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M10 16.5C11.1 15.7 12.9 15.7 14 16.5" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <circle cx="12" cy="19" r="1.4" fill="currentColor" />
    </template>

    <template v-else-if="name === 'phone'">
      <rect x="7" y="2.8" width="10" height="18.4" rx="0" :stroke="color" :stroke-width="strokeWidth" />
      <line x1="10" y1="5.8" x2="14" y2="5.8" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <circle cx="12" cy="17.8" r="1.2" fill="currentColor" />
    </template>

    <template v-else-if="name === 'disconnect'">
      <path d="M4 12H14" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M10 8L14 12L10 16" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" stroke-linejoin="miter" />
      <path d="M17 4H20V20H17" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
    </template>

    <template v-else-if="name === 'params'">
      <line x1="6" y1="6" x2="18" y2="6" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <line x1="6" y1="12" x2="18" y2="12" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <line x1="6" y1="18" x2="18" y2="18" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <rect x="8.6" y="4.1" width="3.6" height="3.6" :stroke="color" :stroke-width="strokeWidth" fill="#ffffff" />
      <rect x="13.8" y="10.1" width="3.6" height="3.6" :stroke="color" :stroke-width="strokeWidth" fill="#ffffff" />
      <rect x="9.8" y="16.1" width="3.6" height="3.6" :stroke="color" :stroke-width="strokeWidth" fill="#ffffff" />
    </template>

    <template v-else-if="name === 'eyes'">
      <path d="M2.5 12C4.8 8.4 8 6.6 12 6.6C16 6.6 19.2 8.4 21.5 12C19.2 15.6 16 17.4 12 17.4C8 17.4 4.8 15.6 2.5 12Z" :stroke="color" :stroke-width="strokeWidth" stroke-linejoin="miter" />
      <rect x="10.2" y="10.2" width="3.6" height="3.6" fill="currentColor" />
    </template>

    <template v-else-if="name === 'clock'">
      <rect x="4" y="4" width="16" height="16" :stroke="color" :stroke-width="strokeWidth" />
      <line x1="12" y1="8" x2="12" y2="12" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <line x1="12" y1="12" x2="15.5" y2="14" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
    </template>

    <template v-else-if="name === 'animation'">
      <rect x="3" y="5" width="18" height="14" :stroke="color" :stroke-width="strokeWidth" />
      <path d="M10 9L15 12L10 15V9Z" fill="currentColor" />
      <line x1="6" y1="3.5" x2="6" y2="5.5" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <line x1="18" y1="3.5" x2="18" y2="5.5" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <line x1="6" y1="18.5" x2="6" y2="20.5" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <line x1="18" y1="18.5" x2="18" y2="20.5" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
    </template>

    <template v-else-if="name === 'theme'">
      <path d="M12 3C7 3 3.3 6.9 3.3 11.7C3.3 16.4 7.1 20.7 11.7 20.7C13.1 20.7 14 19.8 14 18.6C14 17.7 13.4 17.1 13.4 16.2C13.4 15.3 14.1 14.7 15.2 14.7H16.5C19.5 14.7 21.2 12.7 21.2 10C21.2 6 17.4 3 12 3Z" :stroke="color" :stroke-width="strokeWidth" stroke-linejoin="miter" />
      <rect x="7.1" y="8" width="1.8" height="1.8" fill="currentColor" />
      <rect x="9.8" y="6.6" width="1.8" height="1.8" fill="currentColor" />
      <rect x="12.7" y="7.6" width="1.8" height="1.8" fill="currentColor" />
      <rect x="8.6" y="11" width="1.8" height="1.8" fill="currentColor" />
    </template>

    <template v-else-if="name === 'tetris'">
      <rect x="4" y="4" width="4" height="4" fill="currentColor" />
      <rect x="8" y="4" width="4" height="4" fill="currentColor" />
      <rect x="12" y="4" width="4" height="4" fill="currentColor" />
      <rect x="12" y="8" width="4" height="4" fill="currentColor" />
      <rect x="8" y="12" width="4" height="4" fill="currentColor" />
      <rect x="12" y="12" width="4" height="4" fill="currentColor" />
      <rect x="16" y="12" width="4" height="4" fill="currentColor" />
    </template>

    <template v-else-if="name === 'tetris-clock'">
      <rect x="3.5" y="4" width="3.5" height="3.5" fill="currentColor" />
      <rect x="7" y="4" width="3.5" height="3.5" fill="currentColor" />
      <rect x="10.5" y="4" width="3.5" height="3.5" fill="currentColor" />
      <rect x="10.5" y="7.5" width="3.5" height="3.5" fill="currentColor" />
      <circle cx="17.5" cy="14.5" r="4.2" :stroke="color" :stroke-width="strokeWidth" />
      <line x1="17.5" y1="12.3" x2="17.5" y2="14.6" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <line x1="17.5" y1="14.6" x2="19.4" y2="15.8" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
    </template>

    <template v-else-if="name === 'maze'">
      <path d="M4 7V4H10V10H7V7H4Z" :stroke="color" :stroke-width="strokeWidth" stroke-linejoin="miter" />
      <path d="M14 4H20V10H17V7H14V4Z" :stroke="color" :stroke-width="strokeWidth" stroke-linejoin="miter" />
      <path d="M4 14H7V17H10V20H4V14Z" :stroke="color" :stroke-width="strokeWidth" stroke-linejoin="miter" />
      <path d="M14 14H17V17H20V20H14V14Z" :stroke="color" :stroke-width="strokeWidth" stroke-linejoin="miter" />
      <rect x="10.6" y="10.6" width="2.8" height="2.8" fill="currentColor" />
    </template>

    <template v-else-if="name === 'snake'">
      <path d="M4 9.5H10C11.8 9.5 11.8 14.5 13.6 14.5H18.5C19.8 14.5 20.8 13.5 20.8 12.2C20.8 10.9 19.8 9.9 18.5 9.9H17" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" stroke-linejoin="miter" />
      <rect x="4" y="8" width="2.4" height="2.4" fill="currentColor" />
      <rect x="18.2" y="8.8" width="1.8" height="1.8" fill="currentColor" />
    </template>

    <template v-else-if="name === 'matrix'">
      <rect x="4" y="4" width="4" height="4" :stroke="color" :stroke-width="strokeWidth" />
      <rect x="10" y="4" width="4" height="4" :stroke="color" :stroke-width="strokeWidth" />
      <rect x="16" y="4" width="4" height="4" :stroke="color" :stroke-width="strokeWidth" />
      <rect x="4" y="10" width="4" height="4" fill="currentColor" />
      <rect x="10" y="10" width="4" height="4" :stroke="color" :stroke-width="strokeWidth" />
      <rect x="16" y="10" width="4" height="4" fill="currentColor" />
      <rect x="4" y="16" width="4" height="4" :stroke="color" :stroke-width="strokeWidth" />
      <rect x="10" y="16" width="4" height="4" fill="currentColor" />
      <rect x="16" y="16" width="4" height="4" :stroke="color" :stroke-width="strokeWidth" />
    </template>

    <template v-else-if="name === 'planet'">
      <circle cx="12" cy="12" r="5.2" :stroke="color" :stroke-width="strokeWidth" />
      <ellipse cx="12" cy="12.2" rx="9" ry="2.8" :stroke="color" :stroke-width="strokeWidth" />
      <path d="M9.5 10.1H13.2" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M10.5 13.2H14.8" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
    </template>

    <template v-else-if="name === 'canvas'">
      <rect x="4" y="4" width="16" height="16" :stroke="color" :stroke-width="strokeWidth" />
      <path d="M8 8H16" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M8 12H12" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M8 16H10" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M15.5 12.5L18.8 15.8" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M14.4 16.9L12.8 18.5L15.2 19.2L15.9 21.2L17.6 19.6" :stroke="color" :stroke-width="strokeWidth" stroke-linejoin="miter" />
    </template>

    <template v-else-if="name === 'gif'">
      <rect x="3.5" y="5" width="17" height="14" :stroke="color" :stroke-width="strokeWidth" />
      <path d="M7 9.5H10V12.5H8.1" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" stroke-linejoin="miter" />
      <path d="M12 9.5V14.5" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M15 9.5H18.5" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M15 12H17.8" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
      <path d="M15 14.5H18.5" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
    </template>

    <template v-else-if="name === 'water'">
      <path d="M12 4.2C9.8 7.4 7 10.6 7 13.7C7 16.8 9.2 19.2 12 19.2C14.8 19.2 17 16.8 17 13.7C17 10.6 14.2 7.4 12 4.2Z" :stroke="color" :stroke-width="strokeWidth" stroke-linejoin="miter" />
      <path d="M6 18.2C7.6 16.8 9 16.2 10.4 16.2C11.8 16.2 12.6 17.2 14 17.2C15.4 17.2 16 16.2 18 14.8" :stroke="color" :stroke-width="strokeWidth" stroke-linecap="square" />
    </template>
  </svg>
</template>

<script setup>
defineProps({
  name: {
    type: String,
    required: true,
  },
  size: {
    type: Number,
    default: 24,
  },
  color: {
    type: String,
    default: "currentColor",
  },
  strokeWidth: {
    type: Number,
    default: 2,
  },
});
</script>
