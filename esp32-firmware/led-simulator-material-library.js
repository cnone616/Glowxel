const canvas = document.getElementById('materialCanvas');
const ctx = canvas.getContext('2d');
const PANEL_WIDTH = 64;
const PANEL_HEIGHT = 64;
const PIXEL_SIZE = 10;
const LOCAL_STORAGE_KEY = 'ledSimulatorMaterialPixelLibrary';

const BUILT_IN_MATERIALS = [
  { id: "contra", name: "魂斗罗 Contra", path: "material-library/nes-title-screens/contra.png" },
  { id: "super-c", name: "超级魂斗罗 Super C", path: "material-library/nes-title-screens/super-c.png" },
  { id: "battle-city", name: "坦克大战 Battle City", path: "material-library/nes-title-screens/battle-city.png" },
  { id: "super-mario-bros", name: "超级马里奥兄弟", path: "material-library/nes-title-screens/super-mario-bros.png" },
  { id: "super-mario-bros-3", name: "超级马里奥兄弟3", path: "material-library/nes-title-screens/super-mario-bros-3.png" },
  { id: "adventure-island", name: "冒险岛", path: "material-library/nes-title-screens/adventure-island.png" },
  { id: "adventure-island-2", name: "冒险岛2", path: "material-library/nes-title-screens/adventure-island-2.png" },
  { id: "mega-man-2", name: "洛克人2", path: "material-library/nes-title-screens/mega-man-2.png" },
  { id: "ninja-gaiden", name: "忍者龙剑传", path: "material-library/nes-title-screens/ninja-gaiden.png" },
  { id: "double-dragon", name: "双截龙", path: "material-library/nes-title-screens/double-dragon.png" },
  { id: "double-dragon-2", name: "双截龙2", path: "material-library/nes-title-screens/double-dragon-2.png" },
  { id: "tetris", name: "俄罗斯方块", path: "material-library/nes-title-screens/tetris.png" },
  { id: "bomberman", name: "炸弹人", path: "material-library/nes-title-screens/bomberman.png" },
  { id: "chip-n-dale", name: "松鼠大作战", path: "material-library/nes-title-screens/chip-n-dale.png" },
  { id: "tmnt", name: "忍者神龟", path: "material-library/nes-title-screens/tmnt.png" },
  { id: "jackal", name: "赤色要塞 Jackal", path: "material-library/nes-title-screens/jackal.png" },
  { id: "life-force", name: "沙罗曼蛇 Life Force", path: "material-library/nes-title-screens/life-force.png" },
  { id: "gradius", name: "宇宙巡航机 Gradius", path: "material-library/nes-title-screens/gradius.png" },
  { id: "river-city-ransom", name: "热血物语", path: "material-library/nes-title-screens/river-city-ransom.png" },
  { id: "mighty-final-fight", name: "热血快打", path: "material-library/nes-title-screens/mighty-final-fight.png" },
  { id: "circus-charlie", name: "马戏团", path: "material-library/nes-title-screens/circus-charlie.png" },
  { id: "excitebike", name: "越野机车", path: "material-library/nes-title-screens/excitebike.png" },
  { id: "balloon-fight", name: "气球大战", path: "material-library/nes-title-screens/balloon-fight.png" },
  { id: "ice-climber", name: "攀冰 Ice Climber", path: "material-library/nes-title-screens/ice-climber.png" },
  { id: "pac-man", name: "吃豆人 Pac-Man", path: "material-library/nes-title-screens/pac-man.png" },
  { id: "galaga", name: "小蜜蜂 Galaga", path: "material-library/nes-title-screens/galaga.png" },
  { id: "donkey-kong", name: "大金刚 Donkey Kong", path: "material-library/nes-title-screens/donkey-kong.png" },
  { id: "road-fighter", name: "火箭车 Road Fighter", path: "material-library/nes-title-screens/road-fighter.png" },
  { id: "spartan-x", name: "成龙踢馆 Spartan X", path: "material-library/nes-title-screens/spartan-x.png" },
  { id: "yie-ar-kung-fu", name: "功夫 Yie Ar Kung-Fu", path: "material-library/nes-title-screens/yie-ar-kung-fu.png" },
  { id: "rush-n-attack", name: "绿色兵团 Rush'n Attack", path: "material-library/nes-title-screens/rush-n-attack.png" },
  { id: "metroid", name: "银河战士 Metroid", path: "material-library/nes-title-screens/metroid.png" },
  { id: "castlevania", name: "恶魔城 Castlevania", path: "material-library/nes-title-screens/castlevania.png" },
  { id: "dr-mario", name: "马力欧医生 Dr. Mario", path: "material-library/nes-title-screens/dr-mario.png" },
];

let uploadedImage = null;
let uploadedImageSourceData = '';
let currentImagePixels = [];
let selectedBuiltInMaterialId = '';
let savedPixelLibrary = [];
let selectedSavedMaterialId = '';
let isEraseMode = false;
let eraseBrushSize = 1;
let isCanvasPointerDown = false;
const erasedPixelSet = new Set();
let editableSourceCanvas = null;
let editableSourceContext = null;
let baseImageSourceData = '';
let isAspectRatioLocked = true;
let currentLibraryTab = 'builtIn';

function getPixelKey(x, y) {
  return `${x},${y}`;
}

