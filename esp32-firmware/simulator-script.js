const canvas = document.getElementById('ledCanvas');
const ctx = canvas.getContext('2d');
const PANEL_WIDTH = 64;
const PANEL_HEIGHT = 64;
const PIXEL_SIZE = 10;

let uploadedImage = null;
let currentTab = 'time';

// GIF 动画相关变量
let gifFrames = [];
let currentFrameIndex = 0;
let gifAnimationId = null;
let isGifPlaying = false;
let gifSpeed = 1;
let isGifFile = false;
let savedAnimationPresets = [];
let selectedSavedPresetIndex = -1;

// 颜色预设（和小程序一致）
const presetColors = [
  { name: '青色', hex: '#64c8ff', rgb: [100, 200, 255] },
  { name: '绿色', hex: '#00ff9d', rgb: [0, 255, 157] },
  { name: '黄色', hex: '#ffdc00', rgb: [255, 220, 0] },
  { name: '橙色', hex: '#ffa500', rgb: [255, 165, 0] },
  { name: '红色', hex: '#ff6464', rgb: [255, 100, 100] },
  { name: '紫色', hex: '#c864ff', rgb: [200, 100, 255] },
  { name: '白色', hex: '#ffffff', rgb: [255, 255, 255] },
  { name: '灰色', hex: '#787878', rgb: [120, 120, 120] },
  { name: '深灰', hex: '#646464', rgb: [100, 100, 100] }
];

// 当前选中的颜色
let selectedColors = {
  time: '#64c8ff',
  date: '#787878',
  week: '#646464'
};

// 初始化颜色选择器
function initColorPickers() {
  ['time', 'date', 'week'].forEach(type => {
    const picker = document.getElementById(`${type}ColorPicker`);
    presetColors.forEach(color => {
      const item = document.createElement('div');
      item.className = 'color-item';
      item.style.backgroundColor = color.hex;
      item.title = color.name;
      if (color.hex === selectedColors[type]) {
        item.classList.add('active');
      }
      item.onclick = () => selectColor(type, color.hex);
      picker.appendChild(item);
    });
  });
}

// 选择颜色
function selectColor(type, hex) {
  selectedColors[type] = hex;
  const picker = document.getElementById(`${type}ColorPicker`);
  picker.querySelectorAll('.color-item').forEach(item => {
    item.classList.remove('active');
    if (item.style.backgroundColor === hexToRgbString(hex)) {
      item.classList.add('active');
    }
  });
  updateDisplay();
}

// 颜色转换
function hexToRgbString(hex) {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  if (!result) return 'rgb(255, 255, 255)';
  return `rgb(${parseInt(result[1], 16)}, ${parseInt(result[2], 16)}, ${parseInt(result[3], 16)})`;
}

function hexToRgb(hex) {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result ? [
    parseInt(result[1], 16),
    parseInt(result[2], 16),
    parseInt(result[3], 16)
  ] : [255, 255, 255];
}

// 5x7 点阵字体
const font5x7 = {
  '0': [0x3E, 0x51, 0x49, 0x45, 0x3E], '1': [0x00, 0x42, 0x7F, 0x40, 0x00],
  '2': [0x42, 0x61, 0x51, 0x49, 0x46], '3': [0x21, 0x41, 0x45, 0x4B, 0x31],
  '4': [0x18, 0x14, 0x12, 0x7F, 0x10], '5': [0x27, 0x45, 0x45, 0x45, 0x39],
  '6': [0x3C, 0x4A, 0x49, 0x49, 0x30], '7': [0x01, 0x71, 0x09, 0x05, 0x03],
  '8': [0x36, 0x49, 0x49, 0x49, 0x36], '9': [0x06, 0x49, 0x49, 0x29, 0x1E],
  ':': [0x00, 0x36, 0x36, 0x00, 0x00], '-': [0x08, 0x08, 0x08, 0x08, 0x08],
  'S': [0x32, 0x49, 0x49, 0x49, 0x26], 'u': [0x3C, 0x40, 0x40, 0x20, 0x7C],
  'n': [0x7C, 0x08, 0x04, 0x04, 0x78], 'M': [0x7F, 0x02, 0x0C, 0x02, 0x7F],
  'o': [0x38, 0x44, 0x44, 0x44, 0x38], 'T': [0x01, 0x01, 0x7F, 0x01, 0x01],
  'W': [0x7F, 0x20, 0x18, 0x20, 0x7F], 'e': [0x38, 0x54, 0x54, 0x54, 0x18],
  'h': [0x7F, 0x08, 0x04, 0x04, 0x78], 'F': [0x7F, 0x09, 0x09, 0x09, 0x01],
  'r': [0x7C, 0x08, 0x04, 0x04, 0x08], 'i': [0x00, 0x44, 0x7D, 0x40, 0x00],
  'a': [0x20, 0x54, 0x54, 0x54, 0x78], 't': [0x04, 0x3F, 0x44, 0x40, 0x20]
};

// Tab 切换
function switchTab(tabName) {
  currentTab = tabName;
  document.querySelectorAll('.tab').forEach(tab => tab.classList.remove('active'));
  event.target.classList.add('active');
  document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
  document.getElementById(tabName + 'Tab').classList.add('active');
}

function parseNumberInputValue(inputId) {
  const input = document.getElementById(inputId);
  const value = parseInt(input.value, 10);
  if (Number.isNaN(value)) {
    return 0;
  }
  return value;
}

function setInputValueIfPresent(inputId, value) {
  if (value === undefined || value === null) {
    return;
  }
  const input = document.getElementById(inputId);
  if (input === null) {
    return;
  }
  input.value = value;
}

function setCheckedIfPresent(inputId, checked) {
  if (checked === undefined || checked === null) {
    return;
  }
  const input = document.getElementById(inputId);
  if (input === null) {
    return;
  }
  input.checked = checked;
}

