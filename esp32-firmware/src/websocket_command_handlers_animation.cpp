#include "websocket_command_handlers.h"
#include "animation_manager.h"
#include "display_manager.h"
#include "mode_tags.h"
#include "runtime_command_bus.h"
#include "runtime_mode_coordinator.h"
#include "websocket_handler.h"

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}

void abortTransferWithError(StaticJsonDocument<768>& response, const char* message, const char* restoreReason) {
  setErrorResponse(response, message);
  RuntimeCommandBus::enqueueAbortTransientTransferAndRestore(restoreReason);
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
    if (!doc.containsKey("frameCount")) {
      abortTransferWithError(response, "missing frameCount", "动画初始化字段缺失，恢复稳定态");
      return true;
    }

    int frameCount = doc["frameCount"].as<int>();
    if (AnimationManager::beginAnimation(frameCount)) {
      response["status"] = "success";
      response["message"] = "ready to receive frames";
      response["frameCount"] = frameCount;
    } else {
      abortTransferWithError(response, "animation init failed", "动画初始化失败，恢复稳定态");
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
    if (!doc.containsKey("index") ||
        !doc.containsKey("type") ||
        !doc.containsKey("delay") ||
        !doc.containsKey("totalPixels")) {
      abortTransferWithError(response, "missing frame fields", "帧初始化字段缺失，恢复稳定态");
      return true;
    }

    int index = doc["index"].as<int>();
    int type = doc["type"].as<int>();
    int delay = doc["delay"].as<int>();
    int totalPixels = doc["totalPixels"].as<int>();
    if (AnimationManager::beginFrame(index, type, delay, totalPixels)) {
      response["status"] = "success";
      response["message"] = "frame initialized";
      response["index"] = index;
    } else {
      abortTransferWithError(response, "frame init failed", "帧初始化失败，恢复稳定态");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_status") {
    if (!doc.containsKey("index")) {
      abortTransferWithError(response, "missing frame status index", "帧状态查询字段缺失，恢复稳定态");
      return true;
    }

    int index = doc["index"].as<int>();
    int count = 0;
    if (AnimationManager::frameStatus(index, count)) {
      response["status"] = "success";
      response["message"] = "frame status";
      response["index"] = index;
      response["count"] = count;
      Serial.printf("帧 %d 状态提交成功: %d 像素\n", index, count);
    } else {
      abortTransferWithError(response, "frame status mismatch", "帧像素数量不完整，恢复稳定态");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_chunk") {
    if (!doc.containsKey("index") || !doc.containsKey("pixels")) {
      abortTransferWithError(response, "missing chunk fields", "帧 chunk 字段缺失，恢复稳定态");
      return true;
    }

    int index = doc["index"].as<int>();
    JsonArray pixels = doc["pixels"].as<JsonArray>();
    if (AnimationManager::addFrameChunk(index, pixels)) {
      response["status"] = "success";
      response["message"] = "chunk received";
      response["index"] = index;
    } else {
      abortTransferWithError(response, "chunk failed", "帧 chunk 追加失败，恢复稳定态");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "animation_end") {
    AnimationManager::receivingFrameIndex = -1;
    if (AnimationManager::endAnimation()) {
      response["status"] = "success";
      response["message"] = "animation upload ready";
    } else {
      abortTransferWithError(response, "animation finalize failed", "动画上传收尾失败，恢复稳定态");
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
