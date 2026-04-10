<template>
  <view class="create-page glx-page-shell">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 顶部栏 -->
    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--nb-ink)"
        />
      </view>
      <text class="nav-title glx-topbar__title">新建画布</text>
      <view class="nav-right"></view>
    </view>

    <!-- 创建向导 -->
    <view class="wizard-container glx-page-shell__stack">
      <!-- 步骤 0: 名称 -->
      <scroll-view v-if="step === 0" scroll-y class="content-area">
        <view class="step-content step-name" :class="stepAnimationClass">
          <view class="input-group">
            <text class="input-label">画布名称</text>
            <view
              class="name-input-wrapper"
              :class="{ focused: isNameFocused }"
            >
              <input
                v-model="name"
                type="text"
                class="name-input"
                placeholder="例如：2026新年快乐"
                cursor-color="#ffd23f"
                @focus="isNameFocused = true"
                @blur="isNameFocused = false"
                :focus="step === 0"
              />
            </view>
          </view>

          <view class="step-hint">
            <text class="hint-text"
              >为您的创作赋予独特的身份。这将帮助您稍后在画布库中找到它。</text
            >
          </view>
        </view>
      </scroll-view>
      <!-- 步骤 1: 尺寸选择（仅新建画布模式） 或 步骤 1:图片上传 尺寸选择（导入图片模式） -->
      <scroll-view
        v-if="
          (step === 1 && mode === 'blank') || (step === 1 && mode === 'image')
        "
        scroll-y
        class="content-area"
      >
        <view
          class="step-content step-image"
          :class="stepAnimationClass"
          v-if="step === 1 && mode === 'image'"
        >
          <!-- 图片上传 -->
          <view class="image-upload">
            <view v-if="!previewUrl" class="upload-area" @click="chooseImage">
              <view class="upload-icon-wrapper">
                <Icon name="upload" :size="64" />
              </view>
              <text class="upload-text">点击上传图片</text>
              <text class="upload-hint">支持 JPG, PNG</text>
            </view>

            <view v-else class="preview-area">
              <view class="preview-image-shell">
                <image :src="previewUrl" mode="aspectFit" class="preview-image" />
              </view>
              <view class="preview-actions">
                <view class="preview-btn" @click="chooseImage">
                  <text class="preview-btn-text">更换图片</text>
                </view>
                <view class="preview-btn danger" @click="clearImage">
                  <Icon name="close" :size="28" />
                </view>
              </view>
            </view>
          </view>

          <!-- 推荐尺寸提示 -->
          <view
            v-if="recommendedSize && mode === 'image'"
            class="recommend-tip"
            @click="applyRecommendedSize"
          >
            <text class="recommend-text"
              ><Icon name="prompt" :size="24" color="#000000" />
              检测到内容尺寸，推荐 {{ recommendedSize.width }}×{{
                recommendedSize.height
              }}</text
            >
            <text class="recommend-btn-text">点击应用</text>
          </view>
        </view>
        <view class="step-content step-size" :class="stepAnimationClass">
          <!-- 尺寸选择 -->
          <view class="size-section">
            <view class="size-header">
              <Icon name="a-Grid-ninejiugongge" :size="32" />
              <text class="size-title">{{
                mode === "image" ? "目标尺寸 (像素化)" : "画布尺寸"
              }}</text>
            </view>

            <view class="size-grid">
              <!-- 自定义尺寸输入 -->
              <view class="size-column full-width">
                <view class="size-column-header">
                  <text class="size-label">自定义尺寸</text>
                  <view
                    v-if="customWidth > 0 && customHeight > 0"
                    class="size-preview-inline"
                  >
                    <text class="size-preview-value"
                      >{{ customWidth }}×{{ customHeight }}</text
                    >
                  </view>
                  <text v-else class="size-hint"
                    >最大{{ maxDimension }}×{{ maxDimension }}像素</text
                  >
                </view>

                <view class="custom-size-inputs">
                  <view class="size-input-group">
                    <text class="size-input-label">宽度</text>
                    <view class="size-input-shell">
                      <input
                        v-model.number="customWidth"
                        type="number"
                        class="size-input"
                        placeholder="例如: 30"
                        @input="handleSizeInput"
                      />
                    </view>
                  </view>
                  <text class="size-separator">×</text>
                  <view class="size-input-group">
                    <text class="size-input-label">高度</text>
                    <view class="size-input-shell">
                      <input
                        v-model.number="customHeight"
                        type="number"
                        class="size-input"
                        placeholder="例如: 20"
                        @input="handleSizeInput"
                      />
                    </view>
                  </view>
                </view>
              </view>

              <!-- 快捷尺寸选择 -->
              <view class="size-column full-width">
                <view class="size-column-header">
                  <text class="size-label">{{
                    contentRatio
                      ? `快捷尺寸 (${contentRatio.w}:${contentRatio.h})`
                      : "快捷尺寸 (正方形)"
                  }}</text>
                  <text class="size-hint">{{
                    contentRatio ? "按图片内容比例" : "52的倍数标准尺寸"
                  }}</text>
                </view>

                <view class="size-options">
                  <view
                    v-for="preset in sizePresets"
                    :key="`preset-${preset.width}-${preset.height}`"
                    class="size-option"
                    :class="{
                      active:
                        customWidth === preset.width &&
                        customHeight === preset.height,
                    }"
                    @click="selectPresetSize(preset.width, preset.height)"
                  >
                    <text class="size-option-text"
                      >{{ preset.width }}×{{ preset.height }}</text
                    >
                  </view>
                </view>
              </view>
            </view>
          </view>

          <!-- 统计信息 -->
          <view v-if="customWidth > 0 && customHeight > 0" class="stats-card">
            <view class="stat-row">
              <text class="stat-label">板子布局</text>
              <text class="stat-value"
                >{{ boardsX }}×{{ boardsY }} ({{ totalBoards }}块)</text
              >
            </view>
            <view class="stat-row">
              <text class="stat-label">总像素数</text>
              <text class="stat-value">{{
                (customWidth * customHeight).toLocaleString()
              }}</text>
            </view>
          </view>
        </view>
      </scroll-view>

      <!-- 步骤 2: 颜色选择（新建画布模式） 或 步骤 3: 颜色选择（导入图片模式） -->
      <scroll-view
        v-if="
          (step === 2 && mode === 'blank') || (step === 2 && mode === 'image')
        "
        scroll-y
        class="content-area"
      >
        <view class="step-content step-colors" :class="stepAnimationClass">
          <view class="colors-header">
            <Icon name="picture" :size="32" />
            <text class="colors-title">选择 Artkal 拼豆套装</text>
          </view>

          <view class="preset-grid">
            <view
              v-for="preset in presets"
              :key="preset.key"
              class="preset-card"
              :class="{ active: selectedPreset === preset.key }"
              @click="handlePresetChange(preset.key)"
            >
              <text class="preset-name">{{ preset.name }}</text>
              <text class="preset-count">{{ preset.count }} 种颜色</text>
            </view>
          </view>

          <view class="selected-info">
            <view class="info-row">
              <text class="info-label">已选择套装</text>
              <text class="info-value">{{ selectedColors.size }} 种颜色</text>
            </view>
            <text class="info-hint"
              >套装包含精心挑选的 Artkal 拼豆颜色，适合各种创作需求</text
            >
          </view>
        </view>
      </scroll-view>

      <!-- 步骤 3: 预览确认 (仅图片模式) -->
      <scroll-view
        v-if="step === 3 && mode === 'image'"
        scroll-y
        class="content-area"
      >
        <view class="step-content step-preview" :class="stepAnimationClass">
          <view class="preview-header">
            <text class="preview-title">预览效果</text>
            <text class="preview-subtitle">确认后将创建画布</text>
          </view>

          <!-- 预览图 -->
          <view class="preview-canvas-wrapper">
            <!-- #ifdef H5 -->
            <canvas
              canvas-id="previewDisplayCanvas"
              id="previewDisplayCanvas"
              type="2d"
              class="preview-canvas"
            ></canvas>
            <!-- #endif -->

            <!-- #ifdef MP-WEIXIN -->
            <image
              v-if="previewImageUrl"
              :src="previewImageUrl"
              class="preview-image"
              mode="aspectFit"
            />
            <canvas
              v-else
              canvas-id="previewDisplayCanvas"
              id="previewDisplayCanvas"
              type="2d"
              class="preview-canvas"
              style="position: absolute; left: -9999px; opacity: 0"
            ></canvas>
            <!-- #endif -->
          </view>

          <!-- 统计信息 -->
          <view class="preview-stats">
            <view class="preview-stat-card">
              <text class="preview-stat-label">画布尺寸</text>
              <text class="preview-stat-value"
                >{{ customWidth }}×{{ customHeight }}</text
              >
            </view>

            <view class="preview-stat-card">
              <text class="preview-stat-label">板子数量</text>
              <text class="preview-stat-value">{{ totalBoards }} 块</text>
            </view>

            <view class="preview-stat-card">
              <text class="preview-stat-label">使用颜色</text>
              <text class="preview-stat-value">{{ usedColors.length }} 种</text>
            </view>
          </view>

          <!-- 使用的颜色列表 -->
          <view class="used-colors-section">
            <view class="used-colors-header">
              <text class="used-colors-title">使用的颜色</text>
              <text class="used-colors-total"
                >共
                {{ usedColors.reduce((sum, c) => sum + c.count, 0) }} 个</text
              >
            </view>
            <view class="used-colors-grid">
              <view
                v-for="color in usedColors"
                :key="color.code"
                class="used-color-item"
              >
                <view
                  class="color-swatch"
                  :style="{ backgroundColor: color.hex }"
                ></view>
                <text class="color-code">{{ color.code }}</text>
                <text class="color-count">{{ color.count }}</text>
              </view>
            </view>
          </view>
        </view>
      </scroll-view>
    </view>

    <!-- 底部按钮 -->
    <view class="footer">
      <view
        class="next-btn"
        :class="{ disabled: !canProceed || isProcessing }"
        @click="handleNext"
      >
        <text class="next-btn-text">{{ buttonText }}</text>
        <text class="next-icon">›</text>
      </view>
    </view>

    <!-- 隐藏的 Canvas 用于图片导入 -->
    <!-- #ifdef MP-WEIXIN -->
    <canvas
      v-if="mode === 'image'"
      canvas-id="importCanvas"
      id="importCanvas"
      type="2d"
      style="
        position: fixed;
        left: -9999px;
        top: -9999px;
        width: 1px;
        height: 1px;
      "
    />
    <!-- #endif -->

    <!-- #ifdef H5 -->
    <canvas
      v-if="mode === 'image'"
      canvas-id="importCanvas"
      id="importCanvas"
      style="
        position: fixed;
        left: -9999px;
        top: -9999px;
        width: 1px;
        height: 1px;
      "
    />
    <!-- #endif -->

    <!-- 隐藏的 Canvas 用于保存缩略图 -->
    <canvas
      canvas-id="thumbnailSaveCanvas"
      id="thumbnailSaveCanvas"
      type="2d"
      style="
        position: fixed;
        left: -9999px;
        top: -9999px;
        width: 100px;
        height: 100px;
        opacity: 0;
        pointer-events: none;
      "
    />

    <!-- 隐藏的临时处理canvas（仅小程序使用） -->
    <!-- #ifdef MP-WEIXIN -->
    <canvas
      canvas-id="tempProcessCanvas"
      id="tempProcessCanvas"
      type="2d"
      style="
        position: absolute;
        left: -9999px;
        top: -9999px;
        width: 640px;
        height: 640px;
        opacity: 0;
        pointer-events: none;
      "
    />
    <!-- #endif -->

    <!-- 图片裁剪组件 -->
    <ImageCropper
      :visible="showCropper"
      :src="cropperSrc"
      @confirm="onCropConfirm"
      @cancel="onCropCancel"
    />

    <!-- 自定义 Toast 组件 -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useProjectStore } from "../../store/project.js";
