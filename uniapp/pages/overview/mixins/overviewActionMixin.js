import { useUserStore } from "../../../store/user.js";
import {
  exportCanvasAsImage,
  saveImageToAlbum,
} from "../../../utils/exportCanvas.js";
import { getCloudProjects, deleteCloudProject } from "../../../api/project.js";

export default {
  methods: {
    async handleExportBoard(board) {
      try {
        this.toast.showInfo("正在生成图纸...");

        const boardPixels = this.getBoardPixels(board.x, board.y);
        const tempFilePath = await exportCanvasAsImage({
          pixels: boardPixels,
          width: 64,
          height: 64,
          projectName: `${this.project?.name || "未命名"} - ${board.id}`,
          palette: this.project?.palette,
          cellSize: 30,
          showGrid: true,
          showCoordinates: true,
          showColorCodes: true,
          showStatistics: true,
          showLogo: true,
          canvasId: "exportCanvas",
        });

        await saveImageToAlbum(tempFilePath);
        this.toast.showSuccess(`画布 ${board.id} 已保存到相册`);
      } catch (error) {
        console.error("导出失败:", error);
        this.toast.showError(error.message || "导出失败，请重试");
      }
    },

    async handleBatchExport() {
      if (this.isExporting || !this.project) return;

      this.exportType = "batch";
      this.showExportConfirm = true;
    },

    async performBatchExport() {
      if (this.isExporting || !this.project) return;

      this.isExporting = true;
      this.toast.showInfo("正在生成完整图纸...");

      try {
        const width = this.project.paddedWidth || this.project.width;
        const height = this.project.paddedHeight || this.project.height;
        const maxDimension = Math.max(width, height);
        let cellSize = 30;

        if (maxDimension >= 520) {
          cellSize = 7;
        } else if (maxDimension >= 400) {
          cellSize = 9;
        } else if (maxDimension >= 300) {
          cellSize = 12;
        } else if (maxDimension >= 200) {
          cellSize = 18;
        } else if (maxDimension >= 100) {
          cellSize = 25;
        }

        const tempFilePath = await exportCanvasAsImage({
          pixels: this.pixels,
          width: width,
          height: height,
          projectName: this.project.name,
          palette: this.project.palette,
          cellSize: cellSize,
          showGrid: true,
          showCoordinates: true,
          showColorCodes: true,
          showStatistics: true,
          showLogo: true,
          canvasId: "exportCanvas",
        });

        await saveImageToAlbum(tempFilePath);
        this.toast.showSuccess("完整图纸已保存到相册");
      } catch (error) {
        console.error("导出失败:", error);
        if (error.message && error.message.includes("分块导出")) {
          this.toast.showError("图纸过大，建议在下方选择单个看板导出");
        } else {
          this.toast.showError(error.message || "导出失败，请重试");
        }
      } finally {
        this.isExporting = false;
      }
    },

    confirmExport() {
      this.showExportConfirm = false;

      if (this.exportType === "batch") {
        this.performBatchExport();
        return;
      }

      if (this.exportType === "single") {
        const board = this.boards.find((item) => item.id === this.exportBoardData);
        if (board) {
          this.selectedBoard = null;
          this.handleExportBoard(board);
        }
      }
    },

    async confirmDelete() {
      if (!this.project) return;

      const userStore = useUserStore();
      if (userStore.hasLogin && userStore.syncEnabled) {
        this.showDeleteCloudModal = true;
      } else {
        this.deleteProjectLocal();
      }
    },

    deleteProjectLocal() {
      this.projectStore.deleteProject(this.project.id);
      this.toast.showSuccess("本地画布已删除");
      uni.switchTab({
        url: "/pages/workspace/workspace",
      });
    },

    async deleteProjectWithCloud() {
      uni.showLoading({ title: "删除中..." });

      try {
        const cloudRes = await getCloudProjects();
        if (cloudRes.success && cloudRes.data) {
          const cloudProject = cloudRes.data.list.find(
            (project) => project.id === this.project.id,
          );
          if (cloudProject) {
            await deleteCloudProject(cloudProject.id);
          }
        }

        this.projectStore.deleteProject(this.project.id);

        uni.hideLoading();
        this.toast.showSuccess("本地和云端画布已删除");
        uni.switchTab({
          url: "/pages/workspace/workspace",
        });
      } catch (error) {
        uni.hideLoading();
        console.error("删除云端项目失败:", error);
        this.toast.showError("云端删除失败，仅删除了本地");

        this.projectStore.deleteProject(this.project.id);
        setTimeout(() => {
          uni.switchTab({
            url: "/pages/workspace/workspace",
          });
        }, 1500);
      }
    },
  },
};
