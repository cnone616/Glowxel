#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class WebServer {
public:
  static void init();
  static void setupServer();
  static void handleLoop();
  
  static AsyncWebServer server;
  
  // 图片上传缓冲区
  static uint8_t* imageBuffer;
  static size_t imageSize;
  
private:
  static void setupRoutes();
  static void setupConfigRoutes();
  static void setupAPIRoutes();
};

#endif