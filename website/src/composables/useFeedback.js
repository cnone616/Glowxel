import { useFeedbackStore } from "@/stores/feedback.js";

export function useFeedback() {
  const store = useFeedbackStore();

  const push = (type, title, message, duration) => {
    store.pushToast({
      type,
      title,
      message,
      duration,
    });
  };

  return {
    info(title, message, duration) {
      push("info", title, message, duration);
    },
    success(title, message, duration) {
      push("success", title, message, duration);
    },
    warning(title, message, duration) {
      push("warning", title, message, duration);
    },
    error(title, message, duration) {
      push("error", title, message, duration);
    },
    showBlocking(title, message) {
      store.showBlocking(title, message);
    },
    hideBlocking() {
      store.hideBlocking();
    },
  };
}
