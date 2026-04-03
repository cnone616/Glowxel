const canvas = document.getElementById('materialCanvas');
const ctx = canvas.getContext('2d');
const PANEL_WIDTH = 64;
const PANEL_HEIGHT = 64;
const PIXEL_SIZE = 10;

let uploadedImage = null;
let gifFrames = [];
let currentFrameIndex = 0;
let gifAnimationId = null;
let isGifPlaying = false;
let gifSpeed = 1;
let isGifFile = false;
let currentImagePixels = null;
let savedMaterialPresets = [];
let selectedSavedPresetIndex = -1;

function setStatus(message) {
  document.getElementById('statusOutput').textContent = message;
}

function getMaterialName() {
  return document.getElementById('materialName').value.trim();
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

function drawPixel(x, y, r, g, b) {
  if (x < 0 || x >= PANEL_WIDTH || y < 0 || y >= PANEL_HEIGHT) {
    return;
  }
  ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
  ctx.fillRect(x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE);
}

function drawImageToCanvas(img, x, y, width, height) {
  if (!img) {
    return [];
  }

  const tempCanvas = document.createElement('canvas');
  tempCanvas.width = width;
  tempCanvas.height = height;
  const tempCtx = tempCanvas.getContext('2d');
  tempCtx.imageSmoothingEnabled = false;
  tempCtx.drawImage(img, 0, 0, width, height);
  const imageData = tempCtx.getImageData(0, 0, width, height);
  const pixelArray = [];

  for (let py = 0; py < height; py++) {
    for (let px = 0; px < width; px++) {
      const index = (py * width + px) * 4;
      const r = imageData.data[index];
      const g = imageData.data[index + 1];
      const b = imageData.data[index + 2];
      const a = imageData.data[index + 3];
      if (a <= 128) {
        continue;
      }

      const finalX = x + px;
      const finalY = y + py;
      if (finalX < 0 || finalX >= PANEL_WIDTH || finalY < 0 || finalY >= PANEL_HEIGHT) {
        continue;
      }

      drawPixel(finalX, finalY, r, g, b);
      pixelArray.push({ x: finalX, y: finalY, r, g, b });
    }
  }

  return pixelArray;
}

function updateDisplay() {
  clearCanvas();

  if (!uploadedImage) {
    currentImagePixels = [];
    setStatus('请先上传图片或 GIF。');
    return;
  }

  const width = parseInt(document.getElementById('imageWidth').value);
  const height = parseInt(document.getElementById('imageHeight').value);
  const offsetX = parseInt(document.getElementById('imageX').value);
  const offsetY = parseInt(document.getElementById('imageY').value);
  currentImagePixels = drawImageToCanvas(uploadedImage, offsetX, offsetY, width, height);

  const frameText = isGifFile ? `GIF ${gifFrames.length} 帧` : '静态图片';
  setStatus(`${frameText}\n当前像素点: ${currentImagePixels.length}`);
}

function showImagePreview(src) {
  document.getElementById('uploadPlaceholder').style.display = 'none';
  document.getElementById('imagePreviewContainer').style.display = 'block';
  document.getElementById('imagePreview').src = src;
  document.getElementById('uploadArea').classList.add('has-image');
}

function updateGifFrameInfo() {
  document.getElementById('currentFrame').textContent = currentFrameIndex + 1;
  document.getElementById('totalFrames').textContent = gifFrames.length;
}

function showGifControls() {
  document.getElementById('gifControls').style.display = 'block';
}

function hideGifControls() {
  document.getElementById('gifControls').style.display = 'none';
  pauseGif();
}

function handleImageUpload(event) {
  const file = event.target.files[0];
  if (!file) {
    return;
  }

  isGifFile = file.type === 'image/gif';
  if (isGifFile) {
    handleGifUpload(file);
    return;
  }
  handleStaticImageUpload(file);
}

function handleStaticImageUpload(file) {
  const reader = new FileReader();
  reader.onload = function(e) {
    const img = new Image();
    img.onload = function() {
      uploadedImage = img;
      gifFrames = [];
      isGifFile = false;
      currentFrameIndex = 0;
      showImagePreview(e.target.result);
      hideGifControls();
      updateDisplay();
    };
    img.src = e.target.result;
  };
  reader.readAsDataURL(file);
}

async function handleGifUpload(file) {
  try {
    const arrayBuffer = await file.arrayBuffer();
    const parser = new GIFParser();
    const gifData = await parser.parse(arrayBuffer);
    const canvases = parser.processAllFrames();

    gifFrames = [];
    for (let i = 0; i < canvases.length; i++) {
      gifFrames.push({
        canvas: canvases[i],
        delay: Math.max(50, gifData.frames[i].delay || 100),
        index: i,
      });
    }

    currentFrameIndex = 0;
    uploadedImage = gifFrames[0].canvas;
    isGifFile = true;
    showImagePreview(gifFrames[0].canvas.toDataURL());
    showGifControls();
    updateGifFrameInfo();
    updateDisplay();
  } catch (error) {
    alert(`GIF 解析失败: ${error.message}`);
    handleStaticImageUpload(file);
  }
}

function removeMaterial() {
  pauseGif();
  uploadedImage = null;
  gifFrames = [];
  isGifFile = false;
  currentFrameIndex = 0;
  document.getElementById('uploadPlaceholder').style.display = 'block';
  document.getElementById('imagePreviewContainer').style.display = 'none';
  document.getElementById('uploadArea').classList.remove('has-image');
  document.getElementById('imageInput').value = '';
  hideGifControls();
  updateDisplay();
}

function adjustValue(inputId, delta) {
  const input = document.getElementById(inputId);
  const currentValue = parseInt(input.value) || 0;
  const min = parseInt(input.min);
  const max = parseInt(input.max);
  const nextValue = Math.max(min, Math.min(max, currentValue + delta));
  input.value = nextValue;

  if (inputId === 'imageWidth') {
    checkImageSize('width', nextValue);
  }
  if (inputId === 'imageHeight') {
    checkImageSize('height', nextValue);
  }
  updateDisplay();
}

function setSquareSize() {
  const width = parseInt(document.getElementById('imageWidth').value);
  const height = parseInt(document.getElementById('imageHeight').value);
  const size = Math.min(width, height);
  document.getElementById('imageWidth').value = size;
  document.getElementById('imageHeight').value = size;
  checkImageSize('width', size);
  checkImageSize('height', size);
  updateDisplay();
}

function checkImageSize(dimension, value) {
  const input = document.getElementById(`image${dimension.charAt(0).toUpperCase() + dimension.slice(1)}`);
  const warning = document.getElementById(`${dimension}Warning`);
  const size = parseInt(value);
  if (size > 64) {
    input.classList.add('warning');
    warning.style.display = 'block';
    return;
  }
  input.classList.remove('warning');
  warning.style.display = 'none';
}

function playGif() {
  if (!isGifFile || gifFrames.length === 0) {
    return;
  }

  isGifPlaying = true;
  document.getElementById('playBtn').style.display = 'none';
  document.getElementById('pauseBtn').style.display = 'inline-block';

  function playNextFrame() {
    if (!isGifPlaying) {
      return;
    }
    const frame = gifFrames[currentFrameIndex];
    uploadedImage = frame.canvas;
    updateDisplay();
    updateGifFrameInfo();
    currentFrameIndex = (currentFrameIndex + 1) % gifFrames.length;
    gifAnimationId = setTimeout(playNextFrame, Math.max(50, frame.delay / gifSpeed));
  }

  playNextFrame();
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
  if (gifFrames.length === 0) {
    return;
  }
  currentFrameIndex = 0;
  uploadedImage = gifFrames[0].canvas;
  updateGifFrameInfo();
  updateDisplay();
}

function prevFrame() {
  if (!isGifFile || gifFrames.length === 0) {
    return;
  }
  pauseGif();
  currentFrameIndex = (currentFrameIndex - 1 + gifFrames.length) % gifFrames.length;
  uploadedImage = gifFrames[currentFrameIndex].canvas;
  updateGifFrameInfo();
  updateDisplay();
}

function nextFrame() {
  if (!isGifFile || gifFrames.length === 0) {
    return;
  }
  pauseGif();
  currentFrameIndex = (currentFrameIndex + 1) % gifFrames.length;
  uploadedImage = gifFrames[currentFrameIndex].canvas;
  updateGifFrameInfo();
  updateDisplay();
}

function adjustGifSpeed(delta) {
  const input = document.getElementById('gifSpeed');
  const currentValue = parseFloat(input.value);
  const min = parseFloat(input.min);
  const max = parseFloat(input.max);
  const nextValue = Math.max(min, Math.min(max, Math.round((currentValue + delta) * 10) / 10));
  input.value = nextValue;
  updateGifSpeed();
}

function updateGifSpeed() {
  gifSpeed = parseFloat(document.getElementById('gifSpeed').value);
}

function buildAdjustedFramePixels(sourceCanvas) {
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
  tempCtx.drawImage(sourceCanvas, offsetX, offsetY, width, height);
  return tempCtx.getImageData(0, 0, PANEL_WIDTH, PANEL_HEIGHT).data;
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
      if (
        currentRgba[index] !== previousRgba[index] ||
        currentRgba[index + 1] !== previousRgba[index + 1] ||
        currentRgba[index + 2] !== previousRgba[index + 2]
      ) {
        pixels.push([
          x,
          y,
          currentRgba[index],
          currentRgba[index + 1],
          currentRgba[index + 2],
        ]);
      }
    }
  }
  return pixels;
}

