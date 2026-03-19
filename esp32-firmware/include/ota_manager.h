#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

// 当前固件版本号，每次发布新版本递增
#define FIRMWARE_VERSION "1.0.0"
#define DEVICE_TYPE "renlight-64"

class OTAManager {
public:
  static void init();
  static void checkUpdate();
  static bool hasNewVersion();
  static String getLatestVersion();
  static String getChangelog();
  static bool startUpdate();
  
  static String serverUrl;  // OTA 服务器地址
  static String latestVersion;
  static String firmwareUrl;
  static String changelog;
  static String md5Hash;
  static int firmwareSize;
  static bool updateAvailable;
  static bool isForce;
};

#endif

