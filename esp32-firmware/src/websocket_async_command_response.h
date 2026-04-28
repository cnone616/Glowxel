#ifndef WEBSOCKET_ASYNC_COMMAND_RESPONSE_H
#define WEBSOCKET_ASYNC_COMMAND_RESPONSE_H

#include <ArduinoJson.h>
#include <AsyncWebSocket.h>

#include "runtime_command_bus.h"

inline bool wsSendAcceptedResponse(
  AsyncWebSocketClient* client,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  // 新增走 RuntimeCommandBus 的异步 WebSocket 命令时，优先复用这里发送 accepted。
  // 约束：入队成功立即回 accepted，最终成功/失败继续走原有异步结果回包。
  response["status"] = "ok";
  response["message"] = "accepted";

  if (client != nullptr) {
    RuntimeCommandBus::queueOrSendClientResponse(client->id(), response);
  }

  responseSent = true;
  return true;
}

#endif
