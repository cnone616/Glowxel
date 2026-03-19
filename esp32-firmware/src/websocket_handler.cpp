#include "websocket_handler.h"
#include "display_manager.h"
#include "config_manager.h"
#include "animation_manager.h"
#include "wifi_manager.h"
#include "ota_manager.h"
#include "tetris_effect.h"

// 静态成员初始化
AsyncWebSocket WebSocketHandler::ws("/ws");
unsigned long WebSocketHandler::lastBinaryReceiveTime = 0;
bool WebSocketHandler::binaryDataPending = false;
String WebSocketHandler::fragmentBuffer = "";
bool WebSocketHandler::isReceivingFragments = false;
unsigned long WebSocketHandler::lastMessageTime = 0;

void WebSocketHandler::init() {
  ws.onEvent(onWsEvent);
}

void WebSocketHandler::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                                AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket 客户端已连接: %u\n", client->id());
    DisplayManager::clientConnected = true;
    lastMessageTime = millis();
    
    // 连接时不切换模式，保持当前状态

    // 返回当前实际模式
    String modeStr = "canvas";
    if (DisplayManager::currentMode == MODE_ANIMATION) {
      modeStr = "animation";
    }
    String response = "{\"status\":\"connected\",\"device\":\"RenLight\",\"mode\":\"" + modeStr + "\"}";
    client->text(response);
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket 客户端已断开: %u\n", client->id());
    DisplayManager::clientConnected = false;

    // 断开连接后，如果有动画在播放则保持动画模式
    if (AnimationManager::currentGIF != nullptr && AnimationManager::currentGIF->isPlaying) {
      DisplayManager::currentMode = MODE_ANIMATION;
    }
    // 否则保持当前模式和画布数据不变，用户重连后可继续编辑
    // 不清空 canvasBuffer / canvasInitialized / blackPixels
  }
  else if (type == WS_EVT_DATA) {
    lastMessageTime = millis();
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    
    // 处理二进制数据
    if (info->opcode == WS_BINARY) {
      handleBinaryData(client, data, len);
      return;
    }
    
    // 处理JSON命令（支持分片重组）
    handleJsonCommand(client, data, len, info);
  }
}

