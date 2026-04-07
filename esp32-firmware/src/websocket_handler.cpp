#include "websocket_handler.h"
#include "display_manager.h"
#include "config_manager.h"
#include "animation_manager.h"
#include "wifi_manager.h"
#include "ota_manager.h"
#include "tetris_effect.h"
#include "clock_font_renderer.h"
#include "websocket_command_handlers.h"

namespace {
struct ClientTextMessageState {
  uint32_t clientId;
  String fragmentBuffer;
  bool inUse;
};

constexpr size_t kClientTextStateCount = 4;
ClientTextMessageState gClientTextStates[kClientTextStateCount] = {};

ClientTextMessageState* getClientTextState(uint32_t clientId, bool createIfMissing) {
  for (size_t i = 0; i < kClientTextStateCount; i++) {
    if (gClientTextStates[i].inUse && gClientTextStates[i].clientId == clientId) {
      return &gClientTextStates[i];
    }
  }

  if (!createIfMissing) {
    return nullptr;
  }

  for (size_t i = 0; i < kClientTextStateCount; i++) {
    if (!gClientTextStates[i].inUse) {
      gClientTextStates[i].clientId = clientId;
      gClientTextStates[i].fragmentBuffer = "";
      gClientTextStates[i].inUse = true;
      return &gClientTextStates[i];
    }
  }

  return nullptr;
}

void clearClientTextState(uint32_t clientId) {
  ClientTextMessageState* state = getClientTextState(clientId, false);
  if (state == nullptr) {
    return;
  }

  state->fragmentBuffer = "";
  state->clientId = 0;
  state->inUse = false;
}
}

// 静态成员初始化
AsyncWebSocket WebSocketHandler::ws("/ws");
unsigned long WebSocketHandler::lastBinaryReceiveTime = 0;
bool WebSocketHandler::binaryDataPending = false;
String WebSocketHandler::fragmentBuffer = "";
bool WebSocketHandler::isReceivingFragments = false;
unsigned long WebSocketHandler::lastMessageTime = 0;

const char* WebSocketHandler::getCurrentModeString() {
  if (DisplayManager::currentBusinessModeTag.length() > 0) {
    return DisplayManager::currentBusinessModeTag.c_str();
  }
  if (DisplayManager::currentMode == MODE_CLOCK) {
    return "clock";
  }
  if (DisplayManager::currentMode == MODE_CANVAS) {
    return "canvas";
  }
  if (DisplayManager::currentMode == MODE_ANIMATION) {
    return "animation";
  }
  if (DisplayManager::currentMode == MODE_THEME) {
    return "theme";
  }
  if (DisplayManager::currentMode == MODE_TRANSFERRING) {
    return "transferring";
  }
  return "";
}

void WebSocketHandler::init() {
  ws.onEvent(onWsEvent);
  Serial.println("WebSocket 已初始化");
}

