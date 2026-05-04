#include "ble_config.h"
#include "config_manager.h"
#include "display_manager.h"

// 静态成员初始化
BLEServer* BLEConfig::pServer = nullptr;
BLECharacteristic* BLEConfig::pWifiChar = nullptr;
BLECharacteristic* BLEConfig::pStatusChar = nullptr;
bool BLEConfig::deviceConnected = false;
bool BLEConfig::wifiConfigReceived = false;
String BLEConfig::receivedSSID = "";
String BLEConfig::receivedPassword = "";

// BLE 连接回调
class BLEConfigServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    BLEConfig::deviceConnected = true;
    // 发送状态通知
    if (BLEConfig::pStatusChar) {
      BLEConfig::pStatusChar->setValue("ready");
      BLEConfig::pStatusChar->notify();
    }
  }

  void onDisconnect(BLEServer* pServer) {
    BLEConfig::deviceConnected = false;
    // 重新开始广播
    pServer->startAdvertising();
  }
};

// WiFi 配置写入回调
class WiFiConfigCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() == 0) return;

    String data = String(value.c_str());

    // 格式: "SSID\nPASSWORD"
    int separatorIndex = data.indexOf('\n');
    if (separatorIndex <= 0) {
      Serial.println("数据格式错误，需要 SSID\\nPASSWORD");
      if (BLEConfig::pStatusChar) {
        BLEConfig::pStatusChar->setValue("error:format");
        BLEConfig::pStatusChar->notify();
      }
      return;
    }

    BLEConfig::receivedSSID = data.substring(0, separatorIndex);
    BLEConfig::receivedPassword = data.substring(separatorIndex + 1);

    // 保存到 Preferences
    ConfigManager::preferences.begin("wifi", false);
    ConfigManager::preferences.putString("ssid", BLEConfig::receivedSSID);
    ConfigManager::preferences.putString("password", BLEConfig::receivedPassword);
    ConfigManager::preferences.end();

    // 通知客户端配置已保存
    if (BLEConfig::pStatusChar) {
      BLEConfig::pStatusChar->setValue("saved");
      BLEConfig::pStatusChar->notify();
    }

    BLEConfig::wifiConfigReceived = true;

    // 后续提示与重启由 main loop 统一处理，避免在 BLE 回调线程里直接操作显示和重启。
  }
};

void BLEConfig::init() {
  BLEDevice::init("Glowxel PixelBoard-Setup");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BLEConfigServerCallbacks());

  // 创建服务
  BLEService* pService = pServer->createService(BLE_SERVICE_UUID);

  // WiFi 配置特征（可写）
  pWifiChar = pService->createCharacteristic(
    BLE_WIFI_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pWifiChar->setCallbacks(new WiFiConfigCallbacks());

  // 状态通知特征（可读 + 通知）
  pStatusChar = pService->createCharacteristic(
    BLE_STATUS_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pStatusChar->addDescriptor(new BLE2902());
  pStatusChar->setValue("waiting");

  pService->start();

  // 开始广播
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void BLEConfig::stop() {
  if (pServer) {
    BLEDevice::deinit(true);
    pServer = nullptr;
    pWifiChar = nullptr;
    pStatusChar = nullptr;
    deviceConnected = false;
  }
}

bool BLEConfig::isActive() {
  return pServer != nullptr;
}
