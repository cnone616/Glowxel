<template>
  <div v-if="blocking.visible" class="glx-blocking-layer">
    <section class="glx-blocking-layer__card">
      <GlxLoader />
      <strong class="glx-blocking-layer__title">{{ blocking.title }}</strong>
      <p class="glx-blocking-layer__desc">{{ blocking.message }}</p>
    </section>
  </div>
</template>

<script setup>
import { storeToRefs } from "pinia";
import { useFeedbackStore } from "@/stores/feedback.js";
import GlxLoader from "@/components/glx/GlxLoader.vue";

const feedbackStore = useFeedbackStore();
const { blocking } = storeToRefs(feedbackStore);
</script>
