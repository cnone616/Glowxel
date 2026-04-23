#include "websocket_command_handlers.h"
#include "websocket_mode_command_dispatch.h"

bool WebSocketCommandHandlers::handleModeCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (WebSocketModeCommandDispatch::handleModeSwitchCommand(client, doc, response, responseSent)) {
    return true;
  }

  if (WebSocketModeCommandDispatch::handleModeUtilityCommand(client, doc, response, responseSent)) {
    return true;
  }

  return false;
}
