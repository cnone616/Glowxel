#include "websocket_command_handlers.h"

#include "display_manager.h"
#include "runtime_command_bus.h"
#include "websocket_async_command_response.h"

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

  if (cmd == "save_canvas" || cmd == "load_canvas" || cmd == "clear_canvas") {
    setErrorResponse(response, "requires transaction");
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
    return wsSendAcceptedResponse(client, response, responseSent);
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
    return wsSendAcceptedResponse(client, response, responseSent);
  }

  if (cmd == "text") {
    setErrorResponse(response, "canvas text requires transaction");
    return true;
  }

  if (cmd == "pixel") {
    setErrorResponse(response, "canvas pixel requires transaction");
    return true;
  }

  if (cmd == "image") {
    setErrorResponse(response, "canvas image requires transaction");
    return true;
  }

  if (cmd == "image_sparse") {
    setErrorResponse(response, "canvas sparse image requires transaction");
    return true;
  }

  if (cmd == "image_chunk") {
    setErrorResponse(response, "canvas image chunk requires transaction");
    return true;
  }

  return false;
}
