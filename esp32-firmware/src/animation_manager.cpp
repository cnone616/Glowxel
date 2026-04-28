#include "animation_manager.h"
#include "display_manager.h"
#include "mode_tags.h"
#include <LittleFS.h>

namespace {
constexpr const char* kAnimationFilePath = "/anim.bin";
constexpr const char* kAnimationUploadTempPath = "/anim_upload.bin";
constexpr uint16_t kMaxAnimationFrames = 20;
constexpr uint16_t kMaxFullFramePixels = 4096;
constexpr uint16_t kMaxDiffFramePixels = 1024;
bool s_littleFSMounted = false;

static bool s_loopBlendActive = false;
static int s_loopBlendStep = 0;
static unsigned long s_loopBlendDelay = 0;
static const int LOOP_BLEND_STEPS = 4;
static const int LOOP_BRIDGE_PIXELS = 64 * 64;
static uint16_t s_loopBridgeFromBuffer[LOOP_BRIDGE_PIXELS];
static uint16_t s_loopBridgeToBuffer[LOOP_BRIDGE_PIXELS];

uint16_t maxPixelsForFrameType(const String& type) {
  return type == "full" ? kMaxFullFramePixels : kMaxDiffFramePixels;
}

void resetLoopBridgeState() {
  s_loopBlendActive = false;
  s_loopBlendStep = 0;
  s_loopBlendDelay = 0;
}

void freeGIFAnimationInstance(GIFAnimation* gif) {
  if (gif == nullptr) {
    return;
  }

  if (gif->frames != nullptr) {
    for (int i = 0; i < gif->frameCount; i++) {
      if (gif->frames[i].pixels != nullptr) {
        free(gif->frames[i].pixels);
      }
    }
    free(gif->frames);
  }

  free(gif);
}

bool ensureLittleFSMounted() {
  if (s_littleFSMounted) {
    return true;
  }
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS 挂载失败");
    return false;
  }
  s_littleFSMounted = true;
  return true;
}

bool writeAnimationBlobToPath(const char* path, const uint8_t* data, size_t len) {
  if (path == nullptr || data == nullptr || len == 0) {
    return false;
  }
  if (!ensureLittleFSMounted()) {
    return false;
  }

  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.printf("打开动画临时文件写入失败: %s\n", path);
    return false;
  }

  size_t written = file.write(data, len);
  file.flush();
  file.close();
  if (written != len) {
    Serial.printf("写入动画临时文件失败: %s written=%u expected=%u\n",
                  path,
                  static_cast<unsigned>(written),
                  static_cast<unsigned>(len));
    return false;
  }

  return true;
}

bool prepareLoopBridgeFrames() {
  if (AnimationManager::currentGIF == nullptr || AnimationManager::currentGIF->frameCount <= 0) {
    return false;
  }

  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 64; x++) {
      int pos = y * 64 + x;
      s_loopBridgeFromBuffer[pos] = DisplayManager::animationBuffer[y][x];
      s_loopBridgeToBuffer[pos] = 0;
    }
  }

  AnimationFrame& firstFrame = AnimationManager::currentGIF->frames[0];
  if (firstFrame.pixels != nullptr && firstFrame.pixelCount > 0) {
    for (int i = 0; i < firstFrame.pixelCount; i++) {
      const PixelData& p = firstFrame.pixels[i];
      if (p.x < 64 && p.y < 64) {
        int pos = p.y * 64 + p.x;
        s_loopBridgeToBuffer[pos] = DisplayManager::dma_display->color565(p.r, p.g, p.b);
      }
    }
  }
  return true;
}

bool writeFrameHeaderToFile(File& file, const AnimationFrame& frame, uint16_t pixelCount) {
  uint8_t type = frame.type == "full" ? 1 : 0;
  uint16_t delay = static_cast<uint16_t>(frame.delay);

  if (file.write(&type, 1) != 1) {
    return false;
  }
  if (file.write(reinterpret_cast<uint8_t*>(&delay), 2) != 2) {
    return false;
  }
  if (file.write(reinterpret_cast<uint8_t*>(&pixelCount), 2) != 2) {
    return false;
  }

  return true;
}

bool writeFrameToFile(File& file, const AnimationFrame& frame) {
  uint16_t pixelCount = static_cast<uint16_t>(frame.pixelCount);
  if (!writeFrameHeaderToFile(file, frame, pixelCount)) {
    return false;
  }

  for (int i = 0; i < frame.pixelCount; i++) {
    uint8_t buf[5] = {
      frame.pixels[i].x,
      frame.pixels[i].y,
      frame.pixels[i].r,
      frame.pixels[i].g,
      frame.pixels[i].b
    };
    if (file.write(buf, 5) != 5) {
      return false;
    }
  }

  file.flush();
  return true;
}

bool saveGIFAnimationToPath(const char* path, GIFAnimation* gif) {
  if (gif == nullptr || gif->frameCount <= 0) {
    return false;
  }
  if (!ensureLittleFSMounted()) {
    return false;
  }

  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.println("打开动画文件写入失败");
    return false;
  }

  uint16_t frameCount = static_cast<uint16_t>(gif->frameCount);
  if (file.write(reinterpret_cast<uint8_t*>(&frameCount), 2) != 2) {
    file.close();
    return false;
  }

  for (int i = 0; i < gif->frameCount; i++) {
    if (!writeFrameToFile(file, gif->frames[i])) {
      file.close();
      return false;
    }
    yield();
  }

  size_t fileSize = file.size();
  file.close();
  Serial.printf("动画已保存: %d 帧, %d bytes\n", frameCount, fileSize);
  return true;
}

bool loadGIFAnimationFromPath(const char* path, GIFAnimation*& outGif) {
  outGif = nullptr;
  if (!ensureLittleFSMounted()) {
    return false;
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.printf("动画文件打开失败: %s\n", path);
    Serial.println("没有保存的动画数据");
    return false;
  }

  if (file.size() < 2) {
    Serial.printf("动画文件过小: %s size=%u\n", path, static_cast<unsigned>(file.size()));
    file.close();
    Serial.println("动画文件内容无效");
    return false;
  }

  uint16_t frameCount = 0;
  if (file.read(reinterpret_cast<uint8_t*>(&frameCount), 2) != 2) {
    Serial.printf("动画文件帧头读取失败: %s\n", path);
    file.close();
    return false;
  }
  if (frameCount == 0 || frameCount > 30) {
    Serial.printf("动画文件帧数非法: %s frameCount=%u\n", path, frameCount);
    Serial.printf("无效帧数: %d\n", frameCount);
    file.close();
    return false;
  }

  GIFAnimation* nextGIF = static_cast<GIFAnimation*>(malloc(sizeof(GIFAnimation)));
  if (nextGIF == nullptr) {
    Serial.printf("GIFAnimation 分配失败: %s freeHeap=%u\n", path, ESP.getFreeHeap());
    file.close();
    return false;
  }
  memset(nextGIF, 0, sizeof(GIFAnimation));

  nextGIF->frameCount = frameCount;
  nextGIF->frames = static_cast<AnimationFrame*>(calloc(frameCount, sizeof(AnimationFrame)));
  if (nextGIF->frames == nullptr) {
    Serial.printf("动画帧数组分配失败: %s frames=%u freeHeap=%u\n", path, frameCount, ESP.getFreeHeap());
    free(nextGIF);
    file.close();
    return false;
  }

  nextGIF->currentFrame = 0;
  nextGIF->lastFrameTime = 0;
  nextGIF->isPlaying = false;

  bool success = true;
  for (int i = 0; i < frameCount; i++) {
    uint8_t type = 0;
    uint16_t delay = 100;
    uint16_t pixelCount = 0;

    if (file.read(&type, 1) != 1 ||
        file.read(reinterpret_cast<uint8_t*>(&delay), 2) != 2 ||
        file.read(reinterpret_cast<uint8_t*>(&pixelCount), 2) != 2) {
      Serial.printf("动画帧头读取失败: %s frame=%d\n", path, i);
      success = false;
      break;
    }

    AnimationFrame& frame = nextGIF->frames[i];
    frame.type = type == 1 ? "full" : "diff";
    frame.delay = delay;
    frame.pixelCount = pixelCount;
    frame.capacity = pixelCount;
    frame.pixels = nullptr;

    uint16_t maxPixels = maxPixelsForFrameType(frame.type);
    if (pixelCount > maxPixels) {
      Serial.printf("动画帧像素超限: %s frame=%d type=%s pixels=%u max=%u\n",
        path,
        i,
        frame.type.c_str(),
        pixelCount,
        maxPixels);
      success = false;
      break;
    }

    if (pixelCount > 0) {
      frame.pixels = static_cast<PixelData*>(malloc(sizeof(PixelData) * pixelCount));
      if (frame.pixels == nullptr) {
        Serial.printf("动画像素缓冲分配失败: %s frame=%d pixels=%u bytes=%u freeHeap=%u\n",
          path,
          i,
          pixelCount,
          static_cast<unsigned>(sizeof(PixelData) * pixelCount),
          ESP.getFreeHeap());
        success = false;
        break;
      }

      for (int j = 0; j < pixelCount; j++) {
        uint8_t buf[5];
        if (file.read(buf, 5) != 5) {
          Serial.printf("动画像素读取失败: %s frame=%d pixel=%d\n", path, i, j);
          success = false;
          break;
        }
        frame.pixels[j].x = buf[0];
        frame.pixels[j].y = buf[1];
        frame.pixels[j].r = buf[2];
        frame.pixels[j].g = buf[3];
        frame.pixels[j].b = buf[4];
      }
      if (!success) {
        break;
      }
    }
    yield();
  }

  file.close();

  if (!success) {
    freeGIFAnimationInstance(nextGIF);
    return false;
  }

  outGif = nextGIF;
  return true;
}

