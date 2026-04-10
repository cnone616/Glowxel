<template>
  <view class="editor-page light-theme glx-page-shell">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 隐藏的导出 Canvas -->
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
    ></canvas>

    <!-- 隐藏的缩略图 Canvas -->
    <canvas
      canvas-id="thumbnailCanvas"
      id="thumbnailCanvas"
      type="2d"
      style="
        position: fixed;
        left: -9999px;
        top: -9999px;
        width: 100px;
        height: 100px;
      "
    ></canvas>

    <!-- 隐藏的看板缩略图 Canvas -->
    <canvas
      canvas-id="boardThumbnailCanvas"
      id="boardThumbnailCanvas"
      type="2d"
      style="
        position: fixed;
        left: -9999px;
        top: -9999px;
        width: 200px;
        height: 200px;
      "
    ></canvas>

    <!-- 顶部栏 -->
    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--nb-ink)"
        />
      </view>
      <view class="nav-title">
        <text class="project-name glx-topbar__title">
          {{ project?.name }}
        </text>
        <text v-if="boardId" class="board-id">/ {{ boardId }}</text>
      </view>
      <view class="nav-right"></view>
    </view>

    <!-- Canvas 画布区域 -->
    <view
      class="canvas-container"
      ref="containerRef"
      :style="{ visibility: hasModalOpen ? 'hidden' : 'visible' }"
    >
      <PixelCanvas
        v-if="canvasReady && !hasModalOpen"
        :width="boardOffset.w"
        :height="boardOffset.h"
        :pixels="isCalculated ? displayPixels : new Map()"
        :zoom="zoom"
        :offset-x="pan.x"
        :offset-y="pan.y"
        :canvas-width="containerSize.width"
        :canvas-height="containerSize.height"
        :grid-visible="gridVisible"
        :allow-single-touch-pan="tool === 'move'"
        :is-dark-mode="false"
        canvas-id="editorCanvas"
        @pixel-click="handlePixelClick"
        @pan="handlePan"
        @zoom="handlePinchZoom"
      />
    </view>

    <!-- 底部工具栏 -->
    <view class="toolbar">
      <!-- 操作按钮 -->
      <view class="action-bar">
        <view class="action-group">
          <view
            class="action-btn glx-action-tile"
            :class="{ disabled: historyIndex <= 0 }"
            :style="getActionButtonStyle()"
            @click="handleUndo"
          >
            <Icon name="back" :size="32" :color="getActionButtonColor()" />
          </view>
          <view class="divider"></view>
          <view
            class="action-btn glx-action-tile"
            :class="{ disabled: historyIndex >= history.length - 1 }"
            :style="getActionButtonStyle()"
            @click="handleRedo"
          >
            <Icon name="forward" :size="32" :color="getActionButtonColor()" />
          </view>
        </view>

        <view class="action-group">
          <view class="action-btn glx-action-tile" :style="getActionButtonStyle()" @click="handleZoom(-1)">
            <Icon name="zoom-out" :size="32" :color="getActionButtonColor()" />
          </view>
          <view class="divider"></view>
          <view class="action-btn glx-action-tile" :style="getActionButtonStyle()" @click="handleZoom(1)">
            <Icon name="zoom-in" :size="32" :color="getActionButtonColor()" />
          </view>
          <view class="divider"></view>
          <view class="action-btn" :style="getActionButtonStyle()" @click="isHelpOpen = true">
            <Icon name="help" :size="32" :color="getActionButtonColor()" />
          </view>
          <view class="divider"></view>
          <view
            class="action-btn"
            :style="getActionButtonStyle(gridVisible)"
            @click="gridVisible = !gridVisible"
          >
            <Icon
              name="a-Grid-ninejiugongge"
              :size="32"
              :color="getActionButtonColor(gridVisible)"
            />
          </view>
          <view class="divider"></view>
          <view class="action-btn" :style="getActionButtonStyle()" @click="handleFit">
            <Icon name="fullscreen-expand" :size="32" :color="getActionButtonColor()" />
          </view>
        </view>
      </view>

      <!-- 工具选择 -->
      <view class="tools">
        <view
          class="tool-btn move"
          :style="getToolButtonStyle('move')"
          @click="setTool('move')"
        >
          <Icon name="move" :size="34" :color="getToolButtonColor('move')" />
          <text class="tool-label" :style="{ color: getToolButtonColor('move') }">拖动</text>
        </view>

        <view
          class="tool-btn pencil"
          :style="getToolButtonStyle('pencil')"
          @click="setTool('pencil')"
        >
          <Icon name="edit" :size="34" :color="getToolButtonColor('pencil')" />
          <text class="tool-label" :style="{ color: getToolButtonColor('pencil') }">绘画</text>
        </view>

        <view
          class="tool-btn eraser"
          :style="getToolButtonStyle('eraser')"
          @click="setTool('eraser')"
        >
          <Icon name="delete" :size="34" :color="getToolButtonColor('eraser')" />
          <text class="tool-label" :style="{ color: getToolButtonColor('eraser') }">擦除</text>
        </view>

        <view class="tool-btn tool-btn-static" :style="getToolButtonStyle('save')" @click="handleSave">
          <Icon name="save" :size="34" :color="getToolButtonColor('save')" />
          <text class="tool-label" :style="{ color: getToolButtonColor('save') }">保存</text>
        </view>

        <view class="tool-btn tool-btn-static" :style="getToolButtonStyle('publish')" @click="handlePublish">
          <Icon name="upload" :size="34" :color="getToolButtonColor('publish')" />
          <text class="tool-label" :style="{ color: getToolButtonColor('publish') }">发布</text>
        </view>
      </view>

      <!-- 颜色选择器 -->
      <view class="color-palette">
        <ColorPalette
          :colors="paletteColors"
          :selected-color="selectedColor"
          @select-color="setSelectedColor"
        />
      </view>
    </view>

    <!-- 保存确认弹窗 -->
    <view
      v-if="showSaveConfirm"
      class="modal-overlay"
      @click="showSaveConfirm = false"
    >
      <view class="modal" @click.stop>
        <view class="modal-icon">
          <Icon name="save" :size="56" />
        </view>
        <text class="modal-title">保存修改？</text>
        <text class="modal-desc">是否保存本次修改？</text>
        <view class="modal-actions">
          <view class="modal-btn secondary" @click="showSaveConfirm = false">
            <text class="modal-btn-text">取消</text>
          </view>
          <view class="modal-btn primary" @click="confirmSave">
            <text class="modal-btn-text">确认保存</text>
          </view>
        </view>
      </view>
    </view>

    <!-- 离开确认弹窗 -->
    <view
      v-if="showLeaveConfirm"
      class="modal-overlay"
      @click="showLeaveConfirm = false"
    >
      <view class="modal" @click.stop>
        <view class="modal-icon warning">
          <Icon name="close" :size="56" />
        </view>
        <text class="modal-title">有未保存的修改</text>
        <text class="modal-desc">您有未保存的修改，是否保存后再离开？</text>
        <view class="modal-actions vertical">
          <view class="modal-btn primary" @click="saveAndLeave">
            <text class="modal-btn-text">保存并离开</text>
          </view>
          <view class="modal-btn secondary" @click="confirmLeave">
            <text class="modal-btn-text">不保存，直接离开</text>
          </view>
          <view class="modal-btn text" @click="showLeaveConfirm = false">
            <text class="modal-btn-text">取消</text>
          </view>
        </view>
      </view>
    </view>
    <!-- 帮助弹窗 -->
    <HelpModal
      :is-open="isHelpOpen"
      @close="isHelpOpen = false"
      title="编辑器操作指南"
      :items="helpItems"
    />
    <!-- 自定义 Toast 组件 -->
    <Toast ref="toastRef" @show="handleToastShow" @hide="handleToastHide" />
  </view>
