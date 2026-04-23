#ifndef WEBSOCKET_EFFECT_COMMAND_DISPATCH_H
#define WEBSOCKET_EFFECT_COMMAND_DISPATCH_H

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

namespace WebSocketEffectCommandDispatch {
  bool handleVisualEffectCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<768>& response,
    bool& responseSent
  );

  bool handleBoardEffectCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<768>& response,
    bool& responseSent
  );

  bool handleGameEffectCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<768>& response,
    bool& responseSent
  );
}

#endif
