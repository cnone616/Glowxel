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
  gap: 5px;
  min-width: 0;
}

.brand-name {
  color: var(--nb-ink);
  font-size: 18px;
  font-weight: 900;
  letter-spacing: 0.04em;
  line-height: 1;
  white-space: nowrap;
}

.brand-tagline {
  display: inline-flex;
  align-items: center;
  width: fit-content;
  padding: 4px 9px 3px;
  color: var(--nb-ink);
  font-size: 11px;
  font-weight: 900;
  letter-spacing: 0.18em;
  line-height: 1;
  border: var(--nb-border-width) solid var(--nb-ink);
  background: var(--nb-yellow);
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
