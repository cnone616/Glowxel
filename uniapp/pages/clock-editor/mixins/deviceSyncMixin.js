import { getClockTextWidth } from "../../../utils/clockCanvas.js";
import { uploadAnimationFrames } from "../../../utils/animationUploader.js";

export default {
  methods: {
    getClockEditorStorageKey() {
      if (this.clockMode === "animation") {
        return "clock_config_animation";
      }
      if (this.clockMode === "theme") {
        return "clock_config_theme";
      }
      return "clock_config_static";
    },

    startLoading() {
      this.stopLoading();
      this.loadingActive = true;
      const ws = this.deviceStore.getWebSocket();
      ws.send({ cmd: "start_loading" }).catch(() => {});
    },

    stopLoading() {
      if (!this.loadingActive) {
        if (this.loadingTimer) {
          clearInterval(this.loadingTimer);
          this.loadingTimer = null;
        }
        return;
      }

      this.loadingActive = false;
      const ws = this.deviceStore.getWebSocket();
      ws.send({ cmd: "stop_loading" }).catch(() => {});
      if (this.loadingTimer) {
        clearInterval(this.loadingTimer);
        this.loadingTimer = null;
      }
    },

    saveConfig() {
      const configToSave = {
        font: this.config.font,
        showSeconds: this.config.showSeconds,
        hourFormat: this.config.hourFormat,
        time: {
          show: this.config.time.show,
          fontSize: this.config.time.fontSize,
          x: this.config.time.x,
          y: this.config.time.y,
          color: this.config.time.color,
          align: this.config.time.align,
        },
        date: {
          show: this.config.date.show,
          fontSize: this.config.date.fontSize,
          x: this.config.date.x,
          y: this.config.date.y,
          color: this.config.date.color,
          align: this.config.date.align,
        },
        week: {
          show: this.config.week.show,
          x: this.config.week.x,
          y: this.config.week.y,
          color: this.config.week.color,
          align: this.config.week.align,
        },
        image: {
          show: this.config.image.show,
          x: this.config.image.x,
          y: this.config.image.y,
          width: this.config.image.width,
          height: this.config.image.height,
          data: null,
        },
      };

      const saveData = {
        config: configToSave,
        clockMode: this.clockMode,
        themeId: this.lastAppliedClockThemeId,
        hasGif: !!(this.gifAnimationData && this._gifParser),
        imagePixels: this.imagePixels
          ? Array.from(this.imagePixels.entries())
          : null,
      };

      console.log("保存配置:", {
        模式: this.clockMode,
        图片像素数量: this.imagePixels ? this.imagePixels.size : 0,
        hasGif: saveData.hasGif,
      });

      const storageKey = this.getClockEditorStorageKey();

      uni.setStorageSync(storageKey, JSON.stringify(saveData));
      console.log(`配置已保存到 ${storageKey}`);
    },

    loadConfig() {
      const storageKey = this.getClockEditorStorageKey();

      const saved = uni.getStorageSync(storageKey);
      console.log(`从 ${storageKey} 加载配置，模式: ${this.clockMode}`);

      if (saved) {
        try {
          const savedData = JSON.parse(saved);

          if (savedData.config) {
            if (savedData.config.font !== undefined) {
              this.config.font = savedData.config.font;
            }
            if (savedData.config.showSeconds !== undefined) {
              this.config.showSeconds = savedData.config.showSeconds;
            }
            if (savedData.config.hourFormat !== undefined) {
              this.config.hourFormat = savedData.config.hourFormat;
            }

            this.config.time = {
              ...this.config.time,
              ...savedData.config.time,
            };
            this.config.date = {
              ...this.config.date,
              ...savedData.config.date,
            };
            this.config.week = {
              ...this.config.week,
              ...savedData.config.week,
            };
            this.config.image = {
              ...this.config.image,
              ...savedData.config.image,
            };
          }

          if (typeof savedData.themeId === "string") {
            this.lastAppliedClockThemeId = savedData.themeId;
          }

          this.imagePixels =
            savedData.imagePixels && Array.isArray(savedData.imagePixels)
              ? new Map(savedData.imagePixels)
              : null;

          if (this.clockMode === "animation" && savedData.hasGif) {
            const waitForCanvas = () => {
              if (this.canvasCtx) {
                this._restoreGifFromLocal();
              } else {
                setTimeout(waitForCanvas, 50);
              }
            };
            this.$nextTick(waitForCanvas);
          } else {
            this.gifAnimationData = null;
            this.gifRenderedFrameMaps = null;
            this.gifFrameIndex = 0;
            this._gifParser = null;
            this.stopGifAnimation();
          }

          console.log(
            "配置加载成功，图片像素数量:",
            this.imagePixels ? this.imagePixels.size : 0,
            "模式:",
            this.clockMode,
            "hasGif:",
            savedData.hasGif,
          );

          if (this.canvasCtx) {
            this.drawCanvas();
          }
        } catch (e) {
          console.error("加载配置失败:", e);
        }
      } else {
        console.log(`${storageKey} 无保存数据，使用默认配置`);
        this.imagePixels = null;
        this.gifAnimationData = null;
        this.gifRenderedFrameMaps = null;
        this.gifFrameIndex = 0;
        this._gifParser = null;
        this.stopGifAnimation();
      }
    },

    async sendToDevice() {
      if (!this.deviceStore.connected) {
        this.toast.showError("设备未连接");
        return;
      }
      if (this.isSending) {
        this.toast.showInfo("正在传输中，请等待完成");
        return;
      }

      this.isSending = true;
      this.canvasHidden = true;
      const ws = this.deviceStore.getWebSocket();
      try {
        const sendCommand = async (data, delayMs = 120) => {
          await ws.send(data);
          if (delayMs > 0) {
            await new Promise((resolve) => setTimeout(resolve, delayMs));
          }
        };

        const timeText = this.getTimeText();
        const dateText = this.getDateText();
        const weekText = this.getWeekText();
        ws.pauseHeartbeat();

        if (this.clockMode === "theme") {
          if (!this.lastAppliedClockThemeId) {
            throw new Error("请先选择主题");
          }
          if (
            this.displayClockThemePreset &&
            this.displayClockThemePreset.requiresImage &&
            !this.imagePixels
          ) {
            throw new Error("当前主题需要先上传图片");
          }

          await sendCommand({ cmd: "set_mode", mode: "theme" });
          await sendCommand({
            cmd: "set_theme_config",
            themeId: this.lastAppliedClockThemeId,
          });

          const allPixels = new Map();
          if (this.imagePixels) {
            const offsetX = this.config.image.x || 0;
            const offsetY = this.config.image.y || 0;
            this.imagePixels.forEach((color, key) => {
              const [rx, ry] = key.split(",").map(Number);
              const finalX = rx + offsetX;
              const finalY = ry + offsetY;
              if (finalX >= 0 && finalX < 64 && finalY >= 0 && finalY < 64) {
                allPixels.set(`${finalX},${finalY}`, color);
              }
            });
          }

          if (allPixels.size > 0) {
            const pixelArray = [];
            allPixels.forEach((color, key) => {
              const [x, y] = key.split(",").map(Number);
              const rgb = this.hexToRgb(color);
              pixelArray.push({ x, y, r: rgb.r, g: rgb.g, b: rgb.b });
            });
            await this.sendImagePixelsBinary(pixelArray);
            this.toast.showSuccess(
              `主题已发送到设备并保存，包含 ${pixelArray.length} 个像素点`,
            );
          } else {
            this.toast.showSuccess("主题已发送到设备并保存");
          }

          this.deviceStore.setDeviceMode("theme", { businessMode: true });
          this.saveConfig();
          return;
        }

        await sendCommand({ cmd: "set_mode", mode: this.clockMode });
        console.log("模式切换成功");

        const timeAlign = this.config.time.align || "left";
        const timeSize = this.config.time.fontSize || 1;
        let timeX = this.config.time.x;
        if (timeAlign === "center") {
          timeX =
            timeX -
            Math.floor(
              getClockTextWidth(timeText, this.config.font, timeSize) / 2,
            );
        } else if (timeAlign === "right") {
          timeX =
            timeX - getClockTextWidth(timeText, this.config.font, timeSize);
        }

        const dateAlign = this.config.date.align || "left";
        const dateSize = this.config.date.fontSize || 1;
        let dateX = this.config.date.x;
        if (dateAlign === "center") {
          dateX =
            dateX -
            Math.floor(
              getClockTextWidth(dateText, this.config.font, dateSize) / 2,
            );
        } else if (dateAlign === "right") {
          dateX =
            dateX - getClockTextWidth(dateText, this.config.font, dateSize);
        }

        const weekAlign = this.config.week.align || "left";
        let weekX = this.config.week.x;
        if (weekAlign === "center") {
          weekX =
            weekX -
            Math.floor(getClockTextWidth(weekText, this.config.font, 1) / 2);
        } else if (weekAlign === "right") {
          weekX = weekX - getClockTextWidth(weekText, this.config.font, 1);
        }

        await sendCommand({
          cmd: "set_clock_config",
          clockMode: this.clockMode,
          config: {
            font: this.config.font,
            showSeconds: this.config.showSeconds,
            hourFormat: this.config.hourFormat,
            time: {
              show: this.config.time.show,
              fontSize: this.config.time.fontSize,
              x: timeX,
              y: this.config.time.y,
              color: this.hexToRgb(this.config.time.color),
            },
            date: {
              show: this.config.date.show,
              fontSize: this.config.date.fontSize,
              x: dateX,
              y: this.config.date.y,
              color: this.hexToRgb(this.config.date.color),
            },
            week: {
              show: this.config.week.show,
              x: weekX,
              y: this.config.week.y,
              color: this.hexToRgb(this.config.week.color),
            },
            image: {
              show: this.config.image.show,
              x: this.config.image.x,
              y: this.config.image.y,
              width: this.config.image.width,
              height: this.config.image.height,
            },
          },
        });

        console.log("闹钟配置发送成功");

        if (
          this.clockMode === "animation" &&
          this.gifAnimationData &&
          this.gifAnimationData.frameCount > 0
        ) {
          if (this._gifParser) {
            const targetW = this.config.image.width || 64;
            const targetH = this.config.image.height || 64;
            this.gifAnimationData = this._gifParser.generateESP32Data(
              targetW,
              targetH,
              20,
              null,
              this.config.image.x || 0,
              this.config.image.y || 0,
              this.gifRenderedFrameMaps,
            );
          }
          console.log(`发送 GIF 动画: ${this.gifAnimationData.frameCount} 帧`);

          this.startLoading();
          const { frameCount, frames } = this.gifAnimationData;
          const uploadFrames = frames.map((frame, index) => {
            if (!Array.isArray(frame) || frame.length < 4) {
              throw new Error(`第 ${index + 1} 帧数据格式错误`);
            }

            return {
              type: frame[0],
              delay: frame[1],
              totalPixels: frame[2],
              pixels:
                frame[3] instanceof Uint8Array
                  ? frame[3]
                  : new Uint8Array(frame[3]),
            };
          });

          await uploadAnimationFrames(ws, uploadFrames, this.clockMode);
          this.stopLoading();
          this.toast.showSuccess(`GIF 动画已发送！${frameCount} 帧`);
        } else {
          const allPixels = new Map();

          if (this.imagePixels) {
            const offsetX = this.config.image.x || 0;
            const offsetY = this.config.image.y || 0;
            this.imagePixels.forEach((color, key) => {
              const [rx, ry] = key.split(",").map(Number);
              const finalX = rx + offsetX;
              const finalY = ry + offsetY;
              if (finalX >= 0 && finalX < 64 && finalY >= 0 && finalY < 64) {
                allPixels.set(`${finalX},${finalY}`, color);
              }
            });
          }

          if (allPixels.size > 0) {
            console.log("准备发送像素数据，总数:", allPixels.size);

            const pixelArray = [];
            allPixels.forEach((color, key) => {
              const [x, y] = key.split(",").map(Number);
              const rgb = this.hexToRgb(color);
              pixelArray.push({ x, y, r: rgb.r, g: rgb.g, b: rgb.b });
            });

            await this.sendImagePixelsBinary(pixelArray);
            this.toast.showSuccess(
              `已发送到设备并保存！包含 ${pixelArray.length} 个像素点`,
            );
          } else {
            this.toast.showSuccess("已发送到设备并保存");
          }
        }

        this.deviceStore.setDeviceMode(this.clockMode, { businessMode: true });
        this.saveConfig();
      } catch (err) {
        console.error("发送失败:", err);
        this.toast.showError("发送失败: " + err.message);
      } finally {
        this.stopLoading();
        ws.resumeHeartbeat();
        this.isSending = false;
        this.canvasHidden = false;
      }
    },

    async sendImagePixelsBinary(pixels) {
      const ws = this.deviceStore.getWebSocket();

      console.log(`开始发送像素数据，共 ${pixels.length} 个像素`);

      const batchSize = 64;
      let sentCount = 0;

      for (let i = 0; i < pixels.length; i += batchSize) {
        const batch = pixels.slice(i, Math.min(i + batchSize, pixels.length));
        const binaryData = [];

        for (const pixel of batch) {
          binaryData.push(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);
        }

        const buffer = new Uint8Array(binaryData);

        await new Promise((resolve, reject) => {
          ws.socket.send({
            data: buffer.buffer,
            success: () => {
              sentCount += batch.length;
              if (sentCount % 100 === 0 || sentCount === pixels.length) {
                console.log(
                  `已发送 ${sentCount}/${pixels.length} 个像素 (${Math.round((sentCount / pixels.length) * 100)}%)`,
                );
              }
              resolve();
            },
            fail: (err) => {
              console.error("发送失败:", err);
              reject(err);
            },
          });
        });

        if (i + batchSize < pixels.length) {
          await new Promise((resolve) => setTimeout(resolve, 200));
        }
      }

      console.log("所有像素数据发送完成");
    },
  },
};