function buildCompactDataForCurrentMaterial() {
  if (!uploadedImage) {
    return null;
  }

  if (!isGifFile || gifFrames.length === 0) {
    const rgbaData = buildAdjustedFramePixels(uploadedImage);
    const pixels = rgbaToCompactPixels(rgbaData);
    return {
      f: 1,
      d: [
        {
          t: 1,
          d: 100,
          c: pixels.length,
          p: pixels,
        },
      ],
    };
  }

  const frameLimit = gifFrames.length;
  const compactFrames = [];
  let previousRgba = null;

  for (let i = 0; i < frameLimit; i++) {
    const rgbaData = buildAdjustedFramePixels(gifFrames[i].canvas);
    const fullPixels = rgbaToCompactPixels(rgbaData);
    let compactFrame = {
      t: 1,
      d: Math.max(50, Math.round(gifFrames[i].delay / gifSpeed)),
      c: fullPixels.length,
      p: fullPixels,
    };

    if (previousRgba !== null) {
      const diffPixels = diffCompactPixels(rgbaData, previousRgba);
      if (diffPixels.length <= fullPixels.length * 0.5) {
        compactFrame = {
          t: 0,
          d: Math.max(50, Math.round(gifFrames[i].delay / gifSpeed)),
          c: diffPixels.length,
          p: diffPixels,
        };
      }
    }

    compactFrames.push(compactFrame);
    previousRgba = rgbaData;
  }

  return {
    f: compactFrames.length,
    d: compactFrames,
  };
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
      const index = (pixel[1] * PANEL_WIDTH + pixel[0]) * 4;
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
    });
  }

  return frames;
}

