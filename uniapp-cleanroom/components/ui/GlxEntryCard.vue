<template>
  <view class="glx-entry-card" :class="toneClass" @click="$emit('click')">
    <view class="glx-entry-card__leading">
      <view class="glx-entry-card__icon-shell">
        <GlxIcon :name="icon" :size="28" />
      </view>
      <view class="glx-entry-card__copy">
        <text class="glx-entry-card__title">{{ title }}</text>
        <text class="glx-entry-card__description">{{ description }}</text>
      </view>
    </view>
    <view class="glx-entry-card__trailing">
      <text v-if="meta" class="glx-entry-card__meta">{{ meta }}</text>
      <GlxIcon name="arrow-right" :size="22" class="glx-entry-card__arrow" />
    </view>
  </view>
</template>

<script>
import GlxIcon from "./GlxIcon.vue";

export default {
  name: "GlxEntryCard",
  components: {
    GlxIcon,
  },
  props: {
    icon: {
      type: String,
      default: "picture",
    },
    title: {
      type: String,
      required: true,
    },
    description: {
      type: String,
      default: "",
    },
    meta: {
      type: String,
      default: "",
    },
    tone: {
      type: String,
      default: "mint",
    },
  },
  computed: {
    toneClass() {
      return `glx-entry-card--${this.tone}`;
    },
  },
};
</script>

<style scoped lang="scss">
.glx-entry-card {
  min-height: 132rpx;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18rpx;
  padding: 22rpx;
  border-radius: 30rpx;
  background: rgba(255, 255, 255, 0.94);
  border: 2rpx solid rgba(158, 188, 188, 0.38);
  box-shadow: 0 20rpx 48rpx rgba(20, 50, 54, 0.08);
}

.glx-entry-card__leading {
  flex: 1;
  min-width: 0;
  display: flex;
  align-items: center;
  gap: 18rpx;
}

.glx-entry-card__icon-shell {
  width: 76rpx;
  height: 76rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 24rpx;
  background: rgba(119, 215, 198, 0.18);
}

.glx-entry-card__copy {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.glx-entry-card__title {
  color: inherit;
  font-size: 28rpx;
  font-weight: 800;
  line-height: 1.2;
}

.glx-entry-card__description,
.glx-entry-card__meta {
  color: var(--glx-text-secondary);
  font-size: 22rpx;
  line-height: 1.4;
}

.glx-entry-card__trailing {
  display: flex;
  align-items: center;
  gap: 10rpx;
}

.glx-entry-card__arrow {
  color: inherit;
}

.glx-entry-card--mint {
  color: var(--glx-mint-deep);
}

.glx-entry-card--mint .glx-entry-card__icon-shell {
  background: rgba(119, 215, 198, 0.2);
}

.glx-entry-card--blue {
  color: var(--glx-blue-deep);
}

.glx-entry-card--blue .glx-entry-card__icon-shell {
  background: rgba(74, 134, 239, 0.16);
}

.glx-entry-card--amber {
  color: #bf7d10;
}

.glx-entry-card--amber .glx-entry-card__icon-shell {
  background: rgba(247, 191, 88, 0.18);
}
</style>