void WebSocketHandler::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                                AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket 客户端已连接: %u\n", client->id());
    DisplayManager::clientConnected = true;
    lastMessageTime = millis();
    clearClientTextState(client->id());
    
    // 连接时不切换模式，保持当前状态

    // 返回当前实际模式
    String modeStr = getCurrentModeString();
    String response = "{\"status\":\"connected\",\"device\":\"RenLight\",\"mode\":\"" + modeStr + "\"}";
    client->text(response);
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket 客户端已断开: %u\n", client->id());
    DisplayManager::clientConnected = false;
    clearClientTextState(client->id());

    if (DisplayManager::currentMode == MODE_CANVAS) {
      DisplayManager::currentMode = DisplayManager::lastBusinessMode;
      DisplayManager::currentBusinessModeTag = DisplayManager::lastBusinessModeTag;
      ConfigManager::saveClockConfig();
      Serial.printf(
        "客户端断开时自动恢复模式: canvas -> %s\n",
        getCurrentModeString()
      );

      if (DisplayManager::currentMode == MODE_CLOCK) {
        DisplayManager::displayClock(true);
      } else if (DisplayManager::currentMode == MODE_THEME) {
        DisplayManager::displayTheme(true);
      } else if (DisplayManager::currentMode == MODE_ANIMATION) {
        if (DisplayManager::currentBusinessModeTag == "eyes") {
          DisplayManager::activateEyesEffect(ConfigManager::eyesConfig);
        } else if (DisplayManager::currentBusinessModeTag == "ambient_effect") {
          DisplayManager::activateAmbientEffect(DisplayManager::ambientEffectConfig);
        } else if (AnimationManager::currentGIF != nullptr) {
          AnimationManager::currentGIF->isPlaying = true;
          AnimationManager::currentGIF->currentFrame = 0;
          AnimationManager::currentGIF->lastFrameTime = millis();
          AnimationManager::renderGIFFrame(0);
        } else {
          DisplayManager::displayClock(true);
        }
      }
    } else if (AnimationManager::currentGIF != nullptr && AnimationManager::currentGIF->isPlaying) {
      // 断开连接后，如果有动画在播放则保持动画模式
      DisplayManager::currentMode = MODE_ANIMATION;
    }
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

  // 数据完整性检查
  if (len % 5 != 0) {
    Serial.printf("⚠️ 数据长度错误: %d 字节不能被5整除！\n", len);
    client->text("{\"status\":\"error\",\"message\":\"invalid data length\"}");
    return;
  }

  int pixelCount = len / 5;

  if (pixelCount > 0) {
    // 动画帧二进制数据：如果正在接收动画帧，追加到对应帧
    if (AnimationManager::receivingFrameIndex >= 0) {
      int idx = AnimationManager::receivingFrameIndex;

      // 调试：打印接收到的数据详情
      static int chunkCount = 0;
      static int lastFrameIndex = -1;

      // 检测帧切换，重置计数器
      if (idx != lastFrameIndex) {
        chunkCount = 0;
        lastFrameIndex = idx;
      }

      chunkCount++;
      Serial.printf("[ESP32] 帧 %d chunk %d: 接收 %d 字节, %d 像素\n", idx, chunkCount, len, pixelCount);

      // 打印前3个像素用于对比
      if (chunkCount == 1 && len >= 15) {
        Serial.printf("[ESP32] 前3像素: [%d,%d,%d,%d,%d] [%d,%d,%d,%d,%d] [%d,%d,%d,%d,%d]\n",
          data[0], data[1], data[2], data[3], data[4],
          data[5], data[6], data[7], data[8], data[9],
          data[10], data[11], data[12], data[13], data[14]);
      }

      bool ok = AnimationManager::addFrameChunkBinary(idx, data, pixelCount);

      // 减少回复频率，避免队列溢出（但现在前端不等回复了，这个可以去掉）
      // 注释掉回复逻辑，因为前端改为用 frame_status 查询
      /*
      if (chunkCount % 5 == 0) {
        AnimationFrame& frame = AnimationManager::currentGIF->frames[idx];
        static char respBuf[128];
        if (ok) {
          snprintf(respBuf, sizeof(respBuf),
            "{\"status\":\"success\",\"message\":\"binary chunk received\",\"index\":%d,\"count\":%d}",
            idx, frame.pixelCount);
        } else {
          snprintf(respBuf, sizeof(respBuf),
            "{\"status\":\"error\",\"message\":\"binary chunk failed\",\"index\":%d}", idx);
        }
        client->text(respBuf);
      }
      */
      return;
    }

    // 画板模式：保存到 canvasBuffer（64x64完整画布）
    if (DisplayManager::currentMode == MODE_CANVAS) {
      
      // 第一次接收时初始化画布（清空）
      if (!DisplayManager::canvasInitialized) {
        DisplayManager::clearCanvas();
        DisplayManager::canvasInitialized = true;
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
    
    // 静态时钟背景模式：保存到 staticImagePixels
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
      DisplayManager::dma_display->clearScreen();
      DisplayManager::receivingPixels = true;  // 阻止 displayClock 在接收期间画 Logo
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

  ClientTextMessageState* clientState = getClientTextState(client->id(), true);
  if (clientState == nullptr) {
    client->text("{\"error\":\"message state unavailable\"}");
    return;
  }

  // === 分片重组逻辑 ===
  if (info->index == 0) {
    // 第一个分片，清空缓冲区
    clientState->fragmentBuffer = "";
    bool isFragmentedMessage = !info->final || (info->len > len);
    if (isFragmentedMessage) {
      Serial.printf("开始接收分片数据，总大小: %llu bytes\n", info->len);
    }
  }

  // 追加当前分片数据到缓冲区
  clientState->fragmentBuffer += String((char*)data, len);

  // 如果还没收完，等待后续分片
  if (!info->final || (info->index + len) < info->len) {
    Serial.printf("分片接收中: %llu/%llu bytes\n", info->index + len, info->len);
    return;
  }

  // === 完整消息已收齐，开始解析 ===
  size_t totalLen = clientState->fragmentBuffer.length();
  Serial.printf("完整消息已收齐: %d bytes\n", totalLen);

  // 根据实际数据大小动态分配 JSON 内存
  size_t freeHeap = ESP.getFreeHeap();

  // ArduinoJson 内存需求取决于 JSON 结构：
  // - 简单对象(如 set_mode): ~3倍原始大小
  // - 含大量数组的数据(如 animation_frame 的像素数组): ~8倍原始大小
  //   因为每个数组元素(数字)在 ArduinoJson 中占 16 字节
  // frame_chunk 是纯数字嵌套数组，实际只需 4 倍；其他命令用 8 倍
  bool isChunkCmd = (clientState->fragmentBuffer.indexOf("frame_chunk") > 0);
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
    clientState->fragmentBuffer = "";
    return;
  }

  DynamicJsonDocument doc(maxJsonSize);
  DeserializationError error = deserializeJson(doc, clientState->fragmentBuffer.c_str(), totalLen);

  // 解析完成后立即释放缓冲区
  clientState->fragmentBuffer = "";

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

  bool responseSent = false;
  bool handled =
    WebSocketCommandHandlers::handleBasicCommand(client, doc, response, getCurrentModeString()) ||
    WebSocketCommandHandlers::handleModeCommand(client, doc, response) ||
    WebSocketCommandHandlers::handleClockCommand(client, doc, response) ||
    WebSocketCommandHandlers::handleThemeCommand(client, doc, response) ||
    WebSocketCommandHandlers::handleEffectCommand(client, doc, response) ||
    WebSocketCommandHandlers::handleEyesCommand(client, doc, response) ||
    WebSocketCommandHandlers::handleAnimationCommand(client, doc, response, responseSent) ||
    WebSocketCommandHandlers::handleCanvasCommand(client, doc, response) ||
    WebSocketCommandHandlers::handleOtaCommand(client, doc, response, responseSent);

  if (!handled) {
    response["status"] = "error";
    response["message"] = "unknown command";
  }

  if (responseSent) {
    return;
  }

  String responseStr;
  serializeJson(response, responseStr);
  client->text(responseStr);
}
