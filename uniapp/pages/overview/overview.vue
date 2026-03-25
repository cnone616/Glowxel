<template>
  <view class="overview-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 顶部栏 -->
    <view class="header">
      <view class="header-left">
        <view class="back-btn" @click="handleBack">
          <Icon
            name="direction-left"
            :size="40"
            color="var(--color-text-primary)"
          />
        </view>
        <view class="header-info">
          <text class="header-title">画布总览</text>
          <view class="action-btn primary" @click="handleEdit">
            <Icon name="edit" :size="32" class="edit-icon" />
          </view>
          <view class="action-btn danger" @click="handleDeleteClick">
            <Icon name="ashbin" :size="32" />
          </view>
        </view>
      </view>
    </view>

    <!-- 搜索栏 -->
    <view class="search-section">
      <view class="search-bar">
        <view class="search-input-wrapper">
          <Icon name="search" :size="28" color="var(--text-tertiary)" />
          <input
            v-model="searchQuery"
            type="text"
            class="search-input"
            placeholder="搜索画布 (例如: A1, B2)"
          />
          <view v-if="searchQuery" class="clear-btn" @click="searchQuery = ''">
            <Icon name="close" :size="28" />
          </view>
        </view>
      </view>
    </view>

    <!-- 侧边栏索引 - 行号 -->
    <view class="sidebar-index">
      <view class="index-container">
        <view
          v-for="label in rowLabels"
          :key="label"
          class="index-item"
          @click="scrollToRow(label)"
        >
          <text class="index-text">{{ label }}</text>
        </view>
      </view>
    </view>

    <!-- 画布网格 -->
    <scroll-view
      scroll-y
      scroll-x
      class="board-grid-container"
      :scroll-top="scrollTop"
      :scroll-left="scrollLeft"
      :scroll-with-animation="true"
      @scroll="handleScroll"
    >
      <view class="board-grid-wrapper">
        <view
          class="board-grid"
          :style="{ gridTemplateColumns: `repeat(${cols}, 1fr)` }"
        >
          <view
            v-for="board in boards"
            :key="board.id"
            :id="`board-${board.id}`"
            class="board-card"
            @click="selectedBoard = board.id"
          >
            <!-- 缩略图 -->
            <view class="board-thumbnail">
              <view class="grid-pattern"></view>
              <!-- 使用项目缩略图裁剪显示看板区域 -->
              <view v-if="project && project.thumbnail" class="thumbnail-crop">
                <image
                  :src="project.thumbnail"
                  class="thumbnail-full-image"
                  :style="getBoardImagePosition(board)"
                  mode="aspectFit"
                />
              </view>
              <!-- 加载占位符 -->
              <view v-else class="thumbnail-placeholder">
                <text class="placeholder-text">{{ board.id }}</text>
              </view>
            </view>

            <!-- 完成标识 -->
            <view
              v-if="board.progress && board.progress.completion >= 1"
              class="complete-badge"
            >
              <Icon name="check-item" :size="28" color="#000000" />
            </view>

            <!-- 进度条 -->
            <view class="board-progress">
              <view class="progress-info">
                <text class="progress-label">{{ board.id }}</text>
                <text class="progress-value"
                  >{{
                    Math.round((board.progress?.completion || 0) * 100)
                  }}%</text
                >
              </view>
              <view class="progress-bar">
                <view
                  class="progress-fill"
                  :style="{
                    width: (board.progress?.completion || 0) * 100 + '%',
                  }"
                ></view>
              </view>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <!-- 底部统计 -->
    <view class="footer-stats">
      <!-- 导出完整图纸按钮 - 收藏项目不显示 -->
      <view
        v-if="isExportable"
        class="export-button"
        @click="handleBatchExport"
      >
        <Icon name="download" :size="32" color="#ffffff" />
        <text class="export-text">导出完整图纸</text>
      </view>

      <view class="stat-row">
        <text class="stat-label">总画布数: {{ boards.length }}</text>
        <text class="stat-label"
          >{{ project?.width }}×{{ project?.height }} 像素</text
        >
      </view>

      <view
        v-if="
          project?.paddedWidth &&
          (project.paddedWidth !== project.width ||
            project.paddedHeight !== project.height)
        "
        class="stat-row"
      >
        <text class="stat-label stat-hint"
          >填充后: {{ project?.paddedWidth }}×{{ project?.paddedHeight }}</text
        >
      </view>

      <view class="total-progress">
        <text class="total-label">总进度</text>
        <view class="total-bar">
          <view
            class="total-fill"
            :style="{ width: (project?.progress || 0) + '%' }"
          ></view>
        </view>
        <text class="total-value">{{ project?.progress || 0 }}%</text>
      </view>
    </view>

    <!-- 操作弹窗 -->
    <view
      v-if="selectedBoard"
      class="modal-overlay"
      @click="selectedBoard = null"
    >
      <view class="action-modal" @click.stop>
        <view class="modal-close" @click="selectedBoard = null">
          <Icon name="close" :size="40" />
        </view>

        <text class="modal-board-id">{{ selectedBoard }}</text>
        <text class="modal-subtitle">选择操作模式</text>

        <view class="modal-actions">
          <view class="modal-action-btn primary" @click="goToEditor">
            <Icon name="edit" :size="36" />
            <text class="modal-action-text">编辑 (绘图模式)</text>
          </view>

          <view class="modal-action-btn secondary" @click="goToAssist">
            <Icon name="work" :size="36" />
            <text class="modal-action-text">辅助拼豆模式</text>
          </view>

          <view
            v-if="isExportable"
            class="modal-action-btn tertiary"
            @click="exportBoard"
          >
            <Icon name="download" :size="36" />
            <text class="modal-action-text">导出此画布</text>
          </view>
        </view>
      </view>
    </view>

    <!-- 删除确认弹窗 -->
    <view
      v-if="showDeleteConfirm"
      class="modal-overlay"
      @click="showDeleteConfirm = false"
    >
      <view class="confirm-modal" @click.stop>
        <view class="confirm-icon">
          <Icon name="ashbin" :size="96" />
        </view>
        <text class="confirm-title">删除画布？</text>
        <text class="confirm-desc"
          >您确定要删除 "{{ project?.name }}" 吗？<text class="confirm-warning"
            >此操作无法撤销，所有设计数据将永久丢失。</text
          ></text
        >

        <view class="confirm-actions">
          <view class="confirm-btn cancel" @click="showDeleteConfirm = false">
            <text class="confirm-btn-text">取消</text>
          </view>
          <view class="confirm-btn danger" @click="confirmDelete">
            <text class="confirm-btn-text">确认删除</text>
          </view>
        </view>
      </view>
    </view>

    <!-- 重命名弹窗 -->
    <view
      v-if="showRenameModal"
      class="modal-overlay"
      @click="showRenameModal = false"
    >
      <view class="rename-modal" @click.stop>
        <view class="rename-header">
          <text class="rename-title">修改项目名称</text>
          <view class="close-btn" @click="showRenameModal = false">
            <Icon name="close" :size="32" />
          </view>
        </view>
        <view class="rename-body">
          <view class="rename-input-wrap">
            <input
              v-model="newProjectName"
              type="text"
              class="rename-input"
              placeholder="输入新名称"
              placeholder-class="rename-placeholder"
              maxlength="30"
              :focus="showRenameModal"
              @confirm="confirmRename"
            />
          </view>
        </view>
        <view class="rename-footer">
          <view class="rename-btn cancel" @click="showRenameModal = false">
            <text class="rename-btn-text">取消</text>
          </view>
          <view
            class="rename-btn confirm"
            :class="{ disabled: !newProjectName.trim() }"
            @click="confirmRename"
          >
            <text class="rename-btn-text">确认</text>
          </view>
        </view>
      </view>
    </view>

    <!-- 导出确认弹窗 -->
    <view
      v-if="showExportConfirm"
      class="modal-overlay"
      @click="showExportConfirm = false"
    >
      <view class="confirm-modal" @click.stop>
        <view class="confirm-icon">
          <Icon name="download" :size="96" color="#00f3ff" />
        </view>
        <text class="confirm-title">{{
          exportType === "batch" ? "导出完整图纸？" : "导出此画布？"
        }}</text>
        <text class="confirm-desc">
          {{
            exportType === "batch"
              ? `将导出 "${project?.name}" 的完整图纸，包含所有看板和颜色标注。`
              : `将导出看板 "${exportBoardData}" 的图纸，包含颜色标注。`
          }}
        </text>

        <view class="confirm-actions">
          <view class="confirm-btn cancel" @click="showExportConfirm = false">
            <text class="confirm-btn-text">取消</text>
          </view>
          <view class="confirm-btn primary" @click="confirmExport">
            <text class="confirm-btn-text">确认导出</text>
          </view>
        </view>
      </view>
    </view>

    <!-- 隐藏的 Canvas 用于导出 -->
    <canvas
      canvas-id="exportCanvas"
      id="exportCanvas"
      type="2d"
      style="
        position: fixed;
        left: -9999px;
        top: -9999px;
        width: 1px;
        height: 1px;
      "
    />

    <!-- 自定义 Toast 组件 -->
    <Toast ref="toastRef" />

    <!-- 删除云端备份确认弹窗 -->
    <ConfirmModal
      :visible.sync="showDeleteCloudModal"
      title="删除云端备份？"
      content="是否同时删除云端的备份数据？"
      confirmText="同时删除"
      cancelText="仅删本地"
      danger
      @confirm="deleteProjectWithCloud"
      @cancel="deleteProjectLocal"
    />
  </view>
