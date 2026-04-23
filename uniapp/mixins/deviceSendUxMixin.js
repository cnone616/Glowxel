const SEND_BUSY_MESSAGE = "正在传输中，请等待完成";
const SEND_DISCONNECTED_MESSAGE = "设备未连接";
const SEND_SUCCESS_MESSAGE = "已发送到设备";
const SEND_FAILURE_PREFIX = "发送失败：";
const SEND_OVERLAY_TITLE = "正在发送到设备...";
const SEND_OVERLAY_TIP = "请保持连接稳定，完成后会自动恢复交互";

function resolveSendErrorMessage(error) {
  if (error && typeof error.message === "string" && error.message.length > 0) {
    return error.message;
  }

  return "未知错误";
}

export default {
  data() {
    return {
      isSending: false,
      isToastVisible: false,
      sendPreviewKind: "dom",
      sendOverlayTitle: SEND_OVERLAY_TITLE,
      sendOverlayTip: SEND_OVERLAY_TIP,
    };
  },

  computed: {
    shouldHidePreview() {
      return (
        this.sendPreviewKind === "native" &&
        (this.isSending || this.isToastVisible)
      );
    },
  },

  watch: {
    shouldHidePreview(nextValue, previousValue) {
      if (
        previousValue === true &&
        nextValue === false &&
        typeof this.handleDeviceSendPreviewRestored === "function"
      ) {
        this.$nextTick(() => {
          this.handleDeviceSendPreviewRestored();
        });
      }
    },
  },

  methods: {
    beginSendUi() {
      this.sendOverlayTitle = SEND_OVERLAY_TITLE;
      this.sendOverlayTip = SEND_OVERLAY_TIP;
      this.isSending = true;
    },

    endSendUi() {
      this.isSending = false;
    },

    handleToastShow() {
      this.isToastVisible = true;
    },

    handleToastHide() {
      this.isToastVisible = false;
    },

    guardBeforeSend(
      connected,
      disconnectedMessage = SEND_DISCONNECTED_MESSAGE,
      disconnectedType = "error",
    ) {
      if (this.isSending) {
        if (this.toast && typeof this.toast.showInfo === "function") {
          this.toast.showInfo(SEND_BUSY_MESSAGE);
        }
        return false;
      }

      if (!connected) {
        if (
          disconnectedType === "info" &&
          this.toast &&
          typeof this.toast.showInfo === "function"
        ) {
          this.toast.showInfo(disconnectedMessage);
          return false;
        }

        if (this.toast && typeof this.toast.showError === "function") {
          this.toast.showError(disconnectedMessage);
        }
        return false;
      }

      return true;
    },

    showSendSuccess(message = SEND_SUCCESS_MESSAGE) {
      if (this.toast && typeof this.toast.showSuccess === "function") {
        this.toast.showSuccess(message);
      }
    },

    showSendFailure(error) {
      if (this.toast && typeof this.toast.showError === "function") {
        this.toast.showError(
          `${SEND_FAILURE_PREFIX}${resolveSendErrorMessage(error)}`,
        );
      }
    },
  },
};
