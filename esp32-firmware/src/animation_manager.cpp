#include "animation_manager.h"
#include "display_manager.h"
#include <LittleFS.h>

// 静态成员初始化
GIFAnimation* AnimationManager::currentGIF = nullptr;
int AnimationManager::receivingFrameIndex = -1;

// 上一帧像素缓冲，用于 full 帧时清除残留
static PixelData* s_prevPixels = nullptr;
static int s_prevPixelCount = 0;

void AnimationManager::init() {
  // 动画管理器初始化
  currentGIF = nullptr;
  receivingFrameIndex = -1;

  // 尝试从 LittleFS 加载保存的动画
  if (loadAnimation() && currentGIF != nullptr) {
    Serial.printf("已恢复保存的动画: %d 帧\n", currentGIF->frameCount);
    // 只在 ConfigManager 恢复的模式确实是 ANIMATION 时才自动播放
    // 不要覆盖 currentMode，模式由 ConfigManager::loadClockConfig() 恢复
    if (DisplayManager::currentMode == MODE_ANIMATION) {
      currentGIF->isPlaying = true;
      currentGIF->currentFrame = 0;
      currentGIF->lastFrameTime = millis();
      renderGIFFrame(0);
    }
  }
}

void AnimationManager::updateGIFAnimation() {
  if (currentGIF == nullptr || !currentGIF->isPlaying) return;
  if (currentGIF->frameCount <= 1) return;  // 单帧无需更新

  unsigned long currentTime = millis();
  AnimationFrame& frame = currentGIF->frames[currentGIF->currentFrame];

  // 限制最低帧间隔50ms, 防止过快刷新
  unsigned long frameDelay = frame.delay < 50 ? 50 : frame.delay;

  // 检查是否需要切换到下一帧
  if (currentTime - currentGIF->lastFrameTime >= frameDelay) {
    // 切换到下一帧（帧0已在首次播放时渲染，后续循环跳过帧0避免全屏重绘闪烁）
    int nextFrame = (currentGIF->currentFrame + 1) % currentGIF->frameCount;
    currentGIF->currentFrame = nextFrame;

    // 渲染当前帧（diff帧只更新变化的像素，不会闪）
    renderGIFFrame(currentGIF->currentFrame);
    currentGIF->lastFrameTime = currentTime;
  }
}

void AnimationManager::renderGIFFrame(int frameIndex) {
  if (currentGIF == nullptr || frameIndex >= currentGIF->frameCount) return;

  AnimationFrame& frame = currentGIF->frames[frameIndex];

  if (frame.type == "full") {
    // full 帧：直接覆盖，不先擦除（避免闪烁）
    // 更新 prevPixels 缓冲（用于跟踪上一帧覆盖的区域）
    if (s_prevPixels != nullptr) free(s_prevPixels);
    s_prevPixelCount = frame.pixelCount;
    s_prevPixels = (PixelData*)malloc(sizeof(PixelData) * s_prevPixelCount);
    if (s_prevPixels) memcpy(s_prevPixels, frame.pixels, sizeof(PixelData) * s_prevPixelCount);
  }

  // 绘制当前帧像素
  for (int i = 0; i < frame.pixelCount; i++) {
    PixelData& pixel = frame.pixels[i];
    if (pixel.x < DisplayManager::PANEL_RES_X && pixel.y < DisplayManager::PANEL_RES_Y) {
      DisplayManager::dma_display->drawPixelRGB888(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);
    }
  }

  // 动画帧上叠加时钟显示
  DisplayManager::drawClockOverlay();
}

void AnimationManager::freeGIFAnimation() {
  if (currentGIF != nullptr) {
    for (int i = 0; i < currentGIF->frameCount; i++) {
      if (currentGIF->frames[i].pixels != nullptr) {
        free(currentGIF->frames[i].pixels);
      }
    }
    free(currentGIF->frames);
    free(currentGIF);
    currentGIF = nullptr;
  }
  // 释放上一帧缓冲
  if (s_prevPixels != nullptr) {
    free(s_prevPixels);
    s_prevPixels = nullptr;
    s_prevPixelCount = 0;
  }
}

