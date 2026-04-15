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
  static void showTimeSyncScreen();
  static void refreshTimeSync();
  static bool isConfigMode();
  static bool isTimeSynced();
  static String getDeviceIP();
  static String getConnectedSSID();
  static String getConfigPortalSSID();
  static String getConfigPortalIP();
  static size_t getScannedNetworkCount();
  static WiFiScanResultItem getScannedNetwork(size_t index);
  static bool isNetworkScanRunning();
  
  static bool config_mode;
  static String saved_ssid;
  static String saved_password;
  static String config_portal_ssid;
  static unsigned long last_ntp_retry_at;
  static unsigned long portal_restart_at;
  static bool ntp_sync_logged;
  static bool time_synced_once;

private:
  static DNSServer dns_server;
  static WiFiScanResultItem scanned_networks[12];
  static size_t scanned_network_count;
  static bool scan_requested;
  static bool scan_in_progress;
  static void finalizeNetworkScan(int networkCount);
};

#endif
