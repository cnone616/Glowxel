#include "animation_manager.h"
#include "display_manager.h"
#include <LittleFS.h>

namespace {
constexpr const char* kAnimationFilePath = "/anim.bin";
constexpr const char* kAnimationUploadFilePath = "/anim.upload.bin";
constexpr const char* kAnimationBackupFilePath = "/anim.prev.bin";
constexpr uint16_t kMaxAnimationFrames = 20;
constexpr uint16_t kMaxFullFramePixels = 4096;
constexpr uint16_t kMaxDiffFramePixels = 1024;

struct AnimationUploadSession {
  bool active;
  bool readyToActivate;
  int expectedFrameCount;
  int committedFrameCount;
  int inFlightFrameIndex;
  int lastCommittedFrameIndex;
  int lastCommittedPixelCount;
  AnimationFrame inFlightFrame;
  File file;
};

AnimationUploadSession s_uploadSession = {};
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

uint16_t maxPixelsForFrameTypeInt(int type) {
  return type == 1 ? kMaxFullFramePixels : kMaxDiffFramePixels;
}

void resetLoopBridgeState() {
  s_loopBlendActive = false;
  s_loopBlendStep = 0;
  s_loopBlendDelay = 0;
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

void closeUploadFileIfOpen() {
  if (s_uploadSession.file) {
    s_uploadSession.file.flush();
    s_uploadSession.file.close();
  }
}

void clearUploadSessionState(bool removeTempFiles) {
  closeUploadFileIfOpen();
  resetAnimationFrame(s_uploadSession.inFlightFrame);
  s_uploadSession.active = false;
  s_uploadSession.readyToActivate = false;
  s_uploadSession.expectedFrameCount = 0;
  s_uploadSession.committedFrameCount = 0;
  s_uploadSession.inFlightFrameIndex = -1;
  s_uploadSession.lastCommittedFrameIndex = -1;
  s_uploadSession.lastCommittedPixelCount = 0;
  AnimationManager::receivingFrameIndex = -1;

  if (!removeTempFiles) {
    return;
  }
  if (!ensureLittleFSMounted()) {
    return;
  }
  LittleFS.remove(kAnimationUploadFilePath);
  LittleFS.remove(kAnimationBackupFilePath);
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

bool hasReadableAnimationFile(const char* path) {
  if (!ensureLittleFSMounted()) {
    return false;
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    return false;
  }

  size_t fileSize = file.size();
  file.close();
  return fileSize >= 2;
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

bool appendPixelToFrame(AnimationFrame& frame, uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
  if (x >= DisplayManager::PANEL_RES_X || y >= DisplayManager::PANEL_RES_Y) {
    return true;
  }
  if (frame.pixelCount >= frame.capacity) {
    return false;
  }
  if (frame.pixels == nullptr && frame.capacity > 0) {
    return false;
  }

  if (frame.capacity == 0) {
    return false;
  }

  frame.pixels[frame.pixelCount].x = x;
  frame.pixels[frame.pixelCount].y = y;
  frame.pixels[frame.pixelCount].r = r;
  frame.pixels[frame.pixelCount].g = g;
  frame.pixels[frame.pixelCount].b = b;
  frame.pixelCount++;
  return true;
}

bool appendPixelToUploadFile(
  AnimationFrame& frame,
  File& file,
  uint8_t x,
  uint8_t y,
  uint8_t r,
  uint8_t g,
  uint8_t b
) {
  if (x >= DisplayManager::PANEL_RES_X || y >= DisplayManager::PANEL_RES_Y) {
    return true;
  }
  if (frame.pixelCount >= frame.capacity) {
    return false;
  }
  if (frame.capacity == 0 || !file) {
    return false;
  }

  uint8_t buf[5] = {x, y, r, g, b};
  if (file.write(buf, sizeof(buf)) != sizeof(buf)) {
    return false;
  }

  frame.pixelCount++;
  return true;
}

void activateLoadedGif(GIFAnimation* nextGIF) {
  AnimationManager::freeGIFAnimation();
  AnimationManager::currentGIF = nextGIF;
  resetLoopBridgeState();
}

bool restoreRuntimeAnimationFromPath(const char* path) {
  GIFAnimation* restoredGIF = nullptr;
  if (!loadGIFAnimationFromPath(path, restoredGIF)) {
    return false;
  }

  activateLoadedGif(restoredGIF);
  return true;
}
}

// 静态成员初始化
GIFAnimation* AnimationManager::currentGIF = nullptr;
int AnimationManager::receivingFrameIndex = -1;

void AnimationManager::init() {
  currentGIF = nullptr;
  receivingFrameIndex = -1;
  clearUploadSessionState(true);
  resetLoopBridgeState();

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
  if (frameCount <= 0 || frameCount > kMaxAnimationFrames) {
    Serial.printf("帧数无效: %d (最大%d)\n", frameCount, kMaxAnimationFrames);
    return false;
  }
  if (!ensureLittleFSMounted()) {
    return false;
  }

  clearUploadSessionState(true);

  File file = LittleFS.open(kAnimationUploadFilePath, "w");
  if (!file) {
    Serial.println("打开临时动画文件失败");
    return false;
  }

  uint16_t headerFrameCount = static_cast<uint16_t>(frameCount);
  if (file.write(reinterpret_cast<uint8_t*>(&headerFrameCount), 2) != 2) {
    file.close();
    LittleFS.remove(kAnimationUploadFilePath);
    return false;
  }
  file.flush();

  s_uploadSession.active = true;
  s_uploadSession.readyToActivate = false;
  s_uploadSession.expectedFrameCount = frameCount;
  s_uploadSession.committedFrameCount = 0;
  s_uploadSession.inFlightFrameIndex = -1;
  s_uploadSession.lastCommittedFrameIndex = -1;
  s_uploadSession.lastCommittedPixelCount = 0;
  resetAnimationFrame(s_uploadSession.inFlightFrame);
  s_uploadSession.file = file;
  receivingFrameIndex = -1;

  Serial.printf("动画上传会话已创建，准备接收 %d 帧\n", frameCount);
  return true;
}

bool AnimationManager::addFrame(int index, JsonVariant frameData) {
  if (currentGIF == nullptr || index < 0 || index >= currentGIF->frameCount) {
    Serial.printf("添加帧失败: GIF=%p, index=%d\n", currentGIF, index);
    return false;
  }

  JsonArray frameArray = frameData.as<JsonArray>();
  if (frameArray.size() < 4) {
    Serial.printf("帧 %d 数据格式错误\n", index);
    return false;
  }

  AnimationFrame& frame = currentGIF->frames[index];
  resetAnimationFrame(frame);

  frame.type = frameArray[0].as<int>() == 1 ? "full" : "diff";
  frame.delay = frameArray[1] | 100;
  frame.pixelCount = frameArray[2] | 0;
  frame.capacity = frame.pixelCount;

  uint16_t maxPixels = maxPixelsForFrameType(frame.type);
  if (frame.pixelCount > maxPixels) {
    Serial.printf("帧 %d 像素过多 (%d)，限制为 %d\n", index, frame.pixelCount, maxPixels);
    frame.pixelCount = maxPixels;
    frame.capacity = maxPixels;
  }

  if (frame.pixelCount > 0) {
    frame.pixels = static_cast<PixelData*>(malloc(sizeof(PixelData) * frame.pixelCount));
    if (frame.pixels == nullptr) {
      Serial.printf("帧 %d 像素内存分配失败\n", index);
      frame.pixelCount = 0;
      frame.capacity = 0;
      return false;
    }
  }

  JsonArray pixels = frameArray[3];
  for (int j = 0; j < frame.pixelCount && j < static_cast<int>(pixels.size()); j++) {
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

bool AnimationManager::beginFrame(int index, int type, int delay, int totalPixels) {
  if (!s_uploadSession.active || s_uploadSession.readyToActivate) {
    Serial.println("未处于可接收动画帧的上传会话");
    return false;
  }
  if (index < 0 || index >= s_uploadSession.expectedFrameCount) {
    Serial.printf("帧索引无效: %d\n", index);
    return false;
  }
  if (s_uploadSession.inFlightFrameIndex >= 0) {
    Serial.printf("上一帧 %d 尚未提交，拒绝并发初始化新帧\n", s_uploadSession.inFlightFrameIndex);
    return false;
  }
  if (index != s_uploadSession.committedFrameCount) {
    Serial.printf("帧顺序错误: 期望 %d，收到 %d\n", s_uploadSession.committedFrameCount, index);
    return false;
  }
  if (totalPixels < 0) {
    Serial.printf("帧 %d totalPixels 无效: %d\n", index, totalPixels);
    return false;
  }
  if (type != 0 && type != 1) {
    Serial.printf("帧 %d type 无效: %d\n", index, type);
    return false;
  }
  if (delay < 0) {
    Serial.printf("帧 %d delay 无效: %d\n", index, delay);
    return false;
  }

  uint16_t maxPixels = maxPixelsForFrameTypeInt(type);
  if (totalPixels > maxPixels) {
    Serial.printf("帧 %d totalPixels 超限: %d > %d\n", index, totalPixels, maxPixels);
    return false;
  }

  resetAnimationFrame(s_uploadSession.inFlightFrame);
  s_uploadSession.inFlightFrame.type = type == 1 ? "full" : "diff";
  s_uploadSession.inFlightFrame.delay = delay;
  s_uploadSession.inFlightFrame.pixelCount = 0;
  s_uploadSession.inFlightFrame.capacity = totalPixels;

  if (!s_uploadSession.file) {
    Serial.printf("帧 %d 初始化失败：上传文件未打开\n", index);
    resetAnimationFrame(s_uploadSession.inFlightFrame);
    return false;
  }

  if (!writeFrameHeaderToFile(
        s_uploadSession.file,
        s_uploadSession.inFlightFrame,
        static_cast<uint16_t>(totalPixels))) {
    Serial.printf("帧 %d 写入帧头失败\n", index);
    resetAnimationFrame(s_uploadSession.inFlightFrame);
    return false;
  }

  s_uploadSession.inFlightFrameIndex = index;
  receivingFrameIndex = index;

  Serial.printf("帧 %d 初始化: %s, 精确容量 %d 像素, %dms延迟\n",
    index,
    s_uploadSession.inFlightFrame.type.c_str(),
    totalPixels,
    s_uploadSession.inFlightFrame.delay
  );
  return true;
}

bool AnimationManager::addFrameChunk(int index, JsonArray pixels) {
  if (!s_uploadSession.active || index != s_uploadSession.inFlightFrameIndex) {
    Serial.printf("帧 %d 未初始化，无法追加 chunk\n", index);
    return false;
  }

  AnimationFrame& frame = s_uploadSession.inFlightFrame;
  int added = 0;

  for (size_t j = 0; j < pixels.size(); j++) {
    JsonArray pixel = pixels[j];
    if (pixel.size() < 5) {
      continue;
    }

    if (!appendPixelToUploadFile(
          frame,
          s_uploadSession.file,
          pixel[0].as<uint8_t>(),
          pixel[1].as<uint8_t>(),
          pixel[2].as<uint8_t>(),
          pixel[3].as<uint8_t>(),
          pixel[4].as<uint8_t>())) {
      return false;
    }
    added++;
  }

  Serial.printf("帧 %d chunk: +%d 像素, 累计 %d/%d\n",
    index, added, frame.pixelCount, frame.capacity);
  yield();
  return true;
}

bool AnimationManager::addFrameChunkBinary(int index, uint8_t *data, int pixelCount) {
  if (!s_uploadSession.active || index != s_uploadSession.inFlightFrameIndex) {
    Serial.printf("帧 %d 未初始化，无法追加 binary chunk\n", index);
    return false;
  }

  AnimationFrame& frame = s_uploadSession.inFlightFrame;
  int added = 0;

  for (int j = 0; j < pixelCount; j++) {
    int offset = j * 5;
    if (!appendPixelToUploadFile(
          frame,
          s_uploadSession.file,
          data[offset],
          data[offset + 1],
          data[offset + 2],
          data[offset + 3],
          data[offset + 4])) {
      return false;
    }

    if (data[offset] < DisplayManager::PANEL_RES_X &&
        data[offset + 1] < DisplayManager::PANEL_RES_Y) {
      added++;
    }

    if (j % 50 == 0) {
      yield();
    }
  }

  Serial.printf("帧 %d binary chunk: +%d 像素, 累计 %d/%d\n",
    index, added, frame.pixelCount, frame.capacity);
  return true;
}

bool AnimationManager::frameStatus(int index, int& outCount) {
  outCount = 0;
  if (!s_uploadSession.active) {
    return false;
  }

  if (index == s_uploadSession.lastCommittedFrameIndex &&
      s_uploadSession.inFlightFrameIndex != index) {
    outCount = s_uploadSession.lastCommittedPixelCount;
    return true;
  }

  if (index != s_uploadSession.inFlightFrameIndex) {
    return false;
  }

  AnimationFrame& frame = s_uploadSession.inFlightFrame;
  outCount = frame.pixelCount;
  if (frame.pixelCount != frame.capacity) {
    return false;
  }
  if (!s_uploadSession.file) {
    return false;
  }
  s_uploadSession.file.flush();

  s_uploadSession.committedFrameCount++;
  s_uploadSession.lastCommittedFrameIndex = index;
  s_uploadSession.lastCommittedPixelCount = frame.pixelCount;
  resetAnimationFrame(frame);
  s_uploadSession.inFlightFrameIndex = -1;
  receivingFrameIndex = -1;

  Serial.printf("帧 %d 已提交到临时文件，累计 %d/%d 帧\n",
    index,
    s_uploadSession.committedFrameCount,
    s_uploadSession.expectedFrameCount
  );
  return true;
}

bool AnimationManager::endAnimation() {
  if (!s_uploadSession.active) {
    Serial.println("没有可结束的动画上传会话");
    return false;
  }
  if (s_uploadSession.readyToActivate) {
    return false;
  }
  if (s_uploadSession.inFlightFrameIndex >= 0) {
    Serial.printf("仍有未提交帧 %d，拒绝结束上传\n", s_uploadSession.inFlightFrameIndex);
    return false;
  }
  if (s_uploadSession.committedFrameCount != s_uploadSession.expectedFrameCount) {
    Serial.printf("动画上传不完整: %d/%d 帧\n",
      s_uploadSession.committedFrameCount,
      s_uploadSession.expectedFrameCount
    );
    return false;
  }

  closeUploadFileIfOpen();
  s_uploadSession.readyToActivate = true;

  Serial.printf("动画上传已完成，等待最终模式切换: %d 帧\n",
    s_uploadSession.expectedFrameCount);
  return true;
}

bool AnimationManager::saveAnimation() {
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

bool AnimationManager::hasPendingUploadReady() {
  return s_uploadSession.active && s_uploadSession.readyToActivate;
}

bool AnimationManager::isUploadSessionActive() {
  return s_uploadSession.active;
}

bool AnimationManager::finalizeUploadSession() {
  if (!hasPendingUploadReady()) {
    return false;
  }
  closeUploadFileIfOpen();

  const bool hadExistingAnimation =
    AnimationManager::currentGIF != nullptr &&
    AnimationManager::currentGIF->frameCount > 0 &&
    hasReadableAnimationFile(kAnimationFilePath);

  if (AnimationManager::currentGIF != nullptr) {
    Serial.printf("动画提交前释放旧动画内存: %d 帧\n",
      AnimationManager::currentGIF->frameCount);
    AnimationManager::freeGIFAnimation();
  }

  GIFAnimation* nextGIF = nullptr;
  if (!loadGIFAnimationFromPath(kAnimationUploadFilePath, nextGIF)) {
    Serial.printf("动画提交失败：上传文件加载失败 %s\n", kAnimationUploadFilePath);
    if (hadExistingAnimation && restoreRuntimeAnimationFromPath(kAnimationFilePath)) {
      Serial.println("新动画加载失败，已恢复旧动画到运行态");
    }
    return false;
  }
  if (!ensureLittleFSMounted()) {
    Serial.println("动画提交失败：LittleFS 不可用");
    freeGIFAnimationInstance(nextGIF);
    if (hadExistingAnimation && restoreRuntimeAnimationFromPath(kAnimationFilePath)) {
      Serial.println("文件系统不可用，已恢复旧动画到运行态");
    }
    return false;
  }

  if (LittleFS.exists(kAnimationBackupFilePath)) {
    LittleFS.remove(kAnimationBackupFilePath);
  }

  if (hadExistingAnimation && !LittleFS.rename(kAnimationFilePath, kAnimationBackupFilePath)) {
    Serial.printf("动画提交失败：旧动画备份失败 %s -> %s\n",
      kAnimationFilePath,
      kAnimationBackupFilePath);
    freeGIFAnimationInstance(nextGIF);
    if (restoreRuntimeAnimationFromPath(kAnimationFilePath)) {
      Serial.println("旧动画备份失败，已恢复旧动画到运行态");
    }
    return false;
  }

  if (!LittleFS.rename(kAnimationUploadFilePath, kAnimationFilePath)) {
    Serial.printf("动画提交失败：上传文件提交失败 %s -> %s\n",
      kAnimationUploadFilePath,
      kAnimationFilePath);
    if (hadExistingAnimation && LittleFS.exists(kAnimationBackupFilePath)) {
      LittleFS.rename(kAnimationBackupFilePath, kAnimationFilePath);
    }
    freeGIFAnimationInstance(nextGIF);
    if (hadExistingAnimation && restoreRuntimeAnimationFromPath(kAnimationFilePath)) {
      Serial.println("新动画文件提交失败，已恢复旧动画到运行态");
    }
    return false;
  }

  if (hadExistingAnimation && LittleFS.exists(kAnimationBackupFilePath)) {
    LittleFS.remove(kAnimationBackupFilePath);
  }

  activateLoadedGif(nextGIF);
  clearUploadSessionState(false);
  Serial.printf("动画上传事务提交成功: %d 帧\n", currentGIF->frameCount);
  return true;
}

void AnimationManager::abortUploadSession() {
  if (!s_uploadSession.active) {
    return;
  }
  Serial.println("动画上传会话已中止");
  clearUploadSessionState(true);
}