bool AnimationManager::loadGIFAnimation(JsonVariant animData) {
  // 释放旧的动画数据
  freeGIFAnimation();

  // 分配新的动画数据
  currentGIF = (GIFAnimation*)malloc(sizeof(GIFAnimation));
  if (currentGIF == nullptr) {
    return false;
  }

  // 检测数据格式
  if (animData.is<JsonArray>()) {
    // 新的超紧凑数组格式：[[type, delay, count, pixels], ...]
    JsonArray frames = animData.as<JsonArray>();
    currentGIF->frameCount = frames.size();

    // 限制最大帧数
    if (currentGIF->frameCount > 20) {
      Serial.printf("警告：帧数过多 (%d)，限制为 20 帧\n", currentGIF->frameCount);
      currentGIF->frameCount = 20;
    }

    currentGIF->currentFrame = 0;
    currentGIF->lastFrameTime = millis();
    currentGIF->isPlaying = false;

    // 分配帧数组
    currentGIF->frames = (AnimationFrame*)malloc(sizeof(AnimationFrame) * currentGIF->frameCount);
    if (currentGIF->frames == nullptr) {
      free(currentGIF);
      currentGIF = nullptr;
      return false;
    }

    bool success = true;
    int totalPixels = 0;

    // 解析每一帧 [type, delay, count, pixels]
    for (int i = 0; i < currentGIF->frameCount && i < frames.size(); i++) {
      JsonArray frameArray = frames[i];
      if (frameArray.size() < 4) continue;

      AnimationFrame& frame = currentGIF->frames[i];

      // 解析数组格式：[type, delay, count, pixels]
      frame.type = (frameArray[0].as<int>() == 1) ? "full" : "diff";
      frame.delay = frameArray[1] | 100;
      frame.pixelCount = frameArray[2] | 0;

      // 限制每帧像素数量（全帧最多4096=64x64，差分帧最多1024）
      int maxPixels = (frame.type == "full") ? 4096 : 1024;
      if (frame.pixelCount > maxPixels) {
        Serial.printf("警告：帧 %d 像素过多 (%d)，限制为 %d\n", i, frame.pixelCount, maxPixels);
        frame.pixelCount = maxPixels;
      }

      totalPixels += frame.pixelCount;

      // 检查总内存使用量（全屏4096 + 差分帧，给足空间）
      if (totalPixels > 20000) {
        Serial.printf("警告：总像素数过多，截断到帧 %d\n", i);
        currentGIF->frameCount = i;
        break;
      }

      // 分配像素数组
      frame.pixels = (PixelData*)malloc(sizeof(PixelData) * frame.pixelCount);
      if (frame.pixels == nullptr) {
        Serial.printf("帧 %d 像素内存分配失败\n", i);
        success = false;
        break;
      }

      // 解析像素数组 [[x,y,r,g,b], ...]
      JsonArray pixels = frameArray[3];
      for (int j = 0; j < frame.pixelCount && j < pixels.size(); j++) {
        JsonArray pixel = pixels[j];
        if (pixel.size() >= 5) {
          frame.pixels[j].x = pixel[0];
          frame.pixels[j].y = pixel[1];
          frame.pixels[j].r = pixel[2];
          frame.pixels[j].g = pixel[3];
          frame.pixels[j].b = pixel[4];
        }
      }

      yield(); // 每帧处理完让出CPU
    }

    if (success) {
      Serial.printf("紧凑GIF动画加载完成: %d 帧, 总计 %d 像素\n", currentGIF->frameCount, totalPixels);
      return true;
    } else {
      freeGIFAnimation();
      return false;
    }
  }

  // 不支持其他格式
  freeGIFAnimation();
  return false;
}

void AnimationManager::playAnimation() {
  if (currentGIF != nullptr) {
    currentGIF->isPlaying = true;
    currentGIF->lastFrameTime = millis();
  }
}

void AnimationManager::pauseAnimation() {
  if (currentGIF != nullptr) {
    currentGIF->isPlaying = false;
  }
}

void AnimationManager::stopAnimation() {
  if (currentGIF != nullptr) {
    currentGIF->isPlaying = false;
    currentGIF->currentFrame = 0;
    DisplayManager::dma_display->clearScreen();
  }
}

// === 逐帧接收方法（分片传输） ===

