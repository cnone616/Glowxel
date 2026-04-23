#include "websocket_command_handlers.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "config_manager.h"
#include "display_manager.h"
#include "game_screensaver_effect.h"
#include "device_mode_tag_codec.h"
#include "mode_tags.h"
#include "ota_manager.h"
#include "tetris_effect.h"
#include "wifi_manager.h"
#include "ambient_preset_codec.h"

bool WebSocketCommandHandlers::handleBasicCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  const char* currentMode
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "ping") {
    response["message"] = "pong";
    return true;
  }

  if (cmd == "get_info") {
    response["firmware_version"] = FIRMWARE_VERSION;
    response["device_type"] = DEVICE_TYPE;
    response["ip"] = WiFiManager::getDeviceIP();
    response["free_heap"] = ESP.getFreeHeap();
    response["uptime"] = millis() / 1000;
    response["has_update"] = OTAManager::hasNewVersion();
    return true;
  }

  (void)client;
  (void)currentMode;
  return false;
}