void WebSocketHandler::handleBinaryData(AsyncWebSocketClient *client, uint8_t *data, size_t len) {
  // 二进制格式：每5字节一个像素 (x, y, r, g, b)
  int pixelCount = len / 5;

  if (pixelCount > 0) {
    // 动画帧二进制数据：如果正在接收动画帧，追加到对应帧
    if (AnimationManager::receivingFrameIndex >= 0) {
      int idx = AnimationManager::receivingFrameIndex;
      bool ok = AnimationManager::addFrameChunkBinary(idx, data, pixelCount);
      // 发送确认回复（前端等待）
      String resp = ok ?
        "{\"status\":\"success\",\"message\":\"binary chunk received\",\"index\":" + String(idx) + "}" :
        "{\"status\":\"error\",\"message\":\"binary chunk failed\",\"index\":" + String(idx) + "}";
      client->text(resp);
      return;
    }

    // 画板模式：保存到 canvasBuffer（64x64完整画布）
    if (DisplayManager::currentMode == MODE_CANVAS) {
      
      // 第一次接收时初始化画布（清空）
      if (!DisplayManager::canvasInitialized) {
        memset(DisplayManager::canvasBuffer, 0, sizeof(DisplayManager::canvasBuffer));
        DisplayManager::canvasInitialized = true;
        
        // 清空之前的黑色像素记录
        if (DisplayManager::blackPixels != nullptr) {
          free(DisplayManager::blackPixels);
          DisplayManager::blackPixels = nullptr;
        }
        DisplayManager::blackPixelCount = 0;
      }
      
      // 先统计本批有效的黑色像素数量（在范围内的）
      int newBlackCount = 0;
      for (size_t i = 0; i + 4 < len; i += 5) {
        uint8_t x = data[i];
        uint8_t y = data[i + 1];
        uint8_t r = data[i + 2];
        uint8_t g = data[i + 3];
        uint8_t b = data[i + 4];
        if (x < DisplayManager::PANEL_RES_X && y < DisplayManager::PANEL_RES_Y && r == 0 && g == 0 && b == 0) {
          newBlackCount++;
        }
      }
      
      // 扩展黑色像素数组（累加，不是替换）
      if (newBlackCount > 0) {
        DisplayManager::BlackPixel* newBuffer = (DisplayManager::BlackPixel*)realloc(DisplayManager::blackPixels, sizeof(DisplayManager::BlackPixel) * (DisplayManager::blackPixelCount + newBlackCount));
        if (newBuffer == nullptr) {
          return;
        }
        DisplayManager::blackPixels = newBuffer;
      }
      
      int validPixels = 0;
      for (size_t i = 0; i + 4 < len; i += 5) {
        uint8_t x = data[i];
        uint8_t y = data[i + 1];
        uint8_t r = data[i + 2];
        uint8_t g = data[i + 3];
        uint8_t b = data[i + 4];
        
        if (x < DisplayManager::PANEL_RES_X && y < DisplayManager::PANEL_RES_Y) {
          // 存储到画布缓冲区
          DisplayManager::canvasBuffer[y][x][0] = r;
          DisplayManager::canvasBuffer[y][x][1] = g;
          DisplayManager::canvasBuffer[y][x][2] = b;
          validPixels++;
          
          // 如果是黑色，记录坐标
          if (r == 0 && g == 0 && b == 0 && DisplayManager::blackPixels != nullptr) {
            DisplayManager::blackPixels[DisplayManager::blackPixelCount].x = x;
            DisplayManager::blackPixels[DisplayManager::blackPixelCount].y = y;
            DisplayManager::blackPixelCount++;
          }
          
          // 立即绘制
          DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
        }
        
        if (i % 500 == 0) yield();
      }
      
      return;
    }
    
    // 闹钟模式：保存到对应模式的 imagePixels（背景图）
    // 根据当前模式选择对应的像素存储
    PixelData*& imagePixels = (DisplayManager::currentMode == MODE_ANIMATION)
      ? ConfigManager::animImagePixels
      : ConfigManager::staticImagePixels;

    int& imagePixelCount = (DisplayManager::currentMode == MODE_ANIMATION)
      ? ConfigManager::animImagePixelCount
      : ConfigManager::staticImagePixelCount;

    // 如果是新的一批数据（距离上次超过1秒），清空旧数据重新开始
    if (millis() - lastBinaryReceiveTime > 1000) {
      if (imagePixels != nullptr) {
        free(imagePixels);
        imagePixels = nullptr;
      }
      imagePixelCount = 0;
      // 清屏，擦除旧 Logo 或残留像素
      DisplayManager::dma_display->clearScreen();
    }

    // 追加数据
    PixelData* newBuffer = (PixelData*)realloc(imagePixels, sizeof(PixelData) * (imagePixelCount + pixelCount));
    if (newBuffer != nullptr) {
      imagePixels = newBuffer;

      int validPixels = 0;
      // 复制像素数据
      for (size_t i = 0; i + 4 < len; i += 5) {
        // 达到最大像素数量后，忽略后续数据，避免占用过多 NVS 空间
        if (imagePixelCount >= DisplayManager::MAX_PIXELS) {
          break;
        }

        uint8_t x = data[i];
        uint8_t y = data[i + 1];
        uint8_t r = data[i + 2];
        uint8_t g = data[i + 3];
        uint8_t b = data[i + 4];

        if (x < DisplayManager::PANEL_RES_X && y < DisplayManager::PANEL_RES_Y) {
          // 保存到内存
          imagePixels[imagePixelCount].x = x;
          imagePixels[imagePixelCount].y = y;
          imagePixels[imagePixelCount].r = r;
          imagePixels[imagePixelCount].g = g;
          imagePixels[imagePixelCount].b = b;
          imagePixelCount++;
          validPixels++;

          // 立即绘制
          DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
        }

        // 每处理100个像素让出CPU，防止看门狗超时
        if (i % 500 == 0) {
          yield();
        }
      }
      
      // 更新接收时间，标记有待保存的数据
      lastBinaryReceiveTime = millis();
      binaryDataPending = true;
      
    } else {
      Serial.println("✗ 内存分配失败！");
    }
  }
}