</template>

<script>
import { useProjectStore } from "../../store/project.js";
import { useToast } from "../../composables/useToast.js";
import { getColorByCode } from "../../data/artkal-colors-full.js";
import statusBarMixin from "../../mixins/statusBar.js";
import PixelCanvas from "../../components/PixelCanvas.vue";
import ColorPalette from "../../components/ColorPalette.vue";
import HelpModal from "../../components/HelpModal.vue";
import Toast from "../../components/Toast.vue";
import Icon from "../../components/Icon.vue";
import editorPersistenceMixin from "./mixins/editorPersistenceMixin.js";
import { PERLER_BOARD_SIZE } from "../../constants/perler.js";

export default {
  mixins: [statusBarMixin, editorPersistenceMixin],
  components: {
    PixelCanvas,
    ColorPalette,
    HelpModal,
    Toast,
    Icon,
  },

  data() {
    return {
      projectStore: null,
      toast: null,

      projectId: "",
      boardId: "",
      project: null,

      // 像素数据
      pixels: new Map(),
      paletteColors: [],
      selectedColor: "",
      selectedColorCode: "",

      // 视图控制 - 初始值设为合理的默认值（52格子在400px容器中）
      zoom: 3, // 初始缩放级别
      pan: { x: 50, y: 50 }, // 初始偏移，等待onReady重新计算
      tool: "move",
      gridVisible: true,

      // 历史记录
      history: [],
      historyIndex: -1,

      // 容器尺寸
      containerSize: { width: 0, height: 0 },
      canvasReady: false,
      isCalculated: false, // 标记是否已加载像素数据

      // 状态
      showSaveConfirm: false,
      showLeaveConfirm: false,
      hasUnsavedChanges: false,
      savedPixelsSnapshot: "",
      isHelpOpen: false,
      toastVisible: false, // Toast显示状态，用于隐藏canvas

      // 帮助内容
      helpItems: [
        {
          iconName: "edit",
          title: "基础操作",
          description:
            "单指在画布上拖动即可绘画。双指捏合可缩放画布，双指拖动可移动视角。",
        },
        {
          iconName: "move",
          title: "拖动模式",
          description:
            "切换到底部的【拖动】工具后，单指即可随意拖动画布位置，避免误触绘画。",
        },
        {
          iconName: "picture",
          title: "颜色选择",
          description: "点击底部色板选择当前颜色。",
        },
        {
          iconName: "save",
          title: "保存与进度",
          description:
            "记得经常点击右上角的保存图标。系统也会尝试在退出时提醒保存。",
        },
      ],
    };
  },

  computed: {
    // 是否可编辑（自建草稿/已退回可编辑，收藏项目不可编辑）
    isEditable() {
      if (!this.project) return false;
      if (this.project.source === "collected") return false;
      if (this.project.permissions && !this.project.permissions.canEdit)
        return false;
      const status = this.project.status || "draft";
      return status === "draft" || status === "rejected";
    },

    // 是否为收藏项目
    isCollectedProject() {
      return this.project && this.project.source === "collected";
    },

    boardOffset() {
      if (!this.boardId || !this.project) {
        // 全画布模式：使用填充后的尺寸
        const paddedWidth =
          this.project?.paddedWidth || this.project?.width || PERLER_BOARD_SIZE;
        const paddedHeight =
          this.project?.paddedHeight || this.project?.height || PERLER_BOARD_SIZE;
        return {
          x: 0,
          y: 0,
          w: paddedWidth,
          h: paddedHeight,
        };
      }
      // 单板模式：使用52x52
      const row = (this.boardId.charCodeAt(0) || 65) - 65;
      const col = parseInt(this.boardId.slice(1) || "1") - 1;
      return {
        x: col * PERLER_BOARD_SIZE,
        y: row * PERLER_BOARD_SIZE,
        w: PERLER_BOARD_SIZE,
        h: PERLER_BOARD_SIZE,
      };
    },

    displayPixels() {
      if (!this.boardId) {
        return this.pixels;
      }

      const local = new Map();
      this.pixels.forEach((color, key) => {
        const [px, py] = key.split(",").map(Number);
        if (
          px >= this.boardOffset.x &&
          px < this.boardOffset.x + this.boardOffset.w &&
          py >= this.boardOffset.y &&
          py < this.boardOffset.y + this.boardOffset.h
        ) {
          local.set(
            `${px - this.boardOffset.x},${py - this.boardOffset.y}`,
            color,
          );
        }
      });
      return local;
    },

    // 判断是否有弹窗显示
    hasModalOpen() {
      return (
        this.showSaveConfirm ||
        this.showLeaveConfirm ||
        this.isHelpOpen ||
        this.toastVisible
      );
    },
  },

  watch: {
    "containerSize.width"(newVal, oldVal) {
      if (newVal > 0 && newVal !== oldVal) {
        this.handleFit();
      }
    },
    "containerSize.height"(newVal, oldVal) {
      if (newVal > 0 && newVal !== oldVal) {
        this.handleFit();
      }
    },
    "boardOffset.w"() {
      if (this.canvasReady) {
        this.handleFit();
      }
    },
    "boardOffset.h"() {
      if (this.canvasReady) {
        this.handleFit();
      }
    },
    pixels: {
      handler(newPixels) {
        const currentSnapshot = JSON.stringify(Array.from(newPixels.entries()));
        this.hasUnsavedChanges = currentSnapshot !== this.savedPixelsSnapshot;
      },
      deep: true,
    },
  },

  onLoad(options) {
    this.projectStore = useProjectStore();
    this.toast = useToast();

    this.projectId = options.id;
    this.boardId = options.board || "";
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

    // 收藏项目提示只读
    if (this.project.source === "collected") {
      uni.showToast({
        title: "收藏作品仅可查看和辅助拼豆",
        icon: "none",
        duration: 2000,
      });
    }
    // 已发布/审核中项目提示只读
    else if (
      this.project.status === "published" ||
      this.project.status === "reviewing"
    ) {
      uni.showToast({
        title: "已发布作品仅可查看和拼豆",
        icon: "none",
        duration: 2000,
      });
    }

    this.loadPalette();
    this.loadPixels();

    // 首次使用帮助
    const hasSeenHelp = uni.getStorageSync("hasSeenEditorHelp");
    if (!hasSeenHelp) {
      this.isHelpOpen = true;
      uni.setStorageSync("hasSeenEditorHelp", "true");
    }
  },

  onReady() {
    // 注册自定义 Toast 实例
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }

    setTimeout(() => {
      const query = uni.createSelectorQuery().in(this);
      query
        .select(".canvas-container")
        .boundingClientRect((data) => {
          if (data && data.width > 0 && data.height > 0) {
            this.containerSize = {
              width: data.width,
              height: data.height,
            };

            const fitZoomW = (data.width * 0.9) / this.boardOffset.w;
            const fitZoomH = (data.height * 0.9) / this.boardOffset.h;
            const fitZoom = Math.min(fitZoomW, fitZoomH, 50);

            const boardPixelWidth = this.boardOffset.w * fitZoom;
            const boardPixelHeight = this.boardOffset.h * fitZoom;

            this.zoom = fitZoom;
            this.pan = {
              x: (data.width - boardPixelWidth) / 2,
              y: (data.height - boardPixelHeight) / 2,
            };

            console.log("初始化:", {
              containerWidth: data.width,
              containerHeight: data.height,
              boardPixelWidth,
              boardPixelHeight,
              zoom: fitZoom,
              panX: this.pan.x,
              panY: this.pan.y,
            });

            // 确保 pan 和 zoom 设置完成后再显示画布
            this.$nextTick(() => {
              this.canvasReady = true;

              setTimeout(() => {
                this.isCalculated = true;
              }, 10);
            });
          }
        })
        .exec();
    }, 100);
  },

  methods: {
    getActionButtonStyle(isActive = false) {
      if (isActive) {
        return "background:#4f7fff;border:2rpx solid #000000;box-shadow:2rpx 2rpx 0 #000000;color:#ffffff;";
      }
      return "background:#ffd23f;border:2rpx solid #000000;box-shadow:2rpx 2rpx 0 #000000;color:#000000;";
    },

    getActionButtonColor(isActive = false) {
      if (isActive) {
        return "#ffffff";
      }
      return "#000000";
    },

    getToolButtonStyle(toolKey) {
      if (this.tool === toolKey) {
        if (toolKey === "move") {
          return "background:#111111;border:3rpx solid #000000;box-shadow:2rpx 2rpx 0 #000000;color:#ffffff;";
        }
        if (toolKey === "pencil") {
          return "background:#4f7fff;border:3rpx solid #000000;box-shadow:2rpx 2rpx 0 #000000;color:#ffffff;";
        }
        if (toolKey === "eraser") {
          return "background:#d92d20;border:3rpx solid #000000;box-shadow:2rpx 2rpx 0 #000000;color:#ffffff;";
        }
      }
      return "background:#ffd23f;border:3rpx solid #000000;box-shadow:2rpx 2rpx 0 #000000;color:#000000;";
    },

    getToolButtonColor(toolKey) {
      if (
        this.tool === toolKey &&
        (toolKey === "move" || toolKey === "pencil" || toolKey === "eraser")
      ) {
        return "#ffffff";
      }
      return "#000000";
    },

    setTool(newTool) {
      if (this.tool !== newTool) {
        this.tool = newTool;
      }
    },

    loadPalette() {
      if (this.project.palette && this.project.palette.length > 0) {
        this.paletteColors = this.project.palette
          .map((code) => getColorByCode(code))
          .filter((color) => color !== undefined);
      }
    },

    loadPixels() {
      const pixels = this.projectStore.getProjectPixels(this.projectId);
      this.pixels = pixels;
      this.history = [new Map(pixels)];
      this.historyIndex = 0;
      this.savedPixelsSnapshot = JSON.stringify(Array.from(pixels.entries()));
      this.hasUnsavedChanges = false;
    },

    handlePixelClick(x, y) {
      if (!this.isEditable) {
        const msg = this.isCollectedProject
          ? "收藏作品不可编辑"
          : "已发布作品不可编辑";
        uni.showToast({ title: msg, icon: "none" });
        return;
      }
      if (this.tool === "move") return;

      const globalX = x + this.boardOffset.x;
      const globalY = y + this.boardOffset.y;
      const key = `${globalX},${globalY}`;

      const newPixels = new Map(this.pixels);

      if (this.tool === "eraser") {
        if (!newPixels.has(key)) return;
        newPixels.delete(key);
      } else if (this.tool === "pencil") {
        if (newPixels.get(key) === this.selectedColor) return;
        newPixels.set(key, this.selectedColor);
      }

      this.pixels = newPixels;

      const newHistory = this.history.slice(0, this.historyIndex + 1);
      newHistory.push(new Map(newPixels));
      if (newHistory.length > 50) newHistory.shift();

      this.history = newHistory;
      this.historyIndex = newHistory.length - 1;
    },

    handleUndo() {
      if (!this.isEditable) return;
      if (this.historyIndex > 0) {
        this.historyIndex--;
        this.pixels = new Map(this.history[this.historyIndex]);
      }
    },

    handleRedo() {
      if (!this.isEditable) return;
      if (this.historyIndex < this.history.length - 1) {
        this.historyIndex++;
        this.pixels = new Map(this.history[this.historyIndex]);
      }
    },

    handleZoom(delta) {
      const centerX = this.containerSize.width / 2;
      const centerY = this.containerSize.height / 2;

      const newZoom = Math.max(1, Math.min(50, this.zoom + delta));
      const scaleFactor = newZoom / this.zoom;

      this.pan.x = centerX - (centerX - this.pan.x) * scaleFactor;
      this.pan.y = centerY - (centerY - this.pan.y) * scaleFactor;
      this.zoom = newZoom;
    },

    handlePinchZoom(delta, centerX, centerY) {
      const newZoom = Math.max(1, Math.min(50, this.zoom + delta));
      const scaleFactor = newZoom / this.zoom;

      this.pan = {
        x: centerX - (centerX - this.pan.x) * scaleFactor,
        y: centerY - (centerY - this.pan.y) * scaleFactor,
      };
      this.zoom = newZoom;
    },

    handlePan(dx, dy) {
      this.pan = { x: this.pan.x + dx, y: this.pan.y + dy };
    },

    handleFit() {
      if (
        this.containerSize.width > 0 &&
        this.containerSize.height > 0 &&
        this.boardOffset.w > 0
      ) {
        const fitZoomW = (this.containerSize.width * 0.9) / this.boardOffset.w;
        const fitZoomH = (this.containerSize.height * 0.9) / this.boardOffset.h;
        const fitZoom = Math.min(fitZoomW, fitZoomH, 50);

        this.zoom = fitZoom;

        const boardPixelWidth = this.boardOffset.w * fitZoom;
        const boardPixelHeight = this.boardOffset.h * fitZoom;

        this.pan = {
          x: (this.containerSize.width - boardPixelWidth) / 2,
          y: (this.containerSize.height - boardPixelHeight) / 2,
        };
      }
    },

    setSelectedColor(colorHex) {
      if (!colorHex) {
        this.selectedColor = "";
        this.selectedColorCode = "";
        return;
      }

      this.selectedColor = colorHex;
      const colorObj = this.paletteColors.find((c) => c.hex === colorHex);
      if (colorObj) {
        this.selectedColorCode = colorObj.code;
      }
    },

  },
};
</script>