function parseTimeInputParts(timeValue) {
  const parts = timeValue.split(':');
  let hour = 0;
  let minute = 0;
  let second = 0;

  if (parts.length >= 1) {
    const parsedHour = parseInt(parts[0], 10);
    if (!Number.isNaN(parsedHour)) {
      hour = parsedHour;
    }
  }
  if (parts.length >= 2) {
    const parsedMinute = parseInt(parts[1], 10);
    if (!Number.isNaN(parsedMinute)) {
      minute = parsedMinute;
    }
  }
  if (parts.length >= 3) {
    const parsedSecond = parseInt(parts[2], 10);
    if (!Number.isNaN(parsedSecond)) {
      second = parsedSecond;
    }
  }

  if (hour < 0) {
    hour = 0;
  } else if (hour > 23) {
    hour = 23;
  }

  if (minute < 0) {
    minute = 0;
  } else if (minute > 59) {
    minute = 59;
  }

  if (second < 0) {
    second = 0;
  } else if (second > 59) {
    second = 59;
  }

  return {
    hour: String(hour).padStart(2, '0'),
    minute: String(minute).padStart(2, '0'),
    second: String(second).padStart(2, '0')
  };
}

function readSplitTimeLayout() {
  return {
    hour: {
      show: document.getElementById('hourShow').checked,
      x: parseNumberInputValue('hourX'),
      y: parseNumberInputValue('hourY')
    },
    minute: {
      show: document.getElementById('minuteShow').checked,
      x: parseNumberInputValue('minuteX'),
      y: parseNumberInputValue('minuteY')
    },
    second: {
      show: document.getElementById('secondShow').checked,
      x: parseNumberInputValue('secondX'),
      y: parseNumberInputValue('secondY')
    },
    colon1: {
      show: document.getElementById('colon1Show').checked,
      x: parseNumberInputValue('colon1X'),
      y: parseNumberInputValue('colon1Y')
    },
    colon2: {
      show: document.getElementById('colon2Show').checked,
      x: parseNumberInputValue('colon2X'),
      y: parseNumberInputValue('colon2Y')
    }
  };
}

function buildTimeLayoutPayload() {
  const layout = readSplitTimeLayout();
  return {
    hour: {
      show: layout.hour.show,
      x: layout.hour.x,
      y: layout.hour.y
    },
    minute: {
      show: layout.minute.show,
      x: layout.minute.x,
      y: layout.minute.y
    },
    second: {
      show: layout.second.show,
      x: layout.second.x,
      y: layout.second.y
    },
    colon1: {
      show: layout.colon1.show,
      x: layout.colon1.x,
      y: layout.colon1.y
    },
    colon2: {
      show: layout.colon2.show,
      x: layout.colon2.x,
      y: layout.colon2.y
    },
    style: {
      font: document.getElementById('timeFont').value,
      fontSize: parseNumberInputValue('timeFontSize'),
      color: selectedColors.time
    }
  };
}

function drawSplitTimeLayout(timeLayout, timeParts, color, fontSize) {
  if (timeLayout.hour.show) {
    drawText(timeParts.hour, timeLayout.hour.x, timeLayout.hour.y, ...color, fontSize);
  }
  if (timeLayout.minute.show) {
    drawText(timeParts.minute, timeLayout.minute.x, timeLayout.minute.y, ...color, fontSize);
  }
  if (timeLayout.second.show) {
    drawText(timeParts.second, timeLayout.second.x, timeLayout.second.y, ...color, fontSize);
  }
  if (timeLayout.colon1.show) {
    drawText(':', timeLayout.colon1.x, timeLayout.colon1.y, ...color, fontSize);
  }
  if (timeLayout.colon2.show) {
    drawText(':', timeLayout.colon2.x, timeLayout.colon2.y, ...color, fontSize);
  }
}

function syncCurrentTimeInput(syncDateValue) {
  const now = new Date();
  const hours = String(now.getHours()).padStart(2, '0');
  const minutes = String(now.getMinutes()).padStart(2, '0');
  const seconds = String(now.getSeconds()).padStart(2, '0');
  document.getElementById('timeInput').value = `${hours}:${minutes}:${seconds}`;
  if (syncDateValue) {
    document.getElementById('dateInput').valueAsDate = now;
  }
}

function toggleSplitTimeMode() {
  const splitEnabled = document.getElementById('timeSplitMode').checked;
  document.getElementById('timeSplitPanel').style.display = splitEnabled ? 'block' : 'none';
  document.getElementById('timeLegacyPanel').style.display = splitEnabled ? 'none' : 'block';
}

function handleRealtimeToggle() {
  if (document.getElementById('timeRealtime').checked) {
    syncCurrentTimeInput(false);
    updateDisplay();
  }
}

// 图片处理
function handleImageUpload(event) {
  const file = event.target.files[0];
  if (!file) return;
  
  console.log('上传文件:', file.name, file.type, file.size, 'bytes');
  
  // 检查是否为 GIF 文件
  isGifFile = file.type === 'image/gif';
  
  if (isGifFile) {
    console.log('检测到 GIF 文件，开始解析...');
    handleGifUpload(file);
  } else {
    console.log('检测到静态图片文件');
    handleStaticImageUpload(file);
  }
}

// 处理静态图片上传
function handleStaticImageUpload(file) {
  const reader = new FileReader();
  reader.onload = function(e) {
    const img = new Image();
    img.onload = function() {
      uploadedImage = img;
      gifFrames = [];
      isGifFile = false;
      showImagePreview(e.target.result);
      hideGifControls();
      updateDisplay();
    };
    img.src = e.target.result;
  };
  reader.readAsDataURL(file);
}