void WebSocketHandler::handleJsonCommand(AsyncWebSocketClient *client, uint8_t *data, size_t len, AwsFrameInfo *info) {
  Serial.printf("收到 WebSocket 数据: %d bytes, final=%d, index=%llu, total=%llu\n",
    len, info->final, info->index, info->len);

  // === 分片重组逻辑 ===
  if (info->index == 0) {
    // 第一个分片，清空缓冲区
    fragmentBuffer = "";
    isReceivingFragments = !info->final || (info->len > len);
    if (isReceivingFragments) {
      Serial.printf("开始接收分片数据，总大小: %llu bytes\n", info->len);
    }
  }

  // 追加当前分片数据到缓冲区
  fragmentBuffer += String((char*)data, len);

  // 如果还没收完，等待后续分片
  if (!info->final || (info->index + len) < info->len) {
    Serial.printf("分片接收中: %llu/%llu bytes\n", info->index + len, info->len);
    return;
  }

  // === 完整消息已收齐，开始解析 ===
  isReceivingFragments = false;
  size_t totalLen = fragmentBuffer.length();
  Serial.printf("完整消息已收齐: %d bytes\n", totalLen);

  // 根据实际数据大小动态分配 JSON 内存
  size_t freeHeap = ESP.getFreeHeap();

  // ArduinoJson 内存需求取决于 JSON 结构：
  // - 简单对象(如 set_mode): ~3倍原始大小
  // - 含大量数组的数据(如 animation_frame 的像素数组): ~8倍原始大小
  //   因为每个数组元素(数字)在 ArduinoJson 中占 16 字节
  // frame_chunk 是纯数字嵌套数组，实际只需 4 倍；其他命令用 8 倍
  bool isChunkCmd = (fragmentBuffer.indexOf("frame_chunk") > 0);
  size_t multiplier = isChunkCmd ? 4 : 8;
  size_t maxJsonSize = totalLen * multiplier + 2048;

  // 上限不超过可用堆的 70%
  size_t heapLimit = (size_t)(freeHeap * 0.7);
  if (maxJsonSize > heapLimit) maxJsonSize = heapLimit;
  // 最小 2KB（足够解析简单命令）
  if (maxJsonSize < 2048) maxJsonSize = 2048;

  Serial.printf("可用内存: %d bytes, 数据: %d bytes, JSON分配: %d bytes\n",
    freeHeap, totalLen, maxJsonSize);

  if (maxJsonSize > freeHeap * 0.85) {
    Serial.printf("内存不足: 需要 %d bytes，可用 %d bytes\n", maxJsonSize, freeHeap);
    client->text("{\"error\":\"insufficient memory\",\"needed\":" + String(maxJsonSize) + ",\"available\":" + String(freeHeap) + "}");
    fragmentBuffer = "";
    return;
  }

  DynamicJsonDocument doc(maxJsonSize);
  DeserializationError error = deserializeJson(doc, fragmentBuffer.c_str(), totalLen);

  // 解析完成后立即释放缓冲区
  fragmentBuffer = "";

  if (error) {
    Serial.print("JSON解析失败: ");
    Serial.println(error.c_str());
    Serial.printf("数据长度: %d bytes\n", totalLen);

    client->text("{\"error\":\"invalid json\",\"details\":\"" + String(error.c_str()) + "\"}");
    return;
  }

  Serial.println("JSON解析成功，处理命令");
  handleJsonCommand(client, doc);
}

