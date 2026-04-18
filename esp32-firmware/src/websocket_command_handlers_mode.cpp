#include "websocket_command_handlers.h"
#include "websocket_mode_command_dispatch.h"

bool WebSocketCommandHandlers::handleModeCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  if (WebSocketModeCommandDispatch::handleModeSwitchCommand(client, doc, response)) {
    return true;
  }

  if (WebSocketModeCommandDispatch::handleModeUtilityCommand(client, doc, response)) {
    return true;
  }

  return false;
}
