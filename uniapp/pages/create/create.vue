<template>
  <view class="create-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 顶部栏 -->
    <view class="navbar">
      <view class="header-content">
        <view class="nav-left" @click="handleBack">
          <Icon
            name="direction-left"
            :size="32"
            color="var(--color-text-primary)"
          />
        </view>
        <text class="nav-title">新建画布</text>
      </view>
    </view>

    <!-- 创建向导 -->
    <view class="wizard-container">
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
                cursor-color="#00f3ff"
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
              <image :src="previewUrl" mode="aspectFit" class="preview-image" />
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
              ><Icon name="prompt" :size="24" color="var(--accent-color)" />
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
                  <text v-else class="size-hint">最大640×640像素</text>
                </view>

                <view class="custom-size-inputs">
                  <view class="size-input-group">
                    <text class="size-input-label">宽度</text>
                    <input
                      v-model.number="customWidth"
                      type="number"
                      class="size-input"
                      placeholder="例如: 30"
                      @input="handleSizeInput"
                    />
                  </view>
                  <text class="size-separator">×</text>
                  <view class="size-input-group">
                    <text class="size-input-label">高度</text>
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

              <!-- 快捷尺寸选择 -->
              <view class="size-column full-width">
                <view class="size-column-header">
                  <text class="size-label">{{
                    contentRatio
                      ? `快捷尺寸 (${contentRatio.w}:${contentRatio.h})`
                      : "快捷尺寸 (正方形)"
                  }}</text>
                  <text class="size-hint">{{
                    contentRatio ? "按图片内容比例" : "64的倍数标准尺寸"
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
import { challengeAPI, templateAPI } from "../../api/index.js";
import { useProjectStore } from "../../store/project.js";
import { useToast } from "../../composables/useToast.js";
import {
  ARTKAL_COLORS_FULL,
  ARTKAL_OFFICIAL_SETS,
  ARTKAL_PRESETS,
} from "../../data/artkal-colors-full.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import ProjectCard from "../../components/ProjectCard.vue";
import ImageCropper from "../../components/ImageCropper.vue";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    ProjectCard,
    ImageCropper,
  },
  data() {
    return {
      projectStore: null,
      toast: null,

      name: "",
      customWidth: 64,
      customHeight: 64,
      selectedPreset: "set24",
      selectedColors: new Set(ARTKAL_OFFICIAL_SETS.set24.colors),
      step: 0,
      mode: "blank",
      templateId: null, // 模板ID
      challengeId: null, // 挑战ID
      imageFile: null,
      previewUrl: null,
      showCropper: false,
      cropperSrc: "",
      isCropped: false,
      recommendedSize: null, // 根据图片内容推荐的尺寸 { width, height }
      previewPixels: null, // 预览的像素数据
      previewCanvas: null, // 预览用的canvas
      previewImageUrl: "", // 小程序预览图片URL
      usedColors: [], // 使用的颜色列表
      isProcessing: false,
      stepAnimationClass: "step-enter",
      isNameFocused: false,

      presets: ARTKAL_PRESETS,
      defaultSizePresets: [
        { width: 64, height: 64 },
        { width: 128, height: 128 },
        { width: 192, height: 192 },
        { width: 256, height: 256 },
        { width: 320, height: 320 },
        { width: 384, height: 384 },
        { width: 448, height: 448 },
        { width: 512, height: 512 },
        { width: 576, height: 576 },
        { width: 640, height: 640 },
      ],
      contentRatio: null, // 图片内容宽高比 { w, h }（最简比例）
    };
  },

  computed: {
    // 动态快捷尺寸：图片模式下按内容宽高比生成，空白模式下用默认正方形
    sizePresets() {
      return this.defaultSizePresets;
    },

    // 计算填充后的宽度（向上取整到64的倍数）
    paddedWidth() {
      return Math.ceil(this.customWidth / 64) * 64;
    },

    // 计算填充后的高度（向上取整到64的倍数）
    paddedHeight() {
      return Math.ceil(this.customHeight / 64) * 64;
    },

    // 计算横向板子数量
    boardsX() {
      return Math.ceil(this.customWidth / 64);
    },

    // 计算纵向板子数量
    boardsY() {
      return Math.ceil(this.customHeight / 64);
    },

    // 总板子数量
    totalBoards() {
      return this.boardsX * this.boardsY;
    },

    // 过滤后的项目列表
    filteredProjects() {
      if (!this.projectStore || !this.projectStore.projects) {
        return [];
      }
      return this.projectStore.projects;
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
            this.paddedWidth <= 640 &&
            this.paddedHeight <= 640
          );
        } else {
          // 空白画布第1步：需要设置尺寸
          return (
            this.customWidth > 0 &&
            this.customHeight > 0 &&
            this.paddedWidth <= 640 &&
            this.paddedHeight <= 640
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
    attachChallengeToProject(projectId) {
      if (!this.challengeId) {
        return;
      }
      this.projectStore.updateProject(projectId, {
        challengeId: this.challengeId,
      });
    },

    getPaddedSize(value) {
      return Math.ceil(value / 64) * 64;
    },

    parseTemplateSize(sizeText) {
      const matched = /^(\d+)x(\d+)$/i.exec(sizeText);
      if (!matched) {
        throw new Error("invalid template size");
      }
      return {
        width: Number(matched[1]),
        height: Number(matched[2]),
      };
    },

    buildPixelsMap(pixelObject) {
      if (
        !pixelObject ||
        Array.isArray(pixelObject) ||
        typeof pixelObject !== "object"
      ) {
        throw new Error("invalid template pixels");
      }
      return new Map(Object.entries(pixelObject));
    },

    buildPaletteCodes(pixels) {
      const hexColors = [...new Set(Array.from(pixels.values()))];
      return hexColors.map((hex) => {
        const matchedColor = ARTKAL_COLORS_FULL.find(
          (color) => color.hex.toUpperCase() === hex.toUpperCase(),
        );
        if (!matchedColor) {
          throw new Error(`unknown artkal color: ${hex}`);
        }
        return matchedColor.code;
      });
    },

    async loadTemplateProject() {
      uni.showLoading({ title: "加载模板..." });

      try {
        const res = await templateAPI.getTemplateById(this.templateId);
        if (!(res.success && res.data && res.data.template && res.data.pixels)) {
          throw new Error("template load failed");
        }

        const template = res.data.template;
        const size = this.parseTemplateSize(template.size);
        const pixels = this.buildPixelsMap(res.data.pixels);
        const palette = this.buildPaletteCodes(pixels);

        this.name = template.name;
        this.customWidth = size.width;
        this.customHeight = size.height;
        this.selectedColors = new Set(palette);
        this.previewPixels = pixels;
        this.previewUrl = template.image_url;

        const paddedWidth = this.getPaddedSize(size.width);
        const paddedHeight = this.getPaddedSize(size.height);
        const projectId = this.projectStore.addProject(
          template.name,
          size.width,
          size.height,
          palette.length,
          palette,
          paddedWidth,
          paddedHeight,
          template.image_url,
        );

        if (!projectId) {
          throw new Error("project create failed");
        }

        this.projectStore.saveProjectPixels(projectId, pixels);

        try {
          await templateAPI.useTemplate(this.templateId);
        } catch (error) {
          console.error("更新模板使用次数失败:", error);
        }

        uni.hideLoading();
        this.toast.showSuccess("模板已载入");
        uni.redirectTo({
          url: `/pages/overview/overview?id=${projectId}`,
        });
      } catch (error) {
        uni.hideLoading();
        console.error("加载模板失败:", error);
        this.toast.showError("模板加载失败");
        setTimeout(() => {
          uni.navigateBack();
        }, 500);
      }
    },

    async loadChallengeInfo() {
      uni.showLoading({ title: "加载挑战..." });

      try {
        const res = await challengeAPI.getChallengeById(this.challengeId);
        if (!(res.success && res.data && res.data.challenge)) {
          throw new Error("challenge load failed");
        }

        this.name = res.data.challenge.title;
        uni.hideLoading();
        this.toast.showSuccess("已载入挑战");
      } catch (error) {
        uni.hideLoading();
        console.error("加载挑战失败:", error);
        this.toast.showError("挑战加载失败");
        setTimeout(() => {
          uni.navigateBack();
        }, 500);
      }
    },

    resetForm() {
      this.name = "";
      this.customWidth = 64;
      this.customHeight = 64;
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
    },

    // 检测图片是否可能是像素画
    detectPixelArt(imageData, width, height) {
      const data = imageData.data;
      const colorSet = new Set();
      let totalPixels = 0;

      // 采样检测（不需要检查所有像素）
      const sampleRate = Math.max(1, Math.floor((width * height) / 10000));

      for (let i = 0; i < data.length; i += 4 * sampleRate) {
        const a = data[i + 3];
        if (a < 128) continue; // 跳过透明像素

        const r = data[i];
        const g = data[i + 1];
        const b = data[i + 2];
        const color = `${r},${g},${b}`;
        colorSet.add(color);
        totalPixels++;
      }

      // 像素画特征：颜色数量相对较少
      const uniqueColors = colorSet.size;
      const colorRatio = uniqueColors / totalPixels;

      // 如果颜色种类占比小于30%，很可能是像素画
      return colorRatio < 0.3 && uniqueColors < 256;
    },

    handleSizeInput() {
      // 验证尺寸
      if (this.customWidth < 1) this.customWidth = 1;
      if (this.customHeight < 1) this.customHeight = 1;

      // 检查是否超过最大限制
      if (this.paddedWidth > 640) {
        this.customWidth = 640;
        this.toast.showError("宽度最大为640像素（10块板子）");
      }
      if (this.paddedHeight > 640) {
        this.customHeight = 640;
        this.toast.showError("高度最大为640像素（10块板子）");
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

    chooseImage() {
      uni.chooseImage({
        count: 1,
        sizeType: ["compressed"],
        sourceType: ["album", "camera"],
        success: (res) => {
          this.cropperSrc = res.tempFilePaths[0];
          this.showCropper = true;
        },
      });
    },

    onCropConfirm(cropData) {
      this.showCropper = false;
      this.isCropped = true;
      // 用 canvas 裁剪出选区，生成新图片
      // #ifdef H5
      const img = new Image();
      img.crossOrigin = "anonymous";
      img.onload = () => {
        const canvas = document.createElement("canvas");
        canvas.width = cropData.sw;
        canvas.height = cropData.sh;
        const ctx = canvas.getContext("2d");
        ctx.drawImage(
          img,
          cropData.sx,
          cropData.sy,
          cropData.sw,
          cropData.sh,
          0,
          0,
          cropData.sw,
          cropData.sh,
        );
        const dataUrl = canvas.toDataURL("image/png");
        this.imageFile = dataUrl;
        this.previewUrl = dataUrl;
        this.recommendedSize = null;
        this._detectImageContentSize(dataUrl);
      };
      img.src = cropData.src;
      // #endif

      // #ifdef MP-WEIXIN
      // 小程序用 canvas 裁剪
      this.imageFile = cropData.src;
      this.previewUrl = cropData.src;
      this.recommendedSize = null;
      this._detectImageContentSize(cropData.src);
      // 保存裁剪参数供后续处理使用
      this.cropParams = cropData;
      // #endif
    },

    onCropCancel() {
      this.showCropper = false;
    },

    // 检测图片有效内容尺寸，推荐合适的画布大小
    _detectImageContentSize(filePath) {
      // #ifdef H5
      const img = new Image();
      img.onload = () => {
        const canvas = document.createElement("canvas");
        canvas.width = img.width;
        canvas.height = img.height;
        const ctx = canvas.getContext("2d");
        ctx.drawImage(img, 0, 0);
        const data = ctx.getImageData(0, 0, img.width, img.height).data;

        let minX = img.width,
          minY = img.height,
          maxX = 0,
          maxY = 0;
        let hasContent = false;
        for (let y = 0; y < img.height; y++) {
          for (let x = 0; x < img.width; x++) {
            if (data[(y * img.width + x) * 4 + 3] >= 128) {
              hasContent = true;
              if (x < minX) minX = x;
              if (x > maxX) maxX = x;
              if (y < minY) minY = y;
              if (y > maxY) maxY = y;
            }
          }
        }

        if (hasContent) {
          const contentW = maxX - minX + 1;
          const contentH = maxY - minY + 1;
          this._applyRecommendedSize(contentW, contentH);
        }
      };
      img.src = filePath;
      // #endif

      // #ifdef MP-WEIXIN
      uni.getImageInfo({
        src: filePath,
        success: (info) => {
          // 小程序无法直接读像素，用图片原始尺寸推荐
          this._applyRecommendedSize(info.width, info.height);
        },
      });
      // #endif
    },

    // 计算最大公约数
    _gcd(a, b) {
      while (b) {
        [a, b] = [b, a % b];
      }
      return a;
    },

    // 根据内容尺寸计算宽高比并推荐尺寸
    _applyRecommendedSize(contentW, contentH) {
      // 计算最简宽高比
      const g = this._gcd(contentW, contentH);
      const ratioW = contentW / g;
      const ratioH = contentH / g;
      this.contentRatio = { w: ratioW, h: ratioH };
      this.recommendedSize = { width: contentW, height: contentH };
      console.log(
        `内容尺寸: ${contentW}x${contentH}，宽高比: ${ratioW}:${ratioH}`,
      );
    },

    applyRecommendedSize() {
      if (this.recommendedSize) {
        this.customWidth = this.recommendedSize.width;
        this.customHeight = this.recommendedSize.height;
        this.toast.showSuccess(
          `已应用原始尺寸 ${this.recommendedSize.width}×${this.recommendedSize.height}`,
        );
      }
    },

    clearImage() {
      this.imageFile = null;
      this.previewUrl = null;
      this.recommendedSize = null;
      this.contentRatio = null;
      this.isCropped = false;
    },

    async generatePreview() {
      if (!this.imageFile) return;

      this.isProcessing = true;
      this.previewImageUrl = ""; // 清空之前的预览图
      this.toast.showInfo("正在生成预览...");

      try {
        // 使用已导入的 ARTKAL_COLORS_FULL
        const selectedArtkalColors = Array.from(this.selectedColors)
          .map((code) => ARTKAL_COLORS_FULL.find((c) => c.code === code))
          .filter((c) => c !== undefined);

        const artkalPalette = selectedArtkalColors.map((c) => c.hex);

        // #ifdef H5
        // H5环境：使用Image和document
        const img = new Image();
        img.crossOrigin = "anonymous";

        img.onload = () => {
          // 先绘制原始尺寸以检测是否为像素画
          const tempCanvas = document.createElement("canvas");
          tempCanvas.width = img.width;
          tempCanvas.height = img.height;
          const tempCtx = tempCanvas.getContext("2d");
          tempCtx.drawImage(img, 0, 0);
          const detectData = tempCtx.getImageData(0, 0, img.width, img.height);

          // 检测是否为像素画
          const isPixelArt = this.detectPixelArt(
            detectData,
            img.width,
            img.height,
          );
          console.log(
            "检测到像素画:",
            isPixelArt,
            "原图尺寸:",
            img.width,
            "x",
            img.height,
            "目标尺寸:",
            this.customWidth,
            "x",
            this.customHeight,
          );

          if (isPixelArt) {
            this.toast.showSuccess("检测到像素画，已自动优化处理");
          }

          // 创建最终canvas
          const canvas = document.createElement("canvas");
          canvas.width = this.customWidth;
          canvas.height = this.customHeight;
          const ctx = canvas.getContext("2d");
          if (!ctx) {
            this.toast.showError("Canvas 初始化失败");
            this.isProcessing = false;
            return;
          }

          if (this.isCropped) {
            // 裁剪过的图片直接缩放填满目标尺寸
            ctx.imageSmoothingEnabled = !isPixelArt;
            if (!isPixelArt) ctx.imageSmoothingQuality = "high";
            ctx.drawImage(img, 0, 0, this.customWidth, this.customHeight);
          } else if (isPixelArt) {
            // 像素画模式：先检测内容边界，再整数倍缩放
            ctx.imageSmoothingEnabled = false;
            ctx.mozImageSmoothingEnabled = false;
            ctx.webkitImageSmoothingEnabled = false;
            ctx.msImageSmoothingEnabled = false;

            // 先绘制原图到临时canvas，检测有内容的边界框
            const boundsCanvas = document.createElement("canvas");
            boundsCanvas.width = img.width;
            boundsCanvas.height = img.height;
            const boundsCtx = boundsCanvas.getContext("2d");
            boundsCtx.drawImage(img, 0, 0);
            const boundsData = boundsCtx.getImageData(
              0,
              0,
              img.width,
              img.height,
            ).data;

            let minX = img.width,
              minY = img.height,
              maxX = 0,
              maxY = 0;
            for (let py = 0; py < img.height; py++) {
              for (let px = 0; px < img.width; px++) {
                if (boundsData[(py * img.width + px) * 4 + 3] >= 128) {
                  if (px < minX) minX = px;
                  if (px > maxX) maxX = px;
                  if (py < minY) minY = py;
                  if (py > maxY) maxY = py;
                }
              }
            }

            // 用内容区域的尺寸来计算缩放比例
            const contentW = maxX - minX + 1;
            const contentH = maxY - minY + 1;
            console.log(
              "内容边界:",
              minX,
              minY,
              maxX,
              maxY,
              "内容尺寸:",
              contentW,
              "x",
              contentH,
            );

            ctx.imageSmoothingEnabled = false;
            // 直接把内容区域缩放到目标尺寸
            ctx.drawImage(
              boundsCanvas,
              minX,
              minY,
              contentW,
              contentH,
              0,
              0,
              this.customWidth,
              this.customHeight,
            );
          } else {
            // 普通图片模式：先裁剪有内容的区域，再高质量缩放
            ctx.imageSmoothingEnabled = true;
            ctx.imageSmoothingQuality = "high";

            // 检测有内容的边界框
            const boundsData = tempCtx.getImageData(
              0,
              0,
              img.width,
              img.height,
            ).data;
            let minX = img.width,
              minY = img.height,
              maxX = 0,
              maxY = 0;
            let hasContent = false;
            for (let py = 0; py < img.height; py++) {
              for (let px = 0; px < img.width; px++) {
                if (boundsData[(py * img.width + px) * 4 + 3] >= 128) {
                  hasContent = true;
                  if (px < minX) minX = px;
                  if (px > maxX) maxX = px;
                  if (py < minY) minY = py;
                  if (py > maxY) maxY = py;
                }
              }
            }

            if (
              hasContent &&
              (minX > 0 ||
                minY > 0 ||
                maxX < img.width - 1 ||
                maxY < img.height - 1)
            ) {
              const contentW = maxX - minX + 1;
              const contentH = maxY - minY + 1;
              console.log("普通图片裁剪内容区域:", contentW, "x", contentH);
              ctx.drawImage(
                tempCanvas,
                minX,
                minY,
                contentW,
                contentH,
                0,
                0,
                this.customWidth,
                this.customHeight,
              );
            } else {
              ctx.drawImage(img, 0, 0, this.customWidth, this.customHeight);
            }
          }

          const imageData = ctx.getImageData(
            0,
            0,
            this.customWidth,
            this.customHeight,
          );
          const data = imageData.data;

          this.processImageData(data, artkalPalette, selectedArtkalColors);
        };

        img.onerror = () => {
          this.toast.showError("图片加载失败");
          this.isProcessing = false;
        };

        img.src = this.imageFile;
        // #endif

        // #ifdef MP-WEIXIN
        // 小程序环境：使用uni.getImageInfo和canvas
        console.log("小程序环境：开始处理图片");

        const imageInfo = await new Promise((resolve, reject) => {
          uni.getImageInfo({
            src: this.imageFile,
            success: resolve,
            fail: reject,
          });
        });

        console.log("图片信息获取成功:", imageInfo);

        // 延迟等待canvas准备好
        await new Promise((resolve) => setTimeout(resolve, 500));

        // 创建临时canvas处理图片
        const query = uni.createSelectorQuery().in(this);
        query
          .select("#tempProcessCanvas")
          .fields({ node: true, size: true })
          .exec((res) => {
            console.log("Canvas查询结果:", res);

            if (!res || !res[0] || !res[0].node) {
              console.error("Canvas节点未找到");
              this.toast.showError("Canvas 初始化失败，请重试");
              this.isProcessing = false;
              return;
            }

            const canvas = res[0].node;
            const ctx = canvas.getContext("2d", { willReadFrequently: true });

            if (!ctx) {
              console.error("Canvas context 获取失败");
              this.toast.showError("Canvas context 初始化失败");
              this.isProcessing = false;
              return;
            }

            console.log("Canvas初始化成功，开始绘制");

            canvas.width = this.customWidth;
            canvas.height = this.customHeight;

            const img = canvas.createImage();
            img.onload = () => {
              console.log("图片加载成功，开始处理像素");

              // 先绘制原始尺寸以检测是否为像素画
              const tempCanvas = canvas;
              const tempCtx = ctx;
              tempCanvas.width = img.width;
              tempCanvas.height = img.height;
              tempCtx.drawImage(img, 0, 0);
              const detectData = tempCtx.getImageData(
                0,
                0,
                img.width,
                img.height,
              );

              // 检测是否为像素画
              const isPixelArt = this.detectPixelArt(
                detectData,
                img.width,
                img.height,
              );
              console.log(
                "检测到像素画:",
                isPixelArt,
                "原图尺寸:",
                img.width,
                "x",
                img.height,
                "目标尺寸:",
                this.customWidth,
                "x",
                this.customHeight,
              );

              if (isPixelArt) {
                this.toast.showSuccess("检测到像素画，已自动优化处理");
              }

              // 重置canvas为目标尺寸
              canvas.width = this.customWidth;
              canvas.height = this.customHeight;

              if (this.isCropped && this.cropParams) {
                // 裁剪过的图片：直接把裁剪区域缩放填满目标尺寸
                ctx.imageSmoothingEnabled = !isPixelArt;
                ctx.drawImage(
                  img,
                  this.cropParams.sx,
                  this.cropParams.sy,
                  this.cropParams.sw,
                  this.cropParams.sh,
                  0,
                  0,
                  this.customWidth,
                  this.customHeight,
                );
                const imageData = ctx.getImageData(
                  0,
                  0,
                  this.customWidth,
                  this.customHeight,
                );
                this.processImageData(
                  imageData.data,
                  artkalPalette,
                  selectedArtkalColors,
                );
                return;
              }

              if (isPixelArt) {
                // 像素画模式：使用整数倍缩放策略
                ctx.imageSmoothingEnabled = false;

                // 计算最接近的整数缩放比例
                const scaleX = this.customWidth / img.width;
                const scaleY = this.customHeight / img.height;
                const scale = Math.max(scaleX, scaleY);

                // 如果需要放大，使用向上取整；如果缩小，使用向下取整
                let integerScale;
                if (scale >= 1) {
                  integerScale = Math.ceil(scale);
                } else {
                  // 缩小时，找最接近的整数分数（1/2, 1/3, 1/4...）
                  integerScale = 1 / Math.ceil(1 / scale);
                }

                // 计算中间尺寸
                const intermediateWidth = Math.round(img.width * integerScale);
                const intermediateHeight = Math.round(
                  img.height * integerScale,
                );

                // 创建临时canvas进行整数倍缩放
                const query2 = uni.createSelectorQuery().in(this);
                query2
                  .select("#importCanvas")
                  .fields({ node: true })
                  .exec((res2) => {
                    if (!res2 || !res2[0] || !res2[0].node) {
                      // 如果获取不到临时canvas，直接缩放
                      ctx.drawImage(
                        img,
                        0,
                        0,
                        this.customWidth,
                        this.customHeight,
                      );
                      const imageData = ctx.getImageData(
                        0,
                        0,
                        this.customWidth,
                        this.customHeight,
                      );
                      this.processImageData(
                        imageData.data,
                        artkalPalette,
                        selectedArtkalColors,
                      );
                      return;
                    }

                    const intermediateCanvas = res2[0].node;
                    const intermediateCtx = intermediateCanvas.getContext("2d");
                    intermediateCanvas.width = intermediateWidth;
                    intermediateCanvas.height = intermediateHeight;
                    intermediateCtx.imageSmoothingEnabled = false;

                    // 整数倍缩放
                    intermediateCtx.drawImage(
                      img,
                      0,
                      0,
                      intermediateWidth,
                      intermediateHeight,
                    );

                    // 居中裁剪到目标尺寸
                    const offsetX = Math.floor(
                      (intermediateWidth - this.customWidth) / 2,
                    );
                    const offsetY = Math.floor(
                      (intermediateHeight - this.customHeight) / 2,
                    );

                    ctx.drawImage(
                      intermediateCanvas,
                      Math.max(0, offsetX),
                      Math.max(0, offsetY),
                      Math.min(intermediateWidth, this.customWidth),
                      Math.min(intermediateHeight, this.customHeight),
                      Math.max(0, -offsetX),
                      Math.max(0, -offsetY),
                      Math.min(intermediateWidth, this.customWidth),
                      Math.min(intermediateHeight, this.customHeight),
                    );

                    console.log(
                      "像素画模式：整数倍缩放",
                      integerScale,
                      "中间尺寸:",
                      intermediateWidth,
                      "x",
                      intermediateHeight,
                    );

                    const imageData = ctx.getImageData(
                      0,
                      0,
                      this.customWidth,
                      this.customHeight,
                    );
                    this.processImageData(
                      imageData.data,
                      artkalPalette,
                      selectedArtkalColors,
                    );
                  });
              } else {
                // 普通图片模式：先裁剪有内容的区域，再高质量缩放
                ctx.imageSmoothingEnabled = true;

                // 用 detectData 检测有内容的边界框
                const bData = detectData.data;
                let minX = img.width,
                  minY = img.height,
                  maxX = 0,
                  maxY = 0;
                let hasContent = false;
                for (let py = 0; py < img.height; py++) {
                  for (let px = 0; px < img.width; px++) {
                    if (bData[(py * img.width + px) * 4 + 3] >= 128) {
                      hasContent = true;
                      if (px < minX) minX = px;
                      if (px > maxX) maxX = px;
                      if (py < minY) minY = py;
                      if (py > maxY) maxY = py;
                    }
                  }
                }

                canvas.width = this.customWidth;
                canvas.height = this.customHeight;

                if (
                  hasContent &&
                  (minX > 0 ||
                    minY > 0 ||
                    maxX < img.width - 1 ||
                    maxY < img.height - 1)
                ) {
                  const contentW = maxX - minX + 1;
                  const contentH = maxY - minY + 1;
                  console.log(
                    "小程序普通图片裁剪内容区域:",
                    contentW,
                    "x",
                    contentH,
                  );
                  ctx.drawImage(
                    img,
                    minX,
                    minY,
                    contentW,
                    contentH,
                    0,
                    0,
                    this.customWidth,
                    this.customHeight,
                  );
                } else {
                  ctx.drawImage(img, 0, 0, this.customWidth, this.customHeight);
                }

                const imageData = ctx.getImageData(
                  0,
                  0,
                  this.customWidth,
                  this.customHeight,
                );
                this.processImageData(
                  imageData.data,
                  artkalPalette,
                  selectedArtkalColors,
                );
              }
            };

            img.onerror = (err) => {
              console.error("图片加载失败:", err);
              this.toast.showError("图片加载失败");
              this.isProcessing = false;
            };

            img.src = this.imageFile;
          });
        // #endif
      } catch (error) {
        console.error("生成预览失败:", error);
        this.toast.showError("生成预览失败，请重试");
        this.isProcessing = false;
      }
    },

    // 计算图片中有内容的边界框
    calculateContentBounds(imageData) {
      const data = imageData.data;
      const width = imageData.width;
      const height = imageData.height;

      let minX = width,
        minY = height,
        maxX = 0,
        maxY = 0;
      let hasContent = false;

      for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
          const idx = (y * width + x) * 4;
          const alpha = data[idx + 3];

          // 有不透明像素
          if (alpha >= 128) {
            hasContent = true;
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
          }
        }
      }

      return hasContent ? { minX, minY, maxX, maxY } : null;
    },

    processImageData(data, artkalPalette, selectedArtkalColors) {
      // 计算偏移量（居中）
      const offsetX = Math.floor((this.paddedWidth - this.customWidth) / 2);
      const offsetY = Math.floor((this.paddedHeight - this.customHeight) / 2);

      const artkalPixels = new Map();
      const colorUsage = new Map(); // 统计颜色使用次数

      // 将每个像素映射到最接近的 Artkal 颜色
      for (let y = 0; y < this.customHeight; y++) {
        for (let x = 0; x < this.customWidth; x++) {
          const idx = (y * this.customWidth + x) * 4;
          const alpha = data[idx + 3];

          // 只跳过透明像素
          if (alpha < 128) continue;

          const r = data[idx];
          const g = data[idx + 1];
          const b = data[idx + 2];

          // 找到最接近的 Artkal 颜色
          let minDist = Infinity;
          let closestColor = artkalPalette[0];

          for (const artkalHex of artkalPalette) {
            const artkalRgb = this.hexToRgb(artkalHex);
            const dist = Math.sqrt(
              Math.pow(r - artkalRgb[0], 2) +
                Math.pow(g - artkalRgb[1], 2) +
                Math.pow(b - artkalRgb[2], 2),
            );
            if (dist < minDist) {
              minDist = dist;
              closestColor = artkalHex;
            }
          }

          // 使用偏移后的坐标
          artkalPixels.set(`${x + offsetX},${y + offsetY}`, closestColor);

          // 统计颜色使用次数
          colorUsage.set(closestColor, (colorUsage.get(closestColor) || 0) + 1);
        }
      }

      // 保存预览数据
      this.previewPixels = artkalPixels;

      // 生成使用的颜色列表
      this.usedColors = Array.from(colorUsage.entries())
        .map(([hex, count]) => {
          const colorInfo = selectedArtkalColors.find((c) => c.hex === hex);
          return {
            hex,
            code: colorInfo?.code || "",
            name: colorInfo?.name || "",
            count,
          };
        })
        .sort((a, b) => b.count - a.count); // 按使用次数排序

      // 绘制预览
      this.drawPreview(artkalPixels);

      // 进入预览步骤
      this.isProcessing = false;
      this.stepAnimationClass = "step-exit";
      setTimeout(() => {
        this.step = 3; // 图片模式的预览是第4步
        this.stepAnimationClass = "step-enter";
      }, 150);
    },

    drawPreview(pixels) {
      setTimeout(() => {
        const query = uni.createSelectorQuery().in(this);
        query
          .select("#previewDisplayCanvas")
          .fields({ node: true, size: true })
          .exec((res) => {
            if (!res || !res[0]) {
              console.error("Canvas not found, 重试中...");
              // 重试一次
              setTimeout(() => {
                this.drawPreview(pixels);
              }, 500);
              return;
            }

            const canvas = res[0].node;
            if (!canvas) {
              console.error("Canvas node is null");
              return;
            }

            const ctx = canvas.getContext("2d");
            if (!ctx) {
              console.error("Canvas context is null");
              return;
            }

            const containerWidth = res[0].width;
            const containerHeight = res[0].height;

            // canvas 物理尺寸 = 容器尺寸
            canvas.width = containerWidth;
            canvas.height = containerHeight;

            // 清空背景（透明，视觉背景通过CSS容器实现）
            ctx.clearRect(0, 0, containerWidth, containerHeight);

            // 使用最大边作为正方形基准
            const maxDimension = Math.max(this.paddedWidth, this.paddedHeight);

            // 根据项目大小选择倍数
            let multiplier = 1;
            // #ifdef H5
            multiplier = maxDimension > 64 ? 3 : 2;
            // #endif

            // #ifdef MP-WEIXIN
            multiplier = maxDimension > 64 ? 1.5 : 1;
            // #endif

            let scale = Math.min(
              containerWidth / (maxDimension * multiplier),
              containerHeight / (maxDimension * multiplier),
            );
            // scale >= 1 取整，scale < 1 保留两位小数
            if (scale >= 4) {
              // #ifdef H5
              scale = 1.65;
              // #endif

              // #ifdef MP-WEIXIN
              scale = 4;
              // #endif
            } else {
              // #ifdef H5
              scale = Math.round(scale * 100) / 100;
              // #endif

              // #ifdef MP-WEIXIN
              scale = (Math.round(scale * 100) / 100) * 1.5;
              // #endif
            }
            // 计算画布内容的实际尺寸
            const contentWidth = this.paddedWidth * scale;
            const contentHeight = this.paddedHeight * scale;

            // 计算居中偏移量（让整个画布内容在canvas中居中）
            const canvasCenterOffsetX = (containerWidth - contentWidth) / 2;
            const canvasCenterOffsetY = (containerHeight - contentHeight) / 2;

            // 计算图形在画布内的居中偏移量
            const centerOffsetX = (maxDimension - this.paddedWidth) / 2;
            const centerOffsetY = (maxDimension - this.paddedHeight) / 2;

            // 绘制像素（加上两层偏移）
            pixels.forEach((color, key) => {
              const [x, y] = key.split(",").map(Number);
              ctx.fillStyle = color;
              ctx.fillRect(
                canvasCenterOffsetX + (x + centerOffsetX) * scale,
                canvasCenterOffsetY + (y + centerOffsetY) * scale,
                scale,
                scale,
              );
            });

            console.log(
              "Preview drawn:",
              pixels.size,
              "pixels",
              "scale:",
              scale,
              "canvasOffset:",
              canvasCenterOffsetX,
              canvasCenterOffsetY,
            );

            // #ifdef MP-WEIXIN
            // 小程序：将canvas转为图片
            setTimeout(() => {
              uni.canvasToTempFilePath(
                {
                  canvas: canvas,
                  success: (res) => {
                    this.previewImageUrl = res.tempFilePath;
                    console.log("Canvas转图片成功:", res.tempFilePath);
                  },
                  fail: (err) => {
                    console.error("Canvas转图片失败:", err);
                  },
                },
                this,
              );
            }, 100);
            // #endif
          });
      }, 500);
    },

    clearImage() {
      this.imageFile = null;
      this.previewUrl = null;
      this.previewPixels = null;
      this.usedColors = [];
      this.recommendedSize = null;
      this.contentRatio = null;
      this.isCropped = false;
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

    saveThumbnailForProject(projectId, pixels, callback) {
      console.log(
        "开始生成缩略图，projectId:",
        projectId,
        "像素数量:",
        pixels.size,
      );

      const query = uni.createSelectorQuery().in(this);
      query
        .select("#thumbnailSaveCanvas")
        .fields({ node: true, size: true })
        .exec((res) => {
          if (!res || !res[0] || !res[0].node) {
            console.error("Thumbnail canvas not found");
            if (callback) callback();
            return;
          }

          const canvas = res[0].node;
          const ctx = canvas.getContext("2d");
          if (!ctx) {
            console.error("Canvas context is null");
            if (callback) callback();
            return;
          }

          // 缩小到 100x100 节省存储空间
          const thumbSize = 100;
          canvas.width = thumbSize;
          canvas.height = thumbSize;

          ctx.clearRect(0, 0, thumbSize, thumbSize);

          // 计算缩放比例，让图片尽可能填充画布
          const scale =
            Math.min(
              thumbSize / this.paddedWidth,
              thumbSize / this.paddedHeight,
            ) * 0.9;

          // 计算居中偏移
          const scaledWidth = this.paddedWidth * scale;
          const scaledHeight = this.paddedHeight * scale;
          const offsetX = (thumbSize - scaledWidth) / 2;
          const offsetY = (thumbSize - scaledHeight) / 2;

          // 绘制像素
          pixels.forEach((color, key) => {
            const [x, y] = key.split(",").map(Number);
            ctx.fillStyle = color;
            ctx.fillRect(
              offsetX + x * scale,
              offsetY + y * scale,
              scale,
              scale,
            );
          });

          // 等待绘制完成后转换为 Base64
          setTimeout(() => {
            uni.canvasToTempFilePath(
              {
                canvas: canvas,
                x: 0,
                y: 0,
                width: thumbSize,
                height: thumbSize,
                destWidth: thumbSize,
                destHeight: thumbSize,
                fileType: "png",
                quality: 0.8,
                success: (res) => {
                  // #ifdef MP-WEIXIN
                  // 小程序：转为 Base64
                  const fs = uni.getFileSystemManager();
                  fs.readFile({
                    filePath: res.tempFilePath,
                    encoding: "base64",
                    success: (fileRes) => {
                      const base64 = "data:image/png;base64," + fileRes.data;
                      console.log(
                        "缩略图生成成功 (Base64):",
                        Math.round(base64.length / 1024),
                        "KB",
                      );
                      this.projectStore.updateProjectThumbnail(
                        projectId,
                        base64,
                      );
                      if (callback) callback();
                    },
                    fail: (err) => {
                      console.error("读取缩略图文件失败:", err);
                      // 降级：保存临时文件路径
                      this.projectStore.updateProjectThumbnail(
                        projectId,
                        res.tempFilePath,
                      );
                      if (callback) callback();
                    },
                  });
                  // #endif

                  // #ifdef H5
                  // H5：直接使用 canvas.toDataURL
                  try {
                    const base64 = canvas.toDataURL("image/png", 0.8);
                    console.log(
                      "缩略图生成成功 (Base64):",
                      Math.round(base64.length / 1024),
                      "KB",
                    );
                    this.projectStore.updateProjectThumbnail(projectId, base64);
                    if (callback) callback();
                  } catch (err) {
                    console.error("生成 Base64 失败:", err);
                    // 降级：保存临时文件路径
                    this.projectStore.updateProjectThumbnail(
                      projectId,
                      res.tempFilePath,
                    );
                    if (callback) callback();
                  }
                  // #endif
                },
                fail: (err) => {
                  console.error("保存项目缩略图失败:", err);
                  if (callback) callback();
                },
              },
              this,
            );
          }, 300);
        });
    },

    hexToRgb(hex) {
      const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      return result
        ? [
            parseInt(result[1], 16),
            parseInt(result[2], 16),
            parseInt(result[3], 16),
          ]
        : [0, 0, 0];
    },
  },
};
</script>

<style scoped>
.create-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--color-app-background);
  position: relative;
  overflow: hidden;
  user-select: none;
  -webkit-user-drag: none;
  -webkit-touch-callout: none;
  box-sizing: border-box;
}

