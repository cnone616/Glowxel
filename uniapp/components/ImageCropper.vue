<template>
  <view v-if="visible" class="cropper-overlay" @touchmove.prevent>
    <view class="cropper-title-bar">
      <text class="cropper-title">裁剪图片</text>
    </view>

    <view class="cropper-body">
      <!-- 图片容器 -->
      <view class="image-container" ref="container">
        <image
          :src="src"
          class="crop-image"
          mode="aspectFit"
          @load="onImageLoad"
          :style="imageStyle"
        />
        <!-- 裁剪框 -->
        <view
          class="crop-box"
          :style="cropBoxStyle"
          @touchstart="onBoxTouchStart"
          @touchmove="onBoxTouchMove"
        >
          <view class="crop-border"></view>
          <!-- 四角拖拽手柄 -->
          <view
            class="handle tl"
            @touchstart.stop="onHandleStart('tl', $event)"
            @touchmove.stop="onHandleMove"
          ></view>
          <view
            class="handle tr"
            @touchstart.stop="onHandleStart('tr', $event)"
            @touchmove.stop="onHandleMove"
          ></view>
          <view
            class="handle bl"
            @touchstart.stop="onHandleStart('bl', $event)"
            @touchmove.stop="onHandleMove"
          ></view>
          <view
            class="handle br"
            @touchstart.stop="onHandleStart('br', $event)"
            @touchmove.stop="onHandleMove"
          ></view>
        </view>
        <!-- 遮罩 -->
        <view class="mask-top" :style="maskTopStyle"></view>
        <view class="mask-bottom" :style="maskBottomStyle"></view>
        <view class="mask-left" :style="maskLeftStyle"></view>
        <view class="mask-right" :style="maskRightStyle"></view>
      </view>
    </view>

    <!-- 底部按钮 -->
    <view class="cropper-footer">
      <view class="footer-btn cancel" @click="handleCancel">
        <text class="footer-btn-text">取消</text>
      </view>
      <view class="footer-btn confirm" @click="handleConfirm">
        <text class="footer-btn-text">确认裁剪</text>
      </view>
    </view>

    <!-- 隐藏canvas用于裁剪 -->
    <!-- #ifdef H5 -->
    <canvas
      id="cropCanvas"
      style="
        position: fixed;
        left: -9999px;
        top: -9999px;
        width: 1px;
        height: 1px;
      "
    ></canvas>
    <!-- #endif -->
    <!-- #ifdef MP-WEIXIN -->
    <canvas
      canvas-id="cropCanvas"
      id="cropCanvas"
      type="2d"
      style="
        position: fixed;
        left: -9999px;
        top: -9999px;
        width: 1px;
        height: 1px;
      "
    ></canvas>
    <!-- #endif -->
  </view>
</template>