<style scoped>
.editor-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-primary);
}

.project-name-wrapper {
  display: flex;
  align-items: center;
  gap: 8rpx;
}

.project-name {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--text-primary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.board-id {
  font-weight: normal;
  color: var(--text-secondary);
}

.project-size {
  font-size: 20rpx;
  color: var(--text-secondary);
  font-family: monospace;
}

.board-mode {
  font-size: 18rpx;
}

.icon-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--text-secondary);
  transition: var(--transition-base);
}

.icon-btn:active {
  color: var(--nb-yellow);
}

.device-btn.connected {
  color: var(--nb-yellow);
  animation: pulse-glow 2s ease-in-out infinite;
}

@keyframes pulse-glow {
  0%,
  100% {
    opacity: 1;
  }
  50% {
    opacity: 0.6;
  }
}

/* Canvas 区域 */
.canvas-container {
  flex: 1;
  position: relative;
  overflow: hidden;
  background-color: var(--bg-primary);
  touch-action: none;
}

/* 底部工具栏 */
.toolbar {
  background-color: var(--bg-elevated);
  border-top: 2rpx solid var(--nb-ink);
  padding-bottom: var(--layout-bottom-offset);
  z-index: 20;
}

/* 操作栏 */
.action-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16rpx 32rpx;
  border-bottom: 2rpx solid var(--nb-ink);
  background-color: var(--bg-tertiary);
}

