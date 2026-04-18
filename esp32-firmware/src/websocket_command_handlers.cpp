#include "websocket_command_handlers.h"
#include "websocket_effect_command_dispatch.h"

bool WebSocketCommandHandlers::handleEffectCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  if (WebSocketEffectCommandDispatch::handleVisualEffectCommand(client, doc, response)) {
    return true;
  }

  if (WebSocketEffectCommandDispatch::handleBoardEffectCommand(client, doc, response)) {
    return true;
  }

  if (WebSocketEffectCommandDispatch::handleGameEffectCommand(client, doc, response)) {
    return true;
  }

  (void)client;
  return false;
}
