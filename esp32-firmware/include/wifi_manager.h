#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
  static void init();
  static void setupWiFi();
  static void tick();
  static void showTimeSyncScreen();
  static bool isConfigMode();
  static bool isTimeSynced();
  static String getDeviceIP();
  
  static bool config_mode;
  static String saved_ssid;
  static String saved_password;
  static unsigned long last_ntp_retry_at;
  static bool ntp_sync_logged;
};

#endif