bool AnimationManager::beginAnimation(int frameCount) {
  // 释放旧动画
  freeGIFAnimation();

  if (frameCount <= 0 || frameCount > 20) {
    Serial.printf("帧数无效: %d (最大20)\n", frameCount);
    return false;
  }

  // 分配新动画结构
  currentGIF = (GIFAnimation*)malloc(sizeof(GIFAnimation));
  if (currentGIF == nullptr) {
    Serial.println("动画结构内存分配失败");
    return false;
  }

  currentGIF->frameCount = frameCount;
  currentGIF->currentFrame = 0;
  currentGIF->lastFrameTime = millis();
  currentGIF->isPlaying = false;

  // 分配帧数组
  currentGIF->frames = (AnimationFrame*)calloc(frameCount, sizeof(AnimationFrame));
  if (currentGIF->frames == nullptr) {
    Serial.println("帧数组内存分配失败");
    free(currentGIF);
    currentGIF = nullptr;
    return false;
  }

  // 初始化所有帧
  for (int i = 0; i < frameCount; i++) {
    currentGIF->frames[i].pixels = nullptr;
    currentGIF->frames[i].pixelCount = 0;
    currentGIF->frames[i].delay = 100;
    currentGIF->frames[i].type = "full";
  }

  Serial.printf("动画初始化完成，准备接收 %d 帧\n", frameCount);
  return true;
}

bool AnimationManager::addFrame(int index, JsonVariant frameData) {
  if (currentGIF == nullptr || index < 0 || index >= currentGIF->frameCount) {
    Serial.printf("添加帧失败: GIF=%p, index=%d\n", currentGIF, index);
    return false;
  }

  // frameData 格式: [type, delay, count, [[x,y,r,g,b], ...]]
  JsonArray frameArray = frameData.as<JsonArray>();
  if (frameArray.size() < 4) {
    Serial.printf("帧 %d 数据格式错误\n", index);
    return false;
  }

  AnimationFrame& frame = currentGIF->frames[index];

  // 释放旧像素数据（如果有）
  if (frame.pixels != nullptr) {
    free(frame.pixels);
    frame.pixels = nullptr;
  }

  frame.type = (frameArray[0].as<int>() == 1) ? "full" : "diff";
  frame.delay = frameArray[1] | 100;
  frame.pixelCount = frameArray[2] | 0;

  // 全帧最多4096=64x64，差分帧最多1024
  int maxPixels = (frame.type == "full") ? 4096 : 1024;
  if (frame.pixelCount > maxPixels) {
    Serial.printf("帧 %d 像素过多 (%d)，限制为 %d\n", index, frame.pixelCount, maxPixels);
    frame.pixelCount = maxPixels;
  }

  // 分配像素数组
  frame.pixels = (PixelData*)malloc(sizeof(PixelData) * frame.pixelCount);
  if (frame.pixels == nullptr) {
    Serial.printf("帧 %d 像素内存分配失败\n", index);
    frame.pixelCount = 0;
    return false;
  }

  // 解析像素
  JsonArray pixels = frameArray[3];
  for (int j = 0; j < frame.pixelCount && j < (int)pixels.size(); j++) {
    JsonArray pixel = pixels[j];
    if (pixel.size() >= 5) {
      frame.pixels[j].x = pixel[0];
      frame.pixels[j].y = pixel[1];
      frame.pixels[j].r = pixel[2];
      frame.pixels[j].g = pixel[3];
      frame.pixels[j].b = pixel[4];
    }
  }

  Serial.printf("帧 %d 加载完成: %s, %d像素, %dms延迟\n",
    index, frame.type.c_str(), frame.pixelCount, frame.delay);

  yield();
  return true;
}

bool AnimationManager::endAnimation() {
  if (currentGIF == nullptr) return false;

  // 验证所有帧都已加载
  int loadedFrames = 0;
  int totalPixels = 0;
  for (int i = 0; i < currentGIF->frameCount; i++) {
    if (currentGIF->frames[i].pixels != nullptr) {
      loadedFrames++;
      totalPixels += currentGIF->frames[i].pixelCount;
    }
  }

  Serial.printf("动画加载完成: %d/%d 帧, 总计 %d 像素\n",
    loadedFrames, currentGIF->frameCount, totalPixels);

  if (loadedFrames == 0) {
    freeGIFAnimation();
    return false;
  }

  // 更新实际帧数
  currentGIF->frameCount = loadedFrames;
  return true;
}