/* 主要内容 */
.main-content {
  flex: 1;
  overflow-y: auto;
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
  color: var(--color-text-primary);
  margin-bottom: 24rpx;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 24rpx;
}

.project-count {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  font-weight: 500;
}

/* 快速操作 */
.quick-actions {
  margin: 32rpx;
  margin-bottom: 48rpx;
}

.action-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

.action-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

.action-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 32rpx 16rpx;
  background-color: var(--color-card-background);
  border-radius: var(--radius-medium);
  box-shadow: var(--shadow-card);
  transition: all 0.2s ease;
}

.action-card:active {
  transform: scale(0.95);
  box-shadow: var(--shadow-floating);
}

.action-icon {
  width: 96rpx;
  height: 96rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  border-radius: var(--radius-medium);
  margin-bottom: 16rpx;
}

.action-title {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin-bottom: 8rpx;
  text-align: center;
}

.action-subtitle {
  font-size: 22rpx;
  color: var(--color-text-secondary);
  text-align: center;
  line-height: 1.4;
}

/* 我的画布 */
.my-projects {
  margin: 0 32rpx 48rpx;
}

.projects-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

/* 空状态 */
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 64rpx 32rpx;
  text-align: center;
}

.empty-icon {
  width: 160rpx;
  height: 160rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  border-radius: 50%;
  margin-bottom: 32rpx;
}