</template>

<script>
import { useProjectStore } from "../../store/project.js";
import { useUserStore } from "../../store/user.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import ConfirmModal from "../../components/ConfirmModal.vue";
import {
  exportCanvasAsImage,
  saveImageToAlbum,
} from "../../utils/exportCanvas.js";
import { getCloudProjects, deleteCloudProject } from "../../api/project.js";

export default {
  mixins: [statusBarMixin],
  name: "OverviewPage",
  components: {
    Icon,
    Toast,
    ConfirmModal,
  },
  data() {
    return {
      projectStore: null,
      toast: null,
      projectId: "",
      project: null,
      pixels: new Map(),
      selectedBoard: null,
      showDeleteConfirm: false,
      showExportConfirm: false, // 导出确认弹窗
      showRenameModal: false,
      newProjectName: "",
      exportType: "", // 'batch' 或 'single'
      exportBoardData: null, // 要导出的看板数据
      isExporting: false,
      searchQuery: "",
      loadedBoards: new Set(), // 已加载的board ID集合
      loadBatchSize: 6, // 每批加载的数量
      isLoadingMore: false, // 是否正在加载更多
      scrollTop: 0, // 垂直滚动位置
      scrollLeft: 0, // 水平滚动位置
      showDeleteCloudModal: false, // 删除云端备份确认弹窗
    };
  },

  computed: {
    // 是否可导出（收藏项目不可导出）
    isExportable() {
      if (!this.project) return false;
      if (this.project.source === "collected") return false;
      if (this.project.permissions && !this.project.permissions.canExport)
        return false;
      return true;
    },

    cols() {
      // 使用填充后的宽度计算板子数量
      const paddedWidth =
        this.project?.paddedWidth || this.project?.width || 64;
      return Math.ceil(paddedWidth / 64);
    },

    rows() {
      // 使用填充后的高度计算板子数量
      const paddedHeight =
        this.project?.paddedHeight || this.project?.height || 64;
      return Math.ceil(paddedHeight / 64);
    },

    rowLabels() {
      const labels = [];
      for (let y = 0; y < this.rows; y++) {
        labels.push(String.fromCharCode(65 + y));
      }
      return labels;
    },

    boards() {
      const result = [];

      for (let y = 0; y < this.rows; y++) {
        for (let x = 0; x < this.cols; x++) {
          const rowLabel = String.fromCharCode(65 + y);
          const colLabel = x + 1;
          const id = `${rowLabel}${colLabel}`;
          const progress = this.getBoardProgress(id);
          // 延迟计算像素数据，只在需要时才计算
          const boardPixels = null; // 不在这里计算
          const thumbnail = null; // 不在这里获取

          result.push({ id, x, y, progress, pixels: boardPixels, thumbnail });
        }
      }

      // 搜索过滤
      if (this.searchQuery.trim()) {
        const query = this.searchQuery.trim().toUpperCase();
        return result.filter((board) => board.id.toUpperCase().includes(query));
      }

      return result;
    },
  },

  onLoad(options) {
    this.projectStore = useProjectStore();
    this.toast = useToast();

    this.projectId = options.id;
    this.project = this.projectStore.getProject(this.projectId);

    if (!this.project) {
      this.toast.showError("项目不存在");
      setTimeout(() => {
        uni.switchTab({
          url: "/pages/workspace/workspace",
        });
      }, 1000);
      return;
    }

    this.loadPixels();

    // 延迟500ms后开始加载缩略图
    setTimeout(() => {
      this.loadInitialBoards();
    }, 500);
  },

  onShow() {
    // 从 assist 页返回时刷新进度数据
    if (this.projectStore && this.projectId) {
      this.project = this.projectStore.getProject(this.projectId);
    }
  },

  methods: {
    loadPixels() {
      const savedPixels = uni.getStorageSync(`pixels-${this.projectId}`);
      if (savedPixels) {
        try {
          const parsed = JSON.parse(savedPixels);
          this.pixels = new Map(parsed);
        } catch (e) {
          console.error("Failed to load pixels", e);
          this.pixels = new Map();
        }
      }
    },

    // 初始加载前几批缩略图
    loadInitialBoards() {
      const initialCount = Math.min(this.boards.length, this.loadBatchSize); // 初始只加载1批
      for (let i = 0; i < initialCount; i++) {
        this.loadedBoards.add(this.boards[i].id);
      }

      // 继续加载剩余的
      if (this.boards.length > initialCount) {
        setTimeout(() => {
          this.loadMoreBoards();
        }, 200);
      }
    },

    // 加载更多缩略图
    loadMoreBoards() {
      if (this.isLoadingMore) return;

      this.isLoadingMore = true;
      const currentCount = this.loadedBoards.size;
      const totalCount = this.boards.length;

      if (currentCount >= totalCount) {
        this.isLoadingMore = false;
        return;
      }

      // 每次加载一批
      const endIndex = Math.min(currentCount + this.loadBatchSize, totalCount);
      for (let i = currentCount; i < endIndex; i++) {
        this.loadedBoards.add(this.boards[i].id);
      }

      this.isLoadingMore = false;

      // 如果还有未加载的，继续加载
      if (endIndex < totalCount) {
        setTimeout(() => {
          this.loadMoreBoards();
        }, 100);
      }
    },

    // 滚动时触发加载
    handleScroll(e) {
      // 简单的滚动触发，确保所有内容最终都会加载
      if (!this.isLoadingMore && this.loadedBoards.size < this.boards.length) {
        this.loadMoreBoards();
      }
    },

    getBoardPixels(boardX, boardY) {
      const local = new Map();
      const startX = boardX * 64;
      const startY = boardY * 64;

      this.pixels.forEach((color, key) => {
        const [px, py] = key.split(",").map(Number);
        if (
          px >= startX &&
          px < startX + 64 &&
          py >= startY &&
          py < startY + 64
        ) {
          local.set(`${px - startX},${py - startY}`, color);
        }
      });

      return local;
    },

    getBoardImagePosition(board) {
      // 计算整个项目有多少个看板
      const paddedWidth = this.project.paddedWidth || this.project.width;
      const paddedHeight = this.project.paddedHeight || this.project.height;
      const boardsX = Math.ceil(paddedWidth / 64);
      const boardsY = Math.ceil(paddedHeight / 64);

      // 容器尺寸（需要通过CSS变量或固定值）
      // 假设容器是正方形，宽高相等，这里用百分比计算
      // 图片宽度 = 容器宽度 * boardsX
      // 图片高度 = 容器高度 * boardsY

      // 当前看板的偏移 = -容器尺寸 * 看板索引
      // left: board.x = 0 时是 0，board.x = 1 时是 -100%，board.x = 2 时是 -200%

      return {
        width: `${boardsX * 100}%`,
        height: `${boardsY * 100}%`,
        left: `${-board.x * 100}%`,
        top: `${-board.y * 100}%`,
      };
    },

    getBoardProgress(boardId) {
      if (!this.project) return null;
      return this.projectStore.getBoardProgress(this.project.id, boardId);
    },

    scrollToRow(label) {
      // 如果是第一行A，直接滚到顶部并保留边距
      if (label === "A") {
        this.scrollTop = 0;
        this.scrollLeft = 0;
        this.$forceUpdate();
        return;
      }

      // 计算目标行的位置
      const targetId = `board-${label}1`;

      // 使用 uni.createSelectorQuery 获取元素位置
      const query = uni.createSelectorQuery().in(this);
      query.select(`#${targetId}`).boundingClientRect();
      query.select(".board-grid-wrapper").boundingClientRect();
      query.select(".board-grid-container").boundingClientRect();
      query.exec((res) => {
        if (res && res[0] && res[1] && res[2]) {
          const targetRect = res[0];
          const wrapperRect = res[1];

          // 计算滚动位置，保留64rpx的上边距
          const padding = uni.upx2px(64);
          this.scrollTop = targetRect.top - wrapperRect.top - padding;

          // 保持左边距不变，或者重置为初始左边距
          this.scrollLeft = 0;

          // 触发滚动
          this.$forceUpdate();
        }
      });
    },

    scrollToCol(col) {
      // 列跳转暂时不需要，已删除横向索引
    },

    handleBack() {
      uni.navigateBack();
    },

    goToFullEditor() {
      uni.navigateTo({
        url: `/pages/editor/editor?id=${this.projectId}`,
      });
    },

    goToEditor() {
      uni.navigateTo({
        url: `/pages/editor/editor?id=${this.projectId}&board=${this.selectedBoard}`,
      });
      this.selectedBoard = null;
    },

    goToAssist() {
      uni.navigateTo({
        url: `/pages/assist/assist?id=${this.projectId}&board=${this.selectedBoard}`,
      });
      this.selectedBoard = null;
    },

    exportBoard() {
      if (!this.selectedBoard) return;

      const board = this.boards.find((b) => b.id === this.selectedBoard);
      if (!board) return;

      // 显示确认弹窗
      this.exportType = "single";
      this.exportBoardData = this.selectedBoard;
      this.showExportConfirm = true;
    },

    async handleExportBoard(board) {
      try {
        this.toast.showInfo("正在生成图纸...");

        // 实时计算像素数据
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

      // 显示确认弹窗
      this.exportType = "batch";
      this.showExportConfirm = true;
    },

    async performBatchExport() {
      if (this.isExporting || !this.project) return;

      this.isExporting = true;
      this.toast.showInfo("正在生成完整图纸...");

      try {
        // 根据画布大小动态调整 cellSize
        const width = this.project.paddedWidth || this.project.width;
        const height = this.project.paddedHeight || this.project.height;
        const maxDimension = Math.max(width, height);
        let cellSize = 30;

        // 根据最大边长调整 cellSize
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

        console.log(`画布尺寸: ${width}x${height}, 使用 cellSize: ${cellSize}`);

        // 直接使用全局像素数据，不需要遍历boards
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
        // 如果是因为尺寸过大导致的错误，给出更友好的提示
        if (error.message && error.message.includes("分块导出")) {
          this.toast.showError("图纸过大，建议在下方选择单个看板导出");
        } else {
          this.toast.showError(error.message || "导出失败，请重试");
        }
      } finally {
        this.isExporting = false;
      }
    },

    handleEdit() {
      this.newProjectName = this.project?.name || "";
      this.showRenameModal = true;
    },

    async confirmRename() {
      const name = this.newProjectName.trim();
      if (!name || !this.project) return;
      try {
        this.project.name = name;
        this.project.updatedAt = Date.now();
        await this.projectStore.saveProject(this.project);
        this.showRenameModal = false;
        this.toast.showSuccess("已修改");
      } catch (err) {
        console.error("重命名失败:", err);
        this.toast.showError("修改失败");
      }
    },

    handleDeleteClick() {
      this.showDeleteConfirm = true;
    },

    confirmExport() {
      this.showExportConfirm = false;

      if (this.exportType === "batch") {
        // 批量导出
        this.performBatchExport();
      } else if (this.exportType === "single") {
        // 单个看板导出
        const board = this.boards.find((b) => b.id === this.exportBoardData);
        if (board) {
          this.selectedBoard = null;
          this.handleExportBoard(board);
        }
      }
    },

    async confirmDelete() {
      if (!this.project) return;

      // 检查是否已登录且开启云同步
      const userStore = useUserStore();

      if (userStore.hasLogin && userStore.syncEnabled) {
        // 显示删除云端备份确认弹窗
        this.showDeleteCloudModal = true;
      } else {
        // 未登录或未开启云同步，直接删除本地
        this.deleteProjectLocal();
      }
    },

    // 删除本地项目
    deleteProjectLocal() {
      this.projectStore.deleteProject(this.project.id);
      this.toast.showSuccess("本地画布已删除");
      uni.switchTab({
        url: "/pages/workspace/workspace",
      });
    },

    // 删除本地和云端项目
    async deleteProjectWithCloud() {
      uni.showLoading({ title: "删除中..." });

      try {
        // 先获取云端项目
        const cloudRes = await getCloudProjects();
        if (cloudRes.success && cloudRes.data) {
          const cloudProject = cloudRes.data.list.find(
            (p) => p.id === this.project.id,
          );
          if (cloudProject) {
            // 删除云端项目
            await deleteCloudProject(cloudProject.id);
          }
        }

        // 删除本地项目
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

        // 即使云端删除失败，也删除本地
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
</script>

<style scoped>
.overview-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
  position: relative;
  overflow: hidden;
}

.header {
  height: 88rpx;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 20rpx;
  border-bottom: 2rpx solid var(--border-primary);
  background-color: var(--bg-elevated);
  z-index: 30;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 32rpx;
  flex: 1;
}

.back-btn {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  transition: var(--transition-base);
}

.back-btn:active {
  background-color: var(--bg-tertiary);
}

.back-icon {
  font-size: 64rpx;
  color: var(--text-primary);
  line-height: 1;
}

.header-info {
  display: flex;
  flex: 1;
  align-items: center;
}

.header-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.header-subtitle {
  font-size: 20rpx;
  color: var(--text-secondary);
  font-family: monospace;
}

.header-actions {
  display: flex;
  gap: 16rpx;
}

.action-btn {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  transition: var(--transition-base);
  margin-left: 20rpx;
}

.action-btn:active {
  transform: scale(0.95);
}

/* 下载按钮 - 蓝色边框 */
.action-btn:not(.primary):not(.danger) {
  background-color: transparent;
  border-color: var(--accent-primary);
  color: var(--accent-primary);
}

/* 浅色模式下的下载按钮 - 图标和边框同色 */
.light-theme .action-btn:not(.primary):not(.danger) {
  color: var(--accent-primary);
}

/* 编辑按钮 - 蓝底白色 */
.action-btn.primary {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
  color: #ffffff !important;
}

.action-btn.primary:active {
  box-shadow: var(--shadow-glow);
}

/* 删除按钮 - 红底白色 */
.action-btn.danger {
  background-color: #ff3333;
  border-color: #ff3333;
  color: #ffffff !important;
}

/* 浅色模式下的删除按钮 - 白色图标 */
.light-theme .action-btn.danger {
  background-color: #ff3333;
  border-color: #ff3333;
  color: #ffffff !important;
}

.action-icon {
  font-size: 32rpx;
}

.search-section {
  background-color: var(--bg-elevated);
  border-bottom: 2rpx solid var(--border-primary);
  z-index: 25;
}

.search-bar {
  padding: 24rpx 48rpx;
}

.search-input-wrapper {
  display: flex;
  align-items: center;
  gap: 16rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  padding: 16rpx 24rpx;
  transition: var(--transition-base);
}

.search-input-wrapper:focus-within {
  border-color: var(--accent-primary);
  box-shadow: var(--shadow-glow);
}

.search-input {
  flex: 1;
  font-size: 28rpx;
  color: var(--text-primary);
  background-color: transparent;
  border: none;
  outline: none;
}

.search-input::placeholder {
  color: var(--text-tertiary);
}

.clear-btn {
  width: 48rpx;
  height: 48rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  transition: var(--transition-base);
}

.clear-btn:active {
  background-color: var(--bg-secondary);
  transform: scale(0.95);
}

.sidebar-index {
  position: absolute;
  right: 32rpx;
  top: 192rpx;
  bottom: 192rpx;
  width: 64rpx;
  display: flex;
  align-items: center;
  z-index: 20;
  pointer-events: none;
}

.index-container {
  background-color: rgba(0, 0, 0, 0.8);
  backdrop-filter: blur(40rpx);
  border-radius: 32rpx;
  padding: 32rpx 8rpx;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  border: 2rpx solid var(--border-secondary);
  box-shadow: var(--shadow-lg);
  pointer-events: auto;
}

/* 浅色模式下的索引容器 */
.light-theme .index-container {
  background-color: rgba(255, 255, 255, 0.95);
  border: 2rpx solid rgba(0, 0, 0, 0.1);
}

.index-item {
  width: 48rpx;
  height: 48rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.index-item:active {
  background-color: var(--accent-primary);
  transform: scale(1.1);
}

.index-text {
  font-size: 20rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--text-secondary);
}

.index-item:active .index-text {
  color: #ffffff;
}

.board-grid-container {
  flex: 1;
  overflow-y: auto;
  background-color: var(--bg-primary);
}

.board-grid-scroll {
  width: 100%;
  white-space: nowrap;
}

.board-grid-wrapper {
  display: inline-block;
  padding: 64rpx 48rpx 360rpx;
  min-height: 100%;
}

.board-grid {
  display: grid;
  gap: 24rpx;
  width: auto;
}

.board-card {
  position: relative;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  box-shadow: var(--shadow-md);
  transition: var(--transition-base);
  min-width: 150rpx;
  max-width: 250rpx;
  min-height: 214rpx;
  max-width: 300rpx;
}

.board-card:active {
  transform: scale(0.98);
  border-color: var(--accent-primary);
  box-shadow: var(--shadow-glow);
}

.board-thumbnail {
  flex: 1;
  position: relative;
  background-color: var(--bg-primary);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 16rpx;
  padding-bottom: 80rpx;
  overflow: hidden;
}

.thumbnail-crop {
  width: 100%;
  height: 100%;
  position: relative;
  overflow: hidden;
}

.thumbnail-full-image {
  position: absolute;
  left: 0;
  top: 0;
  image-rendering: pixelated;
  image-rendering: -moz-crisp-edges;
  image-rendering: crisp-edges;
  transform-origin: 0 0;
}

.thumbnail-image {
  width: 100%;
  height: 100%;
  display: block;
  object-fit: contain;
  image-rendering: pixelated;
  image-rendering: -moz-crisp-edges;
  image-rendering: crisp-edges;
}

.grid-pattern {
  position: absolute;
  inset: 0;
  opacity: 0.1;
  background-image:
    linear-gradient(var(--canvas-grid) 2rpx, transparent 2rpx),
    linear-gradient(90deg, var(--canvas-grid) 2rpx, transparent 2rpx);
  background-size: 16rpx 16rpx;
}

.complete-badge {
  position: absolute;
  top: 16rpx;
  right: 16rpx;
  width: 48rpx;
  height: 48rpx;
  background-color: rgba(0, 255, 157, 0.9);
  backdrop-filter: blur(20rpx);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
}

.complete-icon {
  font-size: 28rpx;
  color: var(--text-inverse);
  font-weight: bold;
}

.board-progress {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  background-color: rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(20rpx);
  padding: 5rpx 16rpx 10rpx;
}

.progress-info {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8rpx;
}

.progress-label {
  font-size: 20rpx;
  color: #333333;
}

.progress-value {
  font-size: 20rpx;
  font-family: monospace;
  color: var(--text-primary);
}

.progress-bar {
  width: 100%;
  height: 12rpx;
  background-color: rgba(255, 255, 255, 0.1);
  border-radius: 6rpx;
  overflow: hidden;
}

.progress-fill {
  height: 100%;
  background: linear-gradient(
    90deg,
    var(--accent-primary),
    var(--accent-secondary)
  );
  transition: var(--transition-slow);
}

.footer-stats {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  padding: 32rpx 48rpx;
  background-color: var(--bg-elevated);
  border-top: 2rpx solid var(--border-color);
  z-index: 30;
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.export-button {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
  padding: 24rpx;
  background-color: var(--accent-color);
  border-radius: 16rpx;
  box-shadow: 0 8rpx 32rpx rgba(0, 243, 255, 0.2);
  transition: opacity 0.2s;
}

.export-button:active {
  opacity: 0.8;
}

.export-text {
  font-size: 28rpx;
  font-weight: bold;
  color: #ffffff;
}

.stat-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.stat-label {
  font-size: 24rpx;
  color: var(--text-secondary);
  font-family: monospace;
}

.stat-hint {
  font-size: 20rpx;
  opacity: 0.7;
}

.total-progress {
  display: flex;
  align-items: center;
  gap: 24rpx;
}

.total-label {
  font-size: 24rpx;
  color: var(--text-secondary);
  flex-shrink: 0;
}

.total-bar {
  flex: 1;
  height: 16rpx;
  background-color: var(--bg-tertiary);
  border-radius: 8rpx;
  overflow: hidden;
}

.total-fill {
  height: 100%;
  background: linear-gradient(
    90deg,
    var(--accent-primary),
    var(--accent-secondary)
  );
  transition: var(--transition-slow);
}

.total-value {
  font-size: 24rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--accent-color);
  flex-shrink: 0;
}

/* 弹窗样式 */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.8);
  backdrop-filter: blur(20rpx);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  padding: 64rpx;
}

.action-modal {
  width: 100%;
  max-width: 600rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--accent-primary);
  border-radius: 32rpx;
  padding: 64rpx 32rpx;
  position: relative;
  box-shadow: var(--shadow-lg);
  animation: scaleIn var(--transition-base);
  box-sizing: border-box;
}

/* 小程序减小padding */
/* #ifdef MP-WEIXIN */
.action-modal {
  padding: 48rpx 24rpx;
}
/* #endif */

.modal-close {
  position: absolute;
  top: 32rpx;
  right: 32rpx;
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.close-icon {
  font-size: 40rpx;
  color: var(--text-secondary);
}

.modal-board-id {
  font-size: 48rpx;
  font-weight: bold;
  font-family: monospace;
  color: var(--text-primary);
  text-align: center;
  display: block;
  margin-bottom: 16rpx;
}

.modal-subtitle {
  font-size: 24rpx;
  color: var(--text-secondary);
  text-align: center;
  display: block;
  margin-bottom: 64rpx;
}

.modal-actions {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.modal-action-btn {
  padding: 32rpx;
  border-radius: 24rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
  transition: var(--transition-base);
}

.modal-action-btn:active {
  transform: scale(0.98);
}

/* 编辑按钮 - 蓝底，文字颜色根据主题自适应 */
.modal-action-btn.primary {
  background-color: var(--accent-primary);
  box-shadow: var(--shadow-md);
  border: none;
  color: #ffffff;
}

.light-theme .modal-action-btn.primary {
  color: #ffffff;
}

.modal-action-btn.primary:active {
  box-shadow: var(--shadow-glow);
}

/* 编辑按钮文字和图标 - 继承父元素颜色 */
.modal-action-btn.primary .modal-action-text {
  color: inherit;
}

/* 辅助拼豆按钮 - 透明背景蓝色边框 */
.modal-action-btn.secondary {
  background-color: transparent;
  border: 2rpx solid var(--accent-primary);
  color: var(--accent-primary);
}

/* 导出按钮 - 深色背景蓝色边框 */
.modal-action-btn.tertiary {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--accent-primary);
  color: var(--accent-primary);
}

/* 浅色模式下的调整 */
.light-theme .modal-action-btn.tertiary {
  background-color: rgba(0, 153, 204, 0.05);
  border: 2rpx solid var(--accent-primary);
}

.modal-action-icon {
  font-size: 36rpx;
}

.modal-action-text {
  font-size: 28rpx;
  font-weight: bold;
}

/* 辅助拼豆和导出按钮文字和图标 - 继承父元素颜色 */
.modal-action-btn.secondary .modal-action-text,
.modal-action-btn.tertiary .modal-action-text {
  color: inherit;
}

.confirm-modal {
  width: 100%;
  max-width: 600rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid rgba(255, 51, 51, 0.5);
  border-radius: 32rpx;
  padding: 64rpx 32rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  box-shadow: var(--shadow-lg);
  animation: scaleIn var(--transition-base);
  box-sizing: border-box;
}

/* 小程序减小padding */
/* #ifdef MP-WEIXIN */
.confirm-modal {
  padding: 48rpx 24rpx;
}
/* #endif */

.confirm-icon {
  font-size: 96rpx;
  margin-bottom: 32rpx;
}

.confirm-title {
  font-size: 40rpx;
  font-weight: bold;
  color: var(--text-primary);
  margin-bottom: 16rpx;
  text-align: center;
}

.confirm-desc {
  font-size: 28rpx;
  color: var(--text-secondary);
  text-align: center;
  margin-bottom: 64rpx;
  line-height: 1.6;
}

.confirm-warning {
  display: block;
  color: var(--error-color);
  font-size: 24rpx;
  margin-top: 16rpx;
}

.confirm-actions {
  width: 100%;
  display: flex;
  gap: 24rpx;
}

.confirm-btn {
  flex: 1;
  padding: 32rpx;
  border-radius: 16rpx;
  text-align: center;
}

.confirm-btn.cancel {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-secondary);
}

.confirm-btn.primary {
  background-color: var(--accent-primary);
  border: 2rpx solid var(--accent-primary);
  box-shadow: var(--shadow-glow);
}

.confirm-btn.danger {
  background-color: var(--error-color);
}

.confirm-btn-text {
  font-size: 28rpx;
  font-weight: bold;
}

.confirm-btn.cancel .confirm-btn-text {
  color: var(--text-primary);
}

.confirm-btn.primary .confirm-btn-text {
  color: var(--text-inverse);
}

.confirm-btn.danger .confirm-btn-text {
  color: var(--text-inverse);
}

.thumbnail-placeholder {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--bg-secondary);
}

