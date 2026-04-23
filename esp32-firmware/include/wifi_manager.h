#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <DNSServer.h>
#include <WiFi.h>

struct WiFiScanResultItem {
  char ssid[33];
  int32_t rssi;
  bool secure;
};

enum class WifiScanPhase : uint8_t {
  IDLE = 0,
  REQUESTED = 1,
  RUNNING = 2
};

class WiFiManager {
public:
  static void init();
  static void setupWiFi();
  static void startConfigPortal();
  static void stopConfigPortal();
  static void scanNearbyNetworks();
  static void saveConfigPortalCredentials(const String& ssid, const String& password);
  static void schedulePortalRestart(unsigned long delayMs);
  static void tick();
  static void showStationConnectedScreen();
  static void showTimeSyncScreen();
  static void refreshTimeSync();
  static bool hasSavedCredentials();
  static bool isConfigMode();
  static bool isStationConnected();
  static bool isStationConnecting();
  static bool isPortalRestartScheduled();
  static bool isTimeSynced();
  static WifiScanPhase getScanPhase();
  static String getDeviceIP();
  static String getConnectedSSID();
  static String getConfigPortalSSID();
  static String getConfigPortalIP();
  static size_t getScannedNetworkCount();
  static WiFiScanResultItem getScannedNetwork(size_t index);
  static bool isNetworkScanRunning();

private:
  static DNSServer dns_server;
  static WiFiScanResultItem* scanned_networks;
  static size_t scanned_network_count;
  static size_t scanned_network_capacity;
  static bool config_mode;
  static bool sta_connecting;
  static String saved_ssid;
  static String saved_password;
  static String config_portal_ssid;
  static unsigned long last_ntp_retry_at;
  static unsigned long portal_restart_at;
  static unsigned long sta_connect_started_at;
  static unsigned long sta_connect_deadline_at;
  static bool ntp_sync_logged;
  static bool time_synced_once;
  static WifiScanPhase scan_phase;
  static unsigned long scan_started_at;
  static volatile bool scan_result_ready;
  static volatile int scan_result_code;
  static uint32_t scan_attempt_counter;
  static uint32_t active_scan_attempt_id;
  static void beginStationConnect();
  static void finalizeStationConnected();
  static void resetScanState();
  static void clearScannedNetworkCache();
  static void setScanPhase(WifiScanPhase nextPhase, const char* reason);
  static void finalizeNetworkScan(int networkCount);
  static void handleWiFiEvent(arduino_event_t* event);
};

#endif