.empty-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin-bottom: 12rpx;
}

.empty-subtitle {
  font-size: 26rpx;
  color: var(--color-text-secondary);
  line-height: 1.5;
  margin-bottom: 32rpx;
}

.empty-btn {
  padding: 20rpx 40rpx;
  background-color: var(--color-brand-primary);
  border-radius: var(--radius-small);
  transition: all 0.2s ease;
}

.empty-btn:active {
  transform: scale(0.95);
}

.empty-btn-text {
  font-size: 28rpx;
  font-weight: 600;
  color: #ffffff;
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--color-app-background);
  flex-shrink: 0;
}

.back-btn {
  width: 64rpx;
}

.back-icon {
  font-size: 36rpx;
  color: var(--color-text-primary);
  line-height: 1;
}

/* 顶部导航 */
.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--color-card-background);
  border-bottom: 2rpx solid var(--border-primary);
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
  color: var(--color-text-primary);
}
.content-area {
  flex: 1;
  padding: 0 32rpx 32rpx;
  overflow-y: auto;
  z-index: 10;
  box-sizing: border-box;
  margin-bottom: 0; /* 确保不与底部按钮重叠 */
}

/* 步骤1特殊布局 */
.step-1-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.mode-selector-fixed {
  flex-shrink: 0;
  padding: 32rpx 48rpx 24rpx;
}