bool AnimationManager::beginFrame(int index, int type, int delay, int totalPixels) {
  if (currentGIF == nullptr || index < 0 || index >= currentGIF->frameCount) {
    Serial.printf("beginFrame失败: GIF=%p, index=%d\n", currentGIF, index);
    return false;
  }

  // 打印可用内存
  Serial.printf("可用内存: %d bytes, 需要分配: %d bytes\n",
    ESP.getFreeHeap(), sizeof(PixelData) * totalPixels);

  AnimationFrame& frame = currentGIF->frames[index];

  // 释放旧像素数据
  if (frame.pixels != nullptr) {
    free(frame.pixels);
    frame.pixels = nullptr;
  }

  frame.type = (type == 1) ? "full" : "diff";
  frame.delay = delay > 0 ? delay : 100;
  frame.pixelCount = 0;  // 实际像素数，由 addFrameChunk 累加

  // 预分配总像素空间（直接用前端传的实际像素数）
  if (totalPixels <= 0) totalPixels = 1;
  if (totalPixels > 4096) totalPixels = 4096;  // 安全上限

  frame.pixels = (PixelData*)malloc(sizeof(PixelData) * totalPixels);
  if (frame.pixels == nullptr) {
    Serial.printf("帧 %d 像素内存分配失败 (%d)\n", index, totalPixels);
    return false;
  }

  receivingFrameIndex = index;  // 标记当前正在接收此帧的二进制数据

  Serial.printf("帧 %d 初始化: %s, 预分配 %d 像素, %dms延迟\n",
    index, frame.type.c_str(), totalPixels, frame.delay);
  return true;
}

bool AnimationManager::addFrameChunk(int index, JsonArray pixels) {
  if (currentGIF == nullptr || index < 0 || index >= currentGIF->frameCount) {
    return false;
  }

  AnimationFrame& frame = currentGIF->frames[index];
  if (frame.pixels == nullptr) {
    Serial.printf("帧 %d 未初始化，无法追加chunk\n", index);
    return false;
  }

  int maxPixels = (frame.type == "full") ? 4096 : 1024;
  int added = 0;

  for (size_t j = 0; j < pixels.size() && frame.pixelCount < maxPixels; j++) {
    JsonArray pixel = pixels[j];
    if (pixel.size() >= 5) {
      frame.pixels[frame.pixelCount].x = pixel[0];
      frame.pixels[frame.pixelCount].y = pixel[1];
      frame.pixels[frame.pixelCount].r = pixel[2];
      frame.pixels[frame.pixelCount].g = pixel[3];
      frame.pixels[frame.pixelCount].b = pixel[4];
      frame.pixelCount++;
      added++;
    }
  }

  Serial.printf("帧 %d chunk: +%d 像素, 累计 %d\n", index, added, frame.pixelCount);
  yield();
  return true;
}

bool AnimationManager::addFrameChunkBinary(int index, uint8_t *data, int pixelCount) {
  if (currentGIF == nullptr || index < 0 || index >= currentGIF->frameCount) {
    return false;
  }

  AnimationFrame& frame = currentGIF->frames[index];
  if (frame.pixels == nullptr) {
    Serial.printf("帧 %d 未初始化，无法追加binary chunk\n", index);
    return false;
  }

  int maxPixels = (frame.type == "full") ? 4096 : 1024;
  int added = 0;

  for (int j = 0; j < pixelCount && frame.pixelCount < maxPixels; j++) {
    int offset = j * 5;
    uint8_t x = data[offset];
    uint8_t y = data[offset + 1];
    uint8_t r = data[offset + 2];
    uint8_t g = data[offset + 3];
    uint8_t b = data[offset + 4];

    if (x < DisplayManager::PANEL_RES_X && y < DisplayManager::PANEL_RES_Y) {
      frame.pixels[frame.pixelCount].x = x;
      frame.pixels[frame.pixelCount].y = y;
      frame.pixels[frame.pixelCount].r = r;
      frame.pixels[frame.pixelCount].g = g;
      frame.pixels[frame.pixelCount].b = b;
      frame.pixelCount++;
      added++;
    }

    // 每处理 50 个像素让出 CPU，防止看门狗超时
    if (j % 50 == 0) yield();
  }

  Serial.printf("帧 %d binary chunk: +%d 像素, 累计 %d\n", index, added, frame.pixelCount);
  return true;
}