.action-group {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 0;
  background-color: transparent;
  border-radius: 0;
}

.action-btn {
  width: 72rpx;
  height: 72rpx;
  padding: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 8rpx;
  box-sizing: border-box;
  transition: var(--transition-base);
}

.action-btn:active {
  background-color: var(--bg-elevated);
}

.action-btn.active {
  background: #4f7fff !important;
  border: 2rpx solid #000000 !important;
  box-shadow: none !important;
  color: #ffffff !important;
}

.action-btn.row-mode-btn.active {
  animation: pulse-glow 2s ease-in-out infinite;
}

.action-btn.disabled {
  opacity: 0.3;
  color: var(--text-tertiary);
}

.action-btn .iconfont {
  display: block;
  line-height: 1;
}

.action-btn.active .iconfont {
  color: #ffffff !important;
}

.divider {
  display: none;
}

/* 工具选择 */
.tools {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 16rpx;
  padding: 14rpx 24rpx;
}

.row-controls {
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 12rpx 24rpx;
  background-color: var(--bg-secondary);
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
}

.row-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 56rpx;
  height: 56rpx;
  border-radius: 0;
  background-color: var(--bg-tertiary);
  color: var(--nb-yellow);
  transition: var(--transition-base);
  border: 2rpx solid #000000;
}

