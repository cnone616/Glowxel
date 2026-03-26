#include "websocket_command_handlers.h"
#include "animation_manager.h"
#include "clock_font_renderer.h"
#include "config_manager.h"
#include "display_manager.h"
#include "ota_manager.h"
#include "tetris_effect.h"
#include "wifi_manager.h"

namespace {
  void setErrorResponse(StaticJsonDocument<512>& response, const char* message) {
    response["status"] = "error";
    response["message"] = message;
  }

  void sendResponse(AsyncWebSocketClient* client, StaticJsonDocument<512>& response) {
    String responseStr;
    serializeJson(response, responseStr);
    client->text(responseStr);
  }
}

bool WebSocketCommandHandlers::handleBasicCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response,
  const char* currentMode
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "ping") {
    response["message"] = "pong";
    return true;
  }

  if (cmd == "status") {
    response["ip"] = WiFiManager::getDeviceIP();
    response["width"] = DisplayManager::PANEL_RES_X;
    response["height"] = DisplayManager::PANEL_RES_Y;
    response["brightness"] = DisplayManager::currentBrightness;
    response["mode"] = currentMode;
    if (DisplayManager::currentMode == MODE_ANIMATION && AnimationManager::currentGIF != nullptr) {
      response["animationFrames"] = AnimationManager::currentGIF->frameCount;
      response["animationPlaying"] = AnimationManager::currentGIF->isPlaying;
      response["currentFrame"] = AnimationManager::currentGIF->currentFrame;
    }
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

  return false;
}

bool WebSocketCommandHandlers::handleModeCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "set_mode") {
    String mode = doc["mode"].as<String>();

    TetrisEffect::isActive = false;
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    DisplayManager::receivingPixels = false;
    DisplayManager::dma_display->clearScreen();

    if (mode == "clock") {
      DisplayManager::currentMode = MODE_CLOCK;
      ConfigManager::saveClockConfig();
      DisplayManager::displayClock(true);
      response["message"] = "switched to static clock mode";
      return true;
    }

    if (mode == "canvas") {
      DisplayManager::currentMode = MODE_CANVAS;
      ConfigManager::saveClockConfig();
      DisplayManager::renderCanvas();
      response["message"] = "switched to canvas mode";
      return true;
    }

    if (mode == "animation") {
      DisplayManager::currentMode = MODE_ANIMATION;
      ConfigManager::saveClockConfig();

      if (AnimationManager::currentGIF != nullptr) {
        AnimationManager::currentGIF->isPlaying = true;
        AnimationManager::currentGIF->currentFrame = 0;
        AnimationManager::currentGIF->lastFrameTime = millis();
        AnimationManager::renderGIFFrame(0);
        response["message"] = "switched to animation mode";
      } else {
        DisplayManager::displayClock(true);
        response["message"] = "switched to animation mode (no GIF)";
      }
      return true;
    }

    if (mode == "transferring") {
      DisplayManager::currentMode = MODE_TRANSFERRING;
      DisplayManager::dma_display->clearScreen();
      Serial.println("进入传输模式，准备接收动画数据");
      response["message"] = "entered transferring mode";
      return true;
    }

    if (mode == "tetris") {
      DisplayManager::currentMode = MODE_ANIMATION;
      bool clearMode = doc["clearMode"] | true;
      int cellSz = doc["cellSize"] | 2;
      int speed = doc["speed"] | 150;
      bool clock = doc["showClock"] | true;
      uint8_t mask = 0x7F;

      if (doc.containsKey("pieces")) {
        mask = 0;
        JsonArray arr = doc["pieces"].as<JsonArray>();
        for (JsonVariant v : arr) {
          int idx = v.as<int>();
          if (idx >= 0 && idx < 7) {
            mask |= (1 << idx);
          }
        }
        if (mask == 0) {
          mask = 0x7F;
        }
      }

      TetrisEffect::init(clearMode, cellSz, speed, clock, mask);
      response["message"] = "tetris started";
      return true;
    }

    setErrorResponse(response, "invalid mode");
    return true;
  }

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

  return false;
}

