<template>
  <article class="device-setup-step">
    <span class="device-setup-step__badge">{{ step }}</span>
    <div class="device-setup-step__body">
      <h3 class="device-setup-step__title">{{ title }}</h3>
      <p class="device-setup-step__desc">{{ description }}</p>
      <div v-if="options.length > 0" class="device-setup-step__options">
        <span
          v-for="option in options"
          :key="option"
          class="device-setup-step__option"
        >
          {{ option }}
        </span>
      </div>
      <p v-if="note.length > 0" class="device-setup-step__note">{{ note }}</p>
    </div>
  </article>
</template>

<script setup>
defineProps({
  step: {
    type: [String, Number],
    required: true,
  },
  title: {
    type: String,
    required: true,
  },
  description: {
    type: String,
    required: true,
  },
  note: {
    type: String,
    default: "",
  },
  options: {
    type: Array,
    default: () => [],
  },
});
</script>

<style scoped>
.device-setup-step {
  display: grid;
  grid-template-columns: 54px minmax(0, 1fr);
  gap: 16px;
  padding: 18px;
  border: var(--glx-shell-border);
  background: #ffffff;
}

.device-setup-step__badge {
  width: 54px;
  height: 54px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: var(--glx-shell-border);
  background: var(--nb-blue);
  color: var(--nb-ink);
  font-size: 24px;
  font-weight: 900;
}

.device-setup-step__body {
  display: grid;
  gap: 8px;
}

.device-setup-step__title {
  color: var(--nb-ink);
  font-size: 18px;
  font-weight: 900;
}

.device-setup-step__desc,
.device-setup-step__note {
  color: var(--nb-text-secondary);
  font-size: 14px;
  line-height: 1.7;
}

.device-setup-step__options {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.device-setup-step__option {
  min-height: 34px;
  display: inline-flex;
  align-items: center;
  padding: 0 10px;
  border: var(--glx-shell-border);
  background: #ffffff;
  color: var(--nb-ink);
  font-size: 12px;
  font-weight: 900;
}

@media (max-width: 640px) {
  .device-setup-step {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
