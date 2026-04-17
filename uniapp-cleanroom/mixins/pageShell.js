import { getLayoutMetrics } from "../utils/layout";

export default {
  data() {
    return {
      pageReady: false,
      layoutMetrics: getLayoutMetrics(),
    };
  },
  onLoad() {
    this.finishPageLoading();
  },
  methods: {
    finishPageLoading() {
      setTimeout(() => {
        this.pageReady = true;
      }, 280);
    },
  },
};