<script>
export default {
  props: {
    visible: { type: Boolean, default: false },
    src: { type: String, default: "" },
  },
  data() {
    return {
      imgW: 0,
      imgH: 0, // 原图尺寸
      dispW: 0,
      dispH: 0, // 显示尺寸
      offsetX: 0,
      offsetY: 0, // 图片在容器中的偏移
      containerW: 375,
      containerH: 500,
      // 裁剪框（相对于容器）
      cropX: 20,
      cropY: 20,
      cropW: 200,
      cropH: 200,
      // 拖拽状态
      dragging: false,
      dragStartX: 0,
      dragStartY: 0,
      dragStartCropX: 0,
      dragStartCropY: 0,
      // 手柄拖拽
      activeHandle: null,
      handleStartX: 0,
      handleStartY: 0,
      handleStartCrop: {},
    };
  },
  computed: {
    imageStyle() {
      return {};
    },
    cropBoxStyle() {
      return {
        left: this.cropX + "px",
        top: this.cropY + "px",
        width: this.cropW + "px",
        height: this.cropH + "px",
      };
    },
    maskTopStyle() {
      return { left: "0", top: "0", right: "0", height: this.cropY + "px" };
    },
    maskBottomStyle() {
      const bottom = this.containerH - this.cropY - this.cropH;
      return {
        left: "0",
        bottom: "0",
        right: "0",
        height: Math.max(0, bottom) + "px",
      };
    },
    maskLeftStyle() {
      return {
        left: "0",
        top: this.cropY + "px",
        width: this.cropX + "px",
        height: this.cropH + "px",
      };
    },
    maskRightStyle() {
      const right = this.containerW - this.cropX - this.cropW;
      return {
        top: this.cropY + "px",
        right: "0",
        width: Math.max(0, right) + "px",
        height: this.cropH + "px",
      };
    },
  },
  methods: {
    onImageLoad() {
      const query = uni.createSelectorQuery().in(this);
      query
        .select(".image-container")
        .boundingClientRect((rect) => {
          if (!rect) return;
          this.containerW = rect.width;
          this.containerH = rect.height;
          // #ifdef H5
          const img = new Image();
          img.onload = () => this.initCropBox(img.width, img.height);
          img.src = this.src;
          // #endif
          // #ifdef MP-WEIXIN
          uni.getImageInfo({
            src: this.src,
            success: (info) => this.initCropBox(info.width, info.height),
          });
          // #endif
        })
        .exec();
    },
    initCropBox(imgWidth, imgHeight) {
      this.imgW = imgWidth;
      this.imgH = imgHeight;
      // 计算图片在容器中的显示尺寸（aspectFit）
      const cw = this.containerW;
      const ch = this.containerH;
      const scale = Math.min(cw / imgWidth, ch / imgHeight);
      this.dispW = imgWidth * scale;
      this.dispH = imgHeight * scale;
      this.offsetX = (cw - this.dispW) / 2;
      this.offsetY = (ch - this.dispH) / 2;
      // 默认裁剪框 = 整个图片显示区域，留10px边距
      const margin = 10;
      this.cropX = this.offsetX + margin;
      this.cropY = this.offsetY + margin;
      this.cropW = this.dispW - margin * 2;
      this.cropH = this.dispH - margin * 2;
    },
    onBoxTouchStart(e) {
      const t = e.touches[0];
      this.dragStartX = t.clientX;
      this.dragStartY = t.clientY;
      this.dragStartCropX = this.cropX;
      this.dragStartCropY = this.cropY;
    },
    onBoxTouchMove(e) {
      const t = e.touches[0];
      let nx = this.dragStartCropX + (t.clientX - this.dragStartX);
      let ny = this.dragStartCropY + (t.clientY - this.dragStartY);
      nx = Math.max(
        this.offsetX,
        Math.min(nx, this.offsetX + this.dispW - this.cropW),
      );
      ny = Math.max(
        this.offsetY,
        Math.min(ny, this.offsetY + this.dispH - this.cropH),
      );
      this.cropX = nx;
      this.cropY = ny;
    },
    onHandleStart(handle, e) {
      this.activeHandle = handle;
      const t = e.touches[0];
      this.handleStartX = t.clientX;
      this.handleStartY = t.clientY;
      this.handleStartCrop = {
        x: this.cropX,
        y: this.cropY,
        w: this.cropW,
        h: this.cropH,
      };
    },
    onHandleMove(e) {
      if (!this.activeHandle) return;
      const t = e.touches[0];
      const dx = t.clientX - this.handleStartX;
      const dy = t.clientY - this.handleStartY;
      const c = this.handleStartCrop;
      const minSize = 30;
      let nx = c.x,
        ny = c.y,
        nw = c.w,
        nh = c.h;
      if (this.activeHandle.includes("l")) {
        nx = c.x + dx;
        nw = c.w - dx;
      }
      if (this.activeHandle.includes("r")) {
        nw = c.w + dx;
      }
      if (this.activeHandle.includes("t")) {
        ny = c.y + dy;
        nh = c.h - dy;
      }
      if (this.activeHandle.includes("b")) {
        nh = c.h + dy;
      }
      if (nw < minSize) {
        nw = minSize;
        if (this.activeHandle.includes("l")) nx = c.x + c.w - minSize;
      }
      if (nh < minSize) {
        nh = minSize;
        if (this.activeHandle.includes("t")) ny = c.y + c.h - minSize;
      }
      nx = Math.max(this.offsetX, nx);
      ny = Math.max(this.offsetY, ny);
      if (nx + nw > this.offsetX + this.dispW)
        nw = this.offsetX + this.dispW - nx;
      if (ny + nh > this.offsetY + this.dispH)
        nh = this.offsetY + this.dispH - ny;
      this.cropX = nx;
      this.cropY = ny;
      this.cropW = nw;
      this.cropH = nh;
    },
    handleCancel() {
      this.$emit("cancel");
    },
    handleConfirm() {
      const scaleX = this.imgW / this.dispW;
      const scaleY = this.imgH / this.dispH;
      const sx = Math.round((this.cropX - this.offsetX) * scaleX);
      const sy = Math.round((this.cropY - this.offsetY) * scaleY);
      const sw = Math.round(this.cropW * scaleX);
      const sh = Math.round(this.cropH * scaleY);
      this.$emit("confirm", { sx, sy, sw, sh, src: this.src });
    },
  },
  watch: {
    visible() {
      // 裁剪框在 onImageLoad 中初始化
    },
  },
};
</script>