// 文件格式: [4字节帧数] + 每帧 [1字节type + 2字节delay + 2字节pixelCount + pixelCount*5字节像素]
bool AnimationManager::saveAnimation() {
  if (currentGIF == nullptr || currentGIF->frameCount <= 0) return false;

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS 挂载失败");
    return false;
  }

  File f = LittleFS.open("/anim.bin", "w");
  if (!f) {
    Serial.println("打开动画文件写入失败");
    return false;
  }

  // 写帧数
  uint16_t frameCount = currentGIF->frameCount;
  f.write((uint8_t*)&frameCount, 2);

  for (int i = 0; i < currentGIF->frameCount; i++) {
    AnimationFrame& frame = currentGIF->frames[i];
    uint8_t type = (frame.type == "full") ? 1 : 0;
    uint16_t delay = frame.delay;
    uint16_t pixelCount = frame.pixelCount;

    f.write(&type, 1);
    f.write((uint8_t*)&delay, 2);
    f.write((uint8_t*)&pixelCount, 2);

    // 写像素数据 (每个5字节: x,y,r,g,b)
    for (int j = 0; j < frame.pixelCount; j++) {
      uint8_t buf[5] = {
        frame.pixels[j].x, frame.pixels[j].y,
        frame.pixels[j].r, frame.pixels[j].g, frame.pixels[j].b
      };
      f.write(buf, 5);
    }
    yield();
  }

  size_t fileSize = f.size();
  f.close();
  Serial.printf("动画已保存: %d 帧, %d bytes\n", frameCount, fileSize);
  return true;
}

bool AnimationManager::loadAnimation() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS 挂载失败");
    return false;
  }

  if (!LittleFS.exists("/anim.bin")) {
    Serial.println("没有保存的动画数据");
    return false;
  }

  File f = LittleFS.open("/anim.bin", "r");
  if (!f) {
    Serial.println("打开动画文件读取失败");
    return false;
  }

  // 读帧数
  uint16_t frameCount = 0;
  f.read((uint8_t*)&frameCount, 2);

  if (frameCount == 0 || frameCount > 30) {
    Serial.printf("无效帧数: %d\n", frameCount);
    f.close();
    return false;
  }

  // 释放旧动画
  freeGIFAnimation();

  // 分配新动画（统一用 malloc/calloc，与 freeGIFAnimation 中的 free 配对）
  currentGIF = (GIFAnimation*)malloc(sizeof(GIFAnimation));
  if (currentGIF == nullptr) {
    f.close();
    return false;
  }
  memset(currentGIF, 0, sizeof(GIFAnimation));
  currentGIF->frameCount = frameCount;
  currentGIF->frames = (AnimationFrame*)calloc(frameCount, sizeof(AnimationFrame));
  if (currentGIF->frames == nullptr) {
    free(currentGIF);
    currentGIF = nullptr;
    f.close();
    return false;
  }
  currentGIF->currentFrame = 0;
  currentGIF->lastFrameTime = 0;
  currentGIF->isPlaying = false;

  for (int i = 0; i < frameCount; i++) {
    uint8_t type = 0;
    uint16_t delay = 100;
    uint16_t pixelCount = 0;

    f.read(&type, 1);
    f.read((uint8_t*)&delay, 2);
    f.read((uint8_t*)&pixelCount, 2);

    currentGIF->frames[i].type = (type == 1) ? "full" : "diff";
    currentGIF->frames[i].delay = delay;
    currentGIF->frames[i].pixelCount = pixelCount;
    currentGIF->frames[i].pixels = nullptr;

    if (pixelCount > 0 && pixelCount <= 4096) {
      currentGIF->frames[i].pixels = (PixelData*)malloc(sizeof(PixelData) * pixelCount);
      if (currentGIF->frames[i].pixels) {
        for (int j = 0; j < pixelCount; j++) {
          uint8_t buf[5];
          f.read(buf, 5);
          currentGIF->frames[i].pixels[j].x = buf[0];
          currentGIF->frames[i].pixels[j].y = buf[1];
          currentGIF->frames[i].pixels[j].r = buf[2];
          currentGIF->frames[i].pixels[j].g = buf[3];
          currentGIF->frames[i].pixels[j].b = buf[4];
        }
      }
    }
    yield();
  }

  f.close();
  Serial.printf("动画已加载: %d 帧\n", frameCount);
  return true;
}