<template>
  <div v-if="toasts.length > 0" class="glx-toast-viewport">
    <article
      v-for="item in toasts"
      :key="item.id"
      class="glx-toast"
      :class="`glx-toast--${item.type}`"
    >
      <strong class="glx-toast__title">{{ item.title }}</strong>
      <p class="glx-toast__message">{{ item.message }}</p>
    </article>
  </div>
</template>

<script setup>
import { storeToRefs } from "pinia";
import { useFeedbackStore } from "@/stores/feedback.js";

const feedbackStore = useFeedbackStore();
const { toasts } = storeToRefs(feedbackStore);
</script>