function setEraseMode(nextMode) {
  isEraseMode = nextMode;
  updateEraseToggleButton();
}

function updateEraseToggleButton() {
  const button = document.getElementById('eraseToggleBtn');
  if (isEraseMode) {
    button.textContent = '关闭擦除';
    button.classList.add('active');
  } else {
    button.textContent = '开启擦除';
    button.classList.remove('active');
  }
}

function toggleEraseMode() {
  setEraseMode(!isEraseMode);
  updateDisplay();
}

function updateEraseBrushSize() {
  const rawValue = document.getElementById('eraseBrushSize').value;
  const parsedValue = parseInt(rawValue, 10);
  if (Number.isNaN(parsedValue)) {
    document.getElementById('eraseBrushSize').value = String(eraseBrushSize);
    return;
  }
  if (parsedValue < 1) {
    eraseBrushSize = 1;
  } else if (parsedValue > 8) {
    eraseBrushSize = 8;
  } else {
    eraseBrushSize = parsedValue;
  }
  document.getElementById('eraseBrushSize').value = String(eraseBrushSize);
}

function clearEraseMask() {
  erasedPixelSet.clear();
  if (baseImageSourceData.length === 0) {
    updateDisplay();
    return;
  }
  loadImageFromSource(
    baseImageSourceData,
    getCurrentMaterialName(),
    selectedBuiltInMaterialId,
    false,
    false
  ).catch((error) => {
    alert(error.message);
  });
}

function switchLibraryTab(tabName) {
  currentLibraryTab = tabName;

  const builtInButton = document.getElementById('libraryTabBuiltIn');
  const savedButton = document.getElementById('libraryTabSaved');
  const builtInContent = document.getElementById('libraryBuiltInContent');
  const savedContent = document.getElementById('librarySavedContent');

  if (tabName === 'builtIn') {
    builtInButton.classList.add('active');
    savedButton.classList.remove('active');
    builtInContent.classList.add('active');
    savedContent.classList.remove('active');
    return;
  }

  builtInButton.classList.remove('active');
  savedButton.classList.add('active');
  builtInContent.classList.remove('active');
  savedContent.classList.add('active');
}

function setErasePointsFromArray(points) {
  erasedPixelSet.clear();
  if (!Array.isArray(points)) {
    return;
  }

  for (let i = 0; i < points.length; i++) {
    const point = points[i];
    if (point === null || typeof point !== 'object') {
      continue;
    }
    if (!Object.prototype.hasOwnProperty.call(point, 'x')) {
      continue;
    }
    if (!Object.prototype.hasOwnProperty.call(point, 'y')) {
      continue;
    }
    const x = parseInt(point.x, 10);
    const y = parseInt(point.y, 10);
    if (Number.isNaN(x) || Number.isNaN(y)) {
      continue;
    }
    if (x < 0 || y < 0) {
      continue;
    }
    erasedPixelSet.add(getPixelKey(x, y));
  }
}

function initializeEditableSourceFromImage(image) {
  const sourceWidth = image.naturalWidth;
  const sourceHeight = image.naturalHeight;
  if (sourceWidth <= 0 || sourceHeight <= 0) {
    editableSourceCanvas = null;
    editableSourceContext = null;
    return;
  }

  editableSourceCanvas = document.createElement('canvas');
  editableSourceCanvas.width = sourceWidth;
  editableSourceCanvas.height = sourceHeight;
  editableSourceContext = editableSourceCanvas.getContext('2d');
  editableSourceContext.imageSmoothingEnabled = false;
  editableSourceContext.clearRect(0, 0, sourceWidth, sourceHeight);
  editableSourceContext.drawImage(image, 0, 0, sourceWidth, sourceHeight);
}

function getActiveSourceDrawable() {
  if (editableSourceCanvas !== null) {
    return editableSourceCanvas;
  }
  return uploadedImage;
}

function getNumberValue(inputId) {
  const rawValue = document.getElementById(inputId).value;
  const parsedValue = parseInt(rawValue, 10);
  if (Number.isNaN(parsedValue)) {
    throw new Error(`数值输入无效: ${inputId}`);
  }
  return parsedValue;
}

function getCurrentMaterialName() {
  return document.getElementById('materialName').value.trim();
}

function calculateGreatestCommonDivisor(a, b) {
  let x = a;
  let y = b;
  while (y !== 0) {
    const remainder = x % y;
    x = y;
    y = remainder;
  }
  return x;
}

function formatRatioText(width, height) {
  if (width <= 0 || height <= 0) {
    return '--';
  }
  const divisor = calculateGreatestCommonDivisor(width, height);
  const ratioWidth = Math.floor(width / divisor);
  const ratioHeight = Math.floor(height / divisor);
  return `${width}:${height} (${ratioWidth}:${ratioHeight})`;
}

function updateAspectRatioInfo() {
  const sourceNode = document.getElementById('sourceAspectRatioText');
  const currentNode = document.getElementById('currentAspectRatioText');

  if (uploadedImage === null) {
    sourceNode.textContent = '未加载';
  } else {
    const sourceDrawable = getActiveSourceDrawable();
    let sourceWidth = uploadedImage.naturalWidth;
    let sourceHeight = uploadedImage.naturalHeight;
    if (sourceDrawable !== null && sourceDrawable instanceof HTMLCanvasElement) {
      sourceWidth = sourceDrawable.width;
      sourceHeight = sourceDrawable.height;
    }
    sourceNode.textContent = formatRatioText(sourceWidth, sourceHeight);
  }

  const widthRawValue = parseInt(document.getElementById('imageWidth').value, 10);
  const heightRawValue = parseInt(document.getElementById('imageHeight').value, 10);
  if (Number.isNaN(widthRawValue) || Number.isNaN(heightRawValue)) {
    currentNode.textContent = '--';
    return;
  }
  currentNode.textContent = formatRatioText(widthRawValue, heightRawValue);
}