.step-1-scroll {
  flex: 1;
  padding-top: 24rpx;
  padding-right: 32rpx;
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
  padding: 32rpx 0;
  border-bottom: 4rpx solid var(--border-primary);
  transition: var(--transition-base);
  min-height: 120rpx;
  display: flex;
  align-items: center;
}

.name-input-wrapper.focused {
  border-bottom-color: #00f3ff;
  box-shadow: 0 4rpx 0 rgba(0, 243, 255, 0.3);
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

/* 步骤 1: 模式和尺寸 */
.mode-selector {
  display: flex;
  background-color: var(--bg-tertiary);
  padding: 6rpx;
  border-radius: 16rpx;
  gap: 0;
  border: 2rpx solid var(--border-primary);
}

.mode-btn {
  flex: 1;
  padding: 20rpx 24rpx;
  border-radius: 12rpx;
  text-align: center;
  background-color: transparent;
  border: none;
  transition: all 0.3s ease;
}

.mode-btn.active {
  background-color: var(--create-tabs-active-bg);
  box-shadow: var(--create-tabs-active-box-shadow);
}

.mode-text {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-secondary);
  transition: all 0.3s ease;
}

.mode-btn.active .mode-text {
  color: #000000;
  font-weight: bold;
}

/* 浅色模式适配 */
.light-theme .mode-btn.active .mode-text {
  color: #ffffff;
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
  border-radius: 24rpx;
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
  border-radius: 50%;
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
  border-radius: 24rpx;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 480rpx;
}