import { useToast } from "../../composables/useToast.js";
import {
  ARTKAL_OFFICIAL_SETS,
  ARTKAL_PRESETS,
} from "../../data/artkal-colors-full.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import ImageCropper from "../../components/ImageCropper.vue";
import createProjectSourceMixin from "./mixins/projectSourceMixin.js";
import createImagePreviewMixin from "./mixins/imagePreviewMixin.js";
import {
  PERLER_BOARD_SIZE,
  PERLER_MAX_DIMENSION,
  PERLER_SIZE_PRESETS,
} from "../../constants/perler.js";

export default {
  mixins: [statusBarMixin, createProjectSourceMixin, createImagePreviewMixin],
  components: {
    Icon,
    Toast,
    ImageCropper,
  },
  data() {
    return {
      projectStore: null,
      toast: null,

      name: "",
      customWidth: PERLER_BOARD_SIZE,
      customHeight: PERLER_BOARD_SIZE,
      selectedPreset: "set24",
      selectedColors: new Set(ARTKAL_OFFICIAL_SETS.set24.colors),
      step: 0,
      mode: "blank",
      templateId: null, // 模板ID
      challengeId: null, // 挑战ID
      imageFile: null,
      previewUrl: null,
      cropParams: null,
      showCropper: false,
      cropperSrc: "",
      isCropped: false,
      recommendedSize: null, // 根据图片内容推荐的尺寸 { width, height }
      previewPixels: null, // 预览的像素数据
      previewImageUrl: "", // 小程序预览图片URL
      usedColors: [], // 使用的颜色列表
      isProcessing: false,
      stepAnimationClass: "step-enter",
      isNameFocused: false,

      presets: ARTKAL_PRESETS,
      boardSize: PERLER_BOARD_SIZE,
      maxDimension: PERLER_MAX_DIMENSION,
      defaultSizePresets: PERLER_SIZE_PRESETS,
      contentRatio: null, // 图片内容宽高比 { w, h }（最简比例）
    };
  },

  computed: {
    // 动态快捷尺寸：图片模式下按内容宽高比生成，空白模式下用默认正方形
    sizePresets() {
      return this.defaultSizePresets;
    },

    // 计算填充后的宽度（向上取整到52的倍数）
    paddedWidth() {
      return Math.ceil(this.customWidth / this.boardSize) * this.boardSize;
    },

    // 计算填充后的高度（向上取整到52的倍数）
    paddedHeight() {
      return Math.ceil(this.customHeight / this.boardSize) * this.boardSize;
    },

    // 计算横向板子数量
    boardsX() {
      return Math.ceil(this.customWidth / this.boardSize);
    },

    // 计算纵向板子数量
    boardsY() {
      return Math.ceil(this.customHeight / this.boardSize);
    },

    // 总板子数量
    totalBoards() {
      return this.boardsX * this.boardsY;
    },

    canProceed() {
      if (this.step === 0) return this.name.trim().length > 0;
      if (this.step === 1) {
        if (this.mode === "image") {
          // 图片模式第1步：需要上传图片和设置尺寸
          return (
            this.imageFile &&
            this.customWidth > 0 &&
            this.customHeight > 0 &&
            this.paddedWidth <= this.maxDimension &&
            this.paddedHeight <= this.maxDimension
          );
        } else {
          // 空白画布第1步：需要设置尺寸
          return (
            this.customWidth > 0 &&
            this.customHeight > 0 &&
            this.paddedWidth <= this.maxDimension &&
            this.paddedHeight <= this.maxDimension
          );
        }
      }
      if (this.step === 2) {
        // 两种模式的颜色选择步骤，总是可以进行
        return true;
      }
      if (this.step === 3) return true; // 图片模式的预览步骤
      return true;
    },

    buttonText() {
      if (this.step === 0) return "下一步";
      if (this.step === 1) {
        if (this.mode === "image") {
          return "选择颜色";
        } else {
          return "选择颜色";
        }
      }
      if (this.step === 2) {
        if (this.mode === "image") {
          if (this.isProcessing) return "正在生成中...";
          return "生成预览";
        } else {
          // 空白画布直接创建
          return "创建画布";
        }
      }
      if (this.step === 3) {
        if (this.mode === "image") {
          if (this.isProcessing) return "生成中...";
          return "确认创建";
        }
      }
      return "下一步";
    },
  },

  onLoad(options) {
    this.projectStore = useProjectStore();
    this.toast = useToast();

    // 根据 URL 参数自动进入对应模式
    if (options && options.mode) {
      this.mode = options.mode;
      this.step = 0;
      this.resetForm();
    }
    // 检查是否从模板创建
    else if (options && options.templateId) {
      this.mode = "blank";
      this.templateId = options.templateId;
      this.loadTemplateProject();
    }
    // 检查是否从挑战创建
    else if (options && options.challengeId) {
      this.mode = "blank";
      this.challengeId = options.challengeId;
      this.loadChallengeInfo();
    }

    // 注册自定义 Toast 实例
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef);
      }
    });
  },

  onShow() {
    // 页面显示时的处理
  },

  methods: {
    resetForm() {
      this.name = "";
      this.customWidth = this.boardSize;
      this.customHeight = this.boardSize;
      this.selectedPreset = "set24";
      this.selectedColors = new Set(ARTKAL_OFFICIAL_SETS.set24.colors);
      this.imageFile = null;
      this.previewUrl = null;
      this.previewPixels = null;
      this.recommendedSize = null;
      this.contentRatio = null;
      this.previewImageUrl = "";
      this.usedColors = [];
      this.isProcessing = false;
      this.stepAnimationClass = "step-enter";
      this.isNameFocused = false;
      this.cropParams = null;
    },

    handleSizeInput() {
      // 验证尺寸
      if (this.customWidth < 1) this.customWidth = 1;
      if (this.customHeight < 1) this.customHeight = 1;

      // 检查是否超过最大限制
      if (this.paddedWidth > this.maxDimension) {
        this.customWidth = this.maxDimension;
        this.toast.showError(
          `宽度最大为${this.maxDimension}像素（${this.maxDimension / this.boardSize}块板子）`,
        );
      }
      if (this.paddedHeight > this.maxDimension) {
        this.customHeight = this.maxDimension;
        this.toast.showError(
          `高度最大为${this.maxDimension}像素（${this.maxDimension / this.boardSize}块板子）`,
        );
      }
    },

    selectPresetSize(width, height) {
      this.customWidth = width;
      this.customHeight = height;
    },

    handleBack() {
      // 如果正在处理，禁止返回
      if (this.isProcessing) {
        return;
      }

      if (this.step === 0) {
        // 在向导第一步，返回到 workspace 页面
        uni.navigateBack();
      } else {
        // 在向导其他步骤，返回上一步
        this.stepAnimationClass = "step-exit-reverse";
        setTimeout(() => {
          this.step--;
          this.stepAnimationClass = "step-enter-reverse";
        }, 150);
      }
    },

    handlePresetChange(preset) {
      this.selectedPreset = preset;
      this.selectedColors = new Set(ARTKAL_OFFICIAL_SETS[preset].colors);
    },

    handleNext() {
      if (!this.canProceed || this.isProcessing) return;

      if (this.mode === "blank") {
        // 空白画布流程：名称(0) → 尺寸(1) → 颜色(2) → 创建
        if (this.step === 0 || this.step === 1) {
          this.stepAnimationClass = "step-exit";
          setTimeout(() => {
            this.step++;
            this.stepAnimationClass = "step-enter";
          }, 150);
        } else if (this.step === 2) {
          // 空白画布直接创建
          this.handleCreate();
        }
      } else if (this.mode === "image") {
        // 图片导入流程：名称(0) → 图片+尺寸(1) → 颜色(2) → 预览(3) → 创建
        if (this.step === 0 || this.step === 1) {
          this.stepAnimationClass = "step-exit";
          setTimeout(() => {
            this.step++;
            this.stepAnimationClass = "step-enter";
          }, 150);
        } else if (this.step === 2) {
          // 生成预览
          this.generatePreview();
        } else if (this.step === 3) {
          // 确认创建
          this.handleCreate();
        }
      }
    },

    async handleCreate() {
      if (this.mode === "image" && this.previewPixels) {
        // 使用预览数据创建项目
        this.isProcessing = true;

        try {
          // 将临时文件转为base64，避免临时文件被系统清理
          let thumbnailBase64 = null;
          const tempPath = this.previewImageUrl || this.previewUrl;
          if (tempPath) {
            try {
              // #ifdef MP-WEIXIN
              const fs = uni.getFileSystemManager();
              const base64Data = fs.readFileSync(tempPath, "base64");
              thumbnailBase64 = "data:image/png;base64," + base64Data;
              // #endif
              // #ifdef H5
              thumbnailBase64 = tempPath;
              // #endif
            } catch (e) {
              console.error("缩略图转base64失败:", e);
            }
          }

          // 创建项目（未发布状态）
          const palette = Array.from(this.selectedColors);
          const projectId = this.projectStore.addProject(
            this.name,
            this.customWidth,
            this.customHeight,
            palette.length,
            palette,
            this.paddedWidth,
            this.paddedHeight,
            thumbnailBase64,
          );

          if (!projectId) {
            this.toast.showError("创建项目失败");
            this.isProcessing = false;
            return;
          }

          // 将预览像素数据保存到本地存储
          if (this.previewPixels) {
            this.projectStore.saveProjectPixels(projectId, this.previewPixels);
          }

          this.attachChallengeToProject(projectId);
          this.toast.showSuccess("项目创建成功！");
          this.isProcessing = false;

          // 返回 workspace 页面
          setTimeout(() => {
            uni.navigateBack();
          }, 500);
        } catch (error) {
          console.error("创建项目失败:", error);
          this.toast.showError("创建项目失败，请重试");
          this.isProcessing = false;
        }
      } else if (this.mode === "blank") {
        // 空白画布创建项目（未发布状态）
        try {
          const palette = Array.from(this.selectedColors);
          const projectId = this.projectStore.addProject(
            this.name,
            this.customWidth,
            this.customHeight,
            palette.length,
            palette,
            this.paddedWidth,
            this.paddedHeight,
            null,
          );

          if (!projectId) {
            this.toast.showError("创建项目失败");
            return;
          }

          this.attachChallengeToProject(projectId);
          this.toast.showSuccess("项目创建成功！");

          // 返回 workspace 页面
          setTimeout(() => {
            uni.navigateBack();
          }, 500);
        } catch (error) {
          console.error("创建项目失败:", error);
          this.toast.showError("创建项目失败，请重试");
        }
      }
    },

  },
};
</script>