void WebSocketHandler::handleJsonCommand(AsyncWebSocketClient *client, JsonDocument& doc) {
  String cmd = doc["cmd"].as<String>();
  Serial.println("命令: " + cmd);
  
  StaticJsonDocument<512> response;
  response["status"] = "ok";
  
  if (cmd == "ping") {
    response["message"] = "pong";
  }
  else if (cmd == "status") {
    response["ip"] = WiFiManager::isConfigMode() ? WiFiManager::getDeviceIP() : WiFiManager::getDeviceIP();
    response["width"] = DisplayManager::PANEL_RES_X;
    response["height"] = DisplayManager::PANEL_RES_Y;
    response["brightness"] = DisplayManager::currentBrightness;
    if (DisplayManager::currentMode == MODE_CANVAS) {
      // 前端把 canvas 当作 clock 模式显示
      response["mode"] = "clock";
    } else if (DisplayManager::currentMode == MODE_ANIMATION) {
      response["mode"] = "animation";
      if (AnimationManager::currentGIF != nullptr) {
        response["animationFrames"] = AnimationManager::currentGIF->frameCount;
        response["animationPlaying"] = AnimationManager::currentGIF->isPlaying;
        response["currentFrame"] = AnimationManager::currentGIF->currentFrame;
      }
    }
  }
  else if (cmd == "set_mode") {
    String mode = doc["mode"].as<String>();

    // === 统一：先停掉所有活动 ===
    TetrisEffect::isActive = false;
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = false;
    }
    // === 统一：清屏，防止残留 ===
    DisplayManager::dma_display->clearScreen();

    if (mode == "clock") {
      // 静态时钟模式
      DisplayManager::isCanvasMode = false;
      DisplayManager::currentMode = MODE_CANVAS;
      ConfigManager::saveClockConfig();
      DisplayManager::displayClock(true);  // 强制重绘
      response["message"] = "switched to static clock mode";
    } else if (mode == "canvas") {
      // 画板模式（不画时钟）
      DisplayManager::isCanvasMode = true;
      DisplayManager::currentMode = MODE_CANVAS;
      ConfigManager::saveClockConfig();

      if (DisplayManager::canvasInitialized) {
        // 恢复画布数据
        for (int y = 0; y < DisplayManager::PANEL_RES_Y; y++) {
          for (int x = 0; x < DisplayManager::PANEL_RES_X; x++) {
            uint8_t r = DisplayManager::canvasBuffer[y][x][0];
            uint8_t g = DisplayManager::canvasBuffer[y][x][1];
            uint8_t b = DisplayManager::canvasBuffer[y][x][2];
            if (r > 0 || g > 0 || b > 0) {
              DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
            }
          }
          if (y % 8 == 0) yield();
        }
      } else {
        // 没有画布数据，显示 Logo 表示画板就绪
        DisplayManager::drawLogo(12, 12);  // 画板模式居中
      }

      response["message"] = "switched to canvas mode";
    } else if (mode == "animation") {
      // 动态时钟模式
      DisplayManager::isCanvasMode = false;
      DisplayManager::currentMode = MODE_ANIMATION;
      ConfigManager::saveClockConfig();

      if (AnimationManager::currentGIF != nullptr) {
        AnimationManager::currentGIF->isPlaying = true;
        AnimationManager::currentGIF->currentFrame = 0;
        AnimationManager::currentGIF->lastFrameTime = millis();
        AnimationManager::renderGIFFrame(0);
        response["message"] = "switched to animation mode";
      } else {
        // 没有 GIF，显示动态时钟（背景图+时钟）
        DisplayManager::displayClock(true);
        response["message"] = "switched to animation mode (no GIF)";
      }
    } else if (mode == "tetris") {
      // 俄罗斯方块屏保
      DisplayManager::isCanvasMode = false;
      DisplayManager::currentMode = MODE_ANIMATION;
      bool clearMode = doc["clearMode"] | true;
      int cellSz = doc["cellSize"] | 2;
      int speed = doc["speed"] | 150;
      bool clock = doc["showClock"] | true;
      // pieces 数组转 bitmask
      uint8_t mask = 0x7F;
      if (doc.containsKey("pieces")) {
        mask = 0;
        JsonArray arr = doc["pieces"].as<JsonArray>();
        for (JsonVariant v : arr) {
          int idx = v.as<int>();
          if (idx >= 0 && idx < 7) mask |= (1 << idx);
        }
        if (mask == 0) mask = 0x7F;
      }
      TetrisEffect::init(clearMode, cellSz, speed, clock, mask);
      response["message"] = "tetris started";
    } else {
      response["status"] = "error";
      response["message"] = "invalid mode";
    }
  }
  else if (cmd == "set_clock_config") {
    JsonObject config = doc["config"];
    String clockMode = doc["clockMode"] | "clock"; // "clock"=静态, "animation"=动态

    // 根据模式选择写入哪个 config
    ClockConfig& target = (clockMode == "animation")
      ? ConfigManager::animClockConfig
      : ConfigManager::clockConfig;

    if (config.containsKey("time")) {
      JsonObject time = config["time"];
      target.time.fontSize = time["fontSize"] | 1;
      target.time.x = time["x"] | 17;
      target.time.y = time["y"] | 18;
      JsonObject timeColor = time["color"];
      target.time.r = timeColor["r"] | 100;
      target.time.g = timeColor["g"] | 200;
      target.time.b = timeColor["b"] | 255;
    }

    if (config.containsKey("date")) {
      JsonObject date = config["date"];
      target.date.show = date["show"] | true;
      target.date.fontSize = date["fontSize"] | 1;
      target.date.x = date["x"] | 3;
      target.date.y = date["y"] | 30;
      JsonObject dateColor = date["color"];
      target.date.r = dateColor["r"] | 120;
      target.date.g = dateColor["g"] | 120;
      target.date.b = dateColor["b"] | 120;
    }

    if (config.containsKey("week")) {
      JsonObject week = config["week"];
      target.week.show = week["show"] | true;
      target.week.x = week["x"] | 23;
      target.week.y = week["y"] | 44;
      JsonObject weekColor = week["color"];
      target.week.r = weekColor["r"] | 100;
      target.week.g = weekColor["g"] | 100;
      target.week.b = weekColor["b"] | 100;
    }

    if (config.containsKey("image")) {
      JsonObject image = config["image"];
      target.image.show = image["show"] | false;
      target.image.x = image["x"] | 0;
      target.image.y = image["y"] | 0;
      target.image.width = image["width"] | 64;
      target.image.height = image["height"] | 64;
    }

    // 按模式分别保存
    if (clockMode == "animation") {
      ConfigManager::saveAnimClockConfig();
    } else {
      ConfigManager::saveClockConfig();
    }

    // 立即刷新显示
    if (DisplayManager::currentMode == MODE_CANVAS) {
      DisplayManager::displayClock(true);
    }

    response["message"] = "clock config updated";
  }
  else if (cmd == "set_gif_animation") {
    // 接收 GIF 动画数据（超紧凑数组格式）
    JsonVariant animData = doc["animationData"];
    
    if (animData.isNull()) {
      // 停止并清除动画
      AnimationManager::freeGIFAnimation();
      response["status"] = "success";
      response["message"] = "animation cleared";
    } else {
      if (AnimationManager::loadGIFAnimation(animData)) {
        response["status"] = "success";
        response["message"] = "compact animation loaded";
        response["frameCount"] = AnimationManager::currentGIF->frameCount;
      } else {
        response["status"] = "error";
        response["message"] = "animation load failed";
      }
    }
  }
  else if (cmd == "animation_begin") {
    // 逐帧传输：开始接收动画，立即切换模式阻止 displayClock() 刷屏
    int frameCount = doc["frameCount"] | 0;
    DisplayManager::currentMode = MODE_ANIMATION;  // 立即切换，防止闹钟界面闪现
    if (AnimationManager::beginAnimation(frameCount)) {
      response["status"] = "success";
      response["message"] = "ready to receive frames";
      response["frameCount"] = frameCount;
    } else {
      response["status"] = "error";
      response["message"] = "animation init failed";
    }
  }
  else if (cmd == "animation_frame") {
    // 逐帧传输：接收单帧数据
    int index = doc["index"] | -1;
    JsonVariant frameData = doc["frame"];
    if (AnimationManager::addFrame(index, frameData)) {
      response["status"] = "success";
      response["message"] = "frame received";
      response["index"] = index;
    } else {
      response["status"] = "error";
      response["message"] = "frame load failed";
      response["index"] = index;
    }
  }
  else if (cmd == "frame_begin") {
    // 分片传输：初始化一帧（预分配内存）
    int index = doc["index"] | -1;
    int type = doc["type"] | 1;
    int delay = doc["delay"] | 100;
    int totalPixels = doc["totalPixels"] | 0;
    if (AnimationManager::beginFrame(index, type, delay, totalPixels)) {
      response["status"] = "success";
      response["message"] = "frame initialized";
      response["index"] = index;
    } else {
      response["status"] = "error";
      response["message"] = "frame init failed";
      response["index"] = index;
    }
  }
  else if (cmd == "frame_chunk") {
    // 分片传输：追加像素数据
    int index = doc["index"] | -1;
    JsonArray pixels = doc["pixels"];
    if (AnimationManager::addFrameChunk(index, pixels)) {
      response["status"] = "success";
      response["message"] = "chunk received";
      response["index"] = index;
    } else {
      response["status"] = "error";
      response["message"] = "chunk failed";
      response["index"] = index;
    }
  }
  else if (cmd == "animation_end") {
    // 逐帧传输：完成动画加载
    AnimationManager::receivingFrameIndex = -1;  // 停止接收二进制帧数据
    if (AnimationManager::endAnimation()) {
      // 自动切换到动画模式并播放
      DisplayManager::currentMode = MODE_ANIMATION;
      AnimationManager::currentGIF->isPlaying = true;
      AnimationManager::currentGIF->currentFrame = 0;
      AnimationManager::currentGIF->lastFrameTime = millis();
      DisplayManager::dma_display->clearScreen();  // 清掉 loading 圈
      AnimationManager::renderGIFFrame(0);

      // 保存动画到 LittleFS，重启后可恢复
      AnimationManager::saveAnimation();

      // 保存当前模式到 Preferences，重启后恢复
      ConfigManager::saveClockConfig();

      response["status"] = "success";
      response["message"] = "animation loaded and playing";
      response["frameCount"] = AnimationManager::currentGIF->frameCount;
    } else {
      response["status"] = "error";
      response["message"] = "animation finalize failed";
    }
  }
  else if (cmd == "control_gif") {
    // 控制 GIF 动画播放
    String action = doc["action"];
    
    if (AnimationManager::currentGIF == nullptr) {
      response["status"] = "error";
      response["message"] = "no animation loaded";
    } else {
      if (action == "play") {
        AnimationManager::playAnimation();
        response["status"] = "success";
        response["message"] = "animation playing";
      } else if (action == "pause") {
        AnimationManager::pauseAnimation();
        response["status"] = "success";
        response["message"] = "animation paused";
      } else if (action == "stop") {
        AnimationManager::stopAnimation();
        response["status"] = "success";
        response["message"] = "animation stopped";
      } else {
        response["status"] = "error";
        response["message"] = "unknown action";
      }
    }
  }
  else if (cmd == "clear") {
    DisplayManager::dma_display->clearScreen();
    
    // 如果是画板模式，清空画布缓冲区
    if (DisplayManager::currentMode == MODE_CANVAS && DisplayManager::canvasInitialized) {
      Serial.println("清空画布缓冲区");
      memset(DisplayManager::canvasBuffer, 0, sizeof(DisplayManager::canvasBuffer));
    }
    
    response["message"] = "cleared";
  }
  else if (cmd == "brightness") {
    int value = doc["value"];
    if (value >= 0 && value <= 255) {
      DisplayManager::setBrightness(value);
      response["brightness"] = value;
    } else {
      response["status"] = "error";
      response["message"] = "brightness must be 0-255";
    }
  }
  else if (cmd == "highlight_color") {
    // 高亮指定颜色，其他颜色降低亮度
    if (DisplayManager::currentMode == MODE_CANVAS && DisplayManager::canvasInitialized) {
      bool hasHighlight = doc.containsKey("color") && !doc["color"].isNull();
      uint8_t highlightR = 0, highlightG = 0, highlightB = 0;
      
      if (hasHighlight) {
        JsonObject color = doc["color"];
        highlightR = color["r"] | 0;
        highlightG = color["g"] | 0;
        highlightB = color["b"] | 0;
        
        // 如果是黑色，用存储的坐标亮白灯
        if (highlightR == 0 && highlightG == 0 && highlightB == 0) {
          DisplayManager::dma_display->clearScreen();
          for (int i = 0; i < DisplayManager::blackPixelCount; i++) {
            DisplayManager::dma_display->drawPixelRGB888(DisplayManager::blackPixels[i].x, DisplayManager::blackPixels[i].y, 255, 255, 255);
          }
        } else {
          DisplayManager::dma_display->clearScreen();
          
          for (int y = 0; y < DisplayManager::PANEL_RES_Y; y++) {
            for (int x = 0; x < DisplayManager::PANEL_RES_X; x++) {
              uint8_t r = DisplayManager::canvasBuffer[y][x][0];
              uint8_t g = DisplayManager::canvasBuffer[y][x][1];
              uint8_t b = DisplayManager::canvasBuffer[y][x][2];
              
              if (r > 0 || g > 0 || b > 0) {
                bool isHighlight = (abs(r - highlightR) <= 2 && 
                                   abs(g - highlightG) <= 2 && 
                                   abs(b - highlightB) <= 2);
                
                if (isHighlight) {
                  DisplayManager::dma_display->drawPixelRGB888(x, y, 255, 255, 255);
                }
              }
            }
            if (y % 8 == 0) yield();
          }
        }
      } else {
        // 恢复显示完整的彩色像素图
        for (int y = 0; y < DisplayManager::PANEL_RES_Y; y++) {
          for (int x = 0; x < DisplayManager::PANEL_RES_X; x++) {
            uint8_t r = DisplayManager::canvasBuffer[y][x][0];
            uint8_t g = DisplayManager::canvasBuffer[y][x][1];
            uint8_t b = DisplayManager::canvasBuffer[y][x][2];
            DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
          }
          if (y % 8 == 0) yield();
        }
      }
      
      response["message"] = hasHighlight ? "color highlighted" : "highlight cleared";
    } else {
      response["status"] = "error";
      response["message"] = "not in canvas mode or canvas not initialized";
    }
  }
  else if (cmd == "highlight_row") {
    // 高亮指定行
    if (DisplayManager::currentMode == MODE_CANVAS && DisplayManager::canvasInitialized) {
      int highlightRow = doc["row"] | -1;
      
      for (int y = 0; y < DisplayManager::PANEL_RES_Y; y++) {
        for (int x = 0; x < DisplayManager::PANEL_RES_X; x++) {
          uint8_t r = DisplayManager::canvasBuffer[y][x][0];
          uint8_t g = DisplayManager::canvasBuffer[y][x][1];
          uint8_t b = DisplayManager::canvasBuffer[y][x][2];
          
          if (r > 0 || g > 0 || b > 0) {
            uint8_t displayR = r, displayG = g, displayB = b;
            
            if (highlightRow >= 0 && y != highlightRow) {
              displayR = r * 0.2;
              displayG = g * 0.2;
              displayB = b * 0.2;
            }
            
            DisplayManager::dma_display->drawPixelRGB888(x, y, displayR, displayG, displayB);
          }
        }
        if (y % 8 == 0) yield();
      }
      
      response["message"] = (highlightRow >= 0) ? "row highlighted" : "highlight cleared";
    } else {
      response["status"] = "error";
      response["message"] = "not in canvas mode or canvas not initialized";
    }
  }
  else if (cmd == "show_loading") {
    DisplayManager::dma_display->clearScreen();
    int cx = DisplayManager::PANEL_RES_X / 2;
    int cy = DisplayManager::PANEL_RES_Y / 2;
    int r = 12;
    // 暗色底圆（更亮的灰色）
    for (int deg = 0; deg < 360; deg += 20) {
      float rad = deg * 3.14159f / 180.0f;
      int x = cx + (int)(r * cos(rad));
      int y = cy + (int)(r * sin(rad));
      if (x >= 1 && x < DisplayManager::PANEL_RES_X-1 && y >= 1 && y < DisplayManager::PANEL_RES_Y-1)
        DisplayManager::dma_display->fillRect(x-1, y-1, 3, 3, DisplayManager::dma_display->color565(60, 60, 60));
    }
    // 亮弧（白蓝渐变，更大更亮）
    int startDeg = doc["angle"] | 0;
    for (int i = 0; i < 5; i++) {
      float rad = (startDeg + i * 25) * 3.14159f / 180.0f;
      int x = cx + (int)(r * cos(rad));
      int y = cy + (int)(r * sin(rad));
      uint8_t bright = (i == 4) ? 255 : (i == 3 ? 220 : (i == 2 ? 160 : (i == 1 ? 100 : 50)));
      if (x >= 1 && x < DisplayManager::PANEL_RES_X-1 && y >= 1 && y < DisplayManager::PANEL_RES_Y-1)
        DisplayManager::dma_display->fillRect(x-1, y-1, 3, 3, DisplayManager::dma_display->color565(bright, bright, 255));
    }
    response["message"] = "loading displayed";
  }
  else if (cmd == "text") {
    String text = doc["text"].as<String>();
    int x = doc["x"] | 0;
    int y = doc["y"] | 0;
    DisplayManager::dma_display->clearScreen();
    DisplayManager::dma_display->setCursor(x, y);
    DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(255, 255, 255));
    DisplayManager::dma_display->print(text);
    response["message"] = "text displayed";
  }
  else if (cmd == "pixel") {
    int x = doc["x"];
    int y = doc["y"];
    int r = doc["r"] | 255;
    int g = doc["g"] | 255;
    int b = doc["b"] | 255;
    if (x >= 0 && x < DisplayManager::PANEL_RES_X && y >= 0 && y < DisplayManager::PANEL_RES_Y) {
      DisplayManager::dma_display->drawPixelRGB888(x, y, r, g, b);
      response["message"] = "pixel set";
    } else {
      response["status"] = "error";
      response["message"] = "pixel out of range";
    }
  }
  else if (cmd == "image") {
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
      if (y % 8 == 0) yield();
    }
    response["message"] = "image displayed";
  }
  else if (cmd == "image_sparse") {
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
  }
  else if (cmd == "image_chunk") {
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
          
          if (drawX >= 0 && drawX < DisplayManager::PANEL_RES_X && drawY >= 0 && drawY < DisplayManager::PANEL_RES_Y) {
            DisplayManager::dma_display->drawPixelRGB888(drawX, drawY, r, g, b);
          }
        }
      }
      if (y % 8 == 0) yield();
    }
    response["message"] = "chunk displayed";
    response["chunk"] = chunk;
    response["total"] = total;
  }
  else if (cmd == "ota_check") {
    OTAManager::checkUpdate();
    response["firmware_version"] = FIRMWARE_VERSION;
    response["has_update"] = OTAManager::hasNewVersion();
    if (OTAManager::hasNewVersion()) {
      response["latest_version"] = OTAManager::getLatestVersion();
      response["changelog"] = OTAManager::getChangelog();
      response["is_force"] = OTAManager::isForce;
    }
  }
  else if (cmd == "ota_update") {
    if (OTAManager::hasNewVersion()) {
      response["message"] = "starting update";
      String responseStr;
      serializeJson(response, responseStr);
      client->text(responseStr);
      delay(500);
      OTAManager::startUpdate();
      return; // startUpdate 会重启，不会到这里
    } else {
      response["status"] = "error";
      response["message"] = "no update available";
    }
  }
  else if (cmd == "ota_set_server") {
    String url = doc["url"].as<String>();
    if (url.length() > 0) {
      OTAManager::serverUrl = url;
      ConfigManager::preferences.begin("ota", false);
      ConfigManager::preferences.putString("server", url);
      ConfigManager::preferences.end();
      response["message"] = "OTA server set";
      response["server"] = url;
    } else {
      response["status"] = "error";
      response["message"] = "missing url";
    }
  }
  else if (cmd == "get_info") {
    response["firmware_version"] = FIRMWARE_VERSION;
    response["device_type"] = DEVICE_TYPE;
    response["ip"] = WiFiManager::getDeviceIP();
    response["free_heap"] = ESP.getFreeHeap();
    response["uptime"] = millis() / 1000;
    response["has_update"] = OTAManager::hasNewVersion();
  }
  else {
    response["status"] = "error";
    response["message"] = "unknown command";
  }
  
  String responseStr;
  serializeJson(response, responseStr);
  client->text(responseStr);
}