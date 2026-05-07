<template>
  <view class="assist-page glx-page-shell" :class="{ 'light-theme': false }">
    <!-- 状态栏占位 -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>

    <!-- 行导航弹窗 -->
    <view
      v-if="isRowListOpen"
      class="modal-overlay"
      @click="isRowListOpen = false"
    >
      <view class="row-list-modal" @click.stop>
        <view class="modal-header">
          <text class="modal-title">行导航</text>
          <view class="close-btn" @click="goToOverview">
            <text class="close-icon">×</text>
          </view>
        </view>

        <scroll-view scroll-y class="row-grid-scroll">
          <view class="row-grid">
            <view
              v-for="i in boardSize"
              :key="i - 1"
              class="row-grid-item"
              :class="{
                current: currentRow === i - 1,
                completed: completedRows.has(i - 1) && currentRow !== i - 1,
              }"
              @click="handleRowListSelect(i - 1)"
            >
              <text class="row-number">{{ i }}</text>
              <Icon
                name="check-item"
                v-if="completedRows.has(i - 1)"
                :size="32"
                class="check-mark"
              />
            </view>
          </view>
        </scroll-view>

        <view class="modal-footer">
          <view class="legend-item">
            <view class="legend-box completed-box"></view>
            <text class="legend-text">已完成</text>
          </view>
          <view class="legend-item">
            <view class="legend-box current-box"></view>
            <text class="legend-text">当前行</text>
          </view>
          <view class="legend-item">
            <view class="legend-box uncompleted-box"></view>
            <text class="legend-text">未完成</text>
          </view>
        </view>
      </view>
    </view>
    <!-- 导航栏 -->
    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="goToOverview">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--nb-ink)"
        />
      </view>
      <text class="nav-title glx-topbar__title">{{ boardId }}</text>
    </view>
    <!-- 头部 -->
    <view class="header glx-topbar glx-page-shell__fixed" :style="{ paddingRight: capsuleRight + 'px' }">
      <!-- <view class="header-left">
        <view class="back-btn" @click="goToOverview">
          <text class="icon">‹</text>
        </view>
        <text class="board-id">{{ boardId }}</text>
      </view> -->

      <!-- 模式切换 -->
      <!-- <view class="mode-switch">
        <view 
          class="mode-btn"
          :class="{ 'active': assistMode === 'color' }"
          @click="setAssistMode('color')"
        >
          <Icon name="picture" :size="28" />
          <text class="mode-label">颜色</text>
        </view>
        <view 
          class="mode-btn"
          :class="{ 'active': assistMode === 'row' }"
          @click="setAssistMode('row')"
        >
          <Icon name="menu" :size="28" />
          <text class="mode-label">逐行</text>
        </view>
      </view>
      -->
    </view>

    <!-- 当前信息提示 -->
    <view v-if="assistMode === 'row'" class="info-banner">
      <text class="info-text">行 {{ currentRow + 1 }} / {{ boardSize }}</text>
    </view>

    <!--  
    <view v-if="assistMode === 'color' && highlightColor" class="info-banner">
      <view class="color-dot" :style="{ backgroundColor: highlightColor }"></view>
      <text class="info-text">{{ getColorCode(highlightColor) }} ({{ boardColors.get(highlightColor) }})</text>
    </view>
    -->
    <!-- Canvas 画布 -->
    <view
      class="canvas-container"
      :style="{ visibility: hasCanvasOverlayOpen ? 'hidden' : 'visible' }"
    >
      <PixelCanvas
        v-if="canvasReady && !hasCanvasOverlayOpen"
        :width="boardSize"
        :height="boardSize"
        :pixels="isCalculated ? localPixels : new Map()"
        :zoom="zoom"
        :offset-x="pan.x"
        :offset-y="pan.y"
        :canvas-width="canvasWidth"
        :canvas-height="canvasHeight"
        :grid-visible="true"
        :highlight-color="highlightColor"
        :highlight-row="focusMode && assistMode === 'row' ? currentRow : null"
        :allow-single-touch-pan="true"
        :is-dark-mode="false"
        canvas-id="assistCanvas"
        @pan="handlePan"
        @zoom="handlePinchZoom"
      />
    </view>

    <!-- 底部控制区 -->
    <view class="controls">
      <!-- 颜色列表 -->
      <view class="color-section">
        <view class="section-header">
          <text class="section-title"
            >{{ assistMode === "row" ? "行颜色" : "全部颜色" }} ({{
              displayedColors.size
            }})</text
          >
          <view
            v-if="assistMode === 'color' && highlightColor"
            style="
              display: flex;
              justify-content: space-between;
              align-items: center;
            "
          >
            <view
              class="cont-color-dot"
              :style="{ backgroundColor: highlightColor }"
            ></view>
            <text class="info-text"
              >{{ getColorCode(highlightColor) }} ({{
                boardColors.get(highlightColor)
              }})</text
            >
          </view>
          <!-- 操作按钮 -->
          <view class="header-actions">
            <view class="icon-btn" @click="isHelpOpen = true">
              <Icon name="help" :size="24" />
            </view>
            <view
              class="icon-btn device-btn"
              :class="{ connected: deviceConnected }"
              @click="toggleDeviceSync"
            >
              <Icon name="link" :size="24" />
            </view>
            <view
              v-if="deviceConnected"
              class="icon-btn"
              :class="{ active: deviceCanvasSynced }"
              @click="syncConnectedCanvas"
            >
              <Icon name="upload" :size="24" />
            </view>
            <view class="icon-btn" @click="handleFit">
              <Icon name="fullscreen-expand" :size="24" />
            </view>
            <view
              class="icon-btn"
              :class="{ active: focusMode }"
              @click="focusMode = !focusMode"
            >
              <Icon name="browse" v-if="focusMode" :size="24" />
              <Icon name="eye-close" v-else :size="24" />
            </view>
          </view>
        </view>

        <scroll-view
          scroll-y
          class="color-list"
          :scroll-into-view="scrollIntoView"
          :scroll-top="colorListScrollTop"
          :scroll-with-animation="true"
          @scroll="handleColorScroll"
        >
          <view v-if="assistMode === 'color' && colorGroups.length > 0">
            <!-- 按字母分组显示 -->
            <view
              v-for="group in colorGroups"
              :key="group.letter"
              class="color-group"
            >
              <!-- 分组标题 -->
              <view :id="'letter-' + group.letter" class="group-header">
                <text class="group-letter">{{ group.letter }}</text>
                <text class="group-count">({{ group.colors.length }})</text>
              </view>

              <!-- 该字母下的颜色 -->
              <view class="color-grid">
                <view
                  v-for="[color, count] in group.colors"
                  :key="color"
                  class="color-item"
                  :class="{
                    active: highlightColor === color,
                    completed:
                      isColorCompleted(color) && highlightColor !== color,
                  }"
                  @click="
                    setHighlightColor(highlightColor === color ? null : color)
                  "
                >
                  <!-- 左侧：颜色和色号 -->
                  <view class="color-info">
                    <view
                      class="color-swatch"
                      :style="{ backgroundColor: color }"
                    ></view>
                    <text class="color-code">{{ getColorCode(color) }}</text>
                  </view>

                  <!-- 右侧：勾选框和数量 -->
                  <view class="color-actions">
                    <view
                      class="check-btn"
                      :class="{ checked: isColorCompleted(color) }"
                      @click.stop="toggleColorComplete(color)"
                    >
                      <text v-if="isColorCompleted(color)" class="check-icon"
                        >✓</text
                      >
                      <view v-else class="check-box"></view>
                    </view>
                    <text class="color-count">×{{ count }}</text>
                  </view>
                </view>
              </view>
            </view>
          </view>

          <view v-else>
            <!-- 逐行模式：不分组 -->
            <view class="color-grid">
              <view
                v-for="[color, count] in Array.from(displayedColors.entries())"
                :key="color"
                class="color-item"
                :class="{
                  active: highlightColor === color,
                  completed:
                    isColorCompleted(color) && highlightColor !== color,
                }"
                @click="
                  setHighlightColor(highlightColor === color ? null : color)
                "
              >
                <view class="color-info">
                  <view
                    class="color-swatch"
                    :style="{ backgroundColor: color }"
                  ></view>
                  <text class="color-code">{{ getColorCode(color) }}</text>
                </view>

                <view class="color-actions">
                  <view
                    class="check-btn"
                    :class="{ checked: isColorCompleted(color) }"
                    @click.stop="toggleColorComplete(color)"
                  >
                    <Icon
                      name="check-item"
                      v-if="isColorCompleted(color)"
                      :size="28"
                      color="#ffffff"
                    />
                    <view v-else class="check-box"></view>
                  </view>
                  <text class="color-count">×{{ count }}</text>
                </view>
              </view>
            </view>
          </view>
        </scroll-view>

        <!-- 右侧字母索引条（仅颜色模式） -->
        <view
          v-if="assistMode === 'color' && availableColorLetters.length > 1"
          class="index-bar"
          @touchstart="handleIndexTouchStart"
          @touchmove="handleIndexTouchMove"
          @touchend="handleIndexTouchEnd"
        >
          <view
            v-for="letter in availableColorLetters"
            :key="letter"
            :data-letter="letter"
            class="index-letter"
            :class="{ active: currentVisibleLetter === letter }"
            @click="scrollToColorLetter(letter)"
          >
            {{ letter }}
          </view>
        </view>

        <!-- 字母提示气泡 -->
        <view
          v-if="showLetterBubble && currentTouchLetter"
          class="letter-bubble"
          :style="{
            left: bubblePosition.x + 'px',
            top: bubblePosition.y + 'px',
          }"
        >
          {{ currentTouchLetter }}
        </view>
      </view>

      <!-- 行控制（仅逐行模式） -->
      <view v-if="assistMode === 'row'" class="row-controls">
        <view class="row-btn" @click="prevRow">
          <text class="row-icon">▲</text>
        </view>

        <view class="row-info-container">
          <view class="row-list-btn" @click="isRowListOpen = true">
            <Icon name="column-3" :size="32" />
            <text class="list-label">列表</text>
          </view>

          <view class="row-info">
            <text class="row-number">{{ currentRow + 1 }}</text>
            <text class="row-total">/ {{ boardSize }}</text>
          </view>

          <view
            class="row-complete-btn"
            :class="{ completed: completedRows.has(currentRow) }"
            @click="toggleRowComplete"
          >
            <Icon
              name="check-item"
              v-if="completedRows.has(currentRow)"
              :size="28"
              color="#ffffff"
            />
            <view v-else class="complete-box"></view>
            <text class="complete-text">{{
              completedRows.has(currentRow) ? "完成" : "标记"
            }}</text>
          </view>
        </view>

        <view class="row-btn" @click="nextRow">
          <text class="row-icon">▼</text>
        </view>
      </view>
    </view>

    <!-- HelpModal -->
    <HelpModal
      :is-open="isHelpOpen"
      @close="isHelpOpen = false"
      title="拼豆操作指南"
      :items="helpItems"
    />

    <!-- 连接弹窗 -->
    <ConnectModal
      :visible.sync="showConnectModal"
      title="连接 Glowxel PixelBoard"
      description="请输入 Glowxel PixelBoard 的 IP 地址"
      :placeholder="deviceStore.deviceIp || '192.168.31.84'"
      :defaultValue="deviceStore.deviceIp"
      ref="connectModal"
      @confirm="handleConnectConfirm"
      @cancel="handleConnectCancel"
    />

    <!-- 自定义 Toast 组件 -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useProjectStore } from "../../store/project.js";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import { ARTKAL_COLORS_FULL } from "../../data/artkal-colors-full.js";