<style scoped>
.create-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--nb-paper);
  position: relative;
  overflow: hidden;
  user-select: none;
  -webkit-user-drag: none;
  -webkit-touch-callout: none;
  box-sizing: border-box;
}

/* 创建向导容器 */
.wizard-container {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  padding-bottom: 0; /* 确保为底部按钮留出空间 */
}

/* 通用区块样式 */
.section-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--nb-ink);
  margin-bottom: 24rpx;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 24rpx;
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--nb-paper);
  flex-shrink: 0;
}

.back-icon {
  font-size: 36rpx;
  color: var(--nb-ink);
  line-height: 1;
}

.nav-left :deep(.icon),
.nav-left :deep(text),
.nav-left :deep(.iconfont) {
  line-height: 1;
}

.content-area {
  flex: 1;
  padding: 0 32rpx 32rpx;
  overflow-y: auto;
  z-index: 10;
  box-sizing: border-box;
  margin-bottom: 0; /* 确保不与底部按钮重叠 */
}

.step-content {
  display: flex;
  flex-direction: column;
  gap: 48rpx;
  margin-top: 20rpx;
}

/* 步骤切换动画 */
.step-enter {
  animation: slideInRight var(--transition-base) ease-out;
}

.step-enter-reverse {
  animation: slideInLeft var(--transition-base) ease-out;
}

