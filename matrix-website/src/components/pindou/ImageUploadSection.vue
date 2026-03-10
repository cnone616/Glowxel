<template>
  <div class="image-upload-section">
    <h3 class="section-title">1. 图片来源</h3>
    
    <!-- 上传区域 -->
    <div v-if="!imageUrl" class="upload-area" @click="handleUpload" @drop="handleDrop" @dragover.prevent @dragenter.prevent>
      <div class="upload-icon">上传图片</div>
      <p class="upload-text">点击上传图片</p>
      <span class="upload-hint">支持 JPG、PNG、WEBP</span>
      <input 
        ref="fileInput"
        type="file"
        accept="image/jpeg,image/png,image/webp"
        style="display: none"
        @change="handleFileChange"
      />
    </div>
    
    <!-- 预览区域 -->
    <div v-else class="preview-area">
      <img :src="imageUrl" alt="Preview" class="preview-image" />
      <div class="preview-actions">
        <button class="btn-secondary" @click="handleUpload">更换图片</button>
        <button class="btn-danger" @click="clearImage">删除</button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { imageToPixels } from '@/utils/imageProcessor.js'
import { ARTKAL_OFFICIAL_SETS } from '@/data/artkal-colors'

const props = defineProps({
  imageUrl: String,
  imageFile: File
})

const emit = defineEmits(['update:imageUrl', 'update:imageFile', 'image-processed'])

const fileInput = ref(null)

function handleUpload() {
  fileInput.value?.click()
}

function handleFileChange(e) {
  const file = e.target.files[0]
  if (!file) return
  
  processFile(file)
}

function handleDrop(e) {
  e.preventDefault()
  const file = e.dataTransfer.files[0]
  if (!file || !file.type.startsWith('image/')) return
  
  processFile(file)
}

function processFile(file) {
  const reader = new FileReader()
  reader.onload = (event) => {
    const url = event.target.result
    emit('update:imageUrl', url)
    emit('update:imageFile', file)
    
    // 处理图片转换为拼豆像素
    processImageToPixels(url)
  }
  reader.readAsDataURL(file)
}

function processImageToPixels(url) {
  const img = new Image()
  img.onload = () => {
    // 使用默认的中等难度尺寸 50x50
    const colorCodes = ARTKAL_OFFICIAL_SETS.set221?.colors || []
    const pixels = imageToPixels(img, 50, 50, colorCodes)
    emit('image-processed', pixels)
  }
  img.src = url
}

function clearImage() {
  emit('update:imageUrl', null)
  emit('update:imageFile', null)
  if (fileInput.value) {
    fileInput.value.value = ''
  }
}
</script>

<style scoped>
.image-upload-section {
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border-radius: 20px;
  border: 2px solid rgba(255, 107, 107, 0.15);
  padding: 24px;
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.1);
}

.section-title {
  font-size: 18px;
  font-weight: 700;
  color: #2d3748;
  margin-bottom: 20px;
}

.upload-area {
  padding: 40px 20px;
  border: 3px dashed rgba(255, 107, 107, 0.3);
  border-radius: 16px;
  text-align: center;
  cursor: pointer;
  transition: all 0.3s ease;
  background: rgba(255, 255, 255, 0.5);
}

.upload-area:hover {
  border-color: #ff6b6b;
  background: rgba(255, 255, 255, 0.8);
  transform: translateY(-2px);
}

.upload-icon {
  width: 80px;
  height: 80px;
  margin: 0 auto 16px;
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 14px;
  color: #fff;
  font-weight: 700;
  box-shadow: 0 6px 20px rgba(255, 107, 107, 0.3);
}

.upload-text {
  font-size: 16px;
  font-weight: 600;
  color: #2d3748;
  margin-bottom: 8px;
}

.upload-hint {
  font-size: 14px;
  color: #718096;
}

.preview-area {
  text-align: center;
}

.preview-image {
  max-width: 100%;
  max-height: 200px;
  border-radius: 12px;
  margin-bottom: 16px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.1);
}

.preview-actions {
  display: flex;
  gap: 12px;
  justify-content: center;
}

.btn-secondary,
.btn-danger {
  padding: 10px 20px;
  border: none;
  border-radius: 10px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.9);
  color: #ff6b6b;
  border: 2px solid rgba(255, 107, 107, 0.3);
}

.btn-secondary:hover {
  background: white;
  border-color: #ff6b6b;
  transform: translateY(-2px);
}

.btn-danger {
  background: rgba(255, 107, 107, 0.15);
  color: #ff6b6b;
  border: 2px solid rgba(255, 107, 107, 0.3);
}

.btn-danger:hover {
  background: rgba(255, 107, 107, 0.25);
  border-color: #ff6b6b;
  transform: translateY(-2px);
}
</style>