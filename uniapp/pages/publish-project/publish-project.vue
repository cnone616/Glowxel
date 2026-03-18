<template>
  <view class="publish-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 导航栏 -->
    <view class="navbar">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--color-text-primary)"
        />
      </view>
      <text class="nav-title">发布作品</text>
      <view class="nav-right"></view>
    </view>

    <!-- 主要内容 -->
    <scroll-view scroll-y class="content">
      <!-- 作品预览 -->
      <view class="preview-section">
        <view class="preview-header">
          <text class="preview-title">作品预览</text>
          <view class="preview-info">
            <text class="preview-size"
              >{{ canvasData.width }}×{{ canvasData.height }}</text
            >
            <text class="preview-pixels">{{ pixelCount }} 像素</text>
          </view>
        </view>

        <view class="preview-container">
          <canvas
            canvas-id="previewCanvas"
            id="previewCanvas"
            type="2d"
            class="preview-canvas"
          ></canvas>
        </view>
      </view>

      <!-- 发布表单 -->
      <view class="form-section">
        <!-- 作品标题 -->
        <view class="form-group">
          <view class="form-label">
            <text class="label-text">作品标题</text>
            <text class="required-mark">*</text>
          </view>
          <view
            class="input-wrapper"
            :class="{ focused: titleFocused, error: titleError }"
          >
            <input
              v-model="form.title"
              type="text"
              class="form-input"
              placeholder="为您的作品起个好听的名字"
              maxlength="50"
              @focus="titleFocused = true"
              @blur="
                titleFocused = false;
                validateTitle();
              "
              @input="titleError = false"
            />
            <text class="char-count">{{ form.title.length }}/50</text>
          </view>
          <text v-if="titleError" class="error-text">{{ titleErrorMsg }}</text>
        </view>

        <!-- 作品描述 -->
        <view class="form-group">
          <view class="form-label">
            <text class="label-text">作品描述</text>
            <text class="required-mark">*</text>
          </view>
          <view
            class="textarea-wrapper"
            :class="{ focused: descFocused, error: descError }"
          >
            <textarea
              v-model="form.description"
              class="form-textarea"
              placeholder="描述您的创作灵感、使用的技巧或想要表达的内容..."
              maxlength="500"
              auto-height
              @focus="descFocused = true"
              @blur="
                descFocused = false;
                validateDescription();
              "
              @input="descError = false"
            />
            <text class="char-count">{{ form.description.length }}/500</text>
          </view>
          <text v-if="descError" class="error-text">{{ descErrorMsg }}</text>
        </view>

        <!-- 标签选择 -->
        <view class="form-group">
          <view class="form-label">
            <text class="label-text">作品标签</text>
            <text class="optional-mark">选填</text>
          </view>
          <view class="tags-section">
            <!-- 已选标签 -->
            <view v-if="form.tags.length > 0" class="selected-tags">
              <view
                v-for="(tag, index) in form.tags"
                :key="tag"
                class="selected-tag"
                @click="removeTag(index)"
              >
                <text class="tag-text">{{ tag }}</text>
                <Icon name="close" :size="20" />
              </view>
            </view>

            <!-- 推荐标签 -->
            <view class="recommended-tags">
              <text class="tags-subtitle">推荐标签</text>
              <view class="tags-grid">
                <view
                  v-for="tag in availableTags"
                  :key="tag"
                  class="tag-option"
                  :class="{ selected: form.tags.includes(tag) }"
                  @click="toggleTag(tag)"
                >
                  <text class="tag-text">{{ tag }}</text>
                </view>
              </view>
            </view>

            <!-- 自定义标签输入 -->
            <view class="custom-tag-input">
              <view class="input-wrapper">
                <input
                  v-model="customTag"
                  type="text"
                  class="form-input"
                  placeholder="添加自定义标签"
                  maxlength="20"
                  @confirm="addCustomTag"
                />
                <view
                  class="add-tag-btn"
                  :class="{
                    disabled: !customTag.trim() || form.tags.length >= 5,
                  }"
                  @click="addCustomTag"
                >
                  <Icon name="plus" :size="24" />
                </view>
              </view>
              <text class="tags-hint"
                >最多可添加5个标签，每个标签最多20个字符</text
              >
            </view>
          </view>
        </view>

        <!-- 公开设置 -->
        <view class="form-group">
          <view class="form-label">
            <text class="label-text">公开设置</text>
          </view>
          <view class="switch-group">
            <view class="switch-item">
              <view class="switch-info">
                <text class="switch-title">公开作品</text>
                <text class="switch-desc"
                  >其他用户可以在社区中看到您的作品</text
                >
              </view>
              <switch
                :checked="form.isPublic"
                @change="handlePublicChange"
                color="var(--color-brand-primary)"
              />
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <!-- 底部发布按钮 -->
    <view class="footer">
      <view class="publish-info">
        <text class="info-text">发布后作品将无法编辑</text>
      </view>
      <view
        class="publish-btn"
        :class="{ disabled: !canPublish || isPublishing }"
        @click="handlePublish"
      >
        <view v-if="isPublishing" class="loading-spinner">
          <Icon name="loading" :size="32" />
        </view>
        <Icon v-else name="upload" :size="32" />
        <text class="publish-btn-text">{{
          isPublishing ? "发布中..." : "确认发布"
        }}</text>
      </view>
    </view>

    <!-- 隐藏的缩略图生成Canvas -->
    <canvas
      canvas-id="thumbnailCanvas"
      id="thumbnailCanvas"
      type="2d"
      style="
        position: fixed;
        left: -9999px;
        top: -9999px;
        width: 200px;
        height: 200px;
      "
    />

    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useProjectStore } from "../../store/project.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
  },

  data() {
    return {
      projectStore: null,
      toast: null,

      // 画布数据
      canvasData: {
        projectId: "",
        boardId: "",
        pixels: [],
        width: 0,
        height: 0,
        colors: [],
        projectName: "",
      },

      // 表单数据
      form: {
        title: "",
        description: "",
        tags: [],
        isPublic: true,
      },

      // 表单状态
      titleFocused: false,
      descFocused: false,
      titleError: false,
      descError: false,
      titleErrorMsg: "",
      descErrorMsg: "",

      // 标签相关
      customTag: "",
      availableTags: [
        "像素艺术",
        "创意设计",
        "游戏",
        "动漫",
        "风景",
        "人物",
        "抽象",
        "几何",
        "动物",
        "建筑",
        "食物",
        "植物",
        "节日",
        "卡通",
        "复古",
      ],

      // 发布状态
      isPublishing: false,

      // 预览相关
      previewReady: false,
    };
  },

  computed: {
    pixelCount() {
      return this.canvasData.pixels ? this.canvasData.pixels.length : 0;
    },

    canPublish() {
      return (
        this.form.title.trim().length > 0 &&
        this.form.description.trim().length > 0 &&
        !this.titleError &&
        !this.descError &&
        this.previewReady
      );
    },
  },

  onLoad() {
    this.projectStore = useProjectStore();
    this.toast = useToast();

    // 从临时存储中获取画布数据
    const tempData = uni.getStorageSync("temp_publish_data");
    if (tempData) {
      this.canvasData = tempData;
      this.form.title = tempData.projectName || "";

      // 清除临时数据
      uni.removeStorageSync("temp_publish_data");

      // 绘制预览
      this.$nextTick(() => {
        this.drawPreview();
      });
    } else {
      this.toast.showError("未找到画布数据");
      setTimeout(() => {
        uni.navigateBack();
      }, 1500);
    }
  },

  onReady() {
    // 注册Toast实例
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
  },

  methods: {
    handleBack() {
      if (this.isPublishing) {
        return;
      }

      // 检查是否有未保存的内容
      if (
        this.form.title.trim() ||
        this.form.description.trim() ||
        this.form.tags.length > 0
      ) {
        uni.showModal({
          title: "确认离开",
          content: "您填写的内容将会丢失，确定要离开吗？",
          success: (res) => {
            if (res.confirm) {
              uni.navigateBack();
            }
          },
        });
      } else {
        uni.navigateBack();
      }
    },

    // 绘制预览
    drawPreview() {
      setTimeout(() => {
        const query = uni.createSelectorQuery().in(this);
        query
          .select("#previewCanvas")
          .fields({ node: true, size: true })
          .exec((res) => {
            if (!res || !res[0] || !res[0].node) {
              console.error("预览Canvas未找到");
              return;
            }

            const canvas = res[0].node;
            const ctx = canvas.getContext("2d");
            const containerWidth = res[0].width;
            const containerHeight = res[0].height;

            canvas.width = containerWidth;
            canvas.height = containerHeight;

            // 清空背景
            ctx.fillStyle = "#f5f5f5";
            ctx.fillRect(0, 0, containerWidth, containerHeight);

            // 计算缩放比例
            const scale =
              Math.min(
                containerWidth / this.canvasData.width,
                containerHeight / this.canvasData.height,
              ) * 0.8;

            const scaledWidth = this.canvasData.width * scale;
            const scaledHeight = this.canvasData.height * scale;
            const offsetX = (containerWidth - scaledWidth) / 2;
            const offsetY = (containerHeight - scaledHeight) / 2;

            // 绘制像素
            this.canvasData.pixels.forEach(([position, color]) => {
              const [x, y] = position.split(",").map(Number);
              ctx.fillStyle = color;
              ctx.fillRect(
                offsetX + x * scale,
                offsetY + y * scale,
                scale,
                scale,
              );
            });

            this.previewReady = true;
            console.log("预览绘制完成");
          });
      }, 500);
    },

    // 表单验证
    validateTitle() {
      const title = this.form.title.trim();
      if (!title) {
        this.titleError = true;
        this.titleErrorMsg = "请输入作品标题";
        return false;
      }
      if (title.length < 2) {
        this.titleError = true;
        this.titleErrorMsg = "标题至少需要2个字符";
        return false;
      }
      this.titleError = false;
      this.titleErrorMsg = "";
      return true;
    },

    validateDescription() {
      const desc = this.form.description.trim();
      if (!desc) {
        this.descError = true;
        this.descErrorMsg = "请输入作品描述";
        return false;
      }
      if (desc.length < 10) {
        this.descError = true;
        this.descErrorMsg = "描述至少需要10个字符";
        return false;
      }
      this.descError = false;
      this.descErrorMsg = "";
      return true;
    },

    // 标签操作
    toggleTag(tag) {
      const index = this.form.tags.indexOf(tag);
      if (index > -1) {
        this.form.tags.splice(index, 1);
      } else {
        if (this.form.tags.length < 5) {
          this.form.tags.push(tag);
        } else {
          this.toast.showError("最多只能选择5个标签");
        }
      }
    },

    removeTag(index) {
      this.form.tags.splice(index, 1);
    },

    addCustomTag() {
      const tag = this.customTag.trim();
      if (!tag) {
        return;
      }

      if (this.form.tags.length >= 5) {
        this.toast.showError("最多只能添加5个标签");
        return;
      }

      if (this.form.tags.includes(tag)) {
        this.toast.showError("标签已存在");
        return;
      }

      if (tag.length > 20) {
        this.toast.showError("标签最多20个字符");
        return;
      }

      this.form.tags.push(tag);
      this.customTag = "";
    },

    handlePublicChange(e) {
      this.form.isPublic = e.detail.value;
    },

    // 发布作品
    async handlePublish() {
      if (!this.canPublish || this.isPublishing) {
        return;
      }

      // 验证表单
      if (!this.validateTitle() || !this.validateDescription()) {
        this.toast.showError("请完善必填信息");
        return;
      }

      this.isPublishing = true;

      try {
        // 生成缩略图
        const thumbnail = await this.generateThumbnail();

        // 直接使用 projectStore 更新项目状态为已发布
        if (!this.canvasData.projectId || !this.projectStore) {
          throw new Error("项目数据不完整");
        }

        const project = this.projectStore.projects.find(
          (p) => p.id === this.canvasData.projectId,
        );
        if (!project) {
          throw new Error("项目不存在");
        }

        // 更新项目信息
        project.status = "published";
        project.name = this.form.title.trim();
        project.description = this.form.description.trim();
        project.tags = [...this.form.tags];
        project.isPublic = this.form.isPublic;
        if (thumbnail) {
          project.thumbnail = thumbnail;
        }
        project.publishedAt = Date.now();
        this.projectStore.saveToStorage();

        // 显示成功动画和提示
        this.showSuccessAnimation();

        // 延迟跳转到我的作品页面
        setTimeout(() => {
          uni.reLaunch({
            url: "/pages/my-works/my-works",
          });
        }, 2000);
      } catch (error) {
        console.error("发布作品失败:", error);
        this.toast.showError("发布失败，请重试");
        this.isPublishing = false;
      }
    },

    // 生成缩略图
    async generateThumbnail() {
      return new Promise((resolve) => {
        const query = uni.createSelectorQuery().in(this);
        query
          .select("#thumbnailCanvas")
          .fields({ node: true, size: true })
          .exec((res) => {
            if (!res || !res[0] || !res[0].node) {
              resolve("");
              return;
            }

            const canvas = res[0].node;
            const ctx = canvas.getContext("2d");

            const thumbSize = 200;
            canvas.width = thumbSize;
            canvas.height = thumbSize;

            // 清空背景
            ctx.fillStyle = "#ffffff";
            ctx.fillRect(0, 0, thumbSize, thumbSize);

            // 计算缩放比例
            const scale =
              Math.min(
                thumbSize / this.canvasData.width,
                thumbSize / this.canvasData.height,
              ) * 0.9;

            const scaledWidth = this.canvasData.width * scale;
            const scaledHeight = this.canvasData.height * scale;
            const offsetX = (thumbSize - scaledWidth) / 2;
            const offsetY = (thumbSize - scaledHeight) / 2;

            // 绘制像素
            this.canvasData.pixels.forEach(([position, color]) => {
              const [x, y] = position.split(",").map(Number);
              ctx.fillStyle = color;
              ctx.fillRect(
                offsetX + x * scale,
                offsetY + y * scale,
                scale,
                scale,
              );
            });

            // 转换为Base64
            setTimeout(() => {
              uni.canvasToTempFilePath(
                {
                  canvas: canvas,
                  fileType: "png",
                  quality: 0.8,
                  success: (res) => {
                    // #ifdef MP-WEIXIN
                    const fs = uni.getFileSystemManager();
                    fs.readFile({
                      filePath: res.tempFilePath,
                      encoding: "base64",
                      success: (fileRes) => {
                        resolve("data:image/png;base64," + fileRes.data);
                      },
                      fail: () => {
                        resolve(res.tempFilePath);
                      },
                    });
                    // #endif

                    // #ifdef H5
                    try {
                      const base64 = canvas.toDataURL("image/png", 0.8);
                      resolve(base64);
                    } catch (err) {
                      resolve(res.tempFilePath);
                    }
                    // #endif
                  },
                  fail: () => {
                    resolve("");
                  },
                },
                this,
              );
            }, 100);
          });
      });
    },

    // 显示成功动画
    showSuccessAnimation() {
      this.toast.showSuccess("🎉 作品发布成功！");

      // 触觉反馈
      uni.vibrateShort({
        type: "heavy",
      });

      // 可以添加更多成功动画效果
    },
  },
};
</script>