.step-exit {
  animation: slideOutLeft var(--transition-base) ease-in;
}

.step-exit-reverse {
  animation: slideOutRight var(--transition-base) ease-in;
}

@keyframes slideInRight {
  from {
    opacity: 0;
    transform: translateX(60rpx);
  }
  to {
    opacity: 1;
    transform: translateX(0);
  }
}

@keyframes slideInLeft {
  from {
    opacity: 0;
    transform: translateX(-60rpx);
  }
  to {
    opacity: 1;
    transform: translateX(0);
  }
}

@keyframes slideOutLeft {
  from {
    opacity: 1;
    transform: translateX(0);
  }
  to {
    opacity: 0;
    transform: translateX(-60rpx);
  }
}

@keyframes slideOutRight {
  from {
    opacity: 1;
    transform: translateX(0);
  }
  to {
    opacity: 0;
    transform: translateX(60rpx);
  }
}

/* 步骤 0: 名称 */
.input-group {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.input-label {
  font-size: 28rpx;
  color: var(--text-secondary);
  font-weight: 500;
}

.name-input-wrapper {
  position: relative;
  padding: 0 28rpx;
  border: 4rpx solid #000000;
  border-radius: 0;
  background: #ffffff;
  transition: var(--transition-base);
  min-height: 120rpx;
  display: flex;
  align-items: center;
  box-shadow: none;
}

.name-input-wrapper.focused {
  border-color: #ffd23f;
  box-shadow: none;
}

.name-input {
  width: 100%;
  font-size: 64rpx;
  color: var(--text-primary);
  background-color: transparent;
  border: none;
  outline: none;
  line-height: 1.5;
  min-height: 96rpx;
}

.name-input::placeholder {
  color: rgba(136, 136, 136, 0.5);
}

.step-hint {
  margin-top: 64rpx;
}

.hint-text {
  font-size: 28rpx;
  color: var(--text-secondary);
  line-height: 1.6;
}

/* 图片上传滚动容器 */
.image-upload-scroll {
  max-height: none;
  margin-bottom: 32rpx;
}

/* 图片上传 */
.image-upload {
  min-height: 400rpx;
  margin: 20rpx 0;
}

.upload-area {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 400rpx;
  background-color: var(--bg-tertiary);
  border: 4rpx dashed var(--border-color);
  border-radius: 0;
  padding: 48rpx;
  margin: 20rpx 0;
}

.upload-icon-wrapper {
  width: 128rpx;
  height: 128rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--bg-tertiary);
  border-radius: 0;
  border: 4rpx solid #000000;
  margin-bottom: 32rpx;
}

