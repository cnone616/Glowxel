#include "websocket_command_handlers.h"
#include "animation_manager.h"
#include "config_manager.h"
#include "display_manager.h"

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}
}

bool WebSocketCommandHandlers::handleAnimationCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "set_gif_animation") {
    JsonVariant animData = doc["animationData"];

    if (animData.isNull()) {
      AnimationManager::freeGIFAnimation();
      response["status"] = "success";
      response["message"] = "animation cleared";
    } else if (AnimationManager::loadGIFAnimation(animData)) {
      response["status"] = "success";
      response["message"] = "compact animation loaded";
      response["frameCount"] = AnimationManager::currentGIF->frameCount;
    } else {
      setErrorResponse(response, "animation load failed");
    }
    return true;
  }

  if (cmd == "animation_begin") {
    int frameCount = doc["frameCount"] | 0;
    DisplayManager::currentMode = MODE_ANIMATION;
    DisplayManager::setNativeEffectNone();
    if (AnimationManager::beginAnimation(frameCount)) {
      response["status"] = "success";
      response["message"] = "ready to receive frames";
      response["frameCount"] = frameCount;
    } else {
      setErrorResponse(response, "animation init failed");
    }
    return true;
  }

  if (cmd == "animation_frame") {
    int index = doc["index"] | -1;
    JsonVariant frameData = doc["frame"];
    if (AnimationManager::addFrame(index, frameData)) {
      response["status"] = "success";
      response["message"] = "frame received";
      response["index"] = index;
    } else {
      setErrorResponse(response, "frame load failed");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_begin") {
    int index = doc["index"] | -1;
    int type = doc["type"] | 1;
    int delay = doc["delay"] | 100;
    int totalPixels = doc["totalPixels"] | 0;
    if (AnimationManager::beginFrame(index, type, delay, totalPixels)) {
      response["status"] = "success";
      response["message"] = "frame initialized";
      response["index"] = index;
    } else {
      setErrorResponse(response, "frame init failed");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_status") {
    int index = doc["index"] | -1;
    if (AnimationManager::currentGIF != nullptr &&
        index >= 0 &&
        index < AnimationManager::currentGIF->frameCount) {
      AnimationFrame& frame = AnimationManager::currentGIF->frames[index];
      response["status"] = "success";
      response["message"] = "frame status";
      response["index"] = index;
      response["count"] = frame.pixelCount;
      Serial.printf("帧 %d 状态查询: %d 像素已接收\n", index, frame.pixelCount);
    } else {
      setErrorResponse(response, "frame not found");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_chunk") {
    int index = doc["index"] | -1;
    JsonArray pixels = doc["pixels"];
    if (AnimationManager::addFrameChunk(index, pixels)) {
      response["status"] = "success";
      response["message"] = "chunk received";
      response["index"] = index;
    } else {
      setErrorResponse(response, "chunk failed");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "animation_end") {
    AnimationManager::receivingFrameIndex = -1;
    if (AnimationManager::endAnimation()) {
      DisplayManager::currentMode = MODE_ANIMATION;
      AnimationManager::currentGIF->isPlaying = true;
      AnimationManager::currentGIF->currentFrame = 0;
      AnimationManager::currentGIF->lastFrameTime = millis();
      DisplayManager::dma_display->clearScreen();
      AnimationManager::renderGIFFrame(0);
      AnimationManager::saveAnimation();
      ConfigManager::saveClockConfig();

      response["status"] = "success";
      response["message"] = "animation loaded and playing";
      response["frameCount"] = AnimationManager::currentGIF->frameCount;
    } else {
      setErrorResponse(response, "animation finalize failed");
    }
    return true;
  }

  if (cmd == "control_gif") {
    String action = doc["action"];

    if (AnimationManager::currentGIF == nullptr) {
      setErrorResponse(response, "no animation loaded");
      return true;
    }

    if (action == "play") {
      AnimationManager::playAnimation();
      response["status"] = "success";
      response["message"] = "animation playing";
      return true;
    }

    if (action == "pause") {
      AnimationManager::pauseAnimation();
      response["status"] = "success";
      response["message"] = "animation paused";
      return true;
    }

    if (action == "stop") {
      AnimationManager::stopAnimation();
      response["status"] = "success";
      response["message"] = "animation stopped";
      return true;
    }

    setErrorResponse(response, "unknown action");
    return true;
  }

  (void)client;
  (void)responseSent;
  return false;
}
