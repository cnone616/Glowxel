#include "websocket_command_handlers.h"

#include "config_manager.h"
#include "display_manager.h"
#include "runtime_command_bus.h"

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}
}

bool WebSocketCommandHandlers::handleCanvasCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "save_canvas") {
    ConfigManager::saveCanvasPixels();
    response["message"] = "canvas saved";
    return true;
  }

  if (cmd == "load_canvas") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::LOAD_CANVAS;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "clear_canvas") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
        setErrorResponse(response, "out of memory");
        return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::CLEAR_CANVAS;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "highlight_color") {
    if (DisplayManager::currentMode != MODE_CANVAS || !DisplayManager::canvasInitialized) {
      setErrorResponse(response, "not in canvas mode or canvas not initialized");
      return true;
    }

    bool hasHighlight = doc.containsKey("color") && !doc["color"].isNull();
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::HIGHLIGHT_COLOR;
    command->flag1 = hasHighlight;

    if (hasHighlight) {
      JsonObject color = doc["color"];
      command->intValue1 = color["r"] | 0;
      command->intValue2 = color["g"] | 0;
      command->intValue3 = color["b"] | 0;
    }

    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "highlight_row") {
    if (DisplayManager::currentMode != MODE_CANVAS || !DisplayManager::canvasInitialized) {
      setErrorResponse(response, "not in canvas mode or canvas not initialized");
      return true;
    }

    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::HIGHLIGHT_ROW;
    command->intValue1 = doc["row"] | -1;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "text") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::TEXT;
    command->stringValue1 = doc["text"].as<String>();
    command->intValue1 = doc["x"] | 0;
    command->intValue2 = doc["y"] | 0;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "pixel") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::PIXEL;
    command->intValue1 = doc["x"];
    command->intValue2 = doc["y"];
    command->intValue3 = doc["r"] | 255;
    command->intValue4 = doc["g"] | 255;
    command->intValue5 = doc["b"] | 255;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "image") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::IMAGE;
    serializeJson(doc, command->rawJsonPayload);
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "image_sparse") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::IMAGE_SPARSE;
    serializeJson(doc, command->rawJsonPayload);
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "image_chunk") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::IMAGE_CHUNK;
    serializeJson(doc, command->rawJsonPayload);
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  return false;
}