function syncSizeWithAspectRatio(changedDimension) {
  if (!isAspectRatioLocked) {
    return;
  }
  if (uploadedImage === null) {
    return;
  }

  const naturalWidth = uploadedImage.naturalWidth;
  const naturalHeight = uploadedImage.naturalHeight;
  if (naturalWidth <= 0 || naturalHeight <= 0) {
    return;
  }

  const widthInput = document.getElementById('imageWidth');
  const heightInput = document.getElementById('imageHeight');
  const widthValue = parseInt(widthInput.value, 10);
  const heightValue = parseInt(heightInput.value, 10);
  if (Number.isNaN(widthValue) || Number.isNaN(heightValue)) {
    return;
  }

  if (changedDimension === 'width') {
    const syncedHeight = Math.max(1, Math.round((widthValue * naturalHeight) / naturalWidth));
    heightInput.value = String(syncedHeight);
    checkImageSize('height', heightInput.value);
    return;
  }
  if (changedDimension === 'height') {
    const syncedWidth = Math.max(1, Math.round((heightValue * naturalWidth) / naturalHeight));
    widthInput.value = String(syncedWidth);
    checkImageSize('width', widthInput.value);
  }
}

function handleAspectLockChange() {
  isAspectRatioLocked = document.getElementById('lockAspectRatio').checked;
  if (isAspectRatioLocked) {
    syncSizeWithAspectRatio('width');
  }
  updateAspectRatioInfo();
  updateDisplay();
}

function handleSizeInput(changedDimension) {
  const inputId = changedDimension === 'width' ? 'imageWidth' : 'imageHeight';
  const inputValue = document.getElementById(inputId).value;
  checkImageSize(changedDimension, inputValue);
  syncSizeWithAspectRatio(changedDimension);
  updateAspectRatioInfo();
  updateDisplay();
}

function applyFittedPlacementForCurrentImage() {
  if (uploadedImage === null) {
    return;
  }

  const naturalWidth = uploadedImage.naturalWidth;
  const naturalHeight = uploadedImage.naturalHeight;
  if (naturalWidth <= 0 || naturalHeight <= 0) {
    return;
  }

  const scale = Math.min(PANEL_WIDTH / naturalWidth, PANEL_HEIGHT / naturalHeight);
  const fittedWidth = Math.max(1, Math.round(naturalWidth * scale));
  const fittedHeight = Math.max(1, Math.round(naturalHeight * scale));
  const fittedX = Math.floor((PANEL_WIDTH - fittedWidth) / 2);
  const fittedY = Math.floor((PANEL_HEIGHT - fittedHeight) / 2);

  document.getElementById('imageWidth').value = String(fittedWidth);
  document.getElementById('imageHeight').value = String(fittedHeight);
  document.getElementById('imageX').value = String(fittedX);
  document.getElementById('imageY').value = String(fittedY);
  checkImageSize('width', String(fittedWidth));
  checkImageSize('height', String(fittedHeight));
}

function setStatus(message) {
  document.getElementById('statusOutput').textContent = message;
}

function clearCanvas() {
  ctx.fillStyle = '#000000';
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  ctx.strokeStyle = 'rgba(40, 40, 40, 0.55)';
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

  ctx.strokeStyle = 'rgba(0, 243, 255, 0.35)';
  ctx.lineWidth = 1;
  ctx.strokeRect(0.5, 0.5, PANEL_WIDTH * PIXEL_SIZE - 1, PANEL_HEIGHT * PIXEL_SIZE - 1);
}

function getPanelPositionFromClientPoint(clientX, clientY) {
  const rect = canvas.getBoundingClientRect();
  const offsetX = clientX - rect.left;
  const offsetY = clientY - rect.top;

  if (offsetX < 0 || offsetY < 0 || offsetX >= rect.width || offsetY >= rect.height) {
    return null;
  }

  const panelX = Math.floor((offsetX / rect.width) * PANEL_WIDTH);
  const panelY = Math.floor((offsetY / rect.height) * PANEL_HEIGHT);
  if (panelX < 0 || panelX >= PANEL_WIDTH || panelY < 0 || panelY >= PANEL_HEIGHT) {
    return null;
  }

  return { x: panelX, y: panelY };
}