import statusBarMixin from "../../mixins/statusBar.js";
import PixelCanvas from "../../components/PixelCanvas.vue";
import HelpModal from "../../components/HelpModal.vue";
import ConnectModal from "../../components/ConnectModal.vue";
import Toast from "../../components/Toast.vue";
import Icon from "../../components/Icon.vue";
import assistDeviceSyncMixin from "./mixins/assistDeviceSyncMixin.js";
import { PERLER_BOARD_SIZE } from "../../constants/perler.js";

// 创建颜色查找映射
const colorCodeMap = new Map();
ARTKAL_COLORS_FULL.forEach((color) => {
  colorCodeMap.set(color.hex.toLowerCase(), color.code);
});

export default {
  mixins: [statusBarMixin, assistDeviceSyncMixin],
  components: {
    PixelCanvas,
    HelpModal,
    ConnectModal,
    Toast,
    Icon,
  },

  data() {
    return {
      projectStore: null,
      deviceStore: null,
      toast: null,
      projectId: "",
      boardId: "",
      project: null,

      // 数据
      pixels: new Map(),
      localPixels: new Map(),

      // 模式
      assistMode: "color",
      focusMode: true,
      currentRow: 0,
      highlightColor: null,
      lastSyncedRow: undefined,
      deviceCanvasSynced: false,

      // 进度
      completedRows: new Set(),
      completedColors: new Set(),

      // 视图
      zoom: 7,
      pan: { x: 0, y: 0 },

      // 容器
      canvasWidth: 0,
      canvasHeight: 0,
      canvasReady: false,
      isCalculated: false,

      // UI
      isHelpOpen: false,
      isRowListOpen: false,
      showConnectModal: false,
      scrollIntoView: "",
      colorListScrollTop: -1,
      currentVisibleLetter: "",
      showLetterBubble: false,
      currentTouchLetter: "",
      bubblePosition: { x: 0, y: 0 },
      isIndexTouching: false,
      _modeRestoredOnExit: false,
      boardSize: PERLER_BOARD_SIZE,

      helpItems: [
        {
          iconName: "picture",
          title: "颜色模式 (默认)",
          description:
            "查看当前板子用到的所有颜色。点击某个颜色，画布上对应位置会高亮闪烁，方便批量拼装同色豆子。",
        },
        {
          iconName: "menu",
          title: "行导航",
          description:
            "点击行控制栏左侧的「列表」图标，可以查看所有行的完成状态，并快速跳转到指定行。",
        },
        {
          iconName: "prompt",
          title: "高亮辅助",
          description: "点击任意颜色块即可高亮显示。再次点击取消高亮。",
        },
        {
          iconName: "move",
          title: "画布操作",
          description:
            "双指捏合缩放画布，单指拖动查看细节。网格线会随缩放自动显示。",
        },
      ],
    };
  },

  computed: {
    deviceConnected() {
      return this.deviceStore?.connected || false;
    },

    boardX() {
      const colIndex = parseInt(this.boardId.slice(1)) - 1;
      return colIndex * this.boardSize;
    },

    boardY() {
      const rowIndex = this.boardId.charCodeAt(0) - 65;
      return rowIndex * this.boardSize;
    },

    rowColorsMap() {
      const map = new Map();
      this.localPixels.forEach((color, key) => {
        const [, ly] = key.split(",").map(Number);
        if (!map.has(ly)) map.set(ly, new Set());
        map.get(ly).add(color);
      });
      return map;
    },

    colorRowsMap() {
      const map = new Map();
      this.localPixels.forEach((color, key) => {
        const [, ly] = key.split(",").map(Number);
        if (!map.has(color)) map.set(color, new Set());
        map.get(color).add(ly);
      });
      return map;
    },

    currentRowColors() {
      const colors = new Map();
      this.localPixels.forEach((color, key) => {
        const [, ly] = key.split(",").map(Number);
        if (ly === this.currentRow) {
          colors.set(color, (colors.get(color) || 0) + 1);
        }
      });
      return colors;
    },

    boardColors() {
      const colors = new Map();
      this.localPixels.forEach((color) => {
        colors.set(color, (colors.get(color) || 0) + 1);
      });
      return colors;
    },

    displayedColors() {
      return this.assistMode === "row"
        ? this.currentRowColors
        : this.boardColors;
    },

    // 按字母分组颜色（仅颜色模式）
    colorGroups() {
      if (this.assistMode !== "color") return [];

      const groups = new Map();

      // 将颜色按色号分组
      Array.from(this.displayedColors.entries()).forEach(([color, count]) => {
        const code = this.getColorCode(color);
        if (code && code !== "?") {
          const letter = code.charAt(0).toUpperCase();
          if (!groups.has(letter)) {
            groups.set(letter, []);
          }
          groups.get(letter).push([color, count]);
        }
      });

      // 排序并转换为数组
      return Array.from(groups.entries())
        .sort((a, b) => a[0].localeCompare(b[0]))
        .map(([letter, colors]) => ({
          letter,
          colors: colors.sort((a, b) => {
            const codeA = this.getColorCode(a[0]);
            const codeB = this.getColorCode(b[0]);
            return codeA.localeCompare(codeB);
          }),
        }));
    },

    // 可用的字母列表
    availableColorLetters() {
      return this.colorGroups.map((g) => g.letter);
    },
    hasCanvasOverlayOpen() {
      return this.isHelpOpen || this.isRowListOpen || this.showConnectModal;
    },
  },

  watch: {
    // 弹窗关闭时，如果画布尚未初始化则重试
    isHelpOpen(newVal) {
      if (!newVal && !this.canvasReady) {
        this.$nextTick(() => this.initCanvas());
      }
    },
    isRowListOpen(newVal) {
      if (!newVal && !this.canvasReady) {
        this.$nextTick(() => this.initCanvas());
      }
    },
    showConnectModal(newVal) {
      if (!newVal && !this.canvasReady) {
        this.$nextTick(() => this.initCanvas());
      }
    },
    assistMode() {
      this.highlightColor = null;
      this.saveProgress();
    },
    completedRows: {
      handler() {
        this.saveProgress();
      },
      deep: true,
    },
    completedColors: {
      handler() {
        this.saveProgress();
      },
      deep: true,
    },
  },

  onLoad(options) {
    this.projectStore = useProjectStore();
    this.deviceStore = useDeviceStore();
    this.toast = useToast();

    this.projectId = options.id;
    this.boardId = options.board;
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

    // 检查是否首次使用
    const hasSeenHelp = uni.getStorageSync("hasSeenBoardHelp");
    if (!hasSeenHelp) {
      this.isHelpOpen = true;
      uni.setStorageSync("hasSeenBoardHelp", "true");
    }

    this.loadPixels();
    this.loadProgress();
    this._modeRestoredOnExit = false;
  },

  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
    this.initCanvas();
  },

  onShow() {
    if (!this.canvasReady) {
      this.initCanvas();
    }
  },

  onUnload() {
    this.saveProgress();
    this.restoreModeAfterCanvas();
  },

  methods: {
    // Canvas 初始化
    initCanvas() {
      if (this.canvasReady) return;
      setTimeout(() => {
        const query = uni.createSelectorQuery().in(this);
        query
          .select(".canvas-container")
          .boundingClientRect((data) => {
            if (data && data.width > 0 && data.height > 0) {
              this.canvasWidth = data.width;
              this.canvasHeight = data.height;

              const fitZoomW = (data.width * 0.9) / this.boardSize;
              const fitZoomH = (data.height * 0.9) / this.boardSize;
              const fitZoom = Math.min(fitZoomW, fitZoomH, 50);

              const boardPixelWidth = this.boardSize * fitZoom;
              const boardPixelHeight = this.boardSize * fitZoom;

              this.zoom = fitZoom;
              this.pan = {
                x: (data.width - boardPixelWidth) / 2,
                y: (data.height - boardPixelHeight) / 2,
              };

              this.canvasReady = true;

              setTimeout(() => {
                this.isCalculated = true;
              }, 10);
            }
          })
          .exec();
      }, 150);
    },

    // ========== 原有方法 ==========
    handlePan(dx, dy) {
      this.pan = { x: this.pan.x + dx, y: this.pan.y + dy };
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

    handleFit() {
      if (this.canvasWidth > 0 && this.canvasHeight > 0) {
        const maxW = this.canvasWidth - 26;
        const maxH = this.canvasHeight - 26;

        const zoomW = maxW / this.boardSize;
        const zoomH = maxH / this.boardSize;

        const newZoom = Math.max(1, Math.min(zoomW, zoomH, 10));
        this.zoom = newZoom;

        const contentW = this.boardSize * newZoom;
        const contentH = this.boardSize * newZoom;
        this.pan = {
          x: (this.canvasWidth - contentW) / 2,
          y: (this.canvasHeight - contentH) / 2,
        };
      }
    },

    loadPixels() {
      this.pixels = this.projectStore.getProjectPixels(this.projectId);

      // 提取当前画布的像素
      const local = new Map();
      this.pixels.forEach((color, key) => {
        const [px, py] = key.split(",").map(Number);
        if (
          px >= this.boardX &&
          px < this.boardX + this.boardSize &&
          py >= this.boardY &&
          py < this.boardY + this.boardSize
        ) {
          local.set(`${px - this.boardX},${py - this.boardY}`, color);
        }
      });
      this.localPixels = local;
    },

    loadProgress() {
      const progress = this.projectStore.getBoardProgress(
        this.projectId,
        this.boardId,
      );
      if (progress) {
        this.completedRows = new Set(progress.completedRows || []);
        this.completedColors = new Set(progress.completedColors || []);
        if (progress.lastMode) {
          this.assistMode = progress.lastMode;
        }
      }
    },

    saveProgress() {
      const pixelProgress = this.calculatePixelProgress();
      const rowProgress = this.completedRows.size / this.boardSize;
      const completion = Math.max(pixelProgress, rowProgress);

      const progress = {
        boardId: this.boardId,
        completion,
        lastMode: this.assistMode,
        completedRows: Array.from(this.completedRows),
        completedColors: Array.from(this.completedColors),
      };

      this.projectStore.saveBoardProgress(
        this.projectId,
        this.boardId,
        progress,
      );
      this.projectStore.updateProjectProgress(this.projectId);
    },

    calculatePixelProgress() {
      if (this.localPixels.size === 0) return 0;
      let completedPixels = 0;
      this.localPixels.forEach((color, key) => {
        const [, ly] = key.split(",").map(Number);
        if (this.completedColors.has(`${ly}:${color}`)) {
          completedPixels++;
        }
      });
      return completedPixels / this.localPixels.size;
    },

    getColorCode(hex) {
      const code = colorCodeMap.get(hex.toLowerCase());
      return code || "?";
    },

    setHighlightColor(color) {
      const previousColor = this.highlightColor;
      this.highlightColor = color;

      // 如果设备已连接，智能更新
      if (this.deviceConnected) {
        // 颜色模式：使用 highlight_color 命令
        if (this.assistMode === "color") {
          this.updateHighlightBrightness(previousColor, color);
        }
      }
    },

    setAssistMode(mode) {
      if (this.assistMode !== mode) {
        this.assistMode = mode;
        this.highlightColor = null;

        // 切换模式后智能更新设备
        if (this.deviceConnected) {
          if (mode === "row") {
            // 切换到逐行模式：使用 highlight_row 命令
            this.updateRowBrightness();
          } else if (mode === "color") {
            // 切换到颜色模式：取消高亮
            this.updateHighlightBrightness(null, null);
          }
        }
      }
    },

    isColorCompleted(color) {
      if (this.assistMode === "row") {
        return this.completedColors.has(`${this.currentRow}:${color}`);
      } else {
        const rows = this.colorRowsMap.get(color);
        if (!rows || rows.size === 0) return false;
        for (const r of rows) {
          if (!this.completedColors.has(`${r}:${color}`)) return false;
        }
        return true;
      }
    },

    toggleColorComplete(color) {
      const newSet = new Set(this.completedColors);

      if (this.assistMode === "row") {
        const key = `${this.currentRow}:${color}`;
        if (newSet.has(key)) {
          newSet.delete(key);
        } else {
          newSet.add(key);
        }
      } else {
        const rows = this.colorRowsMap.get(color);
        if (!rows || rows.size === 0) return;

        let allChecked = true;
        for (const r of rows) {
          if (!newSet.has(`${r}:${color}`)) {
            allChecked = false;
            break;
          }
        }

        if (allChecked) {
          for (const r of rows) {
            newSet.delete(`${r}:${color}`);
          }
        } else {
          for (const r of rows) {
            newSet.add(`${r}:${color}`);
          }
        }
      }

      this.completedColors = newSet;
    },

    toggleRowComplete() {
      const newSet = new Set(this.completedRows);
      const newColorSet = new Set(this.completedColors);
      const isCompleting = !newSet.has(this.currentRow);

      if (isCompleting) {
        newSet.add(this.currentRow);
        const rowColors = this.rowColorsMap.get(this.currentRow);
        if (rowColors) {
          rowColors.forEach((c) => newColorSet.add(`${this.currentRow}:${c}`));
        }
      } else {
        newSet.delete(this.currentRow);
        const rowColors = this.rowColorsMap.get(this.currentRow);
        if (rowColors) {
          rowColors.forEach((c) =>
            newColorSet.delete(`${this.currentRow}:${c}`),
          );
        }
      }

      this.completedRows = newSet;
      this.completedColors = newColorSet;

      // 同步板子亮度（逐行模式下已完成的行变暗）
    },

    prevRow() {
      if (this.currentRow > 0) {
        this.currentRow--;
        // 逐行模式下，使用快速更新
        if (this.deviceConnected && this.assistMode === "row") {
          this.updateRowBrightness();
        }
      }
    },

    nextRow() {
      if (this.currentRow < 51) {
        this.currentRow++;
        // 逐行模式下，使用快速更新
        if (this.deviceConnected && this.assistMode === "row") {
          this.updateRowBrightness();
        }
      }
    },

    handleRowListSelect(row) {
      this.currentRow = row;
      this.isRowListOpen = false;
      // 逐行模式下，使用快速更新
      if (this.deviceConnected && this.assistMode === "row") {
        this.updateRowBrightness();
      }
    },

    // 字母索引相关方法
    scrollToColorLetter(letter) {
      this.currentVisibleLetter = letter;

      // 如果是A，直接滚到顶部
      if (letter === "A") {
        // 先清空 scrollIntoView，然后设置 scrollTop 为 0
        this.scrollIntoView = "";
        this.$nextTick(() => {
          this.colorListScrollTop = 0;
        });
      } else {
        // 其他字母使用 scroll-into-view
        this.colorListScrollTop = -1; // 重置 scrollTop，让 scroll-into-view 生效
        this.$nextTick(() => {
          this.scrollIntoView = "letter-" + letter;
        });
      }
    },

    handleColorScroll(e) {
      // 更新当前可见字母（简化版，uni-app 中较难实现精确的滚动监听）
      // 可以根据需要进一步优化
    },

    handleIndexTouchStart(e) {
      this.isIndexTouching = true;
      this.showLetterBubble = true;

      const touch = e.touches[0];
      this.handleIndexTouch(touch);
    },

    handleIndexTouchMove(e) {
      if (!this.isIndexTouching) return;

      const touch = e.touches[0];
      this.handleIndexTouch(touch);
    },

    handleIndexTouchEnd() {
      this.isIndexTouching = false;
      this.showLetterBubble = false;
      this.currentTouchLetter = "";
    },

    handleIndexTouch(touch) {
      const letters = this.availableColorLetters;
      if (letters.length === 0) return;

      // 获取索引条的位置信息
      const query = uni.createSelectorQuery().in(this);
      query
        .select(".index-bar")
        .boundingClientRect((rect) => {
          if (!rect) return;

          // 计算相对位置
          const relativeY = touch.pageY - rect.top;
          const itemHeight = rect.height / letters.length;
          const index = Math.max(
            0,
            Math.min(Math.floor(relativeY / itemHeight), letters.length - 1),
          );

          const letter = letters[index];

          // 更新气泡位置和字母
          this.bubblePosition = { x: touch.pageX - 64, y: touch.pageY };

          // 只有当字母改变时才滚动
          if (letter !== this.currentTouchLetter) {
            this.currentTouchLetter = letter;
            this.scrollToColorLetter(letter);
          }
        })
        .exec();
    },

    async goToOverview() {
      this.saveProgress();
      await this.restoreModeAfterCanvas();
      uni.navigateBack();
    },
  },
};
</script>

