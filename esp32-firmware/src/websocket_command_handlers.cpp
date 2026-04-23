#include "websocket_command_handlers.h"
#include "websocket_effect_command_dispatch.h"

bool WebSocketCommandHandlers::handleEffectCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  if (WebSocketEffectCommandDispatch::handleVisualEffectCommand(client, doc, response, responseSent)) {
    return true;
  }

  if (WebSocketEffectCommandDispatch::handleBoardEffectCommand(client, doc, response, responseSent)) {
    return true;
  }

  if (WebSocketEffectCommandDispatch::handleGameEffectCommand(client, doc, response, responseSent)) {
    return true;
  }

  (void)client;
  return false;
}