.preview-image {
  width: 100%;
  max-height: 480rpx;
  object-fit: contain;
}

.preview-actions {
  display: flex;
  gap: 16rpx;
  padding: 24rpx;
}

.preview-btn {
  flex: 1;
  padding: 24rpx;
  background-color: var(--bg-elevated);
  border-radius: 12rpx;
  text-align: center;
}

.preview-btn.danger {
  flex: 0 0 auto;
  width: 96rpx;
  background-color: rgba(255, 51, 51, 0.1);
}

.recommend-tip {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 20rpx 24rpx;
  margin-top: 16rpx;
  background: rgba(0, 243, 255, 0.08);
  border: 1rpx solid rgba(0, 243, 255, 0.2);
  border-radius: 16rpx;
}

.recommend-text {
  font-size: 24rpx;
  color: var(--accent-primary);
}

.recommend-btn-text {
  font-size: 24rpx;
  color: var(--accent-primary);
  font-weight: bold;
  white-space: nowrap;
  margin-left: 16rpx;
}

.preview-btn-text {
  font-size: 28rpx;
  color: var(--text-primary);
}

.preview-btn.danger .preview-btn-text {
  color: var(--error-color);
}

/* 自定义尺寸输入 */
.custom-size-inputs {
  display: flex;
  align-items: center;
  gap: 32rpx;
  margin-bottom: 32rpx;
}

