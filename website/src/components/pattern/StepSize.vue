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
  font-size: 32px;
  font-weight: 700;
  color: #2d3748;
  margin-bottom: 48px;
  text-align: center;
}

.section-title {
  font-size: 22px;
  font-weight: 700;
  color: #2d3748;
  margin-bottom: 28px;
}

.image-upload-section {
  margin-bottom: 40px;
}

.upload-area {
  padding: 60px 40px;
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border: 3px dashed rgba(255, 107, 107, 0.3);
  border-radius: 24px;
  text-align: center;
  cursor: pointer;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.08);
}

.upload-area:hover {
  background: rgba(255, 255, 255, 0.95);
  border-color: #ff6b6b;
  transform: translateY(-4px);
  box-shadow: 0 8px 32px rgba(255, 107, 107, 0.2);
}

.upload-icon {
  width: 90px;
  height: 90px;
  margin: 0 auto 24px;
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18px;
  color: #fff;
  font-weight: 700;
  box-shadow: 0 6px 20px rgba(255, 107, 107, 0.3);
}

.upload-area p {
  font-size: 18px;
  color: #2d3748;
  margin-bottom: 8px;
  font-weight: 600;
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
  padding: 12px 28px;
  border: none;
  border-radius: 12px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.9);
  color: #ff6b6b;
  border: 2px solid rgba(255, 107, 107, 0.3);
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.1);
}

.btn-secondary:hover {
  background: white;
  border-color: #ff6b6b;
  transform: translateY(-2px);
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.2);
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
  font-weight: 600;
  color: #4a5568;
}

.input-group input {
  width: 140px;
  padding: 14px 18px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.2);
  border-radius: 12px;
  color: #2d3748;
  font-size: 28px;
  font-weight: 700;
  text-align: center;
  font-family: 'SF Mono', 'Monaco', monospace;
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.08);
  transition: all 0.3s ease;
}

.input-group input:focus {
  outline: none;
  border-color: #ff6b6b;
  background: #fff5f5;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.2);
}

.separator {
  font-size: 36px;
  color: #cbd5e0;
  margin-top: 28px;
  font-weight: 300;
}

.preset-sizes {
  margin-bottom: 32px;
}

.preset-sizes h4 {
  font-size: 16px;
  font-weight: 600;
  color: #4a5568;
  margin-bottom: 20px;
}

.size-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(110px, 1fr));
  gap: 12px;
}

.size-btn {
  padding: 14px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 12px;
  color: #4a5568;
  font-size: 15px;
  font-family: 'SF Mono', 'Monaco', monospace;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.08);
}

.size-btn:hover {
  background: #fff5f5;
  border-color: rgba(255, 107, 107, 0.3);
  transform: translateY(-2px);
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.15);
}

.size-btn.active {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-color: #ff6b6b;
  color: #fff;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.3);
}

.stats {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 20px;
  padding: 28px;
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border-radius: 20px;
  border: 2px solid rgba(255, 107, 107, 0.15);
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.1);
}

.stat-item {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.stat-label {
  font-size: 14px;
  font-weight: 600;
  color: #718096;
}

.stat-value {
  font-size: 22px;
  font-weight: 700;
  color: #ff6b6b;
  font-family: 'SF Mono', 'Monaco', monospace;
}
</style>

@media (max-width: 768px) {
  .step-size {
    padding: 24px 0;
  }
  
  .step-title {
    font-size: 28px;
    margin-bottom: 32px;
  }
  
  .section-title {
    font-size: 20px;
    margin-bottom: 20px;
  }
  
  .upload-area {
    padding: 40px 24px;
  }
  
  .upload-icon {
    width: 70px;
    height: 70px;
    font-size: 16px;
  }
  
  .size-inputs {
    flex-direction: column;
    gap: 16px;
  }
  
  .input-group {
    width: 100%;
  }
  
  .input-group input {
    width: 100%;
  }
  
  .separator {
    transform: rotate(90deg);
    margin: 0;
  }
  
  .size-grid {
    grid-template-columns: repeat(auto-fill, minmax(90px, 1fr));
    gap: 10px;
  }
  
  .size-btn {
    padding: 12px;
    font-size: 14px;
  }
  
  .stats {
    grid-template-columns: 1fr;
    gap: 16px;
    padding: 20px;
  }
}
