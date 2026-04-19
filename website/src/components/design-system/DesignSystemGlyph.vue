<template>
  <svg v-if="name === 'home'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M3 11.5 12 4l9 7.5" />
    <path d="M6 10.5V20h12v-9.5" />
  </svg>
  <svg v-else-if="name === 'community'" class="ds-glyph" viewBox="0 0 24 24">
    <circle cx="8" cy="9" r="3" />
    <circle cx="16.5" cy="8" r="2.5" />
    <path d="M3.5 19c.8-2.8 3.2-4.5 6.5-4.5s5.7 1.7 6.5 4.5" />
    <path d="M14.5 17.8c.4-1.8 1.9-3 4-3 1.1 0 2 .3 2.8 1" />
  </svg>
  <svg v-else-if="name === 'create'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M4 18.5l4.5-1 8.7-8.7a1.6 1.6 0 0 0 0-2.3L15.5 4.8a1.6 1.6 0 0 0-2.3 0L4.5 13.5 4 18.5z" />
    <path d="m12.5 5.5 6 6" />
  </svg>
  <svg v-else-if="name === 'device'" class="ds-glyph" viewBox="0 0 24 24">
    <rect x="4" y="4" width="16" height="16" rx="1" />
    <path d="M8 8h8v8H8z" />
    <path d="M12 1.5V4M12 20v2.5M1.5 12H4M20 12h2.5" />
  </svg>
  <svg v-else-if="name === 'profile'" class="ds-glyph" viewBox="0 0 24 24">
    <circle cx="12" cy="8" r="4" />
    <path d="M4 20c1.5-3.5 4.1-5.2 8-5.2S18.5 16.5 20 20" />
  </svg>
  <svg v-else-if="name === 'search'" class="ds-glyph" viewBox="0 0 24 24">
    <circle cx="11" cy="11" r="6.5" />
    <path d="M16 16l4.5 4.5" />
  </svg>
  <svg v-else-if="name === 'setting'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M12 3.5 13.4 5.7l2.6.4 1.8 1.8-.4 2.6 2.2 1.4-2.2 1.4.4 2.6-1.8 1.8-2.6.4L12 20.5l-1.4-2.2-2.6-.4-1.8-1.8.4-2.6-2.2-1.4 2.2-1.4-.4-2.6 1.8-1.8 2.6-.4z" />
    <circle cx="12" cy="12" r="3.2" />
  </svg>
  <svg v-else-if="name === 'palette'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M12 4.5c-4.7 0-8.5 3.4-8.5 7.8S7 19.5 10.8 19.5h1.2c1 0 1.8-.8 1.8-1.8 0-.6-.2-1.1-.6-1.5-.4-.4-.6-.9-.6-1.4 0-1.1.9-2 2-2h1.5c3 0 5.4-2.2 5.4-5 0-4.6-4.2-8.3-9.5-8.3Z" />
    <circle cx="7.7" cy="10.2" r="1" />
    <circle cx="10.2" cy="8.2" r="1" />
    <circle cx="13.2" cy="8" r="1" />
    <circle cx="16" cy="10.1" r="1" />
  </svg>
  <svg v-else-if="name === 'edit'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M4 20l4.8-1 8.5-8.5-3.8-3.8L5 15.2 4 20z" />
    <path d="m12.5 5.5 3.8 3.8" />
  </svg>
  <svg v-else-if="name === 'copy'" class="ds-glyph" viewBox="0 0 24 24">
    <rect x="8" y="8" width="11" height="11" rx="1" />
    <path d="M5 15V5h10" />
  </svg>
  <svg v-else-if="name === 'trash'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M4.5 7.5h15" />
    <path d="M9 7.5V5h6v2.5" />
    <path d="m7 7.5 1 12h8l1-12" />
    <path d="M10 11v5.5M14 11v5.5" />
  </svg>
  <svg v-else-if="name === 'check'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="m5 12.5 4.5 4.5L19 7.5" />
  </svg>
  <svg v-else-if="name === 'warning'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M12 4 20 18H4l8-14z" />
    <path d="M12 9v4.8" />
    <path d="M12 17.5h.1" />
  </svg>
  <svg v-else-if="name === 'favorite'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M12 20.2 5.2 13.8A4.4 4.4 0 0 1 11.5 7l.5.5.5-.5a4.4 4.4 0 0 1 6.3 6.1L12 20.2Z" />
  </svg>
  <svg v-else-if="name === 'picture'" class="ds-glyph" viewBox="0 0 24 24">
    <rect x="4" y="5" width="16" height="14" rx="1" />
    <circle cx="9" cy="10" r="1.7" />
    <path d="m6.5 17 4.2-4.3 2.7 2.7 1.7-1.7L18 17" />
  </svg>
  <svg v-else-if="name === 'comment'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M5 6.5h14v10H10l-4 3v-13Z" />
  </svg>
  <svg v-else-if="name === 'notification'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M8 17h8" />
    <path d="M10 19.5a2.2 2.2 0 0 0 4 0" />
    <path d="M6.5 17V11a5.5 5.5 0 1 1 11 0v6l1.5 1.5h-14Z" />
  </svg>
  <svg v-else-if="name === 'sync'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M20 12a8 8 0 0 1-13.7 5.7" />
    <path d="M4 12A8 8 0 0 1 17.7 6.2" />
    <path d="M15.5 4.8h2.8v2.8" />
    <path d="M8.5 19.2H5.7v-2.8" />
  </svg>
  <svg v-else-if="name === 'link'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M10 14 8.2 15.8a3.3 3.3 0 1 1-4.7-4.7l3.2-3.2a3.3 3.3 0 0 1 4.7 0" />
    <path d="M14 10l1.8-1.8a3.3 3.3 0 0 1 4.7 4.7l-3.2 3.2a3.3 3.3 0 0 1-4.7 0" />
    <path d="M8.5 15.5 15.5 8.5" />
  </svg>
  <svg v-else-if="name === 'help'" class="ds-glyph" viewBox="0 0 24 24">
    <circle cx="12" cy="12" r="9" />
    <path d="M9.6 9.4a2.6 2.6 0 1 1 4.5 1.8c-.8.8-1.6 1.3-1.6 2.5" />
    <path d="M12 17.6h.1" />
  </svg>
  <svg v-else-if="name === 'work'" class="ds-glyph" viewBox="0 0 24 24">
    <rect x="4" y="7" width="16" height="11" rx="1" />
    <path d="M9 7V5h6v2" />
    <path d="M4 11h16" />
  </svg>
  <svg v-else-if="name === 'arrow-right'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="m8 5 8 7-8 7" />
  </svg>
  <svg v-else-if="name === 'arrow-left'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="m16 5-8 7 8 7" />
  </svg>
  <svg v-else-if="name === 'arrow-down'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="m5 9 7 8 7-8" />
  </svg>
  <svg v-else-if="name === 'close'" class="ds-glyph" viewBox="0 0 24 24">
    <path d="M6 6 18 18" />
    <path d="M18 6 6 18" />
  </svg>
</template>

<script setup>
defineProps({
  name: {
    type: String,
    required: true,
  },
});
</script>