// 处理 GIF 上传
async function handleGifUpload(file) {
  console.log('开始解析 GIF 文件...');
  
  try {
    const arrayBuffer = await file.arrayBuffer();
    const parser = new GIFParser();
    const gifData = await parser.parse(arrayBuffer);
    
    console.log('GIF 解析成功:', gifData);
    
    // 处理所有帧为 Canvas
    console.log('开始处理所有帧...');
    const canvases = parser.processAllFrames();
    console.log('帧处理完成:', canvases.length, '帧');
    
    // 生成 ESP32 优化的动画数据
    console.log('开始生成差分数据...');
    const esp32Data = parser.generateESP32AnimationData();
    console.log('差分数据生成完成:', esp32Data);
    
    // 验证 ESP32 数据的正确性（暂时跳过验证，避免错误）
    console.log('跳过 ESP32 数据验证');
    // const validation = parser.validateESP32Data(esp32Data.esp32Format);
    // console.log('验证完成:', validation);
    
    // 转换为我们需要的格式
    gifFrames = [];
    for (let i = 0; i < canvases.length; i++) {
      const frame = gifData.frames[i];
      const frameData = esp32Data.frames[i];
      
      gifFrames.push({
        canvas: canvases[i],
        delay: Math.max(50, frame.delay || 100), // 最小 50ms 延迟
        index: i,
        width: gifData.width,
        height: gifData.height,
        // ESP32 优化数据
        esp32Data: frameData
      });
    }
    
    // 保存完整的 ESP32 动画数据
    window.currentGifESP32Data = esp32Data;
    
    if (gifFrames.length > 0) {
      currentFrameIndex = 0;
      uploadedImage = gifFrames[0].canvas;
      
      // 显示预览（使用第一帧）
      showImagePreview(gifFrames[0].canvas.toDataURL());
      showGifControls();
      updateGifFrameInfo();
      updateDisplay();
      
      console.log(`GIF 加载完成: ${gifFrames.length} 帧, 尺寸: ${gifData.width}x${gifData.height}`);
      
      // 检查数据大小
      const esp32Format = esp32Data.esp32Format;
      const testJsonStr = JSON.stringify(esp32Format);
      const sizeKB = (testJsonStr.length / 1024).toFixed(1);
      
      // 显示优化统计信息
      const stats = esp32Data.statistics;
      let message = `GIF 解析成功！
帧数: ${gifFrames.length} → ${esp32Format.f} (优化后)
尺寸: ${gifData.width}x${gifData.height}
数据大小: ${sizeKB} KB

ESP32 优化统计:
• 全帧: ${stats.fullFrames} 个
• 差分帧: ${stats.diffFrames} 个  
• 压缩比: ${stats.compressionRatio}:1
• 预估内存: ${stats.estimatedMemory.kb} KB

所有帧已预处理完成，可以播放动画了！`;

      // 数据大小警告
      if (testJsonStr.length > 100000) {
        message += `\n\n⚠️ 警告：数据过大 (${sizeKB} KB)
建议：
1. 使用更小的 GIF 文件
2. 减少帧数或分辨率
3. 可能导致 ESP32 内存不足`;
      } else if (testJsonStr.length > 50000) {
        message += `\n\n💡 提示：数据较大 (${sizeKB} KB)
ESP32 可能需要较长时间处理`;
      }
      
      alert(message);
    }
    
  } catch (error) {
    console.error('GIF 解析失败:', error);
    alert(`GIF 解析失败: ${error.message}\n请尝试其他 GIF 文件`);
    
    // 降级为静态图片处理
    handleStaticImageUpload(file);
  }
}

// 显示图片预览
function showImagePreview(src) {
  document.getElementById('uploadPlaceholder').style.display = 'none';
  document.getElementById('imagePreviewContainer').style.display = 'block';
  document.getElementById('imagePreview').src = src;
  document.getElementById('uploadArea').classList.add('has-image');
  document.getElementById('showImage').checked = true;
}

// 显示 GIF 控制面板
function showGifControls() {
  document.getElementById('gifControls').style.display = 'block';
  document.getElementById('copyESP32AnimBtn').style.display = 'inline-block';
  document.getElementById('copyCompressedAnimBtn').style.display = 'inline-block';
  document.getElementById('savePresetBtn').style.display = 'inline-block';
}

// 隐藏 GIF 控制面板
function hideGifControls() {
  document.getElementById('gifControls').style.display = 'none';
  document.getElementById('copyESP32AnimBtn').style.display = 'none';
  document.getElementById('copyCompressedAnimBtn').style.display = 'none';
  document.getElementById('savePresetBtn').style.display = 'none';
  stopGif();
}

// 更新 GIF 帧信息显示
function updateGifFrameInfo() {
  document.getElementById('currentFrame').textContent = currentFrameIndex + 1;
  document.getElementById('totalFrames').textContent = gifFrames.length;
}

function removeImage() {
  uploadedImage = null;
  gifFrames = [];
  isGifFile = false;
  stopGif();
  document.getElementById('uploadPlaceholder').style.display = 'block';
  document.getElementById('imagePreviewContainer').style.display = 'none';
  document.getElementById('uploadArea').classList.remove('has-image');
  document.getElementById('showImage').checked = false;
  document.getElementById('imageInput').value = '';
  hideGifControls();
  updateDisplay();
}

function setSquareSize() {
  const width = parseInt(document.getElementById('imageWidth').value);
  const height = parseInt(document.getElementById('imageHeight').value);
  const size = Math.min(width, height, 64);
  document.getElementById('imageWidth').value = size;
  document.getElementById('imageHeight').value = size;
  checkImageSize('width', size);
  checkImageSize('height', size);
  alert(`已设置为 ${size}x${size}`);
  updateDisplay();
}

function checkImageSize(dimension, value) {
  const val = parseInt(value);
  const input = document.getElementById(`image${dimension.charAt(0).toUpperCase() + dimension.slice(1)}`);
  const warning = document.getElementById(`${dimension}Warning`);
  if (val > 64) {
    input.classList.add('warning');
    warning.style.display = 'flex';
  } else {
    input.classList.remove('warning');
    warning.style.display = 'none';
  }
}

// 加减按钮调整数值
function adjustValue(inputId, delta) {
  const input = document.getElementById(inputId);
  const currentValue = parseInt(input.value) || 0;
  const min = parseInt(input.min) || -Infinity;
  const max = parseInt(input.max) || Infinity;
  const newValue = Math.max(min, Math.min(max, currentValue + delta));
  input.value = newValue;
  
  // 触发相应的检查和更新
  if (inputId.includes('image')) {
    if (inputId === 'imageWidth') {
      checkImageSize('width', newValue);
    } else if (inputId === 'imageHeight') {
      checkImageSize('height', newValue);
    }
  }
  
  updateDisplay();
}

// GIF 动画控制函数
function playGif() {
  if (!isGifFile || gifFrames.length === 0) return;
  
  isGifPlaying = true;
  document.getElementById('playBtn').style.display = 'none';
  document.getElementById('pauseBtn').style.display = 'inline-block';
  
  function nextFrame() {
    if (!isGifPlaying) return;
    
    const frame = gifFrames[currentFrameIndex];
    uploadedImage = frame.canvas;
    updateDisplay();
    updateGifFrameInfo();
    
    currentFrameIndex = (currentFrameIndex + 1) % gifFrames.length;
    
    // 根据当前帧的延迟和播放速度设置下一帧时间
    const delay = Math.max(50, frame.delay / gifSpeed); // 最小 50ms 延迟
    gifAnimationId = setTimeout(nextFrame, delay);
  }
  
  nextFrame();
}

