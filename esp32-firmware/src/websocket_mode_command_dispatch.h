#ifndef WEBSOCKET_MODE_COMMAND_DISPATCH_H
#define WEBSOCKET_MODE_COMMAND_DISPATCH_H

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

namespace WebSocketModeCommandDispatch {
  bool handleModeSwitchCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<768>& response,
    bool& responseSent
  );

  bool handleModeUtilityCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<768>& response,
    bool& responseSent
  );
}

#endif
