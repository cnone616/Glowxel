#include "websocket_mode_command_dispatch.h"
#include "config_manager.h"
#include "display_manager.h"
#include "runtime_command_bus.h"
#include "runtime_mode_coordinator.h"

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}
}

namespace WebSocketModeCommandDispatch {
bool handleModeUtilityCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "clear") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::CLEAR;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "brightness") {
    int value = doc["value"];
    if (value >= 0 && value <= 255) {
      RuntimeCommandBus::RuntimeCommand* command =
        RuntimeCommandBus::createCommand(
          RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
          client != nullptr ? client->id() : 0
        );
      if (command == nullptr) {
        setErrorResponse(response, "out of memory");
        return true;
      }
      command->type = RuntimeCommandBus::RuntimeCommandType::BRIGHTNESS;
      command->intValue1 = value;
      if (!RuntimeCommandBus::enqueue(command)) {
        RuntimeCommandBus::destroyCommand(command);
        setErrorResponse(response, "device busy");
        return true;
      }
      responseSent = true;
      return true;
    }
    setErrorResponse(response, "brightness must be 0-255");
    return true;
  }

  if (cmd == "start_loading") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::START_LOADING;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "stop_loading") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::STOP_LOADING;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  if (cmd == "show_loading") {
    RuntimeCommandBus::RuntimeCommand* command =
      RuntimeCommandBus::createCommand(
        RuntimeCommandBus::RuntimeCommandSource::WEBSOCKET,
        client != nullptr ? client->id() : 0
      );
    if (command == nullptr) {
      setErrorResponse(response, "out of memory");
      return true;
    }
    command->type = RuntimeCommandBus::RuntimeCommandType::SHOW_LOADING;
    command->intValue1 = doc["step"] | 0;
    if (!RuntimeCommandBus::enqueue(command)) {
      RuntimeCommandBus::destroyCommand(command);
      setErrorResponse(response, "device busy");
      return true;
    }
    responseSent = true;
    return true;
  }

  (void)client;
  return false;
}
}