<style scoped>
.assist-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-primary);
  overflow: hidden;
}

/* 行导航弹窗 */
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
  z-index: 1000;
  padding: 32rpx;
}

.row-list-modal {
  width: 100%;
  max-width: 600rpx;
  max-height: 80vh;
  background-color: var(--bg-tertiary);
  border-radius: 32rpx;
  border: 1px solid var(--border-color);
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 32rpx;
  border-bottom: 1px solid var(--border-color);
}

.modal-title {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.close-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 12rpx;
  background-color: var(--bg-elevated);
  transition: all 0.2s;
}

.close-icon {
  font-size: 48rpx;
  color: var(--text-secondary);
}

.row-grid-scroll {
  flex: 1;
  padding: 32rpx;
}

.row-grid {
  display: grid;
  grid-template-columns: repeat(6, 1fr);
  gap: 20rpx;
}

.row-grid-item {
  aspect-ratio: 1;
  border-radius: 16rpx;
  border: 2px solid var(--border-color);
  background-color: var(--bg-tertiary);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  position: relative;
  transition: all 0.2s;
}

.row-grid-item.current {
  border-color: var(--accent-color);
  background-color: #f7f7f7;
  box-shadow: 0 0 20rpx rgba(0, 243, 255, 0.3);
}

.row-grid-item.completed {
  border-color: rgba(0, 255, 157, 0.3);
  background-color: rgba(0, 255, 157, 0.1);
}

.row-number {
  font-size: 28rpx;
  color: var(--text-secondary);
  font-family: monospace;
  font-weight: 500;
}

.row-grid-item.current .row-number {
  color: var(--accent-color);
  font-weight: bold;
}

.check-mark {
  position: absolute;
  font-size: 32rpx;
  color: var(--success-color);
  opacity: 0.2;
}

.modal-footer {
  display: flex;
  justify-content: space-between;
  padding: 24rpx 32rpx;
  border-top: 1px solid var(--border-color);
  background-color: var(--bg-tertiary);
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.legend-box {
  width: 24rpx;
  height: 24rpx;
  border-radius: 6rpx;
  border: 2px solid;
}

.completed-box {
  background-color: rgba(0, 255, 157, 0.1);
  border-color: rgba(0, 255, 157, 0.3);
}

.current-box {
  background-color: #f7f7f7;
  border-color: var(--accent-color);
}

.uncompleted-box {
  background-color: var(--bg-tertiary);
  border-color: var(--border-color);
}

.legend-text {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--nb-surface);
  border-bottom: 2rpx solid var(--nb-ink);
  position: relative;
}

.nav-left {
  position: absolute;
  left: 32rpx;
  width: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-start;
}

.nav-right {
  position: absolute;
  right: 32rpx;
  width: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-end;
}

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--nb-ink);
}
/* 头部 */
.header {
  height: 88rpx;
  display: flex;
  align-items: center;
  padding: 0 30rpx;
  border-bottom: 2rpx solid var(--nb-ink);
  background-color: var(--bg-elevated);
  z-index: 20;
  display: none;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 20rpx;
  flex-shrink: 0;
}