function eraseAtPanelPoint(panelX, panelY) {
  if (editableSourceCanvas === null || editableSourceContext === null) {
    return;
  }

  let width = 0;
  let height = 0;
  let imageX = 0;
  let imageY = 0;
  try {
    width = getNumberValue('imageWidth');
    height = getNumberValue('imageHeight');
    imageX = getNumberValue('imageX');
    imageY = getNumberValue('imageY');
  } catch (error) {
    return;
  }
  if (width <= 0 || height <= 0) {
    return;
  }

  const half = Math.floor(eraseBrushSize / 2);
  const startX = panelX - half;
  const startY = panelY - half;
  const sourceWidth = editableSourceCanvas.width;
  const sourceHeight = editableSourceCanvas.height;
  const sourceImageData = editableSourceContext.getImageData(0, 0, sourceWidth, sourceHeight);
  let hasMutation = false;

  for (let y = startY; y < startY + eraseBrushSize; y++) {
    for (let x = startX; x < startX + eraseBrushSize; x++) {
      if (x < 0 || x >= PANEL_WIDTH || y < 0 || y >= PANEL_HEIGHT) {
        continue;
      }

      const localX = x - imageX;
      const localY = y - imageY;
      if (localX < 0 || localX >= width || localY < 0 || localY >= height) {
        continue;
      }

      let sourceXStart = Math.floor((localX * sourceWidth) / width);
      let sourceXEnd = Math.ceil(((localX + 1) * sourceWidth) / width);
      let sourceYStart = Math.floor((localY * sourceHeight) / height);
      let sourceYEnd = Math.ceil(((localY + 1) * sourceHeight) / height);

      if (sourceXStart < 0) {
        sourceXStart = 0;
      }
      if (sourceYStart < 0) {
        sourceYStart = 0;
      }
      if (sourceXStart >= sourceWidth) {
        sourceXStart = sourceWidth - 1;
      }
      if (sourceYStart >= sourceHeight) {
        sourceYStart = sourceHeight - 1;
      }
      if (sourceXEnd <= sourceXStart) {
        sourceXEnd = sourceXStart + 1;
      }
      if (sourceYEnd <= sourceYStart) {
        sourceYEnd = sourceYStart + 1;
      }
      if (sourceXEnd > sourceWidth) {
        sourceXEnd = sourceWidth;
      }
      if (sourceYEnd > sourceHeight) {
        sourceYEnd = sourceHeight;
      }

      for (let sourceY = sourceYStart; sourceY < sourceYEnd; sourceY++) {
        for (let sourceX = sourceXStart; sourceX < sourceXEnd; sourceX++) {
          const sourceIndex = (sourceY * sourceWidth + sourceX) * 4;
          sourceImageData.data[sourceIndex] = 0;
          sourceImageData.data[sourceIndex + 1] = 0;
          sourceImageData.data[sourceIndex + 2] = 0;
          sourceImageData.data[sourceIndex + 3] = 0;
          erasedPixelSet.add(getPixelKey(sourceX, sourceY));
          hasMutation = true;
        }
      }
    }
  }

  if (!hasMutation) {
    return;
  }

  editableSourceContext.putImageData(sourceImageData, 0, 0);
}

function handleCanvasPointerDown(event) {
  if (!isEraseMode || uploadedImage === null) {
    return;
  }

  event.preventDefault();
  isCanvasPointerDown = true;
  const position = getPanelPositionFromClientPoint(event.clientX, event.clientY);
  if (position === null) {
    return;
  }

  eraseAtPanelPoint(position.x, position.y);
  updateDisplay();
}

function handleCanvasPointerMove(event) {
  if (!isEraseMode || uploadedImage === null) {
    return;
  }
  if (!isCanvasPointerDown) {
    return;
  }

  event.preventDefault();
  const position = getPanelPositionFromClientPoint(event.clientX, event.clientY);
  if (position === null) {
    return;
  }

  eraseAtPanelPoint(position.x, position.y);
  updateDisplay();
}

function handleCanvasPointerUp() {
  if (!isCanvasPointerDown) {
    return;
  }
  isCanvasPointerDown = false;
}

function bindCanvasPointerEvents() {
  canvas.addEventListener('pointerdown', handleCanvasPointerDown);
  canvas.addEventListener('pointermove', handleCanvasPointerMove);
  canvas.addEventListener('pointerup', handleCanvasPointerUp);
  canvas.addEventListener('pointerleave', handleCanvasPointerUp);
  canvas.addEventListener('pointercancel', handleCanvasPointerUp);
}

function drawPixel(x, y, r, g, b) {
  if (x < 0 || x >= PANEL_WIDTH || y < 0 || y >= PANEL_HEIGHT) {
    return;
  }
  ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
  ctx.fillRect(x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE);
}

function drawImageToCanvas(img, offsetX, offsetY, width, height) {
  const tempCanvas = document.createElement('canvas');
  tempCanvas.width = width;
  tempCanvas.height = height;
  const tempCtx = tempCanvas.getContext('2d');
  tempCtx.imageSmoothingEnabled = false;
  tempCtx.drawImage(img, 0, 0, width, height);

  const imageData = tempCtx.getImageData(0, 0, width, height);
  const pixels = [];

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const sourceIndex = (y * width + x) * 4;
      const r = imageData.data[sourceIndex];
      const g = imageData.data[sourceIndex + 1];
      const b = imageData.data[sourceIndex + 2];
      const a = imageData.data[sourceIndex + 3];

      if (a <= 128) {
        continue;
      }

      const panelX = x + offsetX;
      const panelY = y + offsetY;
      if (panelX < 0 || panelX >= PANEL_WIDTH || panelY < 0 || panelY >= PANEL_HEIGHT) {
        continue;
      }
      drawPixel(panelX, panelY, r, g, b);
      pixels.push({ x: panelX, y: panelY, r, g, b });
    }
  }

  return pixels;
}