<style scoped>
.cropper-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: #fff;
  z-index: 9999;
  display: flex;
  flex-direction: column;
  padding-top: var(--status-bar-height, 44px);
}
.cropper-title-bar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
}
.cropper-title {
  color: #333;
  font-size: 32rpx;
  font-weight: 600;
}
.cropper-body {
  flex: 1;
  position: relative;
  overflow: hidden;
  background: #f5f5f5;
}
.cropper-footer {
  display: flex;
  padding: 24rpx 32rpx;
  padding-bottom: calc(24rpx + env(safe-area-inset-bottom));
  gap: 24rpx;
}
.footer-btn {
  flex: 1;
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
}
.footer-btn.cancel {
  background: #f0f0f0;
}
.footer-btn.confirm {
  background: #4f7fff;
}
.footer-btn.cancel .footer-btn-text {
  color: #666;
  font-size: 30rpx;
}
.footer-btn.confirm .footer-btn-text {
  color: #fff;
  font-size: 30rpx;
  font-weight: 600;
}
.image-container {
  width: 100%;
  height: 100%;
  position: relative;
}
.crop-image {
  width: 100%;
  height: 100%;
}
.crop-box {
  position: absolute;
  z-index: 10;
}
.crop-border {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  border: 2rpx solid rgba(0, 120, 255, 0.8);
  box-sizing: border-box;
}
.handle {
  position: absolute;
  width: 32rpx;
  height: 32rpx;
  z-index: 11;
}
.handle::after {
  content: "";
  position: absolute;
  background: #4f7fff;
}
.handle.tl {
  top: -4rpx;
  left: -4rpx;
}
.handle.tl::after {
  top: 0;
  left: 0;
  width: 16rpx;
  height: 3rpx;
}
.handle.tr {
  top: -4rpx;
  right: -4rpx;
}
.handle.tr::after {
  top: 0;
  right: 0;
  width: 16rpx;
  height: 3rpx;
}
.handle.bl {
  bottom: -4rpx;
  left: -4rpx;
}
.handle.bl::after {
  bottom: 0;
  left: 0;
  width: 16rpx;
  height: 3rpx;
}
.handle.br {
  bottom: -4rpx;
  right: -4rpx;
}
.handle.br::after {
  bottom: 0;
  right: 0;
  width: 16rpx;
  height: 3rpx;
}
.mask-top,
.mask-bottom,
.mask-left,
.mask-right {
  position: absolute;
  background: rgba(255, 255, 255, 0.5);
  z-index: 5;
}
</style>