.row-btn:active {
  transform: translate(2rpx, 2rpx);
  background-color: var(--nb-yellow);
  color: #000000;
}

.row-text {
  font-size: 24rpx;
  font-weight: bold;
  color: var(--nb-yellow);
  min-width: 120rpx;
  text-align: center;
}

.tool-btn {
  flex: 1;
  max-width: 168rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 4rpx;
  min-height: 92rpx;
  padding: 12rpx 10rpx;
  border-radius: 0;
  box-sizing: border-box;
  text-align: center;
  transition: var(--transition-base);
}

.tool-btn:active {
  transform: scale(0.98);
}

.tool-btn.active {
  border-color: #000000;
}

.tool-btn.pencil.active {
  background: #4f7fff;
  box-shadow: none;
  color: #ffffff;
}

.tool-btn.eraser.active {
  background: #d92d20;
  border-color: #000000;
  box-shadow: none;
  color: #ffffff;
}

.tool-btn.move.active {
  background: #111111;
  border-color: #000000;
  box-shadow: none;
  color: #ffffff;
}

.tool-label {
  font-size: 18rpx;
  font-weight: bold;
  color: currentColor;
  letter-spacing: 1rpx;
}

.tool-btn .iconfont {
  display: block;
  line-height: 1;
  color: currentColor !important;
}

