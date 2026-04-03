#ifndef WEBSOCKET_COMMAND_HANDLERS_H
#define WEBSOCKET_COMMAND_HANDLERS_H

#include <Arduino.h>
#include <AsyncWebSocket.h>
#include <ArduinoJson.h>

namespace WebSocketCommandHandlers {
  bool handleBasicCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<512>& response,
    const char* currentMode
  );

  bool handleModeCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<512>& response
  );

  bool handleClockCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<512>& response
  );

  bool handleAnimationCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<512>& response,
    bool& responseSent
  );

  bool handleCanvasCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<512>& response
  );

  bool handleEffectCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<512>& response
  );

  bool handleEyesCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<512>& response
  );

  bool handleOtaCommand(
    AsyncWebSocketClient* client,
    JsonDocument& doc,
    StaticJsonDocument<512>& response,
    bool& responseSent
  );
}

#endif
