import { ARTKAL_COLORS_FULL } from "../../../data/artkal-colors-full.js";

function hexToRgb(hex) {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result
    ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16),
      }
    : { r: 0, g: 0, b: 0 };
}

export default {
  methods: {
    toggleDeviceSync() {
      if (this.deviceConnected) {
        this.deviceStore.disconnect();
        this.toast.showInfo("设备已断开");
      } else {
        this.showConnectModal = true;
      }
    },

    async ensureCanvasModeAndSync() {
      try {
        const ws = this.deviceStore.getWebSocket();
        if (!ws) {
          return;
        }

        await ws.send({ cmd: "set_mode", mode: "canvas" });
        await new Promise((resolve) => setTimeout(resolve, 200));
        await this.syncToDevice();
      } catch (err) {
        console.error("切换模式或同步失败:", err);
        this.toast.showError("同步失败");
      }
    },

    async handleConnectConfirm(ip) {
      try {
        const result = await this.deviceStore.connect(ip);
        if (result.success) {
          setTimeout(() => {
            this.ensureCanvasModeAndSync();
          }, 500);

          this.$refs.connectModal.onSuccess();
          this.toast.showSuccess("设备已连接");
        } else {
          this.$refs.connectModal.onError("连接失败，请检查 IP 地址");
        }
      } catch (err) {
        console.error("连接失败:", err);
        this.$refs.connectModal.onError("连接失败，请检查 IP 地址");
      }
    },

    handleConnectCancel() {
      // 取消连接
    },

    async syncToDevice() {
      if (!this.deviceConnected) {
        console.log("设备未连接，跳过同步");
        return;
      }

      if (this.localPixels.size === 0) {
        console.log("没有像素数据，跳过同步");
        return;
      }

      try {
        console.log("开始同步画布到设备，模式:", this.assistMode);
        const sparsePixels = [];

        this.localPixels.forEach((hexColor, key) => {
          const [x, y] = key.split(",").map(Number);
          const colorObj = ARTKAL_COLORS_FULL.find(
            (color) => color.hex.toLowerCase() === hexColor.toLowerCase(),
          );

          if (!colorObj) {
            return;
          }

          const rgb = hexToRgb(colorObj.hex);
          let r = rgb.r;
          let g = rgb.g;
          let b = rgb.b;

          if (this.assistMode === "row" && y !== this.currentRow) {
            r = Math.floor(r * 0.2);
            g = Math.floor(g * 0.2);
            b = Math.floor(b * 0.2);
          } else if (
            this.assistMode === "color" &&
            this.highlightColor &&
            hexColor.toLowerCase() !== this.highlightColor.toLowerCase()
          ) {
            r = Math.floor(r * 0.2);
            g = Math.floor(g * 0.2);
            b = Math.floor(b * 0.2);
          }

          sparsePixels.push(x, y, r, g, b);
        });

        console.log(
          `准备发送 ${sparsePixels.length / 5} 个有效像素（稀疏格式）`,
        );

        await this.deviceStore.sendSparseImage(sparsePixels);
        console.log("画布同步成功");
      } catch (err) {
        console.error("同步到设备失败:", err);
        this.toast.showError("同步失败");
      }
    },

    async updateRowBrightness() {
      if (!this.deviceConnected) {
        console.log("设备未连接，跳过行亮度更新");
        return;
      }

      if (this.assistMode !== "row") {
        console.log("不在逐行模式，跳过行亮度更新");
        return;
      }

      try {
        const ws = this.deviceStore.getWebSocket();
        if (!ws) {
          console.error("WebSocket 未初始化");
          return;
        }

        console.log("发送高亮行命令:", this.currentRow);
        await ws.send({
          cmd: "highlight_row",
          row: this.currentRow,
        });

        this.lastSyncedRow = this.currentRow;
      } catch (err) {
        console.error("更新行亮度失败:", err);
        console.log("回退到完整同步");
        await this.syncToDevice();
      }
    },

    async updateHighlightBrightness(previousColor, newColor) {
      if (!this.deviceConnected) {
        console.log("设备未连接，跳过高亮更新");
        return;
      }

      if (this.assistMode !== "color") {
        console.log("不在颜色模式，跳过高亮更新");
        return;
      }

      try {
        const ws = this.deviceStore.getWebSocket();
        if (!ws) {
          console.error("WebSocket 未初始化");
          return;
        }

        if (newColor) {
          const colorObj = ARTKAL_COLORS_FULL.find(
            (color) => color.hex.toLowerCase() === newColor.toLowerCase(),
          );

          if (colorObj) {
            const rgb = hexToRgb(colorObj.hex);
            console.log("发送高亮颜色命令:", colorObj.code, rgb);
            await ws.send({
              cmd: "highlight_color",
              color: rgb,
            });
          }
        } else {
          console.log("发送取消高亮命令");
          await ws.send({
            cmd: "highlight_color",
            color: null,
          });
        }
      } catch (err) {
        console.error("更新高亮亮度失败:", err);
        console.log("回退到完整同步");
        await this.syncToDevice();
      }
    },
  },
};