function applySavedMaterialPreset(compactData, presetIndex) {
  const previewFrames = buildPreviewFramesFromCompactData(compactData);
  gifFrames = previewFrames;
  currentFrameIndex = 0;
  isGifFile = compactData.f > 1;
  selectedSavedPresetIndex = presetIndex;
  document.getElementById('materialName').value = compactData.name;
  document.getElementById('imageWidth').value = 64;
  document.getElementById('imageHeight').value = 64;
  document.getElementById('imageX').value = 0;
  document.getElementById('imageY').value = 0;
  uploadedImage = previewFrames[0].canvas;
  showImagePreview(previewFrames[0].canvas.toDataURL());
  if (isGifFile) {
    showGifControls();
    updateGifFrameInfo();
  } else {
    hideGifControls();
  }
  renderSavedMaterialPresetList();
  updateDisplay();
}

async function saveMaterialPreset() {
  const materialName = getMaterialName();
  if (materialName.length === 0) {
    alert('请先输入素材名称');
    return;
  }

  const compactData = buildCompactDataForCurrentMaterial();
  if (compactData === null) {
    alert('请先上传图片或 GIF');
    return;
  }

  const payload = {
    name: materialName,
    f: compactData.f,
    d: compactData.d,
  };

  try {
    const response = await fetch('/api/animation-presets/append', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    });
    const result = await response.json();
    if (!response.ok) {
      alert(`保存失败: ${result.error}`);
      return;
    }
    await loadSavedMaterialPresets();
    applySavedMaterialPreset(payload, result.index);
    setStatus(`保存成功\n素材名称: ${materialName}\n第 ${result.index + 1} 条素材已追加到 animation-presets.json`);
  } catch (error) {
    alert('保存失败: 请通过 preset_server.py 打开页面');
  }
}