.size-input-group {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.size-input-label {
  font-size: 28rpx;
  color: var(--text-secondary);
  font-weight: 500;
}

.size-input {
  background-color: var(--bg-elevated);
  border: 3rpx solid var(--border-primary);
  border-radius: 16rpx;
  font-size: 56rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--text-primary);
  text-align: center;
  transition: var(--transition-base);
  min-height: 120rpx;
}

.size-input:focus {
  border-color: var(--accent-primary);
  background-color: var(--bg-tertiary);
  box-shadow: 0 0 0 4rpx rgba(0, 243, 255, 0.1);
}

.size-input::placeholder {
  color: var(--text-tertiary);
  opacity: 0.5;
}

.size-separator {
  font-size: 64rpx;
  color: var(--text-secondary);
  margin-top: 48rpx;
  font-weight: bold;
}

.size-preview {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16rpx 24rpx;
  background-color: var(--bg-tertiary);
  border-radius: 12rpx;
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
  color: var(--accent-primary);
}

/* 内联尺寸预览（在标题右侧） */
.size-preview-inline {
  display: flex;
  align-items: center;
}

.size-preview-inline .size-preview-value {
  font-size: 24rpx;
}

/* 尺寸选择 */
.size-section {
  display: flex;
  flex-direction: column;
  gap: 32rpx;
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
  gap: 64rpx;
}

.size-column {
  display: flex;
  flex-direction: column;
  gap: 32rpx;
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
  color: var(--accent-color);
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
  border-radius: 12rpx;
  background-color: var(--bg-tertiary);
  border: 3rpx solid var(--border-primary);
  transition: all 0.2s;
  text-align: center;
}

.size-option.active {
  border-color: var(--create-size-active-border-color);
  background-color: var(--create-size-active-bg);
  box-shadow: var(--create-size-active-box-shadow);
}

.size-option-text {
  font-size: 28rpx;
  font-family: monospace;
  color: var(--text-secondary);
  font-weight: 500;
}

.size-option.active .size-option-text {
  color: var(--accent-primary);
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
  border-radius: 24rpx;
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
  padding: 48rpx 32rpx;
  border-radius: 24rpx;
  background-color: var(--bg-tertiary);
  border: 4rpx solid var(--border-color);
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 8rpx;
  transition: all 0.3s;
}

.preset-card.active {
  background-color: var(--create-preset-card-bg);
  border-color: var(--create-preset-card-border-color);
  color: var(--create-preset-name);
}

.preset-card.active .preset-name {
  color: var(--create-preset-name);
}

.preset-card.active .preset-count {
  color: var(--create-preset-count);
}

.preset-name {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.preset-count {
  font-size: 24rpx;
  color: var(--text-secondary);
  opacity: 0.7;
}

.selected-info {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-color);
  border-radius: 24rpx;
  padding: 48rpx;
  display: flex;
  flex-direction: column;
  gap: 32rpx;
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
  color: var(--create-info-value-clr);
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
  padding: 24rpx;
  background-color: var(--bg-tertiary);
  border-radius: 24rpx;
  border: 2rpx solid var(--border-primary);
  position: relative;
  transform: translateZ(0);
}