.upload-icon {
  font-size: 64rpx;
}

.upload-text {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-primary);
  margin-bottom: 16rpx;
}

.upload-hint {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.preview-area {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 480rpx;
  gap: 12rpx;
}

.preview-image-shell {
  width: 100%;
  border: 0;
  border-radius: 0;
  background: transparent;
  box-shadow: none;
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: center;
}

.preview-image {
  width: auto;
  max-width: 100%;
  max-height: 480rpx;
  height: auto;
  object-fit: contain;
  display: block;
  margin: 0 auto;
}

.preview-actions {
  display: flex;
  gap: 12rpx;
  padding: 0;
  width: 100%;
}

.preview-btn {
  flex: 1;
  min-height: 92rpx;
  padding: 0 24rpx;
  background: #ffffff;
  border: 4rpx solid #000000;
  border-radius: 0;
  box-shadow: none;
  text-align: center;
  display: flex;
  align-items: center;
  justify-content: center;
}

.preview-btn.danger {
  flex: 0 0 auto;
  width: 96rpx;
  background: #d92d20;
  border-color: #000000;
}

.recommend-tip {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16rpx 20rpx;
  margin-top: 8rpx;
  background: #fff4c4;
  border: 0;
  border-radius: 0;
  box-shadow: none;
}

.recommend-text {
  font-size: 24rpx;
  color: #000000;
}

.recommend-btn-text {
  font-size: 24rpx;
  color: #000000;
  font-weight: bold;
  white-space: nowrap;
  margin-left: 16rpx;
}

.preview-btn-text {
  font-size: 28rpx;
  color: var(--text-primary);
}

.preview-btn.danger .preview-btn-text {
  color: #ffffff;
}

/* 自定义尺寸输入 */
.custom-size-inputs {
  display: flex;
  align-items: center;
  gap: 24rpx;
  margin-bottom: 20rpx;
}

.size-input-group {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.size-input-shell {
  min-height: 120rpx;
  padding: 0 20rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: #ffffff;
  border: 4rpx solid #000000;
  border-radius: 0;
  box-shadow: none;
}

.size-input-label {
  font-size: 28rpx;
  color: var(--text-secondary);
  font-weight: 500;
}

.size-input {
  width: 100%;
  background-color: transparent;
  border: 0;
  border-radius: 0;
  font-size: 56rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--text-primary);
  text-align: center;
  transition: var(--transition-base);
  min-height: 120rpx;
  box-shadow: none;
}

.size-input-group:focus-within .size-input-shell {
  border-color: #ffd23f;
  box-shadow: none;
}

.size-input::placeholder {
  color: var(--text-tertiary);
  opacity: 0.5;
}

.size-separator {
  font-size: 64rpx;
  color: #4a4a4a;
  margin-top: 48rpx;
  font-weight: bold;
}

.size-preview {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16rpx 24rpx;
  background-color: var(--bg-tertiary);
  border-radius: 0;
  border: 2rpx solid #000000;
  margin-bottom: 16rpx;
}

.size-preview-label {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.size-preview-value {
  font-size: 28rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--nb-yellow);
}

/* 内联尺寸预览（在标题右侧） */
.size-preview-inline {
  display: flex;
  align-items: center;
}

.size-preview-inline .size-preview-value {
  font-size: 24rpx;
  color: #ffd23f;
}

/* 尺寸选择 */
.size-section {
  display: flex;
  flex-direction: column;
  gap: 20rpx;
}

.size-header {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.size-icon {
  font-size: 32rpx;
}

.size-title {
  font-size: 28rpx;
  color: var(--text-secondary);
  font-weight: 500;
}

.size-grid {
  display: grid;
  grid-template-columns: repeat(1, 1fr);
  gap: 32rpx;
}

.size-column {
  display: flex;
  flex-direction: column;
  gap: 20rpx;
}

.size-column.full-width {
  width: 100%;
}

.size-column-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.size-label {
  font-size: 24rpx;
  font-family: monospace;
  font-weight: bold;
  color: #ffd23f;
}

.size-hint {
  font-size: 20rpx;
  color: var(--text-tertiary);
}

.size-options {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16rpx;
  justify-items: stretch;
}

.size-option {
  padding: 20rpx 16rpx;
  border-radius: 0;
  background-color: var(--bg-tertiary);
  border: 3rpx solid var(--nb-ink);
  transition: all 0.2s;
  text-align: center;
}

.size-option.active {
  border-color: #ffd23f;
  background-color: #ffd23f;
  box-shadow: none;
}

.size-option-text {
  font-size: 28rpx;
  font-family: monospace;
  color: var(--text-secondary);
  font-weight: 500;
}

.size-option.active .size-option-text {
  color: #000000;
  font-weight: bold;
}

.light-theme .size-option.active .size-option-text {
  color: #374151;
}

.size-info {
  font-size: 24rpx;
  color: var(--text-tertiary);
  text-align: center;
  margin-top: 8rpx;
  width: 100%;
  display: block;
}

/* 统计卡片 */
.stats-card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-color);
  border-radius: 0;
  padding: 32rpx;
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.stat-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.stat-label {
  font-size: 28rpx;
  color: var(--text-secondary);
}

.stat-value {
  font-size: 32rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--text-primary);
}