.tool-btn text {
  color: currentColor !important;
}

/* 颜色选择器 */
.color-palette {
  padding: 0 32rpx 16rpx;
}

/* 弹窗样式 */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.8);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 99998;
  padding: 32rpx;
}

.modal {
  width: 100%;
  max-width: 600rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid rgba(0, 243, 255, 0.3);
  border-radius: 32rpx;
  padding: 48rpx 32rpx 32rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24rpx;
  box-shadow: var(--nb-shadow-strong);
}

.modal-icon {
  width: 80rpx;
  height: 80rpx;
  border-radius: 50%;
  background-color: #f7f7f7;
  border: 2rpx solid rgba(0, 243, 255, 0.2);
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--accent-color);
}

.modal-icon.warning {
  background-color: rgba(255, 170, 0, 0.1);
  border-color: rgba(255, 170, 0, 0.2);
  color: #ff976a;
}

.modal-title {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
  text-align: center;
}

.modal-desc {
  font-size: 24rpx;
  color: var(--text-secondary);
  text-align: center;
  line-height: 1.6;
}

.modal-actions {
  display: flex;
  gap: 16rpx;
  width: 100%;
  margin-top: 8rpx;
}

.modal-actions.vertical {
  flex-direction: column;
  gap: 16rpx;
}