.back-btn {
  width: 60rpx;
  height: 60rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--nb-ink);
  transition: var(--transition-base);
}

.icon {
  font-size: 48rpx;
  color: var(--text-primary);
}

.board-id {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
  font-family: monospace;
}

.mode-switch {
  flex: 1;
  display: flex;
  justify-content: center;
  max-width: 400rpx;
  gap: 8rpx;
  padding: 8rpx;
  background-color: var(--bg-tertiary);
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
}

.mode-btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12rpx;
  padding: 16rpx 24rpx;
  border-radius: 16rpx;
  transition: var(--transition-base);
  color: var(--text-primary);
}

.mode-btn.active {
  background-color: var(--nb-yellow);
  box-shadow: none;
  color: var(--text-inverse);
}

.mode-icon {
  font-size: 28rpx;
}

.mode-label {
  font-size: 24rpx;
  font-weight: bold;
  color: inherit;
}

.info-banner {
  padding: 20rpx 30rpx;
  background-color: #f7f7f7;
  border-bottom: 2rpx solid var(--nb-yellow);
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
  z-index: 10;
}

.color-dot {
  width: 28rpx;
  height: 28rpx;
  border-radius: 50%;
  border: 2rpx solid var(--nb-yellow);
  box-shadow: 0 0 8rpx rgba(0, 243, 255, 0.3);
}

