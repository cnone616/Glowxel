#include "web_server.h"
#include "websocket_handler.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "config_manager.h"
#include "ota_manager.h"

// 静态成员初始化
AsyncWebServer WebServer::server(80);
uint8_t* WebServer::imageBuffer = nullptr;
size_t WebServer::imageSize = 0;

void WebServer::init() {
  Serial.println("4. 启动HTTP和WebSocket服务器...");
  setupServer();
}

void WebServer::setupServer() {
  // 设置 WebSocket
  server.addHandler(&WebSocketHandler::ws);
  
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  
  setupRoutes();
  setupAPIRoutes();
  
  server.begin();
  Serial.println("HTTP服务器已启动在端口80");
  Serial.println("等待客户端连接...");
}

void WebServer::setupRoutes() {
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到测试请求");
    request->send(200, "text/plain", "ESP32 is working!");
  });

  server.on("/clear-wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("清除WiFi配置");
    ConfigManager::preferences.begin("wifi", false);
    ConfigManager::preferences.clear();
    ConfigManager::preferences.end();
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"WiFi配置已清除，3秒后重启\"}");
    Serial.println("3秒后重启...");
    delay(3000);
    ESP.restart();
  });
  
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("404: %s %s\n", request->methodToString(), request->url().c_str());
    request->send(404, "text/plain", "Not Found");
  });
}

void WebServer::setupAPIRoutes() {
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到状态查询请求");
    StaticJsonDocument<300> doc;
    doc["status"] = "ok";
    doc["ip"] = WiFiManager::getDeviceIP();
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["device_type"] = DEVICE_TYPE;
    doc["width"] = DisplayManager::PANEL_RES_X;
    doc["height"] = DisplayManager::PANEL_RES_Y;
    doc["brightness"] = DisplayManager::currentBrightness;
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
          int width = request->hasParam("width") ? request->getParam("width")->value().toInt() : DisplayManager::PANEL_RES_X;
          int height = request->hasParam("height") ? request->getParam("height")->value().toInt() : DisplayManager::PANEL_RES_Y;
          DisplayManager::displayImage(imageBuffer, imageSize, width, height);
        } else {
          Serial.println("✗ 图片缓冲区为空，无法显示");
        }
        
        if (imageBuffer) {
          free(imageBuffer);
          imageBuffer = nullptr;
        }
        imageSize = 0;
      }
    }
  );
  
  server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("value", true)) {
      int brightness = request->getParam("value", true)->value().toInt();
      if (brightness >= 0 && brightness <= 255) {
        DisplayManager::setBrightness(brightness);
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
      
      DisplayManager::dma_display->clearScreen();
      DisplayManager::dma_display->setCursor(x, y);
      DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(255, 255, 255));
      DisplayManager::dma_display->print(text);
      
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing content parameter\"}");
    }
  });
}

void WebServer::handleLoop() {
  // Web服务器的循环处理（如果需要）
}