function updateDisplay() {
  clearCanvas();

  const sourceDrawable = getActiveSourceDrawable();
  if (sourceDrawable === null) {
    currentImagePixels = [];
    setPreviewPlaceholder();
    setStatus('请选择内置素材或上传图片。');
    updateAspectRatioInfo();
    return;
  }

  let width = 0;
  let height = 0;
  let imageX = 0;
  let imageY = 0;
  try {
    width = getNumberValue('imageWidth');
    height = getNumberValue('imageHeight');
    imageX = getNumberValue('imageX');
    imageY = getNumberValue('imageY');
  } catch (error) {
    setStatus(error.message);
    return;
  }

  let renderErrorMessage = '';
  try {
    currentImagePixels = drawImageToCanvas(sourceDrawable, imageX, imageY, width, height);
  } catch (error) {
    currentImagePixels = [];
    renderErrorMessage = String(error.message);
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(
      sourceDrawable,
      imageX * PIXEL_SIZE,
      imageY * PIXEL_SIZE,
      width * PIXEL_SIZE,
      height * PIXEL_SIZE
    );
  }

  const materialName = getCurrentMaterialName();
  const nameLabel = materialName.length > 0 ? materialName : '未命名素材';
  const eraseModeLabel = isEraseMode ? '开启' : '关闭';
  let offscreenHint = '';
  if (currentImagePixels.length === 0) {
    offscreenHint = '\n提示: 当前图像可能在可视区域外，点击“按比例适配 64x64”可快速复位。';
  }
  if (renderErrorMessage.length > 0) {
    offscreenHint = `\n提示: 像素读取失败（${renderErrorMessage}），建议通过 http://127.0.0.1 打开页面。`;
  }
  setStatus(
    `素材: ${nameLabel}\n尺寸: ${width}x${height} 位置: (${imageX}, ${imageY})\n像素点: ${currentImagePixels.length}\n擦除模式: ${eraseModeLabel} · 擦除点位: ${erasedPixelSet.size}${offscreenHint}`
  );
  showImagePreviewFromPixels(currentImagePixels);
  updateAspectRatioInfo();
}

function setPreviewPlaceholder() {
  document.getElementById('uploadPlaceholder').style.display = 'block';
  document.getElementById('imagePreviewContainer').style.display = 'none';
  document.getElementById('uploadArea').classList.remove('has-image');
}

function showImagePreviewFromPixels(pixels) {
  const previewSource = buildPreviewImageData(pixels);
  document.getElementById('uploadPlaceholder').style.display = 'none';
  document.getElementById('imagePreviewContainer').style.display = 'block';
  document.getElementById('imagePreview').src = previewSource;
  document.getElementById('uploadArea').classList.add('has-image');
}

function blobToDataUrl(blob) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => {
      resolve(String(reader.result));
    };
    reader.onerror = () => {
      reject(new Error('素材读取失败'));
    };
    reader.readAsDataURL(blob);
  });
}

async function convertSourceToDataUrl(source) {
  if (source.startsWith('data:')) {
    return source;
  }

  const response = await fetch(source);
  if (!response.ok) {
    throw new Error(`素材文件读取失败: ${response.status}`);
  }
  const blob = await response.blob();
  return await blobToDataUrl(blob);
}

function loadImageFromSource(source, materialName, builtInMaterialId, resetEraseMask, autoFitPlacement) {
  return new Promise((resolve, reject) => {
    const image = new Image();
    image.onload = () => {
      uploadedImage = image;
      uploadedImageSourceData = source;
      baseImageSourceData = source;
      initializeEditableSourceFromImage(image);
      selectedBuiltInMaterialId = builtInMaterialId;
      if (resetEraseMask) {
        erasedPixelSet.clear();
      }

      if (materialName.length > 0) {
        document.getElementById('materialName').value = materialName;
      }

      if (autoFitPlacement) {
        applyFittedPlacementForCurrentImage();
      } else {
        syncSizeWithAspectRatio('width');
      }
      updateAspectRatioInfo();
      renderMaterialLibrary();
      updateDisplay();
      resolve();
    };
    image.onerror = () => {
      reject(new Error('素材加载失败'));
    };
    image.src = source;
  });
}

async function selectBuiltInMaterial(materialId) {
  let selectedMaterial = null;
  for (let i = 0; i < BUILT_IN_MATERIALS.length; i++) {
    if (BUILT_IN_MATERIALS[i].id === materialId) {
      selectedMaterial = BUILT_IN_MATERIALS[i];
      break;
    }
  }

  if (selectedMaterial === null) {
    alert('找不到对应素材');
    return;
  }

  setStatus(`正在加载素材: ${selectedMaterial.name}`);
  let source = selectedMaterial.path;

  try {
    source = await convertSourceToDataUrl(selectedMaterial.path);
  } catch (error) {
    source = selectedMaterial.path;
  }

  loadImageFromSource(source, selectedMaterial.name, selectedMaterial.id, true, true).catch((error) => {
    alert(error.message);
  });
}

function filterMaterialLibrary() {
  renderMaterialLibrary();
}

