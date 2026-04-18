#include "websocket_mode_command_dispatch.h"
#include "config_manager.h"
#include "display_manager.h"

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}
}

namespace WebSocketModeCommandDispatch {
bool handleModeUtilityCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "clear") {
    DisplayManager::dma_display->clearScreen();

    if (DisplayManager::currentMode == MODE_CANVAS && DisplayManager::canvasInitialized) {
      Serial.println("清空画布缓冲区");
      DisplayManager::clearCanvas();
    }

    response["message"] = "cleared";
    return true;
  }

  if (cmd == "brightness") {
    int value = doc["value"];
    if (value >= 0 && value <= 255) {
      DisplayManager::setBrightness(value);
      ConfigManager::saveDeviceParamsConfig();
      response["brightness"] = value;
    } else {
      setErrorResponse(response, "brightness must be 0-255");
    }
    return true;
  }

  if (cmd == "start_loading") {
    DisplayManager::startLoadingAnimation();
    response["status"] = "ok";
    response["message"] = "loading started";
    return true;
  }

  if (cmd == "stop_loading") {
    DisplayManager::stopLoadingAnimation();
    DisplayManager::dma_display->clearScreen();
    response["status"] = "ok";
    response["message"] = "loading stopped";
    return true;
  }

  if (cmd == "show_loading") {
    DisplayManager::dma_display->clearScreen();

    int x = 12;
    int y = 18;
    int bs = 11;
    int gap = 3;
    int step = bs + gap;

    uint16_t orange = DisplayManager::dma_display->color565(249, 115, 22);
    uint16_t red = DisplayManager::dma_display->color565(239, 68, 68);
    uint16_t yellow = DisplayManager::dma_display->color565(251, 191, 36);
    uint16_t blue = DisplayManager::dma_display->color565(59, 130, 246);

    uint16_t grid[3][3] = {
      { orange, orange, red },
      { orange, yellow, yellow },
      { orange, blue, blue }
    };

    for (int row = 0; row < 3; row++) {
      for (int col = 0; col < 3; col++) {
        uint8_t r = ((grid[row][col] >> 11) & 0x1F) * 255 / 31 * 0.3;
        uint8_t g = ((grid[row][col] >> 5) & 0x3F) * 255 / 63 * 0.3;
        uint8_t b = (grid[row][col] & 0x1F) * 255 / 31 * 0.3;
        uint16_t dimColor = DisplayManager::dma_display->color565(r, g, b);
        DisplayManager::dma_display->fillRect(x + col * step, y + row * step, bs, bs, dimColor);
      }
    }

    int stepIndex = doc["step"] | 0;
    stepIndex = stepIndex % 8;
    int outer[8][2] = {
      {0, 0}, {0, 1}, {0, 2}, {1, 2},
      {2, 2}, {2, 1}, {2, 0}, {1, 0}
    };

    int row = outer[stepIndex][0];
    int col = outer[stepIndex][1];
    DisplayManager::dma_display->fillRect(x + col * step, y + row * step, bs, bs, grid[row][col]);

    response["message"] = "loading displayed";
    return true;
  }

  (void)client;
  return false;
}
}
