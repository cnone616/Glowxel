#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include <Arduino.h>
#include "config_manager.h"

// GIF 动画数据存储
struct AnimationFrame {
  String type;           // "full" 或 "diff"
  int delay;            // 延迟时间(毫秒)
  int pixelCount;       // 像素数量
  PixelData* pixels;    // 像素数组
};

struct GIFAnimation {
  int frameCount;
  AnimationFrame* frames;
  int currentFrame;
  unsigned long lastFrameTime;
  bool isPlaying;
};

class AnimationManager {
public:
  static void init();
  static void updateGIFAnimation();
  static void renderGIFFrame(int frameIndex);
  static void freeGIFAnimation();
  static bool loadGIFAnimation(JsonVariant animData);
  static void playAnimation();
  static void pauseAnimation();
  static void stopAnimation();

  // 逐帧接收方法（分片传输）
  static bool beginAnimation(int frameCount);
  static bool addFrame(int index, JsonVariant frameData);
  static bool beginFrame(int index, int type, int delay, int totalPixels);
  static bool addFrameChunk(int index, JsonArray pixels);
  static bool addFrameChunkBinary(int index, uint8_t *data, int pixelCount);
  static bool endAnimation();
  static bool saveAnimation();   // 保存动画到 LittleFS
  static bool loadAnimation();   // 从 LittleFS 加载动画

  static GIFAnimation* currentGIF;
  static int receivingFrameIndex;  // 当前正在接收的帧索引，-1 表示未在接收
  
private:
  // 私有辅助函数
};

#endif