function pauseGif() {
  isGifPlaying = false;
  if (gifAnimationId) {
    clearTimeout(gifAnimationId);
    gifAnimationId = null;
  }
  document.getElementById('playBtn').style.display = 'inline-block';
  document.getElementById('pauseBtn').style.display = 'none';
}

function stopGif() {
  pauseGif();
  if (gifFrames.length > 0) {
    currentFrameIndex = 0;
    uploadedImage = gifFrames[0].canvas;
    updateDisplay();
    updateGifFrameInfo();
  }
}

function prevFrame() {
  if (!isGifFile || gifFrames.length === 0) return;
  
  pauseGif();
  currentFrameIndex = (currentFrameIndex - 1 + gifFrames.length) % gifFrames.length;
  uploadedImage = gifFrames[currentFrameIndex].canvas;
  updateDisplay();
  updateGifFrameInfo();
}

function nextFrame() {
  if (!isGifFile || gifFrames.length === 0) return;
  
  pauseGif();
  currentFrameIndex = (currentFrameIndex + 1) % gifFrames.length;
  uploadedImage = gifFrames[currentFrameIndex].canvas;
  updateDisplay();
  updateGifFrameInfo();
}

function adjustGifSpeed(delta) {
  const input = document.getElementById('gifSpeed');
  const currentValue = parseFloat(input.value) || 1;
  const min = parseFloat(input.min) || 0.1;
  const max = parseFloat(input.max) || 3;
  const newValue = Math.max(min, Math.min(max, Math.round((currentValue + delta) * 10) / 10));
  input.value = newValue;
  updateGifSpeed();
}

function updateGifSpeed() {
  gifSpeed = parseFloat(document.getElementById('gifSpeed').value) || 1;
}

function buildAdjustedFramePixels(frameCanvas) {
  const width = parseInt(document.getElementById('imageWidth').value);
  const height = parseInt(document.getElementById('imageHeight').value);
  const offsetX = parseInt(document.getElementById('imageX').value);
  const offsetY = parseInt(document.getElementById('imageY').value);

  const tempCanvas = document.createElement('canvas');
  tempCanvas.width = PANEL_WIDTH;
  tempCanvas.height = PANEL_HEIGHT;
  const tempCtx = tempCanvas.getContext('2d');
  tempCtx.fillStyle = '#000000';
  tempCtx.fillRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
  tempCtx.imageSmoothingEnabled = false;
  tempCtx.drawImage(frameCanvas, offsetX, offsetY, width, height);

  const imageData = tempCtx.getImageData(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
  return imageData.data;
}

function rgbaToCompactPixels(rgbaData) {
  const pixels = [];
  for (let y = 0; y < PANEL_HEIGHT; y++) {
    for (let x = 0; x < PANEL_WIDTH; x++) {
      const index = (y * PANEL_WIDTH + x) * 4;
      const r = rgbaData[index];
      const g = rgbaData[index + 1];
      const b = rgbaData[index + 2];
      if (r > 0 || g > 0 || b > 0) {
        pixels.push([x, y, r, g, b]);
      }
    }
  }
  return pixels;
}

function diffCompactPixels(currentRgba, previousRgba) {
  const pixels = [];
  for (let y = 0; y < PANEL_HEIGHT; y++) {
    for (let x = 0; x < PANEL_WIDTH; x++) {
      const index = (y * PANEL_WIDTH + x) * 4;
      const currentR = currentRgba[index];
      const currentG = currentRgba[index + 1];
      const currentB = currentRgba[index + 2];
      if (
        currentR !== previousRgba[index] ||
        currentG !== previousRgba[index + 1] ||
        currentB !== previousRgba[index + 2]
      ) {
        pixels.push([x, y, currentR, currentG, currentB]);
      }
    }
  }
  return pixels;
}

function buildCompactAnimationData() {
  if (gifFrames.length === 0) {
    return null;
  }

  const maxFrameCount = Math.min(8, gifFrames.length);
  const compactFrames = [];
  let previousRgba = null;

  for (let i = 0; i < maxFrameCount; i++) {
    const frame = gifFrames[i];
    const adjustedRgba = buildAdjustedFramePixels(frame.canvas);
    const fullPixels = rgbaToCompactPixels(adjustedRgba);
    let compactFrame = {
      t: 1,
      d: Math.max(50, Math.round(frame.delay / gifSpeed)),
      c: fullPixels.length,
      p: fullPixels,
    };

    if (previousRgba !== null) {
      const diffPixels = diffCompactPixels(adjustedRgba, previousRgba);
      if (diffPixels.length <= fullPixels.length * 0.5) {
        compactFrame = {
          t: 0,
          d: Math.max(50, Math.round(frame.delay / gifSpeed)),
          c: diffPixels.length,
          p: diffPixels,
        };
      }
    }

    compactFrames.push(compactFrame);
    previousRgba = adjustedRgba;
  }

  return {
    f: compactFrames.length,
    d: compactFrames,
  };
}

function createCanvasFromCompactFrameData(compactFrame) {
  const frameCanvas = document.createElement('canvas');
  frameCanvas.width = PANEL_WIDTH;
  frameCanvas.height = PANEL_HEIGHT;
  const frameCtx = frameCanvas.getContext('2d');
  frameCtx.fillStyle = '#000000';
  frameCtx.fillRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT);

  const pixels = compactFrame.p;
  for (let i = 0; i < pixels.length; i++) {
    const pixel = pixels[i];
    frameCtx.fillStyle = `rgb(${pixel[2]}, ${pixel[3]}, ${pixel[4]})`;
    frameCtx.fillRect(pixel[0], pixel[1], 1, 1);
  }

  return frameCanvas;
}

