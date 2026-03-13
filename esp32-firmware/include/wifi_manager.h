#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>

class WiFiManager {
public:
  static void init();
  static void setupWiFi();
  static bool isConfigMode();
  static String getDeviceIP();
  
  static bool config_mode;
  static String saved_ssid;
  static String saved_password;
  static DNSServer dnsServer;
  
private:
  static const char* ap_ssid;
  static const char* ap_password;
};

#endif