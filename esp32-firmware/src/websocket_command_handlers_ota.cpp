#include "websocket_command_handlers.h"
#include "config_manager.h"
#include "ota_manager.h"

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}

void sendResponse(AsyncWebSocketClient* client, StaticJsonDocument<768>& response) {
  String responseStr;
  serializeJson(response, responseStr);
  client->text(responseStr);
}
}

bool WebSocketCommandHandlers::handleOtaCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "ota_check") {
    OTAManager::checkUpdate();
    response["firmware_version"] = FIRMWARE_VERSION;
    response["has_update"] = OTAManager::hasNewVersion();
    if (OTAManager::hasNewVersion()) {
      response["latest_version"] = OTAManager::getLatestVersion();
      response["changelog"] = OTAManager::getChangelog();
      response["is_force"] = OTAManager::isForce;
    }
    return true;
  }

  if (cmd == "ota_update") {
    if (OTAManager::hasNewVersion()) {
      response["message"] = "starting update";
      sendResponse(client, response);
      responseSent = true;
      delay(500);
      OTAManager::startUpdate();
      return true;
    }

    setErrorResponse(response, "no update available");
    return true;
  }

  if (cmd == "ota_set_server") {
    String url = doc["url"].as<String>();
    if (url.length() > 0) {
      OTAManager::serverUrl = url;
      ConfigManager::preferences.begin("ota", false);
      ConfigManager::preferences.putString("server", url);
      ConfigManager::preferences.end();
      response["message"] = "OTA server set";
      response["server"] = url;
    } else {
      setErrorResponse(response, "missing url");
    }
    return true;
  }

  return false;
}