void activateLoadedGif(GIFAnimation* nextGIF) {
  AnimationManager::freeGIFAnimation();
  AnimationManager::currentGIF = nextGIF;
  resetLoopBridgeState();
}

void resetAnimationFrame(AnimationFrame& frame) {
  if (frame.pixels != nullptr) {
    free(frame.pixels);
    frame.pixels = nullptr;
  }
  frame.type = "full";
  frame.delay = 100;
  frame.pixelCount = 0;
  frame.capacity = 0;
}

bool shouldLoadPersistedAnimationOnInit() {
  return DisplayManager::currentBusinessModeTag == ModeTags::ANIMATION ||
         DisplayManager::currentBusinessModeTag == ModeTags::GIF_PLAYER;
}
}

// 静态成员初始化
GIFAnimation* AnimationManager::currentGIF = nullptr;
int AnimationManager::receivingFrameIndex = -1;

void AnimationManager::init() {
  currentGIF = nullptr;
  receivingFrameIndex = -1;
  resetLoopBridgeState();

  if (!shouldLoadPersistedAnimationOnInit()) {
    return;
  }

  if (loadAnimation() && currentGIF != nullptr) {
    Serial.printf("已恢复保存的动画: %d 帧\n", currentGIF->frameCount);
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
  if (currentGIF->frameCount <= 1) {
    return;
  }

  unsigned long currentTime = millis();
  AnimationFrame& frame = currentGIF->frames[currentGIF->currentFrame];
  unsigned long frameDelay = frame.delay < 50 ? 50 : frame.delay;

  if (s_loopBlendActive) {
    if (currentTime - currentGIF->lastFrameTime >= s_loopBlendDelay) {
      s_loopBlendStep++;

      if (s_loopBlendStep > LOOP_BLEND_STEPS) {
        s_loopBlendActive = false;
        s_loopBlendStep = 0;
        currentGIF->currentFrame = 0;
      } else {
        uint8_t mix = (255 * s_loopBlendStep) / LOOP_BLEND_STEPS;
        DisplayManager::renderAnimationTransitionBuffers(
          s_loopBridgeFromBuffer,
          s_loopBridgeToBuffer,
          mix
        );
      }

      currentGIF->lastFrameTime = currentTime;
    }
    return;
  }

  if (currentTime - currentGIF->lastFrameTime >= frameDelay) {
    int nextFrame = currentGIF->currentFrame + 1;
    if (nextFrame >= currentGIF->frameCount) {
      if (prepareLoopBridgeFrames()) {
        s_loopBlendActive = true;
        s_loopBlendStep = 1;
        s_loopBlendDelay = frameDelay / (LOOP_BLEND_STEPS + 1);
        if (s_loopBlendDelay < 40) {
          s_loopBlendDelay = 40;
        }
        uint8_t mix = (255 * s_loopBlendStep) / LOOP_BLEND_STEPS;
        DisplayManager::renderAnimationTransitionBuffers(
          s_loopBridgeFromBuffer,
          s_loopBridgeToBuffer,
          mix
        );
        currentGIF->lastFrameTime = currentTime;
        return;
      }

      nextFrame = 0;
    }

    currentGIF->currentFrame = nextFrame;
    renderGIFFrame(currentGIF->currentFrame);
    currentGIF->lastFrameTime = currentTime;
  }
}

void AnimationManager::renderGIFFrame(int frameIndex) {
  if (currentGIF == nullptr || frameIndex >= currentGIF->frameCount) return;

  AnimationFrame& frame = currentGIF->frames[frameIndex];
  bool clearFirst = (frameIndex == 0) || (frame.type == "full");
  DisplayManager::renderAnimationFrame(frame.pixels, frame.pixelCount, clearFirst);
}

void AnimationManager::freeGIFAnimation() {
  freeGIFAnimationInstance(currentGIF);
  currentGIF = nullptr;
  receivingFrameIndex = -1;
  resetLoopBridgeState();
}

bool AnimationManager::loadGIFAnimation(JsonVariant animData) {
  if (!animData.is<JsonArray>()) {
    return false;
  }

  GIFAnimation* nextGIF = static_cast<GIFAnimation*>(malloc(sizeof(GIFAnimation)));
  if (nextGIF == nullptr) {
    return false;
  }
  memset(nextGIF, 0, sizeof(GIFAnimation));

  JsonArray frames = animData.as<JsonArray>();
  nextGIF->frameCount = frames.size();
  if (nextGIF->frameCount <= 0) {
    free(nextGIF);
    return false;
  }
  if (nextGIF->frameCount > kMaxAnimationFrames) {
    Serial.printf("警告：帧数过多 (%d)，限制为 %d 帧\n", nextGIF->frameCount, kMaxAnimationFrames);
    nextGIF->frameCount = kMaxAnimationFrames;
  }

  nextGIF->currentFrame = 0;
  nextGIF->lastFrameTime = millis();
  nextGIF->isPlaying = false;

  nextGIF->frames = static_cast<AnimationFrame*>(malloc(sizeof(AnimationFrame) * nextGIF->frameCount));
  if (nextGIF->frames == nullptr) {
    free(nextGIF);
    return false;
  }

  for (int i = 0; i < nextGIF->frameCount; i++) {
    nextGIF->frames[i].pixels = nullptr;
    nextGIF->frames[i].pixelCount = 0;
    nextGIF->frames[i].capacity = 0;
    nextGIF->frames[i].delay = 100;
    nextGIF->frames[i].type = "full";
  }

  bool success = true;
  int totalPixels = 0;

  for (int i = 0; i < nextGIF->frameCount && i < frames.size(); i++) {
    JsonArray frameArray = frames[i];
    if (frameArray.size() < 4) {
      success = false;
      break;
    }

    AnimationFrame& frame = nextGIF->frames[i];
    frame.type = frameArray[0].as<int>() == 1 ? "full" : "diff";
    frame.delay = frameArray[1] | 100;
    frame.pixelCount = frameArray[2] | 0;
    frame.capacity = frame.pixelCount;

    uint16_t maxPixels = maxPixelsForFrameType(frame.type);
    if (frame.pixelCount > maxPixels) {
      Serial.printf("警告：帧 %d 像素过多 (%d)，限制为 %d\n", i, frame.pixelCount, maxPixels);
      frame.pixelCount = maxPixels;
      frame.capacity = maxPixels;
    }

    totalPixels += frame.pixelCount;
    if (totalPixels > 20000) {
      Serial.printf("警告：总像素数过多，截断到帧 %d\n", i);
      nextGIF->frameCount = i;
      break;
    }

    if (frame.pixelCount > 0) {
      frame.pixels = static_cast<PixelData*>(malloc(sizeof(PixelData) * frame.pixelCount));
      if (frame.pixels == nullptr) {
        Serial.printf("帧 %d 像素内存分配失败\n", i);
        success = false;
        break;
      }
    }

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

    yield();
  }

  if (!success) {
    freeGIFAnimationInstance(nextGIF);
    return false;
  }

  if (nextGIF->frameCount <= 0) {
    freeGIFAnimationInstance(nextGIF);
    return false;
  }

  activateLoadedGif(nextGIF);
  Serial.printf("紧凑GIF动画加载完成: %d 帧, 总计 %d 像素\n", currentGIF->frameCount, totalPixels);
  return true;
}

bool AnimationManager::loadGIFAnimationBinary(const uint8_t* data, size_t len) {
  if (!stageGIFAnimationBinaryUpload(data, len)) {
    return false;
  }

  return loadStagedGIFAnimationUpload();
}

bool AnimationManager::stageGIFAnimationBinaryUpload(const uint8_t* data, size_t len) {
  if (data == nullptr || len < 2) {
    Serial.printf("动画二进制数据无效: len=%u\n", static_cast<unsigned>(len));
    return false;
  }

  if (!writeAnimationBlobToPath(kAnimationUploadTempPath, data, len)) {
    return false;
  }

  return true;
}

bool AnimationManager::loadStagedGIFAnimationUpload() {
  GIFAnimation* nextGIF = nullptr;
  bool loaded = loadGIFAnimationFromPath(kAnimationUploadTempPath, nextGIF);
  if (ensureLittleFSMounted() && LittleFS.exists(kAnimationUploadTempPath)) {
    LittleFS.remove(kAnimationUploadTempPath);
  }

  if (!loaded || nextGIF == nullptr) {
    return false;
  }

  activateLoadedGif(nextGIF);
  Serial.printf("动画二进制加载完成: %d 帧\n", currentGIF->frameCount);
  return true;
}

void AnimationManager::playAnimation() {
  if (currentGIF != nullptr) {
    currentGIF->isPlaying = true;
    currentGIF->lastFrameTime = millis();
    s_loopBlendActive = false;
    s_loopBlendStep = 0;
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
    s_loopBlendActive = false;
    s_loopBlendStep = 0;
    DisplayManager::dma_display->clearScreen();
  }
}

bool AnimationManager::beginAnimation(int frameCount) {
  freeGIFAnimation();

  if (frameCount <= 0 || frameCount > kMaxAnimationFrames) {
    Serial.printf("动画帧数无效: %d\n", frameCount);
    return false;
  }

  GIFAnimation* nextGIF = static_cast<GIFAnimation*>(malloc(sizeof(GIFAnimation)));
  if (nextGIF == nullptr) {
    Serial.println("动画结构内存分配失败");
    return false;
  }
  memset(nextGIF, 0, sizeof(GIFAnimation));

  nextGIF->frames = static_cast<AnimationFrame*>(calloc(frameCount, sizeof(AnimationFrame)));
  if (nextGIF->frames == nullptr) {
    free(nextGIF);
    Serial.println("动画帧数组内存分配失败");
    return false;
  }

  nextGIF->frameCount = frameCount;
  nextGIF->currentFrame = 0;
  nextGIF->lastFrameTime = millis();
  nextGIF->isPlaying = false;

  for (int i = 0; i < frameCount; i++) {
    nextGIF->frames[i].type = "full";
    nextGIF->frames[i].delay = 100;
    nextGIF->frames[i].pixelCount = 0;
    nextGIF->frames[i].capacity = 0;
    nextGIF->frames[i].pixels = nullptr;
  }

  currentGIF = nextGIF;
  receivingFrameIndex = -1;
  resetLoopBridgeState();
  Serial.printf("动画上传会话已创建: frameCount=%d\n", frameCount);
  return true;
}

bool AnimationManager::beginFrame(int index, int type, int delay, int totalPixels) {
  if (currentGIF == nullptr || index < 0 || index >= currentGIF->frameCount) {
    Serial.printf("动画帧初始化失败: index=%d\n", index);
    return false;
  }
  if (type != 0 && type != 1) {
    Serial.printf("动画帧类型无效: index=%d type=%d\n", index, type);
    return false;
  }
  if (delay < 0 || totalPixels < 0) {
    Serial.printf("动画帧参数无效: index=%d delay=%d totalPixels=%d\n", index, delay, totalPixels);
    return false;
  }

  const uint16_t maxPixels = type == 1 ? kMaxFullFramePixels : kMaxDiffFramePixels;
  if (totalPixels > maxPixels) {
    Serial.printf("动画帧像素超限: index=%d totalPixels=%d max=%u\n",
                  index,
                  totalPixels,
                  maxPixels);
    return false;
  }

  AnimationFrame& frame = currentGIF->frames[index];
  resetAnimationFrame(frame);
  frame.type = type == 1 ? "full" : "diff";
  frame.delay = delay;
  frame.pixelCount = 0;
  frame.capacity = totalPixels;

  if (totalPixels > 0) {
    frame.pixels = static_cast<PixelData*>(malloc(sizeof(PixelData) * totalPixels));
    if (frame.pixels == nullptr) {
      resetAnimationFrame(frame);
      Serial.printf("动画帧像素内存分配失败: index=%d pixels=%d freeHeap=%u\n",
                    index,
                    totalPixels,
                    ESP.getFreeHeap());
      return false;
    }
  }

  receivingFrameIndex = index;
  Serial.printf("动画帧已初始化: index=%d type=%s totalPixels=%d delay=%d\n",
                index,
                frame.type.c_str(),
                totalPixels,
                delay);
  return true;
}

bool AnimationManager::addFrameChunkBinary(int index, uint8_t* data, int pixelCount) {
  if (currentGIF == nullptr || index < 0 || index >= currentGIF->frameCount) {
    return false;
  }
  if (data == nullptr || pixelCount < 0) {
    return false;
  }

  AnimationFrame& frame = currentGIF->frames[index];
  if (frame.pixelCount + pixelCount > frame.capacity) {
    Serial.printf("动画帧写入越界: index=%d current=%d incoming=%d capacity=%d\n",
                  index,
                  frame.pixelCount,
                  pixelCount,
                  frame.capacity);
    return false;
  }

  for (int pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++) {
    const int offset = pixelIndex * 5;
    const uint8_t x = data[offset];
    const uint8_t y = data[offset + 1];
    const uint8_t r = data[offset + 2];
    const uint8_t g = data[offset + 3];
    const uint8_t b = data[offset + 4];

    if (x >= DisplayManager::PANEL_RES_X || y >= DisplayManager::PANEL_RES_Y) {
      Serial.printf("动画帧像素越界: index=%d x=%u y=%u\n", index, x, y);
      return false;
    }

    if (frame.capacity > 0 && frame.pixels == nullptr) {
      Serial.printf("动画帧像素缓冲缺失: index=%d capacity=%d\n", index, frame.capacity);
      return false;
    }

    frame.pixels[frame.pixelCount].x = x;
    frame.pixels[frame.pixelCount].y = y;
    frame.pixels[frame.pixelCount].r = r;
    frame.pixels[frame.pixelCount].g = g;
    frame.pixels[frame.pixelCount].b = b;
    frame.pixelCount += 1;

    if (pixelIndex % 50 == 0) {
      yield();
    }
  }

  return true;
}

bool AnimationManager::endAnimation() {
  if (currentGIF == nullptr || currentGIF->frameCount <= 0) {
    return false;
  }

  receivingFrameIndex = -1;
  for (int frameIndex = 0; frameIndex < currentGIF->frameCount; frameIndex++) {
    AnimationFrame& frame = currentGIF->frames[frameIndex];
    if (frame.pixelCount != frame.capacity) {
      Serial.printf("动画帧未接收完整: index=%d count=%d capacity=%d\n",
                    frameIndex,
                    frame.pixelCount,
                    frame.capacity);
      return false;
    }
  }

  currentGIF->currentFrame = 0;
  currentGIF->lastFrameTime = millis();
  currentGIF->isPlaying = true;
  Serial.printf("动画上传完成: frameCount=%d\n", currentGIF->frameCount);
  return true;
}

bool AnimationManager::saveAnimation() {
  if (!ensureLittleFSMounted()) {
    return false;
  }

  if (currentGIF == nullptr || currentGIF->frameCount <= 0) {
    if (!LittleFS.exists(kAnimationFilePath)) {
      return true;
    }

    if (!LittleFS.remove(kAnimationFilePath)) {
      Serial.println("清除动画文件失败");
      return false;
    }

    Serial.println("已清除保存的动画文件");
    return true;
  }

  return saveGIFAnimationToPath(kAnimationFilePath, currentGIF);
}

bool AnimationManager::loadAnimation() {
  GIFAnimation* nextGIF = nullptr;
  if (!loadGIFAnimationFromPath(kAnimationFilePath, nextGIF)) {
    return false;
  }

  activateLoadedGif(nextGIF);
  Serial.printf("动画已加载: %d 帧\n", currentGIF->frameCount);
  return true;
}