<style scoped>
.publish-page {
  height: 100vh;
  background-color: var(--color-app-background);
  display: flex;
  flex-direction: column;
}

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
}

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.content {
  flex: 1;
  padding: 32rpx;
}

/* 预览区域 */
.preview-section {
  background-color: var(--color-card-background);
  border-radius: 16rpx;
  padding: 32rpx;
  margin-bottom: 32rpx;
  box-shadow: var(--shadow-card);
}

.preview-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24rpx;
}

.preview-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.preview-info {
  display: flex;
  gap: 16rpx;
  align-items: center;
}

.preview-size,
.preview-pixels {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  background-color: var(--color-app-background);
  padding: 8rpx 16rpx;
  border-radius: 8rpx;
}

.preview-container {
  width: 100%;
  height: 400rpx;
  border-radius: 12rpx;
  overflow: hidden;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
}

.preview-canvas {
  width: 100%;
  height: 100%;
}

/* 表单区域 */
.form-section {
  display: flex;
  flex-direction: column;
  gap: 32rpx;
}

.form-group {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.form-label {
  display: flex;
  align-items: center;
  gap: 8rpx;
}

.label-text {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.required-mark {
  color: var(--color-error);
  font-size: 28rpx;
}

.optional-mark {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}

.input-wrapper,
.textarea-wrapper {
  position: relative;
  background-color: var(--color-card-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  transition: all 0.2s ease;
}

.input-wrapper.focused,
.textarea-wrapper.focused {
  border-color: var(--color-brand-primary);
  box-shadow: 0 0 0 4rpx rgba(0, 243, 255, 0.1);
}

.input-wrapper.error,
.textarea-wrapper.error {
  border-color: var(--color-error);
}

.form-input {
  width: 100%;
  padding: 24rpx;
  font-size: 28rpx;
  color: var(--color-text-primary);
  background-color: transparent;
  border: none;
  outline: none;
  box-sizing: border-box;
}

.form-textarea {
  width: 100%;
  min-height: 120rpx;
  padding: 24rpx;
  font-size: 28rpx;
  color: var(--color-text-primary);
  background-color: transparent;
  border: none;
  outline: none;
  resize: none;
  box-sizing: border-box;
}

.char-count {
  position: absolute;
  bottom: 12rpx;
  right: 16rpx;
  font-size: 20rpx;
  color: var(--color-text-disabled);
}

.error-text {
  font-size: 24rpx;
  color: var(--color-error);
  margin-top: -8rpx;
}

/* 标签区域 */
.tags-section {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.selected-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 12rpx;
}

.selected-tag {
  display: flex;
  align-items: center;
  gap: 8rpx;
  background-color: var(--color-brand-primary);
  color: #000000;
  padding: 12rpx 16rpx;
  border-radius: 20rpx;
  font-size: 24rpx;
}

.tags-subtitle {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  margin-bottom: 12rpx;
}

.tags-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 12rpx;
}

.tag-option {
  padding: 12rpx 20rpx;
  background-color: var(--color-card-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 20rpx;
  font-size: 24rpx;
  color: var(--color-text-secondary);
  transition: all 0.2s ease;
}

.tag-option.selected {
  background-color: var(--color-brand-primary);
  border-color: var(--color-brand-primary);
  color: #000000;
}

.custom-tag-input {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.custom-tag-input .input-wrapper {
  display: flex;
  align-items: center;
}

.add-tag-btn {
  padding: 12rpx;
  margin-right: 12rpx;
  background-color: var(--color-brand-primary);
  border-radius: 8rpx;
  color: #000000;
  transition: all 0.2s ease;
}

.add-tag-btn.disabled {
  background-color: var(--color-text-disabled);
  opacity: 0.5;
}

.tags-hint {
  font-size: 20rpx;
  color: var(--color-text-disabled);
}

/* 开关组件 */
.switch-group {
  background-color: var(--color-card-background);
  border-radius: 12rpx;
  padding: 24rpx;
}

.switch-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.switch-info {
  flex: 1;
  margin-right: 24rpx;
}

.switch-title {
  font-size: 28rpx;
  color: var(--color-text-primary);
  margin-bottom: 8rpx;
}

.switch-desc {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  line-height: 1.4;
}

/* 底部区域 */
.footer {
  padding: 24rpx 32rpx;
  padding-bottom: calc(24rpx + env(safe-area-inset-bottom));
  background-color: var(--color-card-background);
  border-top: 2rpx solid var(--border-primary);
}

.publish-info {
  text-align: center;
  margin-bottom: 16rpx;
}

.info-text {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}

.publish-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12rpx;
  width: 100%;
  padding: 32rpx;
  background: linear-gradient(
    135deg,
    var(--color-brand-primary) 0%,
    var(--color-brand-accent) 100%
  );
  border-radius: 16rpx;
  box-shadow: 0 8rpx 24rpx rgba(0, 243, 255, 0.3);
  transition: all 0.3s ease;
  color: #000000;
}

.publish-btn:active {
  transform: scale(0.98);
}

.publish-btn.disabled {
  background: var(--color-text-disabled);
  box-shadow: none;
  opacity: 0.6;
}

.publish-btn.disabled:active {
  transform: none;
}

.loading-spinner {
  animation: spin 1s linear infinite;
}

@keyframes spin {
  from {
    transform: rotate(0deg);
  }
  to {
    transform: rotate(360deg);
  }
}

.publish-btn-text {
  font-size: 32rpx;
  font-weight: 600;
  color: inherit;
}
</style>
