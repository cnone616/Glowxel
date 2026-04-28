#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class WebServer {
public:
  static void initConfigPortal();
  static void initRuntime();
  static void handleLoop();
  static bool isSettingsSessionActive();
  
  static AsyncWebServer server;
  
  // 图片上传缓冲区
  static uint8_t* imageBuffer;
  static size_t imageSize;
  
private:
  static void setupPortalRoutes();
  static void setupRuntimeRoutes();
  static void setupAPIRoutes();
};

#endif
