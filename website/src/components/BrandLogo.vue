<template>
  <div class="brand-logo" :class="`brand-logo--${variant}`">
    <img :src="iconUrl" :alt="alt" class="brand-mark" />
    <div v-if="showText" class="brand-copy">
      <span class="brand-name">{{ name }}</span>
      <span v-if="showTagline" class="brand-tagline">{{ tagline }}</span>
    </div>
  </div>
</template>

<script setup>
import iconUrl from "@/assets/images/glowxel-icon.svg";

defineProps({
  alt: {
    type: String,
    default: "光格像素工坊 Logo",
  },
  name: {
    type: String,
    default: "光格像素工坊",
  },
  showText: {
    type: Boolean,
    default: true,
  },
  showTagline: {
    type: Boolean,
    default: true,
  },
  tagline: {
    type: String,
    default: "PIXEL STUDIO",
  },
  variant: {
    type: String,
    default: "nav",
  },
});
</script>

<style scoped>
.brand-logo {
  display: inline-flex;
  align-items: center;
  gap: 12px;
  min-width: 0;
}

.brand-mark {
  width: 40px;
  height: 40px;
  display: block;
  flex-shrink: 0;
  image-rendering: pixelated;
}

.brand-copy {
  display: flex;
  flex-direction: column;
  gap: 4px;
  min-width: 0;
}

.brand-name {
  color: var(--nb-ink);
  font-size: 18px;
  font-weight: 700;
  letter-spacing: 0.02em;
  line-height: 1;
  white-space: nowrap;
}

.brand-tagline {
  display: inline-flex;
  align-items: center;
  width: fit-content;
  padding: 4px 9px 3px;
  color: var(--nb-ink);
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.16em;
  line-height: 1;
  border: 1px solid rgba(215, 178, 109, 0.28);
  border-radius: 999px;
  background: rgba(215, 178, 109, 0.18);
  white-space: nowrap;
}

.brand-logo--footer {
  align-items: flex-start;
  gap: 14px;
}

.brand-logo--footer .brand-mark {
  width: 52px;
  height: 52px;
}

.brand-logo--footer .brand-name {
  font-size: 24px;
}

@media (max-width: 768px) {
  .brand-mark {
    width: 34px;
    height: 34px;
  }

  .brand-name {
    font-size: 16px;
  }

  .brand-tagline {
    display: none;
  }
}

@media (max-width: 480px) {
  .brand-name {
    font-size: 15px;
    letter-spacing: 0.02em;
  }
}
</style>