/* 步骤 2: 颜色 */
.colors-header {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.colors-icon {
  font-size: 32rpx;
}

.colors-title {
  font-size: 28rpx;
  color: var(--text-secondary);
  font-weight: 500;
}

.preset-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

.preset-card {
  position: relative;
  padding: 48rpx 32rpx;
  border-radius: 0;
  background-color: #ffffff;
  border: 4rpx solid #000000;
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 8rpx;
  transition: all 0.3s;
  box-shadow: none;
}

.preset-card.active {
  background-color: #ffd23f;
  border-color: #000000;
  color: #000000;
  box-shadow: none;
  transform: none;
}

.preset-card.active .preset-name {
  color: #000000;
}

.preset-card.active .preset-count {
  color: rgba(0, 0, 0, 0.68);
  opacity: 1;
}

.preset-card.active::after {
  display: none;
}

.preset-name {
  font-size: 32rpx;
  font-weight: bold;
  color: #000000;
}

.preset-count {
  font-size: 24rpx;
  color: rgba(0, 0, 0, 0.68);
  opacity: 0.7;
}

.selected-info {
  background-color: #ffffff;
  border: 4rpx solid #000000;
  border-radius: 18rpx;
  padding: 48rpx;
  display: flex;
  flex-direction: column;
  gap: 32rpx;
  box-shadow: none;
}

.info-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.info-label {
  font-size: 28rpx;
  color: var(--text-secondary);
}

.info-value {
  font-size: 36rpx;
  font-family: monospace;
  font-weight: bold;
  color: #000000;
}

.info-hint {
  font-size: 24rpx;
  color: var(--text-tertiary);
  line-height: 1.6;
}

/* 步骤 3: 预览 */
.step-preview {
  display: flex;
  flex-direction: column;
  gap: 32rpx;
}

.preview-header {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  text-align: center;
}

.preview-title {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.preview-subtitle {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.preview-canvas-wrapper {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 0;
  background-color: transparent;
  border-radius: 0;
  border: 0;
  position: relative;
  transform: translateZ(0);
}

.preview-canvas {
  border-radius: 0;
  box-shadow: none;
  display: block;
  background-color: var(--bg-primary);
  width: 260px;
  height: 260px;
  position: relative;
  transform: translateZ(0);
}

.preview-image {
  border-radius: 0;
  box-shadow: none;
  display: block;
  width: 260px;
  height: 260px;
}

.preview-stats {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16rpx;
}

.preview-stat-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 24rpx;
  background-color: var(--bg-tertiary);
  border-radius: 16rpx;
  border: 2rpx solid var(--nb-ink);
}

.preview-stat-label {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.preview-stat-value {
  font-size: 32rpx;
  font-weight: bold;
  font-family: monospace;
  color: var(--nb-yellow);
}

.used-colors-section {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.used-colors-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.used-colors-title {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.used-colors-total {
  font-size: 24rpx;
  font-family: monospace;
  color: var(--text-secondary);
}

.used-colors-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16rpx;
}

.used-color-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx;
  background-color: var(--bg-tertiary);
  border-radius: 12rpx;
  border: 2rpx solid var(--nb-ink);
}

.color-swatch {
  width: 48rpx;
  height: 48rpx;
  border-radius: 8rpx;
  border: 2rpx solid var(--nb-ink);
}

.color-code {
  font-size: 20rpx;
  font-family: monospace;
  color: var(--text-primary);
  font-weight: bold;
}

.color-count {
  font-size: 18rpx;
  color: var(--text-secondary);
}

/* 底部按钮 */
.footer {
  position: relative;
  padding: 24rpx 32rpx;
  padding-bottom: 24rpx;
  z-index: 20;
  box-sizing: border-box;
  flex-shrink: 0;
  background-color: var(--nb-paper);
}

/* 小程序底部安全区域 */
/* #ifdef MP-WEIXIN */
.footer {
  padding-bottom: 36rpx;
}
/* #endif */

.next-btn {
  padding: 32rpx;
  background: #ffd23f;
  color: #000000;
  border-radius: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
  box-shadow: var(--nb-shadow-strong);
  transition: var(--transition-base);
}

.next-btn:active {
  transform: scale(0.98);
}

.next-btn.disabled {
  background: #ffe49a;
  box-shadow: none;
  opacity: 0.75;
  pointer-events: none;
}

.next-btn-text {
  font-size: 32rpx;
  font-weight: bold;
  letter-spacing: 0.2em;
  text-transform: uppercase;
  color: #000000;
  background: transparent;
  border: 0;
  box-shadow: none;
}

.next-icon {
  font-size: 48rpx;
  line-height: 1;
  color: #000000;
  background: transparent;
  border: 0;
  box-shadow: none;
}
</style>
