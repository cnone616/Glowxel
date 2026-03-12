#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <time.h>

const char* ap_ssid = "LED-Matrix-Setup";
const char* ap_password = "12345678";

// NTP 时间服务器
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600; // 中国时区 UTC+8
const int daylightOffset_sec = 0;

Preferences preferences;
String saved_ssid = "";
String saved_password = "";
bool config_mode = false;

// 设备模式
enum DeviceMode {
  MODE_CLOCK,    // 闹钟模式（默认）
  MODE_CANVAS    // 画板模式
};

DeviceMode currentMode = MODE_CLOCK;
bool clientConnected = false;

// 闹钟配置结构
struct ClockConfig {
  struct {
    int fontSize;
    int x;
    int y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } time;
  
  struct {
    bool show;
    int fontSize;
    int x;
    int y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } date;
  
  struct {
    bool show;
    int x;
    int y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } week;
  
  struct {
    bool show;
    int x;
    int y;
    int width;
    int height;
  } image;
};

// 默认闹钟配置
ClockConfig clockConfig = {
  .time = {1, 17, 18, 100, 200, 255},
  .date = {true, 1, 3, 30, 120, 120, 120},
  .week = {true, 23, 44, 100, 100, 100},
  .image = {false, 0, 0, 64, 64}
};

