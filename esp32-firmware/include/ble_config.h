#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE 服务和特征 UUID
#define BLE_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_WIFI_CHAR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BLE_STATUS_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a9"

class BLEConfig {
public:
  static void init();
  static void stop();
  static bool isActive();

  static BLEServer* pServer;
  static BLECharacteristic* pWifiChar;
  static BLECharacteristic* pStatusChar;
  static bool deviceConnected;
  static bool wifiConfigReceived;
  static String receivedSSID;
  static String receivedPassword;
};

#endif

