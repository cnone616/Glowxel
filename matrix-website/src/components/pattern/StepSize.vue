<template>
  <div class="step-size">
    <h2 class="step-title">设置画布尺寸</h2>
    
    <!-- 图片上传（仅图片模式） -->
    <div v-if="mode === 'image'" class="image-upload-section">
      <div v-if="!imageUrl" class="upload-area" @click="handleUpload">
        <div class="upload-icon">上传</div>
        <p>点击上传图片</p>
        <span class="upload-hint">支持 JPG, PNG</span>
      </div>
      
      <div v-else class="preview-area">
        <img :src="imageUrl" alt="Preview" class="preview-image" />
        <div class="preview-actions">
          <button class="btn-secondary" @click="handleUpload">更换图片</button>
          <button class="btn-danger" @click="clearImage">删除</button>
        </div>
      </div>
      
      <input 
        ref="fileInput"
        type="file"
        accept="image/jpeg,image/png"
        style="display: none"
        @change="handleFileChange"
      />
    </div>
    
    <!-- 尺寸设置 -->
    <div class="size-section">
      <h3 class="section-title">{{ mode === 'image' ? '目标尺寸（像素化）' : '画布尺寸' }}</h3>
      
      <!-- 自定义尺寸 -->
      <div class="custom-size">
        <div class="size-inputs">
          <div class="input-group">
            <label>宽度</label>
            <input 
              type="number"
              :value="width"
              @input="$emit('update:width', Number($event.target.value))"
              min="1"
              max="520"
              placeholder="52"
            />
          </div>
          <span class="separator">×</span>
          <div class="input-group">
            <label>高度</label>
            <input 
              type="number"
              :value="height"
              @input="$emit('update:height', Number($event.target.value))"
              min="1"
              max="520"
              placeholder="52"
            />
          </div>
        </div>
      </div>
      
      <!-- 快捷尺寸 -->
      <div class="preset-sizes">
        <h4>快捷尺寸</h4>
        <div class="size-grid">
          <button
            v-for="size in presetSizes"
            :key="size"
            class="size-btn"
            :class="{ active: width === size && height === size }"
            @click="selectSize(size)"
          >
            {{ size }}×{{ size }}
          </button>
        </div>
      </div>
      
      <!-- 统计信息 -->
      <div v-if="width > 0 && height > 0" class="stats">
        <div class="stat-item">
          <span class="stat-label">总像素</span>
          <span class="stat-value">{{ (width * height).toLocaleString() }}</span>
        </div>
        <div class="stat-item">
          <span class="stat-label">板子数量</span>
          <span class="stat-value">{{ boardsX }}×{{ boardsY }} ({{ totalBoards }}块)</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const props = defineProps({
  mode: String,
  width: Number,
  height: Number,
  imageUrl: String,
  imageFile: File
})

const emit = defineEmits(['update:width', 'update:height', 'update:imageUrl', 'update:imageFile'])

const fileInput = ref(null)
const presetSizes = [52, 104, 156, 208, 260, 312, 364, 416, 468, 520]

const boardsX = computed(() => Math.ceil(props.width / 52))
const boardsY = computed(() => Math.ceil(props.height / 52))
const totalBoards = computed(() => boardsX.value * boardsY.value)

function handleUpload() {
  fileInput.value?.click()
}

function handleFileChange(e) {
  const file = e.target.files[0]
  if (!file) return
  
  const reader = new FileReader()
  reader.onload = (event) => {
    emit('update:imageUrl', event.target.result)
    emit('update:imageFile', file)
  }
  reader.readAsDataURL(file)
}

function clearImage() {
  emit('update:imageUrl', null)
  emit('update:imageFile', null)
  if (fileInput.value) {
    fileInput.value.value = ''
  }
}

function selectSize(size) {
  emit('update:width', size)
  emit('update:height', size)
}
</script>

<style scoped>
.step-size {
  padding: 40px 0;
  max-width: 800px;
  margin: 0 auto;
}

.step-title {
  font-size: 28px;
  color: #fff;
  margin-bottom: 40px;
  text-align: center;
}

.section-title {
  font-size: 20px;
  color: #fff;
  margin-bottom: 24px;
}

.image-upload-section {
  margin-bottom: 40px;
}

.upload-area {
  padding: 60px 40px;
  background: rgba(255, 255, 255, 0.05);
  border: 2px dashed rgba(255, 255, 255, 0.2);
  border-radius: 16px;
  text-align: center;
  cursor: pointer;
  transition: all 0.3s;
}

.upload-area:hover {
  background: rgba(255, 255, 255, 0.08);
  border-color: #00f3ff;
}

.upload-icon {
  width: 80px;
  height: 80px;
  margin: 0 auto 20px;
  background: rgba(0, 243, 255, 0.1);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 16px;
  color: #00f3ff;
}

.upload-area p {
  font-size: 18px;
  color: #fff;
  margin-bottom: 8px;
}

.upload-hint {
  font-size: 14px;
  color: rgba(255, 255, 255, 0.5);
}

.preview-area {
  text-align: center;
}

.preview-image {
  max-width: 100%;
  max-height: 400px;
  border-radius: 12px;
  margin-bottom: 20px;
}

.preview-actions {
  display: flex;
  gap: 12px;
  justify-content: center;
}

.btn-secondary, .btn-danger {
  padding: 12px 24px;
  border: none;
  border-radius: 8px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.1);
  color: #fff;
}

.btn-secondary:hover {
  background: rgba(255, 255, 255, 0.15);
}

.btn-danger {
  background: rgba(255, 51, 51, 0.2);
  color: #ff3333;
}

.btn-danger:hover {
  background: rgba(255, 51, 51, 0.3);
}

.size-section {
  margin-top: 40px;
}

.custom-size {
  margin-bottom: 32px;
}

.size-inputs {
  display: flex;
  align-items: center;
  gap: 20px;
  justify-content: center;
}

.input-group {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.input-group label {
  font-size: 14px;
  color: rgba(255, 255, 255, 0.7);
}

.input-group input {
  width: 120px;
  padding: 12px 16px;
  background: rgba(255, 255, 255, 0.05);
  border: 2px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  color: #fff;
  font-size: 24px;
  font-weight: bold;
  text-align: center;
  font-family: monospace;
}

.input-group input:focus {
  outline: none;
  border-color: #00f3ff;
  background: rgba(255, 255, 255, 0.08);
}

.separator {
  font-size: 32px;
  color: rgba(255, 255, 255, 0.5);
  margin-top: 24px;
}

.preset-sizes {
  margin-bottom: 32px;
}

.preset-sizes h4 {
  font-size: 16px;
  color: rgba(255, 255, 255, 0.7);
  margin-bottom: 16px;
}

.size-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(100px, 1fr));
  gap: 12px;
}

.size-btn {
  padding: 12px;
  background: rgba(255, 255, 255, 0.05);
  border: 2px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
  font-family: monospace;
  cursor: pointer;
  transition: all 0.3s;
}

.size-btn:hover {
  background: rgba(255, 255, 255, 0.08);
  border-color: rgba(0, 243, 255, 0.3);
}

.size-btn.active {
  background: rgba(0, 243, 255, 0.1);
  border-color: #00f3ff;
  color: #00f3ff;
}

.stats {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
  padding: 24px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 12px;
}

.stat-item {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.stat-label {
  font-size: 14px;
  color: rgba(255, 255, 255, 0.6);
}

.stat-value {
  font-size: 20px;
  font-weight: bold;
  color: #00f3ff;
  font-family: monospace;
}
</style>