// 图片像素数据存储（闹钟模式背景图）
struct PixelData {
  uint8_t x;
  uint8_t y;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

PixelData* imagePixels = nullptr;
int imagePixelCount = 0;

// 画板模式像素缓冲区（64x64完整画布，RGB格式）
uint8_t canvasBuffer[64][64][3]; // [y][x][rgb]
bool canvasInitialized = false;

// 黑色像素坐标存储
struct BlackPixel {
  uint8_t x;
  uint8_t y;
};
BlackPixel* blackPixels = nullptr;
int blackPixelCount = 0;

// 二进制数据接收状态
unsigned long lastBinaryReceiveTime = 0;
bool binaryDataPending = false;

#define PANEL_RES_X 64
#define PANEL_RES_Y 64
#define PANEL_CHAIN 1

// 最大支持的像素数量（64x64 单格 RGB）
const int MAX_PIXELS = PANEL_RES_X * PANEL_RES_Y;

MatrixPanel_I2S_DMA *dma_display = nullptr;
AsyncWebServer server(80);
DNSServer dnsServer;
AsyncWebSocket ws("/ws");

uint8_t* imageBuffer = nullptr;
size_t imageSize = 0;
int currentBrightness = 50; // 默认亮度50%，对应设备端约20%

// 函数声明
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

void setupWiFi() {
  preferences.begin("wifi", false);
  saved_ssid = preferences.getString("ssid", "");
  saved_password = preferences.getString("password", "");
  preferences.end();
  
  if (saved_ssid.length() > 0) {
    Serial.println("尝试连接到保存的WiFi: " + saved_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(saved_ssid.c_str(), saved_password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi已连接");
      Serial.print("IP地址: ");
      Serial.println(WiFi.localIP());
      config_mode = false;
      
      // 同步NTP时间
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      Serial.println("正在同步网络时间...");
      
      // 在LED上显示连接成功和IP
      dma_display->clearScreen();
      dma_display->setTextSize(1);
      dma_display->setCursor(2, 2);
      dma_display->setTextColor(dma_display->color565(0, 255, 0));
      dma_display->println("WiFi OK");
      dma_display->setCursor(2, 12);
      dma_display->setTextColor(dma_display->color565(255, 255, 255));
      dma_display->println(WiFi.localIP().toString());
      
      delay(2000);
      
      // 进入闹钟模式
      currentMode = MODE_CLOCK;
      
      return;
    }
    
    Serial.println("\n连接失败，进入配置模式");
  } else {
    Serial.println("未找到WiFi配置，进入配置模式");
  }
  
  config_mode = true;
  WiFi.mode(WIFI_AP);
  
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  WiFi.softAP(ap_ssid, ap_password, 1, 0, 4);
  delay(500);
  
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  Serial.println("\n=================================");
  Serial.println("配置模式已启动");
  Serial.println("热点名称: " + String(ap_ssid));
  Serial.println("热点密码: " + String(ap_password));
  Serial.print("配置地址: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("DNS服务器已启动");
  Serial.println("=================================");
  
  dma_display->clearScreen();
  dma_display->setTextSize(1);
  
  dma_display->setCursor(2, 10);
  dma_display->setTextColor(dma_display->color565(0, 255, 255));
  dma_display->print("WiFi:");
  dma_display->setTextColor(dma_display->color565(255, 255, 255));
  dma_display->println(ap_ssid);
  
  dma_display->setCursor(2, 40);
  dma_display->setTextColor(dma_display->color565(0, 255, 255));
  dma_display->print("Pass:");
  dma_display->setTextColor(dma_display->color565(255, 255, 255));
  dma_display->println(ap_password);
}

void setupMatrix() {
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  mxconfig.gpio.e = 18;
  mxconfig.clkphase = false;
  
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(51); // 默认亮度约20% (51/255)
  dma_display->clearScreen();
  
  Serial.println("显示开机Logo...");
  
  uint16_t red = dma_display->color565(255, 100, 100);
  uint16_t orange = dma_display->color565(255, 165, 0);
  uint16_t yellow = dma_display->color565(255, 220, 0);
  uint16_t green = dma_display->color565(100, 255, 100);
  uint16_t blue = dma_display->color565(100, 150, 255);
  
  int blockSize = 8;
  int gridSize = 12;
  int startPos = 4;  // 往下2，往右2
  
  for(int row = 0; row < 5; row++) {
    dma_display->fillRect(startPos, startPos + row * gridSize, blockSize, blockSize, red);
  }
  
  dma_display->fillRect(startPos + gridSize, startPos + gridSize, blockSize, blockSize, orange);
  dma_display->fillRect(startPos + gridSize * 2, startPos + gridSize * 2, blockSize, blockSize, yellow);
  dma_display->fillRect(startPos + gridSize * 3, startPos + gridSize, blockSize, blockSize, green);
  
  for(int row = 0; row < 5; row++) {
    dma_display->fillRect(startPos + gridSize * 4, startPos + row * gridSize, blockSize, blockSize, blue);
  }
  
  delay(2000);
  Serial.println("LED灯板初始化完成");
}

void displayImage(uint8_t* data, size_t len, int width, int height) {
  dma_display->clearScreen();
  
  for (int y = 0; y < height && y < PANEL_RES_Y; y++) {
    for (int x = 0; x < width && x < PANEL_RES_X; x++) {
      int idx = (y * width + x) * 3;
      if (idx + 2 < len) {
        uint8_t r = data[idx];
        uint8_t g = data[idx + 1];
        uint8_t b = data[idx + 2];
        dma_display->drawPixelRGB888(x, y, r, g, b);
      }
    }
  }
}

// 显示时钟
void displayClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // 时间未同步，显示 "--:--"
    dma_display->clearScreen();
    dma_display->setTextSize(1);
    dma_display->setCursor(17, 18);
    dma_display->setTextColor(dma_display->color565(100, 200, 255));
    dma_display->print("--:--");
    return;
  }
  
  dma_display->clearScreen();
  
  // 先绘制背景图片（如果有）
  Serial.printf("displayClock: image.show=%d, imagePixels=%p, imagePixelCount=%d\n", 
    clockConfig.image.show, imagePixels, imagePixelCount);
  
  if (clockConfig.image.show && imagePixels != nullptr && imagePixelCount > 0) {
    Serial.printf("开始绘制 %d 个像素\n", imagePixelCount);
    // 绘制所有保存的像素
    for (int i = 0; i < imagePixelCount; i++) {
      PixelData& pixel = imagePixels[i];
      if (pixel.x < PANEL_RES_X && pixel.y < PANEL_RES_Y) {
        dma_display->drawPixelRGB888(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);
      }
      // 每100个像素让出CPU
      if (i % 100 == 0) yield();
    }
    Serial.println("像素绘制完成");
  } else {
    Serial.println("不绘制像素画");
  }
  
  // 显示时间 HH:MM
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  
  dma_display->setTextSize(clockConfig.time.fontSize);
  dma_display->setCursor(clockConfig.time.x, clockConfig.time.y);
  dma_display->setTextColor(dma_display->color565(
    clockConfig.time.r, 
    clockConfig.time.g, 
    clockConfig.time.b
  ));
  dma_display->print(timeStr);
  
  // 显示日期 MM-DD
  if (clockConfig.date.show) {
    char dateStr[6];
    sprintf(dateStr, "%02d-%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
    
    dma_display->setTextSize(clockConfig.date.fontSize);
    dma_display->setCursor(clockConfig.date.x, clockConfig.date.y);
    dma_display->setTextColor(dma_display->color565(
      clockConfig.date.r, 
      clockConfig.date.g, 
      clockConfig.date.b
    ));
    dma_display->print(dateStr);
  }
  
  // 显示星期
  if (clockConfig.week.show) {
    const char* weekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    dma_display->setTextSize(1);
    dma_display->setCursor(clockConfig.week.x, clockConfig.week.y);
    dma_display->setTextColor(dma_display->color565(
      clockConfig.week.r, 
      clockConfig.week.g, 
      clockConfig.week.b
    ));
    dma_display->print(weekDays[timeinfo.tm_wday]);
  }
}

// WebSocket 事件处理
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket 客户端已连接: %u\n", client->id());
    clientConnected = true;
    
    // 每次连接都强制回到闹钟模式
    currentMode = MODE_CLOCK;
    canvasInitialized = false; // 重置画布初始化标志
    
    // 清空画布相关数据
    if (blackPixels != nullptr) {
      free(blackPixels);
      blackPixels = nullptr;
    }
    blackPixelCount = 0;
    Serial.println("✓ WebSocket连接时已清空黑色像素数据");
    
    displayClock();
    
    // 发送连接成功消息，始终返回 clock 模式
    String response = "{\"status\":\"connected\",\"device\":\"LED-Matrix\",\"mode\":\"clock\"}";
    client->text(response);
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket 客户端已断开: %u\n", client->id());
    clientConnected = false;
    
    // 断开连接后，回到闹钟模式
    currentMode = MODE_CLOCK;
    canvasInitialized = false; // 重置画布初始化标志
    
    // 清空画布相关数据
    if (blackPixels != nullptr) {
      free(blackPixels);
      blackPixels = nullptr;
    }
    blackPixelCount = 0;
    Serial.println("✓ WebSocket断开时已清空黑色像素数据");
    
    displayClock();
  }
  else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    
    // 处理二进制数据
    if (info->opcode == WS_BINARY) {
      // 二进制格式：每5字节一个像素 (x, y, r, g, b)
      int pixelCount = len / 5;
      
      if (pixelCount > 0) {
        // 画板模式：保存到 canvasBuffer（64x64完整画布）
        if (currentMode == MODE_CANVAS) {
          
          // 第一次接收时初始化画布（清空）
          if (!canvasInitialized) {
            memset(canvasBuffer, 0, sizeof(canvasBuffer));
            canvasInitialized = true;
            
            // 清空之前的黑色像素记录
            if (blackPixels != nullptr) {
              free(blackPixels);
              blackPixels = nullptr;
            }
            blackPixelCount = 0;
          }
          
          // 先统计本批有效的黑色像素数量（在范围内的）
          int newBlackCount = 0;
          for (size_t i = 0; i + 4 < len; i += 5) {
            uint8_t x = data[i];
            uint8_t y = data[i + 1];
            uint8_t r = data[i + 2];
            uint8_t g = data[i + 3];
            uint8_t b = data[i + 4];
            if (x < PANEL_RES_X && y < PANEL_RES_Y && r == 0 && g == 0 && b == 0) {
              newBlackCount++;
            }
          }
          
          // 扩展黑色像素数组（累加，不是替换）
          if (newBlackCount > 0) {
            BlackPixel* newBuffer = (BlackPixel*)realloc(blackPixels, sizeof(BlackPixel) * (blackPixelCount + newBlackCount));
            if (newBuffer == nullptr) {
              return;
            }
            blackPixels = newBuffer;
          }
          
          int validPixels = 0;
          for (size_t i = 0; i + 4 < len; i += 5) {
            uint8_t x = data[i];
            uint8_t y = data[i + 1];
            uint8_t r = data[i + 2];
            uint8_t g = data[i + 3];
            uint8_t b = data[i + 4];
            
            if (x < PANEL_RES_X && y < PANEL_RES_Y) {
              // 存储到画布缓冲区
              canvasBuffer[y][x][0] = r;
              canvasBuffer[y][x][1] = g;
              canvasBuffer[y][x][2] = b;
              validPixels++;
              
              // 如果是黑色，记录坐标
              if (r == 0 && g == 0 && b == 0 && blackPixels != nullptr) {
                blackPixels[blackPixelCount].x = x;
                blackPixels[blackPixelCount].y = y;
                blackPixelCount++;
              }
              
              // 立即绘制
              dma_display->drawPixelRGB888(x, y, r, g, b);
            }
            
            if (i % 500 == 0) yield();
          }
          
          return;
        }
        
        // 闹钟模式：保存到 imagePixels（背景图）
        // 如果是新的一批数据（距离上次超过1秒），清空旧数据重新开始
        if (millis() - lastBinaryReceiveTime > 1000) {
          if (imagePixels != nullptr) {
            free(imagePixels);
            imagePixels = nullptr;
          }
          imagePixelCount = 0;
        }
        
        // 追加数据
        PixelData* newBuffer = (PixelData*)realloc(imagePixels, sizeof(PixelData) * (imagePixelCount + pixelCount));
        if (newBuffer != nullptr) {
          imagePixels = newBuffer;
          
          int validPixels = 0;
          // 复制像素数据
          for (size_t i = 0; i + 4 < len; i += 5) {
            // 达到最大像素数量后，忽略后续数据，避免占用过多 NVS 空间
            if (imagePixelCount >= MAX_PIXELS) {
              break;
            }
            
            uint8_t x = data[i];
            uint8_t y = data[i + 1];
            uint8_t r = data[i + 2];
            uint8_t g = data[i + 3];
            uint8_t b = data[i + 4];
            
            if (x < PANEL_RES_X && y < PANEL_RES_Y) {
              // 保存到内存
              imagePixels[imagePixelCount].x = x;
              imagePixels[imagePixelCount].y = y;
              imagePixels[imagePixelCount].r = r;
              imagePixels[imagePixelCount].g = g;
              imagePixels[imagePixelCount].b = b;
              imagePixelCount++;
              validPixels++;
              
              // 立即绘制
              dma_display->drawPixelRGB888(x, y, r, g, b);
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
      
      return;
    }
    
    // 处理JSON命令
    Serial.printf("收到 WebSocket 数据: %d bytes\n", len);
    
    // 解析JSON命令 - 使用动态分配以支持大图片数据
    DynamicJsonDocument doc(40960); // 40KB 缓冲区
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
      Serial.print("JSON解析失败: ");
      Serial.println(error.c_str());
      client->text("{\"error\":\"invalid json\"}");
      return;
    }
    
    String cmd = doc["cmd"].as<String>();
    Serial.println("命令: " + cmd);
    
    StaticJsonDocument<512> response;
    response["status"] = "ok";
    
    if (cmd == "ping") {
      response["message"] = "pong";
    }
    else if (cmd == "status") {
      response["ip"] = config_mode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
      response["width"] = PANEL_RES_X;
      response["height"] = PANEL_RES_Y;
      response["brightness"] = currentBrightness;
      if (currentMode == MODE_CLOCK) {
        response["mode"] = "clock";
      } else if (currentMode == MODE_CANVAS) {
        response["mode"] = "canvas";
      }
    }
    else if (cmd == "set_mode") {
      String mode = doc["mode"].as<String>();
      if (mode == "clock") {
        currentMode = MODE_CLOCK;
        displayClock();
        response["message"] = "switched to clock mode";
      } else if (mode == "canvas") {
        currentMode = MODE_CANVAS;
        
        // 如果有画布缓冲区数据，恢复显示
        if (canvasInitialized) {
          Serial.println("恢复画布缓冲区数据");
          for (int y = 0; y < PANEL_RES_Y; y++) {
            for (int x = 0; x < PANEL_RES_X; x++) {
              uint8_t r = canvasBuffer[y][x][0];
              uint8_t g = canvasBuffer[y][x][1];
              uint8_t b = canvasBuffer[y][x][2];
              if (r > 0 || g > 0 || b > 0) {
                dma_display->drawPixelRGB888(x, y, r, g, b);
              }
            }
            if (y % 8 == 0) yield();
          }
        } else {
          dma_display->clearScreen();
        }
        
        response["message"] = "switched to canvas mode";
      } else {
        response["status"] = "error";
        response["message"] = "invalid mode";
      }
    }
    else if (cmd == "set_clock_config") {
      // 接收闹钟配置
      JsonObject config = doc["config"];
      
      // 时间配置
      if (config.containsKey("time")) {
        JsonObject time = config["time"];
        clockConfig.time.fontSize = time["fontSize"] | 1;
        clockConfig.time.x = time["x"] | 17;
        clockConfig.time.y = time["y"] | 18;
        JsonObject timeColor = time["color"];
        clockConfig.time.r = timeColor["r"] | 100;
        clockConfig.time.g = timeColor["g"] | 200;
        clockConfig.time.b = timeColor["b"] | 255;
      }
      
      // 日期配置
      if (config.containsKey("date")) {
        JsonObject date = config["date"];
        clockConfig.date.show = date["show"] | true;
        clockConfig.date.fontSize = date["fontSize"] | 1;
        clockConfig.date.x = date["x"] | 3;
        clockConfig.date.y = date["y"] | 30;
        JsonObject dateColor = date["color"];
        clockConfig.date.r = dateColor["r"] | 120;
        clockConfig.date.g = dateColor["g"] | 120;
        clockConfig.date.b = dateColor["b"] | 120;
      }
      
      // 星期配置
      if (config.containsKey("week")) {
        JsonObject week = config["week"];
        clockConfig.week.show = week["show"] | true;
        clockConfig.week.x = week["x"] | 23;
        clockConfig.week.y = week["y"] | 44;
        JsonObject weekColor = week["color"];
        clockConfig.week.r = weekColor["r"] | 100;
        clockConfig.week.g = weekColor["g"] | 100;
        clockConfig.week.b = weekColor["b"] | 100;
      }
      
      // 图片配置
      if (config.containsKey("image")) {
        JsonObject image = config["image"];
        clockConfig.image.show = image["show"] | false;
        clockConfig.image.x = image["x"] | 0;
        clockConfig.image.y = image["y"] | 0;
        clockConfig.image.width = image["width"] | 64;
        clockConfig.image.height = image["height"] | 64;
      }
      
      // 保存到 Preferences
      preferences.begin("clock", false);
      preferences.putBytes("config", &clockConfig, sizeof(ClockConfig));
      preferences.end();
      
      Serial.println("闹钟配置已更新并保存");
      
      // 如果当前是闹钟模式，立即刷新显示
      if (currentMode == MODE_CLOCK) {
        displayClock();
      }
      
      response["message"] = "clock config updated and saved";
    }
    else if (cmd == "set_image_pixels") {
      // 接收图片像素数据
      JsonArray pixels = doc["pixels"];
      
      if (pixels.size() == 0) {
        response["status"] = "error";
        response["message"] = "no pixels data";
      } else {
        // 释放旧的像素数据
        if (imagePixels != nullptr) {
          free(imagePixels);
          imagePixels = nullptr;
        }
        
        // 分配新的像素数据（最多 64x64 像素）
        imagePixelCount = pixels.size();
        if (imagePixelCount > MAX_PIXELS) {
          Serial.printf("像素数量过大（%d），截断到最大值 %d\n", imagePixelCount, MAX_PIXELS);
          imagePixelCount = MAX_PIXELS;
        }
        imagePixels = (PixelData*)malloc(sizeof(PixelData) * imagePixelCount);
        
        if (imagePixels == nullptr) {
          response["status"] = "error";
          response["message"] = "memory allocation failed";
          imagePixelCount = 0;
        } else {
          // 复制像素数据
          for (int i = 0; i < imagePixelCount; i++) {
            JsonObject pixel = pixels[i];
            imagePixels[i].x = pixel["x"];
            imagePixels[i].y = pixel["y"];
            imagePixels[i].r = pixel["r"];
            imagePixels[i].g = pixel["g"];
            imagePixels[i].b = pixel["b"];
            
            // 每100个像素让出CPU
            if (i % 100 == 0) yield();
          }
          
          // 保存像素数据到 Preferences（限制为最多 64x64 像素）
          preferences.begin("clock", false);
          size_t dataSize = imagePixelCount * sizeof(PixelData);
          size_t maxDataSize = MAX_PIXELS * sizeof(PixelData);
          if (dataSize <= maxDataSize) {
            preferences.putBytes("pixels", imagePixels, dataSize);
            preferences.putInt("pixelCount", imagePixelCount);
            Serial.printf("像素数据已保存: %d 个像素（%d bytes）\n", imagePixelCount, dataSize);
          } else {
            Serial.printf("像素数据太大（%d bytes），无法保存到 Preferences，最大允许 %d bytes\n",
              dataSize, maxDataSize);
          }
          preferences.end();
          
          // 如果当前是闹钟模式，立即刷新显示
          if (currentMode == MODE_CLOCK) {
            displayClock();
          }
          
          response["message"] = "image pixels updated and saved";
          response["count"] = imagePixelCount;
        }
      }
    }
    else if (cmd == "get_pixel_count") {
      // 查询当前保存的像素数量
      response["pixelCount"] = imagePixelCount;
      response["message"] = "current pixel count";
    }
    else if (cmd == "clear") {
      dma_display->clearScreen();
      
      // 如果是画板模式，清空画布缓冲区
      if (currentMode == MODE_CANVAS && canvasInitialized) {
        Serial.println("清空画布缓冲区");
        memset(canvasBuffer, 0, sizeof(canvasBuffer));
      }
      
      response["message"] = "cleared";
    }
    else if (cmd == "brightness") {
      int value = doc["value"];
      if (value >= 0 && value <= 255) {
        currentBrightness = value;
        dma_display->setBrightness8(value);
        response["brightness"] = value;
      } else {
        response["status"] = "error";
        response["message"] = "brightness must be 0-255";
      }
    }
    else if (cmd == "highlight_color") {
      // 高亮指定颜色，其他颜色降低亮度
      // 如果 color 为空或 null，则恢复所有颜色的原亮度
      if (currentMode == MODE_CANVAS && canvasInitialized) {
        bool hasHighlight = doc.containsKey("color") && !doc["color"].isNull();
        uint8_t highlightR = 0, highlightG = 0, highlightB = 0;
        
        if (hasHighlight) {
          JsonObject color = doc["color"];
          highlightR = color["r"] | 0;
          highlightG = color["g"] | 0;
          highlightB = color["b"] | 0;
          
          // 如果是黑色，用存储的坐标亮白灯
          if (highlightR == 0 && highlightG == 0 && highlightB == 0) {
            // 先清空屏幕
            dma_display->clearScreen();
            // 然后让黑色像素位置亮白灯
            for (int i = 0; i < blackPixelCount; i++) {
              dma_display->drawPixelRGB888(blackPixels[i].x, blackPixels[i].y, 255, 255, 255);
            }
          } else {
            // 处理其他颜色：匹配的亮白灯，不匹配的不亮
            dma_display->clearScreen(); // 先清空屏幕
            
            for (int y = 0; y < PANEL_RES_Y; y++) {
              for (int x = 0; x < PANEL_RES_X; x++) {
                uint8_t r = canvasBuffer[y][x][0];
                uint8_t g = canvasBuffer[y][x][1];
                uint8_t b = canvasBuffer[y][x][2];
                
                if (r > 0 || g > 0 || b > 0) {
                  // 判断是否是高亮颜色（允许一定误差）
                  bool isHighlight = (abs(r - highlightR) <= 2 && 
                                     abs(g - highlightG) <= 2 && 
                                     abs(b - highlightB) <= 2);
                  
                  if (isHighlight) {
                    // 匹配的颜色，亮白灯
                    dma_display->drawPixelRGB888(x, y, 255, 255, 255);
                  }
                  // 不匹配的颜色不亮（保持黑色）
                }
              }
              if (y % 8 == 0) yield();
            }
          }
        } else {
          // 恢复显示完整的彩色像素图
          for (int y = 0; y < PANEL_RES_Y; y++) {
            for (int x = 0; x < PANEL_RES_X; x++) {
              uint8_t r = canvasBuffer[y][x][0];
              uint8_t g = canvasBuffer[y][x][1];
              uint8_t b = canvasBuffer[y][x][2];
              dma_display->drawPixelRGB888(x, y, r, g, b);
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
      // 高亮指定行，其他行降低亮度
      // 如果 row 为 -1 或 null，则恢复所有行的原亮度
      if (currentMode == MODE_CANVAS && canvasInitialized) {
        int highlightRow = doc["row"] | -1;
        
        // 遍历画布缓冲区，调整亮度
        for (int y = 0; y < PANEL_RES_Y; y++) {
          for (int x = 0; x < PANEL_RES_X; x++) {
            uint8_t r = canvasBuffer[y][x][0];
            uint8_t g = canvasBuffer[y][x][1];
            uint8_t b = canvasBuffer[y][x][2];
            
            if (r > 0 || g > 0 || b > 0) {
              uint8_t displayR = r, displayG = g, displayB = b;
              
              if (highlightRow >= 0 && y != highlightRow) {
                // 不是高亮行，降低亮度到20%
                displayR = r * 0.2;
                displayG = g * 0.2;
                displayB = b * 0.2;
              }
              
              dma_display->drawPixelRGB888(x, y, displayR, displayG, displayB);
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
    else if (cmd == "text") {
      String text = doc["text"].as<String>();
      int x = doc["x"] | 0;
      int y = doc["y"] | 0;
      dma_display->clearScreen();
      dma_display->setCursor(x, y);
      dma_display->setTextColor(dma_display->color565(255, 255, 255));
      dma_display->print(text);
      response["message"] = "text displayed";
    }
    else if (cmd == "pixel") {
      int x = doc["x"];
      int y = doc["y"];
      int r = doc["r"] | 255;
      int g = doc["g"] | 255;
      int b = doc["b"] | 255;
      if (x >= 0 && x < PANEL_RES_X && y >= 0 && y < PANEL_RES_Y) {
        dma_display->drawPixelRGB888(x, y, r, g, b);
        response["message"] = "pixel set";
      } else {
        response["status"] = "error";
        response["message"] = "pixel out of range";
      }
    }
    else if (cmd == "image") {
      JsonArray pixels = doc["pixels"];
      int width = doc["width"] | PANEL_RES_X;
      int height = doc["height"] | PANEL_RES_Y;
      
      dma_display->clearScreen();
      int idx = 0;
      for (int y = 0; y < height && y < PANEL_RES_Y; y++) {
        for (int x = 0; x < width && x < PANEL_RES_X; x++) {
          if (idx + 2 < pixels.size()) {
            uint8_t r = pixels[idx++];
            uint8_t g = pixels[idx++];
            uint8_t b = pixels[idx++];
            dma_display->drawPixelRGB888(x, y, r, g, b);
          }
        }
        // 每行让出CPU，防止看门狗超时
        if (y % 8 == 0) yield();
      }
      response["message"] = "image displayed";
    }
    else if (cmd == "image_sparse") {
      // 稀疏像素格式：只传输非空像素
      // 不自动清屏，由客户端控制
      JsonArray pixels = doc["pixels"];
      
      for (JsonVariant pixel : pixels) {
        int x = pixel["x"];
        int y = pixel["y"];
        uint8_t r = pixel["r"];
        uint8_t g = pixel["g"];
        uint8_t b = pixel["b"];
        
        if (x >= 0 && x < PANEL_RES_X && y >= 0 && y < PANEL_RES_Y) {
          dma_display->drawPixelRGB888(x, y, r, g, b);
        }
      }
      response["message"] = "sparse pixels drawn";
      response["count"] = pixels.size();
    }
    else if (cmd == "image_chunk") {
      // 分块传输格式
      JsonArray pixels = doc["pixels"];
      int width = doc["width"];
      int offsetX = doc["offsetX"] | 0;
      int offsetY = doc["offsetY"] | 0;
      int height = doc["height"];
      int chunk = doc["chunk"] | 0;
      int total = doc["total"] | 1;
      
      // 第一块时清空屏幕
      if (chunk == 0) {
        dma_display->clearScreen();
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
            
            if (drawX >= 0 && drawX < PANEL_RES_X && drawY >= 0 && drawY < PANEL_RES_Y) {
              dma_display->drawPixelRGB888(drawX, drawY, r, g, b);
            }
          }
        }
        // 每行让出CPU，防止看门狗超时
        if (y % 8 == 0) yield();
      }
      response["message"] = "chunk displayed";
      response["chunk"] = chunk;
      response["total"] = total;
    }
    else {
      response["status"] = "error";
      response["message"] = "unknown command";
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    client->text(responseStr);
  }
}

void setupServer() {
  // 设置 WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到测试请求");
    request->send(200, "text/plain", "ESP32 is working!");
  });
  
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("404: %s %s\n", request->methodToString(), request->url().c_str());
    if (config_mode) {
      request->redirect("/");
    } else {
      request->send(404, "text/plain", "Not Found");
    }
  });
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到根路径请求");
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>LED 矩阵配置</title>";
    html += "<style>body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;padding:20px;max-width:400px;margin:0 auto;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh}";
    html += ".card{background:white;padding:24px;border-radius:16px;box-shadow:0 8px 32px rgba(0,0,0,0.2)}";
    html += "h2{color:#333;margin-top:0;text-align:center;font-size:24px}";
    html += ".subtitle{color:#666;text-align:center;font-size:14px;margin-bottom:20px}";
    html += "label{display:block;color:#555;font-size:14px;margin-bottom:8px;font-weight:500}";
    html += "input{width:100%;padding:12px;margin-bottom:16px;border:2px solid #e0e0e0;border-radius:8px;box-sizing:border-box;font-size:15px}";
    html += "input:focus{outline:none;border-color:#667eea}";
    html += "button{width:100%;padding:14px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;border:none;border-radius:8px;cursor:pointer;font-size:16px;font-weight:600}";
    html += "button:active{transform:scale(0.98)}";
    html += ".status{padding:12px;border-radius:8px;margin-bottom:16px;text-align:center;font-size:14px}";
    html += ".success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}";
    html += ".error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}";
    html += ".info{background:#d1ecf1;color:#0c5460;border:1px solid #bee5eb}";
    html += "</style></head><body>";
    html += "<div class='card'><h2>LED 矩阵配置</h2>";
    html += "<div class='subtitle'>连接到您的 WiFi 网络</div>";
    html += "<div id='status'></div>";
    html += "<label>WiFi 名称 (SSID)：</label>";
    html += "<input type='text' id='ssid' placeholder='请输入 WiFi 名称'>";
    html += "<label>WiFi 密码：</label>";
    html += "<input type='password' id='password' placeholder='请输入 WiFi 密码'>";
    html += "<button onclick='saveWiFi()'>连接</button></div>";
    html += "<script>async function saveWiFi(){";
    html += "const ssid=document.getElementById('ssid').value;";
    html += "const pwd=document.getElementById('password').value;";
    html += "if(!ssid||!pwd){showStatus('请填写完整的 WiFi 信息','error');return;}";
    html += "showStatus('正在连接 '+ssid+'...','info');";
    html += "const formData=new FormData();formData.append('ssid',ssid);formData.append('password',pwd);";
    html += "try{const res=await fetch('/save',{method:'POST',body:formData});";
    html += "const data=await res.json();showStatus('配置已保存！设备重启中...','success');";
    html += "setTimeout(()=>{showStatus('设备已重启，请关闭此页面并连接到您的 WiFi','success');},3000);";
    html += "}catch(e){showStatus('连接失败，请检查密码是否正确','error');}}";
    html += "function showStatus(msg,type){document.getElementById('status').innerHTML='<div class=\"'+type+'\">'+msg+'</div>';}</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });
  
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String password = request->getParam("password", true)->value();
      
      Serial.println("保存WiFi配置: " + ssid);
      
      preferences.begin("wifi", false);
      preferences.putString("ssid", ssid);
      preferences.putString("password", password);
      preferences.end();
      
      request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"配置已保存，正在重启...\"}");
      
      Serial.println("3秒后重启...");
      delay(3000);
      ESP.restart();
    } else {
      request->send(400, "application/json", "{\"error\":\"参数错误\"}");
    }
  });
  
  // 清除WiFi配置
  server.on("/clear-wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("清除WiFi配置");
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"WiFi配置已清除，3秒后重启\"}");
    Serial.println("3秒后重启...");
    delay(3000);
    ESP.restart();
  });
  
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到状态查询请求");
    StaticJsonDocument<300> doc;
    doc["status"] = "ok";
    doc["ip"] = config_mode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    doc["width"] = PANEL_RES_X;
    doc["height"] = PANEL_RES_Y;
    doc["brightness"] = currentBrightness;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/upload", HTTP_POST, 
    [](AsyncWebServerRequest *request){
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if (index == 0) {
        Serial.println("开始接收图片...");
        if (imageBuffer) free(imageBuffer);
        imageBuffer = nullptr;
        imageSize = 0;
      }
      
      uint8_t* newBuffer = (uint8_t*)realloc(imageBuffer, imageSize + len);
      if (newBuffer) {
        imageBuffer = newBuffer;
        memcpy(imageBuffer + imageSize, data, len);
        imageSize += len;
      }
      
      if (final) {
        Serial.printf("接收完成，大小: %d bytes\n", imageSize);
        if (imageBuffer != nullptr && imageSize > 0) {
          int width = request->hasParam("width") ? request->getParam("width")->value().toInt() : PANEL_RES_X;
          int height = request->hasParam("height") ? request->getParam("height")->value().toInt() : PANEL_RES_Y;
          displayImage(imageBuffer, imageSize, width, height);
        } else {
          Serial.println("✗ 图片缓冲区为空，无法显示");
        }
        
        // 释放缓冲区，避免内存泄漏
        if (imageBuffer) {
          free(imageBuffer);
          imageBuffer = nullptr;
        }
        imageSize = 0;
      }
    }
  );
  
  server.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request){
    dma_display->clearScreen();
    request->send(200, "application/json", "{\"status\":\"cleared\"}");
  });
  
  server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("value", true)) {
      int brightness = request->getParam("value", true)->value().toInt();
      if (brightness >= 0 && brightness <= 255) {
        currentBrightness = brightness;
        dma_display->setBrightness8(brightness);
        request->send(200, "application/json", "{\"status\":\"ok\",\"brightness\":" + String(brightness) + "}");
      } else {
        request->send(400, "application/json", "{\"error\":\"brightness must be 0-255\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"missing value parameter\"}");
    }
  });
  
  server.on("/text", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("content", true)) {
      String text = request->getParam("content", true)->value();
      int x = request->hasParam("x", true) ? request->getParam("x", true)->value().toInt() : 0;
      int y = request->hasParam("y", true) ? request->getParam("y", true)->value().toInt() : 0;
      
      dma_display->clearScreen();
      dma_display->setCursor(x, y);
      dma_display->setTextColor(dma_display->color565(255, 255, 255));
      dma_display->print(text);
      
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing content parameter\"}");
    }
  });
  
  server.begin();
  Serial.println("HTTP服务器已启动在端口80");
  Serial.println("等待客户端连接...");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n=================================");
  Serial.println("ESP32 LED图片显示系统启动...");
  Serial.println("=================================");
  
  Serial.println("1. 初始化LED灯板...");
  setupMatrix();
  
  Serial.println("2. 加载闹钟配置...");
  preferences.begin("clock", true);
  
  // 加载配置
  size_t configSize = preferences.getBytesLength("config");
  Serial.printf("配置大小: %d bytes (期望: %d bytes)\n", configSize, sizeof(ClockConfig));
  
  if (configSize == sizeof(ClockConfig)) {
    preferences.getBytes("config", &clockConfig, sizeof(ClockConfig));
    Serial.println("已加载保存的闹钟配置");
    Serial.printf("  image.show = %d\n", clockConfig.image.show);
  } else {
    Serial.println("使用默认闹钟配置");
  }
  
  // 加载像素数据
  imagePixelCount = preferences.getInt("pixelCount", 0);
  Serial.printf("像素数量: %d\n", imagePixelCount);
  
  if (imagePixelCount > 0) {
    size_t pixelDataSize = preferences.getBytesLength("pixels");
    Serial.printf("像素数据大小: %d bytes (期望: %d bytes)\n", 
      pixelDataSize, sizeof(PixelData) * imagePixelCount);
    
    if (pixelDataSize == sizeof(PixelData) * imagePixelCount) {
      imagePixels = (PixelData*)malloc(pixelDataSize);
      if (imagePixels != nullptr) {
        preferences.getBytes("pixels", imagePixels, pixelDataSize);
        Serial.printf("✓ 已加载保存的像素数据: %d 个像素\n", imagePixelCount);
        
        // 如果有像素数据，自动启用图片显示
        clockConfig.image.show = true;
        Serial.println("✓ 已自动启用图片显示");
      } else {
        Serial.println("✗ 像素数据内存分配失败");
        imagePixelCount = 0;
      }
    } else {
      Serial.println("✗ 像素数据大小不匹配");
      imagePixelCount = 0;
    }
  } else {
    Serial.println("没有保存的像素数据");
  }
  
  preferences.end();
  
  Serial.println("3. 连接WiFi...");
  setupWiFi();
  
  Serial.println("4. 启动HTTP和WebSocket服务器...");
  setupServer();
  
  Serial.println("\n=================================");
  Serial.println("系统就绪！");
  if (config_mode) {
    Serial.print("AP模式访问地址: http://");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.print("STA模式访问地址: http://");
    Serial.println(WiFi.localIP());
    Serial.println("WebSocket 路径: ws://" + WiFi.localIP().toString() + "/ws");
  }
  Serial.println("=================================\n");
}