function buildPreviewFramesFromCompactData(compactData) {
  const frames = [];
  const state = new Uint8ClampedArray(PANEL_WIDTH * PANEL_HEIGHT * 4);

  for (let i = 0; i < compactData.d.length; i++) {
    const compactFrame = compactData.d[i];

    if (compactFrame.t === 1) {
      state.fill(0);
    }

    for (let j = 0; j < compactFrame.p.length; j++) {
      const pixel = compactFrame.p[j];
      const x = pixel[0];
      const y = pixel[1];
      const index = (y * PANEL_WIDTH + x) * 4;
      state[index] = pixel[2];
      state[index + 1] = pixel[3];
      state[index + 2] = pixel[4];
      state[index + 3] = 255;
    }

    const frameCanvas = document.createElement('canvas');
    frameCanvas.width = PANEL_WIDTH;
    frameCanvas.height = PANEL_HEIGHT;
    const frameCtx = frameCanvas.getContext('2d');
    const frameImage = frameCtx.createImageData(PANEL_WIDTH, PANEL_HEIGHT);
    frameImage.data.set(state);
    frameCtx.putImageData(frameImage, 0, 0);

    frames.push({
      canvas: frameCanvas,
      delay: compactFrame.d,
      index: i,
      width: PANEL_WIDTH,
      height: PANEL_HEIGHT,
    });
  }

  return frames;
}

function estimateCompactDataMemory(compactData) {
  let pixelCount = 0;
  for (let i = 0; i < compactData.d.length; i++) {
    pixelCount += compactData.d[i].c;
  }
  return {
    kb: (pixelCount * 5 / 1024).toFixed(1),
  };
}

function applySavedAnimationPreset(compactData, presetIndex) {
  const previewFrames = buildPreviewFramesFromCompactData(compactData);
  gifFrames = previewFrames;
  currentFrameIndex = 0;
  isGifFile = true;
  selectedSavedPresetIndex = presetIndex;
  document.getElementById('showImage').checked = true;
  document.getElementById('imageWidth').value = 64;
  document.getElementById('imageHeight').value = 64;
  document.getElementById('imageX').value = 0;
  document.getElementById('imageY').value = 0;
  uploadedImage = previewFrames[0].canvas;
  window.currentGifESP32Data = {
    frameCount: compactData.f,
    statistics: {
      compressionRatio: 'N/A',
      estimatedMemory: estimateCompactDataMemory(compactData),
    },
    esp32Format: compactData,
  };

  showImagePreview(previewFrames[0].canvas.toDataURL());
  showGifControls();
  updateGifFrameInfo();
  renderSavedAnimationPresetList();
  updateDisplay();
}

async function saveAnimationPreset() {
  const compactData = buildCompactAnimationData();
  if (compactData === null) {
    alert('没有可保存的 GIF 动画数据');
    return;
  }

  try {
    const response = await fetch('/api/animation-presets/append', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(compactData),
    });

    const result = await response.json();
    if (!response.ok) {
      alert(`保存失败: ${result.error}`);
      return;
    }

    await loadSavedAnimationPresets();
    applySavedAnimationPreset(compactData, result.index);
    alert(`动画已追加到预设文件，第 ${result.index + 1} 条`);
  } catch (error) {
    alert('保存失败: 请使用 preset_server.py 打开页面');
  }
}

function renderSavedAnimationPresetList() {
  const list = document.getElementById('savedPresetList');
  list.innerHTML = '';

  if (savedAnimationPresets.length === 0) {
    list.innerHTML = '<div class="preset-empty">还没有保存的动画数据</div>';
    return;
  }

  for (let i = 0; i < savedAnimationPresets.length; i++) {
    const preset = savedAnimationPresets[i];
    const item = document.createElement('div');
    item.className = 'preset-item';
    if (i === selectedSavedPresetIndex) {
      item.classList.add('active');
    }
    item.onclick = () => applySavedAnimationPreset(preset, i);

    const thumb = document.createElement('canvas');
    thumb.className = 'preset-thumb';
    thumb.width = 64;
    thumb.height = 64;
    const thumbCtx = thumb.getContext('2d');
    const previewFrames = buildPreviewFramesFromCompactData(preset);
    thumbCtx.imageSmoothingEnabled = false;
    thumbCtx.drawImage(previewFrames[0].canvas, 0, 0, 64, 64);

    const meta = document.createElement('div');
    meta.className = 'preset-meta';
    const totalPixels = preset.d.reduce((sum, frame) => sum + frame.c, 0);
    meta.innerHTML = `
      <div class="preset-name">动画 ${i + 1}</div>
      <div class="preset-desc">帧数 ${preset.f} / 像素 ${totalPixels}</div>
    `;

    item.appendChild(thumb);
    item.appendChild(meta);
    list.appendChild(item);
  }
}

async function loadSavedAnimationPresets() {
  const list = document.getElementById('savedPresetList');
  try {
    const response = await fetch('/api/animation-presets');
    const result = await response.json();
    if (!response.ok) {
      list.innerHTML = `<div class="preset-error">读取失败: ${result.error}</div>`;
      return;
    }
    savedAnimationPresets = result;
    renderSavedAnimationPresetList();
  } catch (error) {
    list.innerHTML = '<div class="preset-error">请使用 preset_server.py 打开页面，当前无法读取预设文件</div>';
  }
}

// 绘制函数
function clearCanvas() {
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  
  ctx.strokeStyle = 'rgba(40, 40, 40, 0.5)';
  ctx.lineWidth = 0.5;
  for (let x = 0; x <= PANEL_WIDTH; x++) {
    ctx.beginPath();
    ctx.moveTo(x * PIXEL_SIZE + 0.5, 0);
    ctx.lineTo(x * PIXEL_SIZE + 0.5, PANEL_HEIGHT * PIXEL_SIZE);
    ctx.stroke();
  }
  for (let y = 0; y <= PANEL_HEIGHT; y++) {
    ctx.beginPath();
    ctx.moveTo(0, y * PIXEL_SIZE + 0.5);
    ctx.lineTo(PANEL_WIDTH * PIXEL_SIZE, y * PIXEL_SIZE + 0.5);
    ctx.stroke();
  }
  
  ctx.strokeStyle = 'rgba(0, 243, 255, 0.4)';
  ctx.lineWidth = 1;
  for (let x = 0; x <= PANEL_WIDTH; x += 8) {
    ctx.beginPath();
    ctx.moveTo(x * PIXEL_SIZE + 0.5, 0);
    ctx.lineTo(x * PIXEL_SIZE + 0.5, PANEL_HEIGHT * PIXEL_SIZE);
    ctx.stroke();
  }
  for (let y = 0; y <= PANEL_HEIGHT; y += 8) {
    ctx.beginPath();
    ctx.moveTo(0, y * PIXEL_SIZE + 0.5);
    ctx.lineTo(PANEL_WIDTH * PIXEL_SIZE, y * PIXEL_SIZE + 0.5);
    ctx.stroke();
  }
}

function drawPixel(x, y, r, g, b) {
  if (x < 0 || x >= PANEL_WIDTH || y < 0 || y >= PANEL_HEIGHT) return;
  ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
  ctx.fillRect(x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE);
}

function drawChar(char, x, y, r, g, b, size = 1) {
  const fontData = font5x7[char];
  if (!fontData) return 5 * size;
  for (let col = 0; col < 5; col++) {
    for (let row = 0; row < 8; row++) {
      if (fontData[col] & (1 << row)) {
        for (let sy = 0; sy < size; sy++) {
          for (let sx = 0; sx < size; sx++) {
            drawPixel(x + col * size + sx, y + row * size + sy, r, g, b);
          }
        }
      }
    }
  }
  return (5 + 1) * size;
}

function drawText(text, x, y, r, g, b, size = 1) {
  let offsetX = 0;
  for (let i = 0; i < text.length; i++) {
    offsetX += drawChar(text[i], x + offsetX, y, r, g, b, size);
  }
}

function drawImage(img, x, y, width, height) {
  if (!img) return null;
  const tempCanvas = document.createElement('canvas');
  tempCanvas.width = width;
  tempCanvas.height = height;
  const tempCtx = tempCanvas.getContext('2d');
  tempCtx.drawImage(img, 0, 0, width, height);
  const imageData = tempCtx.getImageData(0, 0, width, height);
  const pixels = imageData.data;
  
  // 保存像素数据用于导出
  const pixelArray = [];
  
  for (let py = 0; py < height; py++) {
    for (let px = 0; px < width; px++) {
      const idx = (py * width + px) * 4;
      const r = pixels[idx];
      const g = pixels[idx + 1];
      const b = pixels[idx + 2];
      const a = pixels[idx + 3];
      
      if (a > 128) {
        const finalX = x + px;
        const finalY = y + py;
        
        // 只有在有效范围内才绘制和保存
        if (finalX >= 0 && finalX < PANEL_WIDTH && finalY >= 0 && finalY < PANEL_HEIGHT) {
          drawPixel(finalX, finalY, r, g, b);
          // 保存像素数据
          pixelArray.push({
            x: finalX,
            y: finalY,
            r: r,
            g: g,
            b: b
          });
        }
      }
    }
  }
  
  return pixelArray;
}

// 全局变量保存当前图片的像素数据
let currentImagePixels = null;

// 更新显示
function updateDisplay() {
  clearCanvas();
  
  const config = {
    showImage: document.getElementById('showImage').checked,
    imageWidth: parseNumberInputValue('imageWidth'),
    imageHeight: parseNumberInputValue('imageHeight'),
    imageX: parseNumberInputValue('imageX'),
    imageY: parseNumberInputValue('imageY'),
    
    timeFont: document.getElementById('timeFont').value,
    timeFontSize: parseNumberInputValue('timeFontSize'),
    timeX: parseNumberInputValue('timeX'),
    timeY: parseNumberInputValue('timeY'),
    timeColor: hexToRgb(selectedColors.time),
    timeSplitMode: document.getElementById('timeSplitMode').checked,
    timeLayout: readSplitTimeLayout(),
    
    showDate: document.getElementById('showDate').checked,
    dateFontSize: parseNumberInputValue('dateFontSize'),
    dateX: parseNumberInputValue('dateX'),
    dateY: parseNumberInputValue('dateY'),
    dateColor: hexToRgb(selectedColors.date),
    
    showWeek: document.getElementById('showWeek').checked,
    weekX: parseNumberInputValue('weekX'),
    weekY: parseNumberInputValue('weekY'),
    weekColor: hexToRgb(selectedColors.week)
  };
  
  // 绘制图片并保存像素数据
  if (config.showImage && uploadedImage) {
    currentImagePixels = drawImage(uploadedImage, config.imageX, config.imageY, config.imageWidth, config.imageHeight);
  } else {
    currentImagePixels = null;
  }
  
  const timeInput = document.getElementById('timeInput').value;
  const timeParts = parseTimeInputParts(timeInput);
  if (config.timeSplitMode) {
    drawSplitTimeLayout(config.timeLayout, timeParts, config.timeColor, config.timeFontSize);
  } else {
    const timeOnly = `${timeParts.hour}:${timeParts.minute}`;
    drawText(timeOnly, config.timeX, config.timeY, ...config.timeColor, config.timeFontSize);
  }
  
  if (config.showDate) {
    const dateInput = document.getElementById('dateInput').value;
    const date = new Date(dateInput);
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    const shortDate = `${month}-${day}`;
    drawText(shortDate, config.dateX, config.dateY, ...config.dateColor, config.dateFontSize);
  }
  
  if (config.showWeek) {
    const dateInput = document.getElementById('dateInput').value;
    const date = new Date(dateInput);
    const weekDays = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
    const weekDay = weekDays[date.getDay()];
    drawText(weekDay, config.weekX, config.weekY, ...config.weekColor, 1);
  }
}

function showCurrentTime() {
  syncCurrentTimeInput(true);
  updateDisplay();
}