bool WebSocketCommandHandlers::handleClockCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response
) {
  String cmd = doc["cmd"].as<String>();
  if (cmd != "set_clock_config") {
    return false;
  }

  if (!doc.containsKey("config") || !doc.containsKey("clockMode")) {
    setErrorResponse(response, "missing config or clockMode");
    return true;
  }

  JsonObject config = doc["config"];
  const char* clockMode = doc["clockMode"];

  if (strcmp(clockMode, "clock") != 0 && strcmp(clockMode, "animation") != 0) {
    setErrorResponse(response, "invalid clockMode");
    return true;
  }

  if (!config.containsKey("font") ||
      !config.containsKey("showSeconds") ||
      !config.containsKey("hourFormat") ||
      !config.containsKey("time") ||
      !config.containsKey("date") ||
      !config.containsKey("week") ||
      !config.containsKey("image")) {
    setErrorResponse(response, "missing required clock config fields");
    return true;
  }

  ClockConfig& target = (strcmp(clockMode, "animation") == 0)
    ? ConfigManager::animClockConfig
    : ConfigManager::clockConfig;

  const char* fontName = config["font"];
  uint8_t fontId = 0;
  if (!clockFontIdFromString(fontName, fontId)) {
    setErrorResponse(response, "invalid font");
    return true;
  }

  int hourFormat = config["hourFormat"];
  if (hourFormat != 12 && hourFormat != 24) {
    setErrorResponse(response, "invalid hourFormat");
    return true;
  }

  JsonObject time = config["time"];
  JsonObject date = config["date"];
  JsonObject week = config["week"];
  JsonObject image = config["image"];

  if (!time.containsKey("show") ||
      !time.containsKey("fontSize") ||
      !time.containsKey("x") ||
      !time.containsKey("y") ||
      !time.containsKey("color") ||
      !date.containsKey("show") ||
      !date.containsKey("fontSize") ||
      !date.containsKey("x") ||
      !date.containsKey("y") ||
      !date.containsKey("color") ||
      !week.containsKey("show") ||
      !week.containsKey("x") ||
      !week.containsKey("y") ||
      !week.containsKey("color") ||
      !image.containsKey("show") ||
      !image.containsKey("x") ||
      !image.containsKey("y") ||
      !image.containsKey("width") ||
      !image.containsKey("height")) {
    setErrorResponse(response, "missing nested clock config fields");
    return true;
  }

  JsonObject timeColor = time["color"];
  JsonObject dateColor = date["color"];
  JsonObject weekColor = week["color"];

  if (!timeColor.containsKey("r") ||
      !timeColor.containsKey("g") ||
      !timeColor.containsKey("b") ||
      !dateColor.containsKey("r") ||
      !dateColor.containsKey("g") ||
      !dateColor.containsKey("b") ||
      !weekColor.containsKey("r") ||
      !weekColor.containsKey("g") ||
      !weekColor.containsKey("b")) {
    setErrorResponse(response, "missing color fields");
    return true;
  }

  target.font = fontId;
  target.showSeconds = config["showSeconds"];
  target.hourFormat = hourFormat;

  target.time.show = time["show"];
  target.time.fontSize = time["fontSize"];
  target.time.x = time["x"];
  target.time.y = time["y"];
  target.time.r = timeColor["r"];
  target.time.g = timeColor["g"];
  target.time.b = timeColor["b"];

  target.date.show = date["show"];
  target.date.fontSize = date["fontSize"];
  target.date.x = date["x"];
  target.date.y = date["y"];
  target.date.r = dateColor["r"];
  target.date.g = dateColor["g"];
  target.date.b = dateColor["b"];

  target.week.show = week["show"];
  target.week.x = week["x"];
  target.week.y = week["y"];
  target.week.r = weekColor["r"];
  target.week.g = weekColor["g"];
  target.week.b = weekColor["b"];

  target.image.show = image["show"];
  target.image.x = image["x"];
  target.image.y = image["y"];
  target.image.width = image["width"];
  target.image.height = image["height"];

  if (strcmp(clockMode, "animation") == 0) {
    ConfigManager::saveAnimClockConfig();
  } else {
    ConfigManager::saveClockConfig();
  }

  response["message"] = "clock config updated";
  return true;
}