.preview-canvas {
  border-radius: 12rpx;
  box-shadow: var(--shadow-md);
  display: block;
  background-color: var(--bg-primary);
  width: 260px;
  height: 260px;
  position: relative;
  transform: translateZ(0);
}

.preview-image {
  border-radius: 12rpx;
  box-shadow: var(--shadow-md);
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
  border: 2rpx solid var(--border-primary);
}

.preview-stat-label {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.preview-stat-value {
  font-size: 32rpx;
  font-weight: bold;
  font-family: monospace;
  color: var(--accent-primary);
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
  border: 2rpx solid var(--border-primary);
}

.color-swatch {
  width: 48rpx;
  height: 48rpx;
  border-radius: 8rpx;
  border: 2rpx solid var(--border-primary);
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
  padding-bottom: 48rpx; /* 增加底部安全距离 */
  z-index: 20;
  box-sizing: border-box;
  flex-shrink: 0;
  background-color: var(--color-app-background);
}

/* 小程序底部安全区域 */
/* #ifdef MP-WEIXIN */
.footer {
  padding-bottom: 68rpx; /* 小程序需要更多底部空间 */
}
/* #endif */

.next-btn {
  padding: 32rpx;
  background: linear-gradient(
    135deg,
    var(--text-primary) 0%,
    var(--text-secondary) 100%
  );
  color: var(--text-inverse);
  border-radius: 24rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
  box-shadow: var(--shadow-md);
  transition: var(--transition-base);
}

.next-btn:active {
  transform: scale(0.98);
}

.next-btn.disabled {
  background: linear-gradient(
    135deg,
    var(--text-primary) 0%,
    var(--text-secondary) 100%
  );
  box-shadow: none;
  opacity: 1;
  pointer-events: none;
}

.next-btn-text {
  font-size: 32rpx;
  font-weight: bold;
  letter-spacing: 0.2em;
  text-transform: uppercase;
}

.next-icon {
  font-size: 48rpx;
  line-height: 1;
}
</style>
