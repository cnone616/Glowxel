import { getClockTextWidth } from "../../../utils/clockCanvas.js";

export default {
  methods: {
    startLoading() {
      this.stopLoading();
      const ws = this.deviceStore.getWebSocket();
      ws.send({ cmd: "start_loading" }).catch(() => {});
    },

    stopLoading() {
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
        hasGif: !!(this.gifAnimationData && this._gifParser),
        pixels: Array.from(this.canvasPixels.entries()),
        imagePixels: this.imagePixels
          ? Array.from(this.imagePixels.entries())
          : null,
      };

      console.log("保存配置:", {
        模式: this.clockMode,
        手绘像素数量: this.canvasPixels.size,
        图片像素数量: this.imagePixels ? this.imagePixels.size : 0,
        hasGif: saveData.hasGif,
      });

      const storageKey =
        this.clockMode === "animation"
          ? "clock_config_animation"
          : "clock_config_static";

      uni.setStorageSync(storageKey, JSON.stringify(saveData));
      console.log(`配置已保存到 ${storageKey}`);
    },

    loadConfig() {
      const storageKey =
        this.clockMode === "animation"
          ? "clock_config_animation"
          : "clock_config_static";

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

          this.canvasPixels =
            savedData.pixels && Array.isArray(savedData.pixels)
              ? new Map(savedData.pixels)
              : new Map();

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
            "配置加载成功，手绘像素数量:",
            this.canvasPixels.size,
            "图片像素数量:",
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
        this.canvasPixels = new Map();
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
      try {
        const ws = this.deviceStore.getWebSocket();

        const sendAndWait = (data, timeoutMs = 10000) => {
          return new Promise((resolve, reject) => {
            let resolved = false;
            const handler = (msg) => {
              if (resolved) return;

              try {
                const resp =
                  typeof msg === "string"
                    ? JSON.parse(msg)
                    : msg.data
                      ? JSON.parse(msg.data)
                      : msg;

                const message = resp.message || "";
                if (
                  message === "loading displayed" ||
                  message === "loading started" ||
                  message === "loading stopped" ||
                  message === "pong" ||
                  message === "pixels_received"
                ) {
                  return;
                }

                if (resp.error) {
                  resolved = true;
                  ws.offMessage(handler);
                  clearTimeout(timer);
                  reject(
                    new Error(
                      `ESP32 错误: ${resp.error}${resp.details ? " - " + resp.details : ""}`,
                    ),
                  );
                  return;
                }

                resolved = true;
                ws.offMessage(handler);
                clearTimeout(timer);
                resolve(resp);
              } catch (e) {
                resolved = true;
                ws.offMessage(handler);
                clearTimeout(timer);
                resolve(msg);
              }
            };

            ws.onMessage(handler);
            const timer = setTimeout(() => {
              if (!resolved) {
                resolved = true;
                ws.offMessage(handler);
                reject(new Error("ESP32 响应超时"));
              }
            }, timeoutMs);
            ws.send(data).catch((err) => {
              if (!resolved) {
                resolved = true;
                ws.offMessage(handler);
                clearTimeout(timer);
                reject(err);
              }
            });
          });
        };

        const timeText = this.getTimeText();
        const dateText = this.getDateText();
        const weekText = this.getWeekText();

        await sendAndWait({ cmd: "set_mode", mode: this.clockMode });
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

        await sendAndWait({
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

          let resp = await sendAndWait({
            cmd: "set_mode",
            mode: "transferring",
          });
          if (resp.status !== "ok") {
            this.toast.showError("进入传输模式失败");
            return;
          }

          this.startLoading();
          const { frameCount, frames } = this.gifAnimationData;

          resp = await sendAndWait({ cmd: "animation_begin", frameCount });
          console.log("animation_begin 回复:", JSON.stringify(resp));

          for (let i = 0; i < frames.length; i++) {
            const frame = frames[i];
            const type = frame[0];
            const delay = frame[1];
            const totalPixels = frame[2];
            const pixels = frame[3];

            let CHUNK_SIZE;
            if (totalPixels <= 100) {
              CHUNK_SIZE = totalPixels;
            } else if (totalPixels <= 500) {
              CHUNK_SIZE = 200;
            } else {
              CHUNK_SIZE = 200;
            }

            resp = await sendAndWait({
              cmd: "frame_begin",
              index: i,
              type,
              delay,
              totalPixels,
            });
            console.log(
              `帧 ${i + 1}/${frameCount} 开始二进制传输 (${totalPixels} 像素, chunk=${CHUNK_SIZE}):`,
              resp.status,
            );
            if (resp.status === "error") {
              this.toast.showError(`帧 ${i} 初始化失败: ${resp.message}`);
              return;
            }

            const totalChunks = Math.ceil(totalPixels / CHUNK_SIZE);
            const pixelsArray =
              pixels instanceof Uint8Array ? pixels : new Uint8Array(pixels);
            console.log(
              `  pixels 类型: ${pixels.constructor.name}, 长度: ${pixels.length || pixels.byteLength}`,
            );

            for (let offset = 0; offset < totalPixels; offset += CHUNK_SIZE) {
              const end = Math.min(offset + CHUNK_SIZE, totalPixels);
              const chunkIndex = Math.floor(offset / CHUNK_SIZE);
              const binaryData = new Uint8Array(
                pixelsArray.subarray(offset * 5, end * 5),
              );
              const independentBuffer = binaryData.buffer.slice(
                binaryData.byteOffset,
                binaryData.byteOffset + binaryData.byteLength,
              );

              console.log(
                `  [Buffer] byteOffset=${binaryData.byteOffset}, byteLength=${binaryData.byteLength}, independentBuffer.byteLength=${independentBuffer.byteLength}`,
              );

              const pixelCount = end - offset;
              const byteCount = binaryData.length;

              let invalidCount = 0;
              for (let p = 0; p < pixelCount; p++) {
                const px = binaryData[p * 5];
                const py = binaryData[p * 5 + 1];
                if (px >= 64 || py >= 64) {
                  invalidCount++;
                  if (invalidCount <= 3) {
                    console.warn(`  ⚠️ 越界像素: [${px},${py}]`);
                  }
                }
              }

              console.log(
                `  [发送] chunk ${chunkIndex + 1}/${totalChunks}: ${pixelCount} 像素, ${byteCount} 字节` +
                  (invalidCount > 0 ? ` ⚠️ ${invalidCount} 个越界` : ""),
              );

              if (offset === 0 && binaryData.length >= 15) {
                console.log(
                  `  [前端] 前3像素: ` +
                    `[${binaryData[0]},${binaryData[1]},${binaryData[2]},${binaryData[3]},${binaryData[4]}] ` +
                    `[${binaryData[5]},${binaryData[6]},${binaryData[7]},${binaryData[8]},${binaryData[9]}] ` +
                    `[${binaryData[10]},${binaryData[11]},${binaryData[12]},${binaryData[13]},${binaryData[14]}]`,
                );
              }

              await new Promise((resolve, reject) => {
                ws.socket.send({
                  data: independentBuffer,
                  success: () => {
                    resolve();
                  },
                  fail: (err) => {
                    console.error(`chunk ${chunkIndex + 1} 发送失败:`, err);
                    reject(err);
                  },
                });
              });

              if (offset + CHUNK_SIZE < totalPixels) {
                const delayMs = pixelCount > 500 ? 200 : 100;
                await new Promise((resolve) => setTimeout(resolve, delayMs));
              }
            }

            await new Promise((resolve) => setTimeout(resolve, 300));

            resp = await sendAndWait({
              cmd: "frame_status",
              index: i,
            });
            console.log(
              `  [确认] 帧 ${i}: ${resp.status}, 累计像素: ${resp.count || 0}`,
            );
            if (resp.status === "error") {
              this.toast.showError(`帧 ${i} 接收失败: ${resp.message}`);
              return;
            }

            console.log(`✅ 帧 ${i + 1}/${frameCount} 完成`);
          }

          resp = await sendAndWait({ cmd: "animation_end" });
          console.log("animation_end 回复:", JSON.stringify(resp));
          this.stopLoading();
          this.toast.showSuccess(`GIF 动画已发送！${frameCount} 帧`);
        } else {
          const allPixels = new Map();

          if (this.canvasPixels) {
            this.canvasPixels.forEach((color, key) => {
              allPixels.set(key, color);
            });
          }

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

        this.saveConfig();
      } catch (err) {
        console.error("发送失败:", err);
        this.toast.showError("发送失败: " + err.message);
      } finally {
        this.stopLoading();
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