function renderMaterialLibrary() {
  const grid = document.getElementById('materialLibraryGrid');
  const keyword = document.getElementById('materialSearch').value.trim().toLowerCase();
  grid.innerHTML = '';

  for (let i = 0; i < BUILT_IN_MATERIALS.length; i++) {
    const material = BUILT_IN_MATERIALS[i];
    const materialNameLower = material.name.toLowerCase();
    const shouldShow = keyword.length === 0 || materialNameLower.includes(keyword);

    if (!shouldShow) {
      continue;
    }

    const item = document.createElement('div');
    item.className = 'material-item';
    if (selectedBuiltInMaterialId === material.id) {
      item.classList.add('active');
    }

    const thumb = document.createElement('img');
    thumb.className = 'material-thumb';
    thumb.src = material.path;
    thumb.alt = material.name;

    const name = document.createElement('div');
    name.className = 'material-name';
    name.textContent = material.name;

    item.appendChild(thumb);
    item.appendChild(name);
    item.onclick = () => selectBuiltInMaterial(material.id);
    grid.appendChild(item);
  }
}

function handleImageUpload(event) {
  const fileList = event.target.files;
  if (fileList.length === 0) {
    return;
  }

  const file = fileList[0];
  const reader = new FileReader();
  reader.onload = () => {
    const sourceData = String(reader.result);
    selectedBuiltInMaterialId = '';
    loadImageFromSource(sourceData, file.name, '', true, true).catch((error) => {
      alert(error.message);
    });
  };
  reader.readAsDataURL(file);
}

function removeCurrentMaterial() {
  uploadedImage = null;
  uploadedImageSourceData = '';
  baseImageSourceData = '';
  editableSourceCanvas = null;
  editableSourceContext = null;
  currentImagePixels = [];
  selectedBuiltInMaterialId = '';
  selectedSavedMaterialId = '';
  erasedPixelSet.clear();
  setEraseMode(false);
  document.getElementById('imageInput').value = '';
  renderMaterialLibrary();
  setPreviewPlaceholder();
  updateDisplay();
}

function adjustValue(inputId, delta) {
  const input = document.getElementById(inputId);
  const currentValue = parseInt(input.value, 10);
  const minValue = parseInt(input.min, 10);
  const maxValue = parseInt(input.max, 10);

  if (Number.isNaN(currentValue)) {
    alert(`数值输入无效: ${inputId}`);
    return;
  }

  let nextValue = currentValue + delta;
  if (!Number.isNaN(minValue) && nextValue < minValue) {
    nextValue = minValue;
  }
  if (!Number.isNaN(maxValue) && nextValue > maxValue) {
    nextValue = maxValue;
  }
  input.value = String(nextValue);

  if (inputId === 'imageWidth') {
    handleSizeInput('width');
    return;
  }
  if (inputId === 'imageHeight') {
    handleSizeInput('height');
    return;
  }
  if (inputId === 'eraseBrushSize') {
    updateEraseBrushSize();
    return;
  }
  updateAspectRatioInfo();
  updateDisplay();
}

function checkImageSize(dimension, value) {
  const inputId = dimension === 'width' ? 'imageWidth' : 'imageHeight';
  const warningId = dimension === 'width' ? 'widthWarning' : 'heightWarning';
  const input = document.getElementById(inputId);
  const warning = document.getElementById(warningId);
  const size = parseInt(value, 10);

  if (Number.isNaN(size)) {
    warning.style.display = 'none';
    return;
  }

  if (size > 64) {
    input.classList.add('warning');
    warning.style.display = 'block';
  } else {
    input.classList.remove('warning');
    warning.style.display = 'none';
  }
}

function setSquareSize() {
  let width = 0;
  let height = 0;
  try {
    width = getNumberValue('imageWidth');
    height = getNumberValue('imageHeight');
  } catch (error) {
    alert(error.message);
    return;
  }

  const target = width < height ? width : height;
  document.getElementById('imageWidth').value = String(target);
  document.getElementById('imageHeight').value = String(target);
  checkImageSize('width', String(target));
  checkImageSize('height', String(target));
  updateDisplay();
}

function fitToPanel() {
  if (uploadedImage === null) {
    alert('请先选择或上传素材');
    return;
  }
  applyFittedPlacementForCurrentImage();
  updateAspectRatioInfo();
  updateDisplay();
}

function buildSourceImageData() {
  if (editableSourceCanvas !== null) {
    return editableSourceCanvas.toDataURL('image/png');
  }
  const sourceCanvas = document.createElement('canvas');
  sourceCanvas.width = uploadedImage.naturalWidth;
  sourceCanvas.height = uploadedImage.naturalHeight;
  const sourceCtx = sourceCanvas.getContext('2d');
  sourceCtx.imageSmoothingEnabled = false;
  sourceCtx.drawImage(uploadedImage, 0, 0);
  return sourceCanvas.toDataURL('image/png');
}

function buildPreviewImageData(pixels) {
  const previewCanvas = document.createElement('canvas');
  previewCanvas.width = PANEL_WIDTH;
  previewCanvas.height = PANEL_HEIGHT;
  const previewCtx = previewCanvas.getContext('2d');
  previewCtx.fillStyle = '#000000';
  previewCtx.fillRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT);

  for (let i = 0; i < pixels.length; i++) {
    const pixel = pixels[i];
    previewCtx.fillStyle = `rgb(${pixel.r}, ${pixel.g}, ${pixel.b})`;
    previewCtx.fillRect(pixel.x, pixel.y, 1, 1);
  }

  return previewCanvas.toDataURL('image/png');
}