.cont-color-dot {
  width: 28rpx;
  height: 28rpx;
  border-radius: 50%;
  border: 2rpx solid var(--nb-yellow);
  box-shadow: 0 0 8rpx rgba(0, 243, 255, 0.3);
  background-color: #00f3ff;
  margin-right: 20rpx;
}

.info-text {
  font-size: 24rpx;
  color: var(--nb-yellow);
  font-weight: bold;
  font-family: monospace;
}

/* Canvas 画布 */
.canvas-container {
  flex: 1;
  position: relative;
  overflow: hidden;
  background-color: var(--bg-primary);
  min-height: 0;
  max-height: calc(100vh - 112rpx - 600rpx);
}

.assist-canvas {
  width: 100%;
  height: 100%;
}

/* 底部控制区 */
.controls {
  flex-shrink: 0;
  max-height: 600rpx;
  background-color: var(--bg-elevated);
  border-top: 1px solid var(--border-color);
  padding-bottom: var(--layout-bottom-offset);
  z-index: 20;
  display: flex;
  flex-direction: column;
}

.color-section {
  padding: 24rpx 30rpx;
  position: relative;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16rpx;
}

.section-title {
  font-size: 24rpx;
  color: var(--text-secondary);
  font-family: monospace;
}

.header-actions {
  display: flex;
  gap: 12rpx;
  flex-shrink: 0;
}