async function deleteMaterialPreset(index) {
  try {
    const response = await fetch('/api/animation-presets/delete', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ index }),
    });
    const result = await response.json();
    if (!response.ok) {
      alert(`删除失败: ${result.error}`);
      return;
    }

    if (selectedSavedPresetIndex === index) {
      selectedSavedPresetIndex = -1;
    } else if (selectedSavedPresetIndex > index) {
      selectedSavedPresetIndex -= 1;
    }

    await loadSavedMaterialPresets();
    setStatus(`删除成功\n剩余素材: ${result.count}`);
  } catch (error) {
    alert('删除失败: 请通过 preset_server.py 打开页面');
  }
}

function renderSavedMaterialPresetList() {
  const list = document.getElementById('savedPresetList');
  list.innerHTML = '';

  if (savedMaterialPresets.length === 0) {
    list.innerHTML = '<div class="preset-empty">还没有保存的素材</div>';
    return;
  }

  for (let i = 0; i < savedMaterialPresets.length; i++) {
    const preset = savedMaterialPresets[i];
    const item = document.createElement('div');
    item.className = 'preset-item';
    if (i === selectedSavedPresetIndex) {
      item.classList.add('active');
    }
    item.onclick = () => applySavedMaterialPreset(preset, i);

    const thumb = document.createElement('canvas');
    thumb.className = 'preset-thumb';
    thumb.width = 64;
    thumb.height = 64;
    const thumbCtx = thumb.getContext('2d');
    const previewFrames = buildPreviewFramesFromCompactData(preset);
    thumbCtx.imageSmoothingEnabled = false;
    thumbCtx.drawImage(previewFrames[0].canvas, 0, 0, 64, 64);

    const totalPixels = preset.d.reduce((sum, frame) => sum + frame.c, 0);
    const meta = document.createElement('div');
    meta.className = 'preset-meta';
    meta.innerHTML = `
      <div class="preset-name">${preset.name}</div>
      <div class="preset-desc">帧数 ${preset.f} / 像素 ${totalPixels}</div>
    `;

    const actions = document.createElement('div');
    actions.className = 'preset-actions';
    const deleteButton = document.createElement('button');
    deleteButton.className = 'danger-btn';
    deleteButton.textContent = '删除';
    deleteButton.onclick = (event) => {
      event.stopPropagation();
      deleteMaterialPreset(i);
    };
    actions.appendChild(deleteButton);

    item.appendChild(thumb);
    item.appendChild(meta);
    item.appendChild(actions);
    list.appendChild(item);
  }
}

async function loadSavedMaterialPresets() {
  const list = document.getElementById('savedPresetList');
  try {
    const response = await fetch('/api/animation-presets');
    const result = await response.json();
    if (!response.ok) {
      list.innerHTML = `<div class="preset-error">读取失败: ${result.error}</div>`;
      return;
    }
    savedMaterialPresets = result;
    renderSavedMaterialPresetList();
  } catch (error) {
    list.innerHTML = '<div class="preset-error">请使用 preset_server.py 打开当前页面</div>';
  }
}

clearCanvas();
loadSavedMaterialPresets();