function createSavedMaterialId() {
  const timestamp = Date.now();
  const index = savedPixelLibrary.length + 1;
  return `${timestamp}-${index}`;
}

function persistSavedLibrary() {
  localStorage.setItem(LOCAL_STORAGE_KEY, JSON.stringify(savedPixelLibrary));
}

function buildErasePointArray() {
  const points = [];
  erasedPixelSet.forEach((key) => {
    const parts = key.split(',');
    if (parts.length !== 2) {
      return;
    }
    const x = parseInt(parts[0], 10);
    const y = parseInt(parts[1], 10);
    if (Number.isNaN(x) || Number.isNaN(y)) {
      return;
    }
    points.push({ x, y });
  });
  return points;
}

function saveCurrentMaterial() {
  if (uploadedImage === null) {
    alert('请先选择或上传素材');
    return;
  }

  const materialName = getCurrentMaterialName();
  if (materialName.length === 0) {
    alert('请先输入素材名称');
    return;
  }

  if (currentImagePixels.length === 0) {
    alert('当前没有可保存的像素点，请调整尺寸或位置后重试');
    return;
  }

  let width = 0;
  let height = 0;
  let imageX = 0;
  let imageY = 0;
  try {
    width = getNumberValue('imageWidth');
    height = getNumberValue('imageHeight');
    imageX = getNumberValue('imageX');
    imageY = getNumberValue('imageY');
  } catch (error) {
    alert(error.message);
    return;
  }

  const sourceImageData = buildSourceImageData();
  const previewImageData = buildPreviewImageData(currentImagePixels);

  const savedItem = {
    id: createSavedMaterialId(),
    name: materialName,
    sourceMaterialId: selectedBuiltInMaterialId,
    sourceImageData,
    previewImageData,
    width,
    height,
    imageX,
    imageY,
    erasePoints: buildErasePointArray(),
    pixels: currentImagePixels,
    savedAt: new Date().toISOString(),
  };

  savedPixelLibrary.push(savedItem);
  selectedSavedMaterialId = savedItem.id;
  persistSavedLibrary();
  renderSavedMaterialList();
  switchLibraryTab('saved');
  setStatus(`已保存: ${savedItem.name}\n像素点: ${savedItem.pixels.length}\n擦除点位: ${savedItem.erasePoints.length}\n可在右侧列表回显`);
}

function replaySavedMaterial(savedId) {
  let target = null;
  for (let i = 0; i < savedPixelLibrary.length; i++) {
    if (savedPixelLibrary[i].id === savedId) {
      target = savedPixelLibrary[i];
      break;
    }
  }

  if (target === null) {
    alert('素材不存在或已删除');
    return;
  }

  selectedSavedMaterialId = target.id;
  document.getElementById('materialName').value = target.name;
  document.getElementById('imageWidth').value = String(target.width);
  document.getElementById('imageHeight').value = String(target.height);
  document.getElementById('imageX').value = String(target.imageX);
  document.getElementById('imageY').value = String(target.imageY);
  checkImageSize('width', String(target.width));
  checkImageSize('height', String(target.height));
  setErasePointsFromArray(target.erasePoints);

  loadImageFromSource(target.sourceImageData, target.name, target.sourceMaterialId, false, false).then(() => {
    renderSavedMaterialList();
    setStatus(`已回显: ${target.name}\n像素点: ${target.pixels.length}\n擦除点位: ${erasedPixelSet.size}`);
  }).catch((error) => {
    alert(error.message);
  });
}

function deleteSavedMaterial(savedId) {
  const confirmed = window.confirm('确定删除这条本地像素素材吗？');
  if (!confirmed) {
    return;
  }

  const nextList = [];
  for (let i = 0; i < savedPixelLibrary.length; i++) {
    if (savedPixelLibrary[i].id !== savedId) {
      nextList.push(savedPixelLibrary[i]);
    }
  }
  savedPixelLibrary = nextList;

  if (selectedSavedMaterialId === savedId) {
    selectedSavedMaterialId = '';
  }

  persistSavedLibrary();
  renderSavedMaterialList();
  setStatus('已删除一条本地素材');
}

function downloadSavedMaterial(savedId) {
  let target = null;
  for (let i = 0; i < savedPixelLibrary.length; i++) {
    if (savedPixelLibrary[i].id === savedId) {
      target = savedPixelLibrary[i];
      break;
    }
  }

  if (target === null) {
    alert('素材不存在或已删除');
    return;
  }

  const payload = {
    name: target.name,
    width: target.width,
    height: target.height,
    imageX: target.imageX,
    imageY: target.imageY,
    erasePoints: target.erasePoints,
    pixels: target.pixels,
    savedAt: target.savedAt,
  };

  const jsonContent = JSON.stringify(payload, null, 2);
  const blob = new Blob([jsonContent], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement('a');
  anchor.href = url;
  anchor.download = `${target.name}-pixels.json`;
  document.body.appendChild(anchor);
  anchor.click();
  document.body.removeChild(anchor);
  URL.revokeObjectURL(url);
}

function clearAllSavedMaterials() {
  const confirmed = window.confirm('确定清空全部本地像素素材吗？');
  if (!confirmed) {
    return;
  }

  savedPixelLibrary = [];
  selectedSavedMaterialId = '';
  persistSavedLibrary();
  renderSavedMaterialList();
  setStatus('已清空全部本地像素素材');
}

function renderSavedMaterialList() {
  const list = document.getElementById('savedMaterialList');
  list.innerHTML = '';

  if (savedPixelLibrary.length === 0) {
    list.innerHTML = '<div class="saved-empty">暂无保存数据，先在左侧编辑后点击“保存像素数据到本地”。</div>';
    return;
  }

  for (let i = 0; i < savedPixelLibrary.length; i++) {
    const item = savedPixelLibrary[i];

    const card = document.createElement('div');
    card.className = 'saved-item';
    if (selectedSavedMaterialId === item.id) {
      card.classList.add('active');
    }

    const top = document.createElement('div');
    top.className = 'saved-top';

    const thumb = document.createElement('img');
    thumb.className = 'saved-thumb';
    thumb.src = item.previewImageData;
    thumb.alt = item.name;

    const meta = document.createElement('div');
    meta.className = 'saved-meta';

    const name = document.createElement('div');
    name.className = 'saved-name';
    name.textContent = item.name;

    const desc = document.createElement('div');
    desc.className = 'saved-desc';
    let eraseCount = 0;
    if (Array.isArray(item.erasePoints)) {
      eraseCount = item.erasePoints.length;
    }
    desc.textContent = `像素 ${item.pixels.length} · 擦除 ${eraseCount} · ${item.width}x${item.height} · (${item.imageX}, ${item.imageY})`;

    meta.appendChild(name);
    meta.appendChild(desc);
    top.appendChild(thumb);
    top.appendChild(meta);

    const buttons = document.createElement('div');
    buttons.className = 'saved-buttons';

    const replayButton = document.createElement('button');
    replayButton.className = 'small secondary';
    replayButton.textContent = '回显';
    replayButton.onclick = () => replaySavedMaterial(item.id);

    const downloadButton = document.createElement('button');
    downloadButton.className = 'small secondary';
    downloadButton.textContent = '导出';
    downloadButton.onclick = () => downloadSavedMaterial(item.id);

    const deleteButton = document.createElement('button');
    deleteButton.className = 'small danger';
    deleteButton.textContent = '删除';
    deleteButton.onclick = () => deleteSavedMaterial(item.id);

    buttons.appendChild(replayButton);
    buttons.appendChild(downloadButton);
    buttons.appendChild(deleteButton);

    card.appendChild(top);
    card.appendChild(buttons);
    list.appendChild(card);
  }
}

function loadSavedPixelLibrary() {
  const raw = localStorage.getItem(LOCAL_STORAGE_KEY);
  if (raw === null) {
    savedPixelLibrary = [];
    renderSavedMaterialList();
    return;
  }

  try {
    const parsed = JSON.parse(raw);
    if (!Array.isArray(parsed)) {
      alert('本地数据格式异常，已忽略');
      savedPixelLibrary = [];
      renderSavedMaterialList();
      return;
    }

    const nextList = [];
    for (let i = 0; i < parsed.length; i++) {
      const item = parsed[i];
      const isObject = item !== null && typeof item === 'object';
      if (!isObject) {
        continue;
      }
      nextList.push(item);
    }

    savedPixelLibrary = nextList;
    renderSavedMaterialList();
  } catch (error) {
    alert('读取本地数据失败，已忽略');
    savedPixelLibrary = [];
    renderSavedMaterialList();
  }
}

function copyCurrentPixels() {
  if (currentImagePixels.length === 0) {
    alert('当前没有像素数据可复制');
    return;
  }

  const jsonContent = JSON.stringify(currentImagePixels, null, 2);
  navigator.clipboard.writeText(jsonContent).then(() => {
    alert(`已复制 ${currentImagePixels.length} 个像素点`);
  }).catch(() => {
    alert('复制失败，请检查浏览器权限');
  });
}

function downloadCurrentPixels() {
  if (currentImagePixels.length === 0) {
    alert('当前没有像素数据可下载');
    return;
  }

  const materialName = getCurrentMaterialName();
  if (materialName.length === 0) {
    alert('请先填写素材名称');
    return;
  }

  let width = 0;
  let height = 0;
  let imageX = 0;
  let imageY = 0;
  try {
    width = getNumberValue('imageWidth');
    height = getNumberValue('imageHeight');
    imageX = getNumberValue('imageX');
    imageY = getNumberValue('imageY');
  } catch (error) {
    alert(error.message);
    return;
  }

  const payload = {
    name: materialName,
    width,
    height,
    imageX,
    imageY,
    erasePoints: buildErasePointArray(),
    pixels: currentImagePixels,
    exportedAt: new Date().toISOString(),
  };

  const jsonContent = JSON.stringify(payload, null, 2);
  const blob = new Blob([jsonContent], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement('a');
  anchor.href = url;
  anchor.download = `${materialName}-pixels.json`;
  document.body.appendChild(anchor);
  anchor.click();
  document.body.removeChild(anchor);
  URL.revokeObjectURL(url);
}

function initializePage() {
  clearCanvas();
  bindCanvasPointerEvents();
  updateEraseBrushSize();
  updateEraseToggleButton();
  isAspectRatioLocked = document.getElementById('lockAspectRatio').checked;
  switchLibraryTab('builtIn');
  renderMaterialLibrary();
  loadSavedPixelLibrary();
  setPreviewPlaceholder();
  updateAspectRatioInfo();
  updateDisplay();
}

initializePage();