.icon-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 12rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--nb-ink);
  transition: var(--transition-base);
  color: var(--text-primary);
}


.icon-btn.active {
  background-color: #f7f7f7;
  border-color: var(--nb-yellow);
  box-shadow: none;
  color: var(--nb-yellow);
}

.icon-btn.device-btn.connected {
  background-color: #f7f7f7;
  border-color: var(--nb-yellow);
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

.color-list {
  max-height: 400rpx;
}

.color-group {
  margin-bottom: 24rpx;
}

.group-header {
  position: sticky;
  top: 0;
  display: flex;
  align-items: center;
  min-height: 52rpx;
  padding: 8rpx 0;
  margin-bottom: 16rpx;
  background-color: #ffffff;
  border-left: 4rpx solid var(--nb-yellow);
  border-radius: 0;
  z-index: 10;
}

.group-letter {
  font-size: 26rpx;
  font-weight: bold;
  color: #000000;
  font-family: monospace;
  margin-left: 12rpx;
  line-height: 1;
}

.group-count {
  font-size: 20rpx;
  color: #777777;
  margin-left: 14rpx;
  line-height: 1;
}

.color-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16rpx;
  margin-bottom: 16rpx;
}

.color-item {
  display: flex;
  align-items: flex-start;
  gap: 12rpx;
  padding: 16rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 16rpx;
  box-shadow: var(--nb-shadow-soft);
  transition: var(--transition-base);
  position: relative;
  overflow: hidden;
}

