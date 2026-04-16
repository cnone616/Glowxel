import { PERLER_BOARD_SIZE } from "../../../constants/perler.js";

export default {
  methods: {
    handleSave() {
      if (!this.isEditable) {
        this.toast.showInfo("已发布作品不可编辑");
        return;
      }
      this.showSaveConfirm = true;
    },

    handleRename() {
      if (!this.isEditable) {
        this.toast.showInfo("已发布作品不可重命名");
        return;
      }
      if (!this.newProjectName.trim()) {
        this.toast.showError("名称不能为空");
        return;
      }

      this.projectStore.updateProjectName(
        this.projectId,
        this.newProjectName.trim(),
      );

      const updatedProject = this.projectStore.getProject(this.projectId);
      if (updatedProject) {
        this.project = { ...updatedProject };
      }

      this.showRenameModal = false;
      this.newProjectName = "";
      this.toast.showSuccess("重命名成功");
    },

    async confirmSave() {
      if (!this.isEditable) {
        this.toast.showInfo("已发布作品不可编辑");
        return;
      }

      this.projectStore.saveProjectPixels(this.projectId, this.pixels);
      this.savedPixelsSnapshot = JSON.stringify(
        Array.from(this.pixels.entries()),
      );
      this.hasUnsavedChanges = false;

      await this.generateAndSaveThumbnail();

      this.toast.showSuccess("已保存");
      this.showSaveConfirm = false;
    },

    handleBack() {
      if (this.hasUnsavedChanges) {
        this.showLeaveConfirm = true;
      } else {
        uni.navigateBack();
      }
    },

    confirmLeave() {
      this.showLeaveConfirm = false;
      uni.navigateBack();
    },

    async saveAndLeave() {
      if (!this.isEditable) {
        uni.navigateBack();
        return;
      }

      this.projectStore.saveProjectPixels(this.projectId, this.pixels);
      await this.generateAndSaveThumbnail();

      this.toast.showSuccess("已保存");
      this.showLeaveConfirm = false;
      setTimeout(() => {
        uni.navigateBack();
      }, 500);
    },

    async generateAndSaveThumbnail() {
      try {
        const query = uni.createSelectorQuery().in(this);
        query
          .select("#thumbnailCanvas")
          .fields({ node: true, size: true })
          .exec((res) => {
            if (!res || !res[0]) {
              console.error("无法获取缩略图canvas");
              return;
            }

            const canvas = res[0].node;
            const ctx = canvas.getContext("2d");
            const thumbnailSize = 100;
            canvas.width = thumbnailSize;
            canvas.height = thumbnailSize;
            ctx.clearRect(0, 0, thumbnailSize, thumbnailSize);

            const scale = Math.min(
              thumbnailSize / this.project.width,
              thumbnailSize / this.project.height,
            );
            const scaledWidth = this.project.width * scale;
            const scaledHeight = this.project.height * scale;
            const offsetX = (thumbnailSize - scaledWidth) / 2;
            const offsetY = (thumbnailSize - scaledHeight) / 2;

            this.pixels.forEach((color, key) => {
              const [x, y] = key.split(",").map(Number);
              ctx.fillStyle = color;
              ctx.fillRect(
                offsetX + x * scale,
                offsetY + y * scale,
                Math.ceil(scale) + 1,
                Math.ceil(scale) + 1,
              );
            });

            setTimeout(() => {
              uni.canvasToTempFilePath(
                {
                  canvas: canvas,
                  canvasId: "thumbnailCanvas",
                  fileType: "png",
                  quality: 0.8,
                  success: (fileRes) => {
                    // #ifdef MP-WEIXIN
                    const fs = uni.getFileSystemManager();
                    fs.readFile({
                      filePath: fileRes.tempFilePath,
                      encoding: "base64",
                      success: (resBase64) => {
                        const base64 =
                          "data:image/png;base64," + resBase64.data;
                        this.projectStore.updateProjectThumbnail(
                          this.projectId,
                          base64,
                        );
                      },
                      fail: () => {
                        this.projectStore.updateProjectThumbnail(
                          this.projectId,
                          fileRes.tempFilePath,
                        );
                      },
                    });
                    // #endif

                    // #ifdef H5
                    try {
                      const base64 = canvas.toDataURL("image/png", 0.8);
                      this.projectStore.updateProjectThumbnail(
                        this.projectId,
                        base64,
                      );
                    } catch (err) {
                      console.error("生成 Base64 失败:", err);
                      this.projectStore.updateProjectThumbnail(
                        this.projectId,
                        fileRes.tempFilePath,
                      );
                    }
                    // #endif
                  },
                  fail: (err) => {
                    console.error("生成缩略图失败:", err);
                  },
                },
                this,
              );
            }, 100);
          });

        if (this.boardId) {
          await this.generateBoardThumbnail();
        }
      } catch (error) {
        console.error("生成缩略图失败:", error);
      }
    },

    async generateBoardThumbnail() {
      try {
        const query = uni.createSelectorQuery().in(this);
        query
          .select("#boardThumbnailCanvas")
          .fields({ node: true, size: true })
          .exec((res) => {
            if (!res || !res[0]) {
              return;
            }

            const canvas = res[0].node;
            const ctx = canvas.getContext("2d");
            const thumbnailSize = 200;
            canvas.width = thumbnailSize;
            canvas.height = thumbnailSize;
            ctx.clearRect(0, 0, thumbnailSize, thumbnailSize);

            const scale = thumbnailSize / PERLER_BOARD_SIZE;
            this.displayPixels.forEach((color, key) => {
              const [x, y] = key.split(",").map(Number);
              ctx.fillStyle = color;
              ctx.fillRect(
                x * scale,
                y * scale,
                Math.ceil(scale),
                Math.ceil(scale),
              );
            });

            uni.canvasToTempFilePath(
              {
                canvas: canvas,
                success: (fileRes) => {
                  this.projectStore.saveBoardThumbnail(
                    this.projectId,
                    this.boardId,
                    fileRes.tempFilePath,
                  );
                },
                fail: (err) => {
                  console.error("生成看板缩略图失败:", err);
                },
              },
              this,
            );
          });
      } catch (error) {
        console.error("生成看板缩略图失败:", error);
      }
    },

    handleToastShow() {
      this.toastVisible = true;
    },

    handleToastHide() {
      this.toastVisible = false;
    },

    handlePublish() {
      if (this.pixels.size === 0) {
        this.toast.showError("画布为空，无法发布");
        return;
      }

      if (this.hasUnsavedChanges) {
        this.dialog
          .confirm({
            title: "有未保存的修改",
            content: "检测到您有未保存的修改，是否先保存再发布？",
            confirmText: "保存并发布",
            cancelText: "直接发布",
          })
          .then((confirmed) => {
            if (confirmed) {
              this.saveAndPublish();
              return;
            }
            this.navigateToPublish();
          });
      } else {
        this.navigateToPublish();
      }
    },

    async saveAndPublish() {
      if (!this.isEditable) {
        this.toast.showInfo("已发布作品不可重复发布");
        return;
      }

      try {
        this.projectStore.saveProjectPixels(this.projectId, this.pixels);
        this.savedPixelsSnapshot = JSON.stringify(
          Array.from(this.pixels.entries()),
        );
        this.hasUnsavedChanges = false;

        await this.generateAndSaveThumbnail();
        this.toast.showSuccess("已保存");

        setTimeout(() => {
          this.navigateToPublish();
        }, 500);
      } catch (error) {
        console.error("保存失败:", error);
        this.toast.showError("保存失败，请重试");
      }
    },

    navigateToPublish() {
      const canvasData = {
        projectId: this.projectId,
        boardId: this.boardId,
        challengeId: this.project.challengeId,
        pixels: Array.from(this.pixels.entries()),
        width: this.project.width,
        height: this.project.height,
        colors: this.paletteColors.map((color) => color.code),
        projectName: this.project?.name || "未命名画布",
      };

      uni.setStorageSync("temp_publish_data", canvasData);
      uni.navigateTo({
        url: "/pages/publish-project/publish-project",
        animationType: "slide-in-right",
        animationDuration: 300,
      });
    },
  },
};