void loop() {
  if (config_mode) {
    dnsServer.processNextRequest();
  }

  // 检查是否有待保存的二进制像素数据
  // 如果距离最后一次接收超过500ms，认为接收完成，保存到 Preferences
  if (binaryDataPending && millis() - lastBinaryReceiveTime > 500) {
    binaryDataPending = false;
    
    if (imagePixels != nullptr && imagePixelCount > 0) {
      Serial.printf("=== 开始保存像素数据到 Preferences: %d 个像素 ===\n", imagePixelCount);
      
      preferences.begin("clock", false);
      
      size_t dataSize = imagePixelCount * sizeof(PixelData);
      Serial.printf("数据大小: %d bytes\n", dataSize);
      
      size_t maxDataSize = MAX_PIXELS * sizeof(PixelData);
      if (dataSize <= maxDataSize) {
        preferences.putBytes("pixels", imagePixels, dataSize);
        preferences.putInt("pixelCount", imagePixelCount);
        
        // 同时保存 clockConfig，确保 image.show 也被保存
        preferences.putBytes("config", &clockConfig, sizeof(ClockConfig));
        
        preferences.end();
        
        Serial.println("✓✓✓ 像素数据已保存到 Preferences ✓✓✓");
        Serial.printf("保存了 %d 个像素（%d bytes），image.show=%d\n", imagePixelCount, dataSize, clockConfig.image.show);
        
        // 发送确认消息给客户端，告知实际接收的像素数量
        StaticJsonDocument<128> confirmDoc;
        confirmDoc["status"] = "ok";
        confirmDoc["message"] = "pixels_received";
        confirmDoc["count"] = imagePixelCount;
        
        String confirmMsg;
        serializeJson(confirmDoc, confirmMsg);
        ws.textAll(confirmMsg);
        
      } else {
        preferences.end();
        Serial.printf("✗✗✗ 像素数据太大（%d bytes），无法保存，最大允许 %d bytes ✗✗✗\n",
          dataSize, maxDataSize);
      }
      
      // 保存完成后，如果是闹钟模式，刷新显示
      if (currentMode == MODE_CLOCK) {
        displayClock();
      }
    }
  }

  // 在闹钟模式下，每分钟更新一次时钟（而不是每秒）
  // 但在配置模式下不执行，避免覆盖热点信息显示
  if (currentMode == MODE_CLOCK && !config_mode) {
    static unsigned long lastClockUpdate = 0;
    static int lastMinute = -1;
    
    // 获取当前时间
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      // 只有分钟变化时才刷新
      if (timeinfo.tm_min != lastMinute) {
        lastMinute = timeinfo.tm_min;
        displayClock();
      }
    } else {
      // 如果时间未同步，每10秒尝试刷新一次
      if (millis() - lastClockUpdate > 10000) {
        lastClockUpdate = millis();
        displayClock();
      }
    }
  }
  
  // 清理WebSocket，防止内存泄漏
  ws.cleanupClients();
  
  yield();
}
