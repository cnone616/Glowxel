import { defineStore } from "pinia";

let feedbackToastId = 0;

export const useFeedbackStore = defineStore("feedback", {
  state: () => ({
    toasts: [],
    blocking: {
      visible: false,
      title: "",
      message: "",
    },
  }),
  actions: {
    pushToast(payload) {
      feedbackToastId += 1;
      const id = feedbackToastId;
      const duration = typeof payload.duration === "number" ? payload.duration : 3200;
      const toast = {
        id,
        type: payload.type,
        title: payload.title,
        message: payload.message,
      };
      this.toasts.push(toast);
      window.setTimeout(() => {
        this.removeToast(id);
      }, duration);
    },
    removeToast(id) {
      this.toasts = this.toasts.filter((item) => item.id !== id);
    },
    showBlocking(title, message) {
      this.blocking = {
        visible: true,
        title,
        message,
      };
    },
    hideBlocking() {
      this.blocking = {
        visible: false,
        title: "",
        message: "",
      };
    },
  },
});
