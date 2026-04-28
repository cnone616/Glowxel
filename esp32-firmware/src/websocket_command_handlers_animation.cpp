#include "websocket_command_handlers.h"
#include "animation_manager.h"
#include "websocket_handler.h"
#include "websocket_async_command_response.h"

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
    if (!doc.containsKey("animationData")) {
      if (client == nullptr) {
        setErrorResponse(response, "client unavailable");
        return true;
      }

      WebSocketHandler::prepareBinaryAnimationUpload(client->id());
      return wsSendAcceptedResponse(client, response, responseSent);
    }

    JsonVariant animData = doc["animationData"];

    if (animData.isNull()) {
      AnimationManager::freeGIFAnimation();
      if (!AnimationManager::saveAnimation()) {
        setErrorResponse(response, "animation persistence failed");
      } else {
        response["status"] = "success";
        response["message"] = "animation cleared";
      }
    } else if (AnimationManager::loadGIFAnimation(animData)) {
      if (!AnimationManager::saveAnimation()) {
        setErrorResponse(response, "animation persistence failed");
      } else {
        response["status"] = "success";
        response["message"] = "compact animation loaded";
        response["frameCount"] = AnimationManager::currentGIF->frameCount;
      }
    } else {
      setErrorResponse(response, "animation load failed");
    }
    return true;
  }

  if (cmd == "animation_begin") {
    if (client == nullptr) {
      setErrorResponse(response, "client unavailable");
      return true;
    }
    if (!doc.containsKey("frameCount") || !doc["frameCount"].is<int>()) {
      setErrorResponse(response, "invalid frameCount");
      return true;
    }

    int frameCount = doc["frameCount"].as<int>();
    WebSocketHandler::prepareStreamingAnimationUpload(client->id());
    if (AnimationManager::beginAnimation(frameCount)) {
      response["status"] = "success";
      response["message"] = "ready to receive frames";
      response["frameCount"] = frameCount;
    } else {
      setErrorResponse(response, "animation init failed");
      WebSocketHandler::restoreAfterAnimationUploadFailure();
    }
    return true;
  }

  if (cmd == "frame_begin") {
    if (!doc.containsKey("index") ||
        !doc.containsKey("type") ||
        !doc.containsKey("delay") ||
        !doc.containsKey("totalPixels") ||
        !doc["index"].is<int>() ||
        !doc["type"].is<int>() ||
        !doc["delay"].is<int>() ||
        !doc["totalPixels"].is<int>()) {
      setErrorResponse(response, "invalid frame fields");
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
      setErrorResponse(response, "frame init failed");
      response["index"] = index;
      WebSocketHandler::restoreAfterAnimationUploadFailure();
    }
    return true;
  }

  if (cmd == "frame_status") {
    if (!doc.containsKey("index") || !doc["index"].is<int>()) {
      setErrorResponse(response, "invalid frame index");
      return true;
    }

    int index = doc["index"].as<int>();
    if (AnimationManager::currentGIF == nullptr ||
        index < 0 ||
        index >= AnimationManager::currentGIF->frameCount) {
      setErrorResponse(response, "frame not found");
      response["index"] = index;
      WebSocketHandler::restoreAfterAnimationUploadFailure();
      return true;
    }

    AnimationFrame& frame = AnimationManager::currentGIF->frames[index];
    if (frame.pixelCount != frame.capacity) {
      setErrorResponse(response, "frame incomplete");
      response["index"] = index;
      response["count"] = frame.pixelCount;
      WebSocketHandler::restoreAfterAnimationUploadFailure();
      return true;
    }

    response["status"] = "success";
    response["message"] = "frame status";
    response["index"] = index;
    response["count"] = frame.pixelCount;
    return true;
  }

  if (cmd == "animation_end") {
    if (AnimationManager::endAnimation()) {
      if (!AnimationManager::saveAnimation()) {
        setErrorResponse(response, "animation persistence failed");
        WebSocketHandler::restoreAfterAnimationUploadFailure();
      } else {
        WebSocketHandler::clearStreamingAnimationUpload(client != nullptr ? client->id() : 0);
        response["status"] = "success";
        response["message"] = "animation loaded and playing";
        response["frameCount"] =
          AnimationManager::currentGIF != nullptr ? AnimationManager::currentGIF->frameCount : 0;
      }
    } else {
      setErrorResponse(response, "animation finalize failed");
      WebSocketHandler::restoreAfterAnimationUploadFailure();
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