function saveConfig() {
  const timeLayout = buildTimeLayoutPayload();
  const config = {
    timeInput: document.getElementById('timeInput').value,
    timeFont: document.getElementById('timeFont').value,
    timeFontSize: parseNumberInputValue('timeFontSize'),
    timeX: parseNumberInputValue('timeX'),
    timeY: parseNumberInputValue('timeY'),
    timeColor: selectedColors.time,
    timeSplitMode: document.getElementById('timeSplitMode').checked,
    timeRealtime: document.getElementById('timeRealtime').checked,
    timeLayout: timeLayout,
    showDate: document.getElementById('showDate').checked,
    dateFontSize: parseNumberInputValue('dateFontSize'),
    dateX: parseNumberInputValue('dateX'),
    dateY: parseNumberInputValue('dateY'),
    dateColor: selectedColors.date,
    showWeek: document.getElementById('showWeek').checked,
    weekX: parseNumberInputValue('weekX'),
    weekY: parseNumberInputValue('weekY'),
    weekColor: selectedColors.week,
    showImage: document.getElementById('showImage').checked,
    imageWidth: parseNumberInputValue('imageWidth'),
    imageHeight: parseNumberInputValue('imageHeight'),
    imageX: parseNumberInputValue('imageX'),
    imageY: parseNumberInputValue('imageY'),
    imageData: uploadedImage ? document.getElementById('imagePreview').src : null,
    imagePixels: currentImagePixels // 保存像素数据数组
  };
  
  const imageInfo = currentImagePixels 
    ? `图片像素数据: ${currentImagePixels.length} 个像素点` 
    : '无图片数据';
  
  const configText = `时钟配置参数：
━━━━━━━━━━━━━━━━━━━━
时间: 字体${config.timeFont} 大小${config.timeFontSize} 颜色${config.timeColor}
时间布局模式: ${config.timeSplitMode ? '分离布局' : '整串布局'}
时间布局字段 timeLayout:
${JSON.stringify(config.timeLayout, null, 2)}
日期: ${config.showDate?'显示':'隐藏'} 字体${config.dateFontSize} X${config.dateX} Y${config.dateY} 颜色${config.dateColor}
星期: ${config.showWeek?'显示':'隐藏'} X${config.weekX} Y${config.weekY} 颜色${config.weekColor}
图片: ${config.showImage?'显示':'隐藏'} 尺寸${config.imageWidth}x${config.imageHeight} 位置(${config.imageX},${config.imageY})
      ${imageInfo}
━━━━━━━━━━━━━━━━━━━━

像素数据格式示例（前3个点）:
${currentImagePixels ? JSON.stringify(currentImagePixels.slice(0, 3), null, 2) : '无数据'}

提示：完整配置已保存到浏览器本地存储
可用于发送到 ESP32 设备控制 LED 灯`;
  
  document.getElementById('configOutput').textContent = configText;
  document.getElementById('configOutput').style.display = 'block';
  localStorage.setItem('ledClockConfig', JSON.stringify(config));
  
  // 同时保存为可下载的 JSON 文件
  console.log('完整配置数据:', config);
  console.log('图片像素数据数组:', currentImagePixels);
  
  alert('配置已保存！\n包含 ' + (currentImagePixels ? currentImagePixels.length : 0) + ' 个像素点数据');
}

// 复制完整 JSON 配置
function copyFullJSON() {
  const saved = localStorage.getItem('ledClockConfig');
  if (!saved) {
    alert('请先保存配置');
    return;
  }
  
  const config = JSON.parse(saved);
  
  // 如果有 GIF 数据，添加 ESP32 优化信息
  if (window.currentGifESP32Data) {
    config.gifAnimation = window.currentGifESP32Data;
  }
  
  const jsonStr = JSON.stringify(config, null, 2);
  
  navigator.clipboard.writeText(jsonStr).then(() => {
    const pixelCount = config.imagePixels ? config.imagePixels.length : 0;
    const gifInfo = window.currentGifESP32Data 
      ? `\n- GIF 动画数据 (${window.currentGifESP32Data.frameCount} 帧, ${window.currentGifESP32Data.statistics.estimatedMemory.kb} KB)`
      : '';
    
    alert(`✅ 完整配置已复制！\n\n包含:\n- 时间/日期/星期设置\n- 图片像素数据 (${pixelCount} 个点)${gifInfo}\n\n可以直接粘贴到小程序代码中使用`);
  }).catch(() => {
    const output = document.getElementById('configOutput');
    output.textContent = jsonStr;
    output.style.display = 'block';
    alert('自动复制失败，JSON 已显示在下方，请手动复制');
  });
}

// 复制 ESP32 差分动画数据（简单压缩版）
function copyESP32AnimationData() {
  const compactData = buildCompactAnimationData();
  if (compactData === null) {
    alert('没有 GIF 动画数据');
    return;
  }
  
  const jsonStr = JSON.stringify(compactData);
  const sizeKB = (jsonStr.length / 1024).toFixed(1);
  
  // 检查数据大小
  if (jsonStr.length > 300000) { // 提高到300KB限制
    alert(`数据过大 (${sizeKB} KB)\n\n建议:\n1. 使用更小的 GIF 文件\n2. 减少帧数\n3. 降低分辨率\n\n当前数据可能导致 ESP32 内存不足`);
    return;
  }

  navigator.clipboard.writeText(jsonStr).then(() => {
    const stats = window.currentGifESP32Data && window.currentGifESP32Data.statistics
      ? window.currentGifESP32Data.statistics
      : { compressionRatio: 'N/A', estimatedMemory: estimateCompactDataMemory(compactData) };
    const compressionRatioText = stats.compressionRatio === 'N/A'
      ? 'N/A'
      : `${stats.compressionRatio}:1`;
    const message = `ESP32 紧凑动画数据已复制

数据大小: ${sizeKB} KB
格式说明:
• f: 总帧数
• d[i].t: 1=全刷新, 0=差分更新  
• d[i].d: 帧延迟(毫秒)
• d[i].c: 像素数量
• d[i].p: 像素数组 [x,y,r,g,b]

优化统计:
• 帧数: ${compactData.f}
• 原始帧数: ${compactData.f}
• 压缩比: ${compressionRatioText}
• 内存: ${stats.estimatedMemory.kb} KB

ESP32 渲染逻辑:
1. t=1: 清屏后绘制所有像素
2. t=0: 只更新 p 数组中的像素
3. 使用 drawPixelRGB888(p[i][0], p[i][1], p[i][2], p[i][3], p[i][4])`;
    
    alert(message);
  }).catch(() => {
    const output = document.getElementById('configOutput');
    output.textContent = `ESP32 紧凑动画数据 (${sizeKB} KB)：\n\n${jsonStr}`;
    output.style.display = 'block';
    alert('自动复制失败，数据已显示在下方，请手动复制');
  });
}

// 复制像素数据（只复制像素数组，方便小程序使用）
function copyPixelData() {
  if (!currentImagePixels || currentImagePixels.length === 0) {
    alert('没有图片像素数据');
    return;
  }
  
  const pixelDataJson = JSON.stringify(currentImagePixels, null, 2);
  navigator.clipboard.writeText(pixelDataJson).then(() => {
    alert(`✅ 已复制 ${currentImagePixels.length} 个像素点！\n\n格式: [{x, y, r, g, b}, ...]\n可以直接发送到 ESP32`);
  }).catch(() => {
    const output = document.getElementById('configOutput');
    output.textContent = `像素数据数组（${currentImagePixels.length} 个点）：\n\n${pixelDataJson}`;
    output.style.display = 'block';
    alert('自动复制失败，数据已显示在下方，请手动复制');
  });
}

