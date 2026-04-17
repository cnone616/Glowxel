#include "websocket_command_handlers.h"
#include "config_manager.h"
#include "display_manager.h"

namespace {
void setErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}
}

bool WebSocketCommandHandlers::handleCanvasCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<768>& response
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "save_canvas") {
    ConfigManager::saveCanvasPixels();
    response["message"] = "canvas saved";
    return true;
  }

  if (cmd == "load_canvas") {
    ConfigManager::loadCanvasPixels();
    if (DisplayManager::currentMode == MODE_CANVAS) {
      DisplayManager::renderCanvas();
    }
    response["message"] = "canvas loaded";
    response["initialized"] = DisplayManager::canvasInitialized;
    return true;
  }

  if (cmd == "clear_canvas") {
    DisplayManager::clearCanvas();
    ConfigManager::clearCanvasPixels();
    if (DisplayManager::currentMode == MODE_CANVAS) {
      DisplayManager::dma_display->clearScreen();
    }
    response["message"] = "canvas cleared";
    return true;
  }

  if (cmd == "highlight_color") {
    if (DisplayManager::currentMode == MODE_CANVAS && DisplayManager::canvasInitialized) {
      bool hasHighlight = doc.containsKey("color") && !doc["color"].isNull();
      uint8_t highlightR = 0;
      uint8_t highlightG = 0;
      uint8_t highlightB = 0;

      if (hasHighlight) {
        JsonObject color = doc["color"];
        highlightR = color["r"] | 0;
        highlightG = color["g"] | 0;
        highlightB = color["b"] | 0;

        if (highlightR == 0 && highlightG == 0 && highlightB == 0) {
          DisplayManager::highlightCanvasColor(0, 0, 0);
        } else {
          DisplayManager::highlightCanvasColor(highlightR, highlightG, highlightB);
        }
      } else {
        DisplayManager::renderCanvas();
      }

      response["message"] = hasHighlight ? "color highlighted" : "highlight cleared";
    } else {
      setErrorResponse(response, "not in canvas mode or canvas not initialized");
    }
    return true;
  }

  if (cmd == "highlight_row") {
    if (DisplayManager::currentMode == MODE_CANVAS && DisplayManager::canvasInitialized) {
      int highlightRow = doc["row"] | -1;
      DisplayManager::highlightCanvasRow(highlightRow);
      response["message"] = (highlightRow >= 0) ? "row highlighted" : "highlight cleared";
    } else {
      setErrorResponse(response, "not in canvas mode or canvas not initialized");
    }
    return true;
  }

  if (cmd == "text") {
    String text = doc["text"].as<String>();
    int x = doc["x"] | 0;
    int y = doc["y"] | 0;
    DisplayManager::dma_display->clearScreen();
    DisplayManager::dma_display->setCursor(x, y);
    DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(255, 255, 255));
    DisplayManager::dma_display->print(text);
    response["message"] = "text displayed";
    return true;
  }

  if (cmd == "pixel") {
    int x = doc["x"];
    int y = doc["y"];
    int r = doc["r"] | 255;
    int g = doc["g"] | 255;
    int b = doc["b"] | 255;
    if (x >= 0 && x < DisplayManager::PANEL_RES_X && y >= 0 && y < DisplayManager::PANEL_RES_Y) {
      DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
      response["message"] = "pixel set";
    } else {
      setErrorResponse(response, "pixel out of range");
    }
    return true;
  }

  if (cmd == "image") {
    JsonArray pixels = doc["pixels"];
    int width = doc["width"] | DisplayManager::PANEL_RES_X;
    int height = doc["height"] | DisplayManager::PANEL_RES_Y;

    DisplayManager::dma_display->clearScreen();
    int idx = 0;
    for (int y = 0; y < height && y < DisplayManager::PANEL_RES_Y; y++) {
      for (int x = 0; x < width && x < DisplayManager::PANEL_RES_X; x++) {
        if (idx + 2 < pixels.size()) {
          uint8_t r = pixels[idx++];
          uint8_t g = pixels[idx++];
          uint8_t b = pixels[idx++];
          DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
        }
      }
      if (y % 8 == 0) {
        yield();
      }
    }
    response["message"] = "image displayed";
    return true;
  }

  if (cmd == "image_sparse") {
    JsonArray pixels = doc["pixels"];

    for (JsonVariant pixel : pixels) {
      int x = pixel["x"];
      int y = pixel["y"];
      uint8_t r = pixel["r"];
      uint8_t g = pixel["g"];
      uint8_t b = pixel["b"];

      if (x >= 0 && x < DisplayManager::PANEL_RES_X && y >= 0 && y < DisplayManager::PANEL_RES_Y) {
        DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
      }
    }

    response["message"] = "sparse pixels drawn";
    response["count"] = pixels.size();
    return true;
  }

  if (cmd == "image_chunk") {
    JsonArray pixels = doc["pixels"];
    int width = doc["width"];
    int offsetX = doc["offsetX"] | 0;
    int offsetY = doc["offsetY"] | 0;
    int height = doc["height"];
    int chunk = doc["chunk"] | 0;
    int total = doc["total"] | 1;

    if (chunk == 0) {
      DisplayManager::dma_display->clearScreen();
    }

    int idx = 0;
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        if (idx + 2 < pixels.size()) {
          uint8_t r = pixels[idx++];
          uint8_t g = pixels[idx++];
          uint8_t b = pixels[idx++];

          int drawX = x + offsetX;
          int drawY = y + offsetY;

          if (drawX >= 0 && drawX < DisplayManager::PANEL_RES_X &&
              drawY >= 0 && drawY < DisplayManager::PANEL_RES_Y) {
            DisplayManager::dma_display->drawPixelRGB888(drawX, drawY, r, g, b);
          }
        }
      }
      if (y % 8 == 0) {
        yield();
      }
    }

    response["message"] = "chunk displayed";
    response["chunk"] = chunk;
    response["total"] = total;
    return true;
  }

  (void)client;
  return false;
}
