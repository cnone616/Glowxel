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
  gap: 7px;
  min-width: 0;
}

.brand-name {
  color: var(--nb-ink);
  font-size: 18px;
  font-weight: 800;
  letter-spacing: 0.02em;
  line-height: 1;
  white-space: nowrap;
}

.brand-tagline {
  display: inline-flex;
  align-items: center;
  width: fit-content;
  min-height: 26px;
  padding: 0 10px;
  color: var(--nb-ink);
  font-size: 12px;
  font-weight: 900;
  letter-spacing: 0.16em;
  line-height: 1;
  border: 2px solid var(--nb-ink);
  background: var(--nb-yellow);
  box-shadow: 2px 2px 0 #111111;
  white-space: nowrap;
}

.brand-logo--page {
  gap: 16px;
}

.brand-logo--page .brand-mark {
  width: 54px;
  height: 54px;
}

.brand-logo--page .brand-name {
  font-size: 22px;
}

.brand-logo--page .brand-tagline {
  min-height: 30px;
  padding: 0 12px;
  font-size: 13px;
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
  color: #ffffff;
}

.brand-logo--footer .brand-tagline {
  color: var(--nb-ink);
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
    min-height: 24px;
    padding: 0 8px;
    font-size: 10px;
    border-width: 2px;
    box-shadow: 1px 1px 0 #111111;
  }

  .brand-logo--page {
    gap: 12px;
  }

  .brand-logo--page .brand-mark {
    width: 42px;
    height: 42px;
  }

  .brand-logo--page .brand-name {
    font-size: 18px;
  }
}

@media (max-width: 480px) {
  .brand-name {
    font-size: 15px;
    letter-spacing: 0.02em;
  }
}
</style>