function copyConfig() {
  const output = document.getElementById('configOutput');
  if (output.style.display === 'none') {
    alert('请先点击"保存配置"按钮');
    return;
  }
  navigator.clipboard.writeText(output.textContent).then(() => {
    alert('配置文本已复制到剪贴板！');
  }).catch(() => {
    alert('复制失败，请手动复制');
  });
}

// 下载配置文件
function downloadConfig() {
  const saved = localStorage.getItem('ledClockConfig');
  if (!saved) {
    alert('请先保存配置');
    return;
  }
  
  const config = JSON.parse(saved);
  const blob = new Blob([JSON.stringify(config, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `led-clock-config-${Date.now()}.json`;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
  alert('配置文件已下载！\n可以导入到小程序使用');
}

// 初始化
document.getElementById('dateInput').valueAsDate = new Date();
initColorPickers();
loadSavedAnimationPresets();

// 加载保存的配置
function loadSavedConfig() {
  const saved = localStorage.getItem('ledClockConfig');
  if (saved) {
    try {
      const config = JSON.parse(saved);
      
      // 时间设置
      setInputValueIfPresent('timeInput', config.timeInput);
      setInputValueIfPresent('timeFont', config.timeFont);
      setInputValueIfPresent('timeFontSize', config.timeFontSize);
      setInputValueIfPresent('timeX', config.timeX);
      setInputValueIfPresent('timeY', config.timeY);
      setCheckedIfPresent('timeSplitMode', config.timeSplitMode);
      setCheckedIfPresent('timeRealtime', config.timeRealtime);
      if (config.timeColor) {
        selectedColors.time = config.timeColor;
        selectColor('time', config.timeColor);
      }
      if (config.timeLayout) {
        if (config.timeLayout.hour) {
          setCheckedIfPresent('hourShow', config.timeLayout.hour.show);
          setInputValueIfPresent('hourX', config.timeLayout.hour.x);
          setInputValueIfPresent('hourY', config.timeLayout.hour.y);
        }
        if (config.timeLayout.minute) {
          setCheckedIfPresent('minuteShow', config.timeLayout.minute.show);
          setInputValueIfPresent('minuteX', config.timeLayout.minute.x);
          setInputValueIfPresent('minuteY', config.timeLayout.minute.y);
        }
        if (config.timeLayout.second) {
          setCheckedIfPresent('secondShow', config.timeLayout.second.show);
          setInputValueIfPresent('secondX', config.timeLayout.second.x);
          setInputValueIfPresent('secondY', config.timeLayout.second.y);
        }
        if (config.timeLayout.colon1) {
          setCheckedIfPresent('colon1Show', config.timeLayout.colon1.show);
          setInputValueIfPresent('colon1X', config.timeLayout.colon1.x);
          setInputValueIfPresent('colon1Y', config.timeLayout.colon1.y);
        }
        if (config.timeLayout.colon2) {
          setCheckedIfPresent('colon2Show', config.timeLayout.colon2.show);
          setInputValueIfPresent('colon2X', config.timeLayout.colon2.x);
          setInputValueIfPresent('colon2Y', config.timeLayout.colon2.y);
        }
        if (config.timeLayout.style) {
          setInputValueIfPresent('timeFont', config.timeLayout.style.font);
          setInputValueIfPresent('timeFontSize', config.timeLayout.style.fontSize);
          if (config.timeLayout.style.color) {
            selectedColors.time = config.timeLayout.style.color;
            selectColor('time', config.timeLayout.style.color);
          }
        }
      }
      
      // 日期设置
      setCheckedIfPresent('showDate', config.showDate);
      setInputValueIfPresent('dateFontSize', config.dateFontSize);
      setInputValueIfPresent('dateX', config.dateX);
      setInputValueIfPresent('dateY', config.dateY);
      if (config.dateColor) {
        selectedColors.date = config.dateColor;
        selectColor('date', config.dateColor);
      }
      
      // 星期设置
      setCheckedIfPresent('showWeek', config.showWeek);
      setInputValueIfPresent('weekX', config.weekX);
      setInputValueIfPresent('weekY', config.weekY);
      if (config.weekColor) {
        selectedColors.week = config.weekColor;
        selectColor('week', config.weekColor);
      }
      
      // 图片设置
      setCheckedIfPresent('showImage', config.showImage);
      setInputValueIfPresent('imageWidth', config.imageWidth);
      setInputValueIfPresent('imageHeight', config.imageHeight);
      setInputValueIfPresent('imageX', config.imageX);
      setInputValueIfPresent('imageY', config.imageY);
      
      // 恢复图片
      if (config.imageData) {
        const img = new Image();
        img.onload = function() {
          uploadedImage = img;
          document.getElementById('uploadPlaceholder').style.display = 'none';
          document.getElementById('imagePreviewContainer').style.display = 'block';
          document.getElementById('imagePreview').src = config.imageData;
          document.getElementById('uploadArea').classList.add('has-image');
          updateDisplay();
        };
        img.src = config.imageData;
      }
      
      console.log('配置已加载');
    } catch (e) {
      console.error('加载配置失败:', e);
    }
  }
}

toggleSplitTimeMode();
loadSavedConfig();
toggleSplitTimeMode();
if (document.getElementById('timeRealtime').checked) {
  syncCurrentTimeInput(false);
}
updateDisplay();

setInterval(() => {
  const realtimeInput = document.getElementById('timeRealtime');
  if (realtimeInput && realtimeInput.checked) {
    syncCurrentTimeInput(false);
    updateDisplay();
  }
}, 1000);

// 页面加载完成后的初始化
document.addEventListener('DOMContentLoaded', function() {
  console.log('LED 模拟器已加载');
  
  // 检查 GIF 库是否加载成功
  if (typeof gifuct !== 'undefined') {
    console.log('GIF 解析库加载成功');
  } else {
    console.warn('GIF 解析库未加载，GIF 功能可能不可用');
  }
});