.modal-btn {
  flex: 1;
  min-height: 88rpx;
  padding: 0 24rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 0;
  box-sizing: border-box;
  border: 2rpx solid var(--nb-ink);
  box-shadow: 2rpx 2rpx 0 var(--nb-ink);
  transition: opacity 0.2s;
}

.modal-btn:active {
  opacity: 0.8;
  box-shadow: none;
}

.modal-btn.primary {
  background-color: var(--nb-yellow);
}

.modal-btn.secondary {
  background-color: #ffffff;
}

.modal-btn.text {
  background-color: transparent;
  border: 0;
  box-shadow: none;
}

.modal-btn-text {
  font-size: 28rpx;
  font-weight: bold;
  color: inherit;
}

.modal-btn.primary .modal-btn-text {
  color: #000000;
}

.modal-btn.secondary .modal-btn-text,
.modal-btn.text .modal-btn-text {
  color: var(--text-primary);
}

/* 浅色主题适配 */
.light-theme .tool-btn.move.active {
  background-color: rgba(0, 153, 204, 0.1);
  border-color: var(--nb-yellow);
  color: var(--nb-yellow);
}

/* 重命名弹窗 */
.rename-modal {
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.8);
  backdrop-filter: none;
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  padding: 48rpx;
}

.rename-modal {
  width: 100%;
  max-width: 500rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  overflow: hidden;
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--nb-ink);
}

.modal-title {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.modal-close {
  width: 48rpx;
  height: 48rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--text-secondary);
}

.modal-body {
  padding: 32rpx;
}

.rename-input {
  width: 100%;
  padding: 20rpx 24rpx;
  font-size: 28rpx;
  color: var(--text-primary);
  background-color: #fff8d6;
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: all 0.2s;
  box-sizing: border-box;
}

.rename-input::placeholder {
  color: var(--text-tertiary);
  opacity: 0.6;
}

.name-input-wrapper {
  position: relative;
  padding: 24rpx 0;
  border-bottom: 3rpx solid var(--nb-ink);
  transition: var(--transition-base);
  min-height: 80rpx;
  display: flex;
  align-items: center;
  width: 100%;
}

.name-input-wrapper.focused {
  border-bottom-color: #00f3ff;
  box-shadow: 0 3rpx 0 rgba(0, 243, 255, 0.3);
}

.name-input {
  width: 100%;
  font-size: 32rpx;
  color: var(--text-primary);
  background-color: transparent;
  border: none;
  outline: none;
  line-height: 1.2;
}

.name-input::placeholder {
  color: rgba(136, 136, 136, 0.5);
}

.modal-actions {
  display: flex;
  gap: 16rpx;
  padding: 24rpx 32rpx;
  border-top: 2rpx solid var(--nb-ink);
}

.modal-btn {
  flex: 1;
  min-height: 88rpx;
  padding: 0 24rpx;
  border-radius: 0;
  text-align: center;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  border: 2rpx solid var(--nb-ink);
  box-shadow: 2rpx 2rpx 0 var(--nb-ink);
  transition: all 0.2s;
}

.modal-btn.cancel {
  background-color: #ffffff;
}

.modal-btn.confirm {
  background-color: var(--nb-yellow);
}

.modal-btn-text {
  font-size: 28rpx;
  font-weight: bold;
}

.modal-btn.cancel .modal-btn-text {
  color: var(--text-primary);
}

.modal-btn.confirm .modal-btn-text {
  color: #000000;
}

.project-name-wrapper {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.edit-icon {
  color: var(--text-tertiary);
  opacity: 0.6;
}
</style>