.color-item.active {
  border-color: var(--nb-yellow);
  background-color: #f7f7f7;
  box-shadow: none;
}

.color-item.completed {
  opacity: 0.6;
  border-color: rgba(0, 255, 157, 0.5);
  background-color: rgba(0, 255, 157, 0.05);
}

.color-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
}

.color-swatch {
  width: 48rpx;
  height: 48rpx;
  border-radius: 12rpx;
  border: 2rpx solid var(--border-color);
  flex-shrink: 0;
}

.color-code {
  font-size: 20rpx;
  font-weight: bold;
  color: var(--text-primary);
  font-family: monospace;
}

.color-actions {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  flex-shrink: 0;
}

.check-btn {
  width: 48rpx;
  height: 48rpx;
  border: 2rpx solid var(--nb-ink);
  border-radius: 12rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: var(--transition-base);
  z-index: 10;
}


.check-btn.checked {
  background-color: var(--success-color);
  border-color: var(--success-color);
  box-shadow: 0 0 16rpx rgba(0, 255, 157, 0.4);
}

.check-icon {
  font-size: 28rpx;
  color: var(--text-inverse);
  font-weight: bold;
}

.check-box {
  width: 32rpx;
  height: 32rpx;
}

.color-count {
  font-size: 20rpx;
  color: var(--text-tertiary);
  font-family: monospace;
}

/* 字母索引条 */
.index-bar {
  position: absolute;
  right: 30rpx;
  top: 80rpx;
  bottom: 20rpx;
  width: 40rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 4rpx;
  z-index: 15;
}

.index-letter {
  font-size: 20rpx;
  font-weight: bold;
  color: var(--text-tertiary);
  font-family: monospace;
  width: 32rpx;
  height: 32rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 8rpx;
  transition: var(--transition-fast);
}

.index-letter.active {
  color: var(--nb-yellow);
  background-color: #ffe49a;
  transform: scale(1.3);
}

/* 字母提示气泡 */
.letter-bubble {
  position: fixed;
  width: 128rpx;
  height: 128rpx;
  background-color: var(--nb-yellow);
  color: var(--text-inverse);
  font-size: 56rpx;
  font-weight: bold;
  font-family: monospace;
  border-radius: 32rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: none;
  z-index: 2000;
  pointer-events: none;
  transform: translate(-50%, -50%);
  animation: bubble-in var(--transition-fast);
}

@keyframes bubble-in {
  from {
    opacity: 0;
    transform: translate(-50%, -50%) scale(0.8);
  }
  to {
    opacity: 1;
    transform: translate(-50%, -50%) scale(1);
  }
}

/* 行控制 */
.row-controls {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 30rpx;
  gap: 16rpx;
  border-top: 1px solid var(--border-color);
}

.row-btn {
  width: 96rpx;
  height: 96rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: var(--transition-base);
  flex-shrink: 0;
}

.row-btn:active {
  background-color: var(--bg-elevated);
}

.row-icon {
  font-size: 40rpx;
  color: var(--text-primary);
}

.row-info-container {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 16rpx;
  height: 96rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
}

.row-list-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 0 24rpx;
  height: 100%;
  border-right: 1px solid var(--border-color);
  transition: all 0.2s;
}

.row-list-btn:active {
  background-color: var(--bg-elevated);
}

.grid-icon {
  font-size: 32rpx;
  color: var(--text-secondary);
}

.list-label {
  font-size: 18rpx;
  color: var(--text-secondary);
  font-family: monospace;
  margin-top: 4rpx;
}

.row-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

.row-number {
  font-size: 48rpx;
  font-weight: bold;
  color: var(--nb-yellow);
  font-family: monospace;
  line-height: 1;
  text-shadow: 0 0 8rpx rgba(0, 243, 255, 0.3);
}

.row-total {
  font-size: 20rpx;
  color: var(--text-secondary);
  font-family: monospace;
  margin-top: 4rpx;
}

.row-complete-btn {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 20rpx 32rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 16rpx;
  transition: var(--transition-base);
}


.row-complete-btn.completed {
  background-color: var(--success-color);
  border-color: var(--success-color);
  box-shadow: 0 0 20rpx rgba(0, 255, 157, 0.4);
}

.complete-icon {
  font-size: 28rpx;
  color: var(--text-inverse);
  font-weight: bold;
}

.complete-box {
  width: 28rpx;
  height: 28rpx;
  border: 2px solid currentColor;
  border-radius: 6rpx;
}

.complete-text {
  font-size: 24rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.row-complete-btn.completed .complete-text {
  color: var(--text-inverse);
}
</style>
