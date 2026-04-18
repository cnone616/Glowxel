#ifndef WEBSOCKET_EFFECT_COMMAND_DISPATCH_H
#define WEBSOCKET_EFFECT_COMMAND_DISPATCH_H

#include <ArduinoJson.h>

class AsyncWebSocketClient;

namespace WebSocketEffectCommandDispatch {
  bool handleVisualEffectCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<768>& response
  );

  bool handleBoardEffectCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<768>& response
  );

  bool handleGameEffectCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<768>& response
  );
}

#endif