bool WebSocketCommandHandlers::handleAnimationCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response,
  bool& responseSent
) {
  String cmd = doc["cmd"].as<String>();

  if (cmd == "set_gif_animation") {
    JsonVariant animData = doc["animationData"];

    if (animData.isNull()) {
      AnimationManager::freeGIFAnimation();
      response["status"] = "success";
      response["message"] = "animation cleared";
    } else if (AnimationManager::loadGIFAnimation(animData)) {
      response["status"] = "success";
      response["message"] = "compact animation loaded";
      response["frameCount"] = AnimationManager::currentGIF->frameCount;
    } else {
      setErrorResponse(response, "animation load failed");
    }
    return true;
  }

  if (cmd == "animation_begin") {
    int frameCount = doc["frameCount"] | 0;
    DisplayManager::currentMode = MODE_ANIMATION;
    if (AnimationManager::beginAnimation(frameCount)) {
      response["status"] = "success";
      response["message"] = "ready to receive frames";
      response["frameCount"] = frameCount;
    } else {
      setErrorResponse(response, "animation init failed");
    }
    return true;
  }

  if (cmd == "animation_frame") {
    int index = doc["index"] | -1;
    JsonVariant frameData = doc["frame"];
    if (AnimationManager::addFrame(index, frameData)) {
      response["status"] = "success";
      response["message"] = "frame received";
      response["index"] = index;
    } else {
      setErrorResponse(response, "frame load failed");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_begin") {
    int index = doc["index"] | -1;
    int type = doc["type"] | 1;
    int delay = doc["delay"] | 100;
    int totalPixels = doc["totalPixels"] | 0;
    if (AnimationManager::beginFrame(index, type, delay, totalPixels)) {
      response["status"] = "success";
      response["message"] = "frame initialized";
      response["index"] = index;
    } else {
      setErrorResponse(response, "frame init failed");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_status") {
    int index = doc["index"] | -1;
    if (AnimationManager::currentGIF != nullptr &&
        index >= 0 &&
        index < AnimationManager::currentGIF->frameCount) {
      AnimationFrame& frame = AnimationManager::currentGIF->frames[index];
      response["status"] = "success";
      response["message"] = "frame status";
      response["index"] = index;
      response["count"] = frame.pixelCount;
      Serial.printf("帧 %d 状态查询: %d 像素已接收\n", index, frame.pixelCount);
    } else {
      setErrorResponse(response, "frame not found");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "frame_chunk") {
    int index = doc["index"] | -1;
    JsonArray pixels = doc["pixels"];
    if (AnimationManager::addFrameChunk(index, pixels)) {
      response["status"] = "success";
      response["message"] = "chunk received";
      response["index"] = index;
    } else {
      setErrorResponse(response, "chunk failed");
      response["index"] = index;
    }
    return true;
  }

  if (cmd == "animation_end") {
    AnimationManager::receivingFrameIndex = -1;
    if (AnimationManager::endAnimation()) {
      DisplayManager::currentMode = MODE_ANIMATION;
      AnimationManager::currentGIF->isPlaying = true;
      AnimationManager::currentGIF->currentFrame = 0;
      AnimationManager::currentGIF->lastFrameTime = millis();
      DisplayManager::dma_display->clearScreen();
      AnimationManager::renderGIFFrame(0);
      AnimationManager::saveAnimation();
      ConfigManager::saveClockConfig();

      response["status"] = "success";
      response["message"] = "animation loaded and playing";
      response["frameCount"] = AnimationManager::currentGIF->frameCount;
    } else {
      setErrorResponse(response, "animation finalize failed");
    }
    return true;
  }

  if (cmd == "control_gif") {
    String action = doc["action"];

    if (AnimationManager::currentGIF == nullptr) {
      setErrorResponse(response, "no animation loaded");
      return true;
    }

    if (action == "play") {
      AnimationManager::playAnimation();
      response["status"] = "success";
      response["message"] = "animation playing";
      return true;
    }

    if (action == "pause") {
      AnimationManager::pauseAnimation();
      response["status"] = "success";
      response["message"] = "animation paused";
      return true;
    }

    if (action == "stop") {
      AnimationManager::stopAnimation();
      response["status"] = "success";
      response["message"] = "animation stopped";
      return true;
    }

    setErrorResponse(response, "unknown action");
    return true;
  }

  (void)client;
  (void)responseSent;
  return false;
}

bool WebSocketCommandHandlers::handleCanvasCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response
) {
  String cmd = doc["cmd"].as<String>();

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

bool WebSocketCommandHandlers::handleOtaCommand(
  AsyncWebSocketClient* client,
  JsonDocument& doc,
  StaticJsonDocument<512>& response,
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