.placeholder-text {
  font-size: 24rpx;
  color: var(--text-tertiary);
  font-family: monospace;
  opacity: 0.5;
}

/* 重命名弹窗 */
.rename-modal {
  width: 100%;
  max-width: 600rpx;
  background-color: var(--bg-tertiary);
  border-radius: 24rpx;
  border: 2rpx solid var(--border-primary);
  overflow: hidden;
  animation: scaleIn 0.2s ease-out;
}
@keyframes scaleIn {
  from {
    transform: scale(0.9);
    opacity: 0;
  }
  to {
    transform: scale(1);
    opacity: 1;
  }
}
.rename-header {
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.rename-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--text-primary);
}
.rename-body {
  padding: 32rpx;
}
.rename-input-wrap {
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  padding: 0 24rpx;
  height: 88rpx;
  display: flex;
  align-items: center;
}
.rename-input {
  width: 100%;
  font-size: 28rpx;
  color: var(--text-primary);
  background: transparent;
}
.rename-placeholder {
  color: var(--text-tertiary);
}
.rename-footer {
  padding: 0 32rpx 32rpx;
  display: flex;
  gap: 24rpx;
}
.rename-btn {
  flex: 1;
  padding: 24rpx;
  border-radius: 16rpx;
  text-align: center;
}
.rename-btn.cancel {
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-secondary);
}
.rename-btn.confirm {
  background-color: var(--accent-primary);
}
.rename-btn.confirm.disabled {
  opacity: 0.5;
}
.rename-btn.cancel .rename-btn-text {
  color: var(--text-primary);
  font-size: 28rpx;
  font-weight: 600;
}
.rename-btn.confirm .rename-btn-text {
  color: #fff;
  font-size: 28rpx;
  font-weight: 600;
}
</style>
