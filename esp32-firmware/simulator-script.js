const canvas = document.getElementById('ledCanvas');
const ctx = canvas.getContext('2d');
const PANEL_WIDTH = 64;
const PANEL_HEIGHT = 64;
const PIXEL_SIZE = 10;

let uploadedImage = null;
let currentTab = 'time';

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

// 图片处理
function handleImageUpload(event) {
  const file = event.target.files[0];
  if (!file) return;
  
  const reader = new FileReader();
  reader.onload = function(e) {
    const img = new Image();
    img.onload = function() {
      uploadedImage = img;
      document.getElementById('uploadPlaceholder').style.display = 'none';
      document.getElementById('imagePreviewContainer').style.display = 'block';
      document.getElementById('imagePreview').src = e.target.result;
      document.getElementById('uploadArea').classList.add('has-image');
      document.getElementById('showImage').checked = true;
      updateDisplay();
    };
    img.src = e.target.result;
  };
  reader.readAsDataURL(file);
}

function removeImage() {
  uploadedImage = null;
  document.getElementById('uploadPlaceholder').style.display = 'block';
  document.getElementById('imagePreviewContainer').style.display = 'none';
  document.getElementById('uploadArea').classList.remove('has-image');
  document.getElementById('showImage').checked = false;
  document.getElementById('imageInput').value = '';
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
    imageWidth: parseInt(document.getElementById('imageWidth').value),
    imageHeight: parseInt(document.getElementById('imageHeight').value),
    imageX: parseInt(document.getElementById('imageX').value),
    imageY: parseInt(document.getElementById('imageY').value),
    
    timeFontSize: parseInt(document.getElementById('timeFontSize').value),
    timeX: parseInt(document.getElementById('timeX').value),
    timeY: parseInt(document.getElementById('timeY').value),
    timeColor: hexToRgb(selectedColors.time),
    
    showDate: document.getElementById('showDate').checked,
    dateFontSize: parseInt(document.getElementById('dateFontSize').value),
    dateX: parseInt(document.getElementById('dateX').value),
    dateY: parseInt(document.getElementById('dateY').value),
    dateColor: hexToRgb(selectedColors.date),
    
    showWeek: document.getElementById('showWeek').checked,
    weekX: parseInt(document.getElementById('weekX').value),
    weekY: parseInt(document.getElementById('weekY').value),
    weekColor: hexToRgb(selectedColors.week)
  };
  
  // 绘制图片并保存像素数据
  if (config.showImage && uploadedImage) {
    currentImagePixels = drawImage(uploadedImage, config.imageX, config.imageY, config.imageWidth, config.imageHeight);
  } else {
    currentImagePixels = null;
  }
  
  const timeInput = document.getElementById('timeInput').value;
  const timeOnly = timeInput.substring(0, 5);
  drawText(timeOnly, config.timeX, config.timeY, ...config.timeColor, config.timeFontSize);
  
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
  const now = new Date();
  const hours = String(now.getHours()).padStart(2, '0');
  const minutes = String(now.getMinutes()).padStart(2, '0');
  document.getElementById('timeInput').value = `${hours}:${minutes}`;
  document.getElementById('dateInput').valueAsDate = now;
  updateDisplay();
}

function saveConfig() {
  const config = {
    timeFontSize: parseInt(document.getElementById('timeFontSize').value),
    timeX: parseInt(document.getElementById('timeX').value),
    timeY: parseInt(document.getElementById('timeY').value),
    timeColor: selectedColors.time,
    showDate: document.getElementById('showDate').checked,
    dateFontSize: parseInt(document.getElementById('dateFontSize').value),
    dateX: parseInt(document.getElementById('dateX').value),
    dateY: parseInt(document.getElementById('dateY').value),
    dateColor: selectedColors.date,
    showWeek: document.getElementById('showWeek').checked,
    weekX: parseInt(document.getElementById('weekX').value),
    weekY: parseInt(document.getElementById('weekY').value),
    weekColor: selectedColors.week,
    showImage: document.getElementById('showImage').checked,
    imageWidth: parseInt(document.getElementById('imageWidth').value),
    imageHeight: parseInt(document.getElementById('imageHeight').value),
    imageX: parseInt(document.getElementById('imageX').value),
    imageY: parseInt(document.getElementById('imageY').value),
    imageData: uploadedImage ? document.getElementById('imagePreview').src : null,
    imagePixels: currentImagePixels // 保存像素数据数组
  };
  
  const imageInfo = currentImagePixels 
    ? `图片像素数据: ${currentImagePixels.length} 个像素点` 
    : '无图片数据';
  
  const configText = `时钟配置参数：
━━━━━━━━━━━━━━━━━━━━
时间: 字体${config.timeFontSize} X${config.timeX} Y${config.timeY} 颜色${config.timeColor}
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
  const jsonStr = JSON.stringify(config, null, 2);
  
  navigator.clipboard.writeText(jsonStr).then(() => {
    const pixelCount = config.imagePixels ? config.imagePixels.length : 0;
    alert(`✅ 完整配置已复制！\n\n包含:\n- 时间/日期/星期设置\n- 图片像素数据 (${pixelCount} 个点)\n\n可以直接粘贴到小程序代码中使用`);
  }).catch(() => {
    const output = document.getElementById('configOutput');
    output.textContent = jsonStr;
    output.style.display = 'block';
    alert('自动复制失败，JSON 已显示在下方，请手动复制');
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

// 加载保存的配置
function loadSavedConfig() {
  const saved = localStorage.getItem('ledClockConfig');
  if (saved) {
    try {
      const config = JSON.parse(saved);
      
      // 时间设置
      if (config.timeFontSize) document.getElementById('timeFontSize').value = config.timeFontSize;
      if (config.timeX !== undefined) document.getElementById('timeX').value = config.timeX;
      if (config.timeY !== undefined) document.getElementById('timeY').value = config.timeY;
      if (config.timeColor) {
        selectedColors.time = config.timeColor;
        selectColor('time', config.timeColor);
      }
      
      // 日期设置
      if (config.showDate !== undefined) document.getElementById('showDate').checked = config.showDate;
      if (config.dateFontSize) document.getElementById('dateFontSize').value = config.dateFontSize;
      if (config.dateX !== undefined) document.getElementById('dateX').value = config.dateX;
      if (config.dateY !== undefined) document.getElementById('dateY').value = config.dateY;
      if (config.dateColor) {
        selectedColors.date = config.dateColor;
        selectColor('date', config.dateColor);
      }
      
      // 星期设置
      if (config.showWeek !== undefined) document.getElementById('showWeek').checked = config.showWeek;
      if (config.weekX !== undefined) document.getElementById('weekX').value = config.weekX;
      if (config.weekY !== undefined) document.getElementById('weekY').value = config.weekY;
      if (config.weekColor) {
        selectedColors.week = config.weekColor;
        selectColor('week', config.weekColor);
      }
      
      // 图片设置
      if (config.showImage !== undefined) document.getElementById('showImage').checked = config.showImage;
      if (config.imageWidth) document.getElementById('imageWidth').value = config.imageWidth;
      if (config.imageHeight) document.getElementById('imageHeight').value = config.imageHeight;
      if (config.imageX !== undefined) document.getElementById('imageX').value = config.imageX;
      if (config.imageY !== undefined) document.getElementById('imageY').value = config.imageY;
      
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

loadSavedConfig();
updateDisplay();
