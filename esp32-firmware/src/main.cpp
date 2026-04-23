#include <Arduino.h>
#include <time.h>
#include "config_manager.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "config.h"
#include "mode_tags.h"
#include "websocket_handler.h"
#include "web_server.h"
#include "ota_manager.h"
#include "runtime_command_bus.h"
#include "runtime_mode_coordinator.h"
#include "tetris_effect.h"
#include "eyes_effect.h"
#include "game_screensaver_effect.h"
#include "theme_renderer.h"

namespace {
constexpr unsigned long kWebSocketIdleWarnMs = 180000;

enum class BootPhase : uint8_t {
  BOOT_MINIMAL = 0,
  STA_CONNECTING = 1,
  PORTAL_ACTIVE = 2,
  RESTART_PENDING = 3,
  RUNTIME_STARTING = 4,
  RUNTIME_ACTIVE = 5
};

BootPhase gBootPhase = BootPhase::BOOT_MINIMAL;
bool gPortalServerInitialized = false;
bool gRuntimeInitialized = false;
bool gTimeSyncRestorePending = false;

const char* bootPhaseLabel(BootPhase phase) {
  switch (phase) {
    case BootPhase::BOOT_MINIMAL:
      return "BOOT_MINIMAL";
    case BootPhase::STA_CONNECTING:
      return "STA_CONNECTING";
    case BootPhase::PORTAL_ACTIVE:
      return "PORTAL_ACTIVE";
    case BootPhase::RESTART_PENDING:
      return "RESTART_PENDING";
    case BootPhase::RUNTIME_STARTING:
      return "RUNTIME_STARTING";
    case BootPhase::RUNTIME_ACTIVE:
      return "RUNTIME_ACTIVE";
    default:
      return "UNKNOWN";
  }
}

void setBootPhase(BootPhase nextPhase, const char* reason) {
  if (gBootPhase == nextPhase) {
    return;
  }

  Serial.printf("[BOOT] Phase %s -> %s (%s)\n",
                bootPhaseLabel(gBootPhase),
                bootPhaseLabel(nextPhase),
                reason == nullptr ? "no reason" : reason);
  gBootPhase = nextPhase;
}

void blankMatrixOutputsEarly() {
  pinMode(OE_PIN, OUTPUT);
  digitalWrite(OE_PIN, HIGH);

  const uint8_t lowPins[] = {
    R1_PIN, G1_PIN, B1_PIN,
    R2_PIN, G2_PIN, B2_PIN,
    A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
    LAT_PIN, CLK_PIN
  };

  for (uint8_t pin : lowPins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

void showRestoreWaitScreen() {
  if (DisplayManager::dma_display == nullptr) {
    return;
  }

  DisplayManager::dma_display->clearScreen();
  DisplayManager::drawLogo(12, 8);
  DisplayManager::drawTinyTextCentered("STARTING", 46, DisplayManager::dma_display->color565(220, 220, 220));
  DisplayManager::drawTinyTextCentered("RESTORE WAIT", 54, DisplayManager::dma_display->color565(150, 150, 150));
  DisplayManager::dma_display->flipDMABuffer();
}

bool renderStartupRestoredFrame() {
  return RuntimeModeCoordinator::restoreCurrentModeFrame();
}

void restoreRuntimeFrame() {
  gTimeSyncRestorePending = !WiFiManager::isTimeSynced();
  if (gTimeSyncRestorePending) {
    Serial.println("[BOOT] 时间未同步，先停留在 WiFi 已连接 IP 页面");
    WiFiManager::showStationConnectedScreen();
    return;
  }

  Serial.printf("[BOOT] 准备恢复业务态画面，模式=%d，业务模式=%s\n",
                DisplayManager::currentMode,
                DisplayManager::currentBusinessModeTag.c_str());

  if (renderStartupRestoredFrame()) {
    Serial.println("[BOOT] 业务态画面已恢复");
    return;
  }

  gTimeSyncRestorePending = false;
  Serial.println("[BOOT] 无法精确恢复当前业务态，显示 RESTORE WAIT");
  showRestoreWaitScreen();
}

void finishClockRestoreIfReady() {
  if (!gTimeSyncRestorePending) {
    return;
  }
  if (!WiFiManager::isTimeSynced()) {
    return;
  }

  gTimeSyncRestorePending = false;
  Serial.println("[BOOT] 时间同步完成，恢复业务态画面");

  if (renderStartupRestoredFrame()) {
    Serial.println("[BOOT] 时间同步后的业务态画面已恢复");
    return;
  }

  Serial.println("[BOOT] 时间同步后仍无法恢复业务态，显示 RESTORE WAIT");
  showRestoreWaitScreen();
}

void printPortalReadyBanner() {
  Serial.println("\n=================================");
  Serial.println("系统就绪！");
  Serial.println("WiFi 热点配网模式已启动");
  Serial.print("热点名称: ");
  Serial.println(WiFiManager::getConfigPortalSSID());
  Serial.print("配网页地址: http://");
  Serial.println(WiFiManager::getConfigPortalIP());
  Serial.println("=================================\n");
}

void printRuntimeReadyBanner() {
  Serial.println("\n=================================");
  Serial.println("系统就绪！");
  Serial.print("STA模式访问地址: http://");
  Serial.println(WiFiManager::getDeviceIP());
  Serial.println("WebSocket 路径: ws://" + WiFiManager::getDeviceIP() + "/ws");
  Serial.println("=================================\n");
}

void startPortalIfNeeded() {
  if (!gPortalServerInitialized) {
    setBootPhase(BootPhase::PORTAL_ACTIVE, "进入热点配网模式");
    WebServer::initConfigPortal();
    gPortalServerInitialized = true;
    printPortalReadyBanner();
    return;
  }

  if (WiFiManager::isPortalRestartScheduled()) {
    setBootPhase(BootPhase::RESTART_PENDING, "门户已保存凭据，等待重启");
    return;
  }

  setBootPhase(BootPhase::PORTAL_ACTIVE, "保持热点配网模式");
}

void startRuntimeIfNeeded() {
  if (gRuntimeInitialized || !WiFiManager::isStationConnected()) {
    return;
  }

  setBootPhase(BootPhase::RUNTIME_STARTING, "STA 已连接，开始启动运行态");

  ConfigManager::init();
  WebSocketHandler::init();
  WebServer::initRuntime();

  AnimationManager::init();
  EyesEffect::init();
  GameScreensaverEffect::init();
  BoardNativeEffect::init();

  OTAManager::init();
  OTAManager::checkUpdate();

  gRuntimeInitialized = true;
  restoreRuntimeFrame();

  setBootPhase(BootPhase::RUNTIME_ACTIVE, "运行态初始化完成");
  printRuntimeReadyBanner();
}
}

void setup() {
  blankMatrixOutputsEarly();
  Serial.begin(115200);
  Serial.println("[BOOT] 已提前关闭 HUB75 输出，避免上电瞬间乱亮");
  delay(2000);
  
  Serial.println("\n\n=================================");
  Serial.println("ESP32 LED图片显示系统启动...");
  Serial.println("=================================");
  
  // 打印内存信息
  Serial.printf("总内存: %d bytes (%.1f KB)\n", ESP.getHeapSize(), ESP.getHeapSize()/1024.0);
  Serial.printf("可用内存: %d bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap()/1024.0);
  Serial.printf("最大可分配块: %d bytes (%.1f KB)\n", ESP.getMaxAllocHeap(), ESP.getMaxAllocHeap()/1024.0);
  Serial.printf("PSRAM 总量: %d bytes (%.1f KB)\n", ESP.getPsramSize(), ESP.getPsramSize()/1024.0);
  Serial.printf("PSRAM 可用: %d bytes (%.1f KB)\n", ESP.getFreePsram(), ESP.getFreePsram()/1024.0);
  Serial.println("=================================");

  Serial.printf("[BOOT] 初始阶段=%s\n", bootPhaseLabel(gBootPhase));

  ConfigManager::preloadDeviceParamsConfig();
  DisplayManager::init();
  WiFiManager::init();

  if (WiFiManager::isConfigMode()) {
    startPortalIfNeeded();
  } else {
    setBootPhase(BootPhase::STA_CONNECTING, "最小启动完成，等待 STA 连接");
    if (WiFiManager::isStationConnected()) {
      startRuntimeIfNeeded();
    } else {
      Serial.println("[BOOT] 已进入 STA 连接等待态，运行态服务暂不启动");
    }
  }
}

void loop() {
  WiFiManager::tick();
  WebSocketHandler::tick();
  RuntimeCommandBus::tick();

  if (WiFiManager::isConfigMode()) {
    startPortalIfNeeded();
    if (WiFiManager::isPortalRestartScheduled()) {
      setBootPhase(BootPhase::RESTART_PENDING, "门户已保存凭据，等待重启");
    }
    delay(10);
    return;
  }

  if (!gRuntimeInitialized) {
    if (WiFiManager::isStationConnecting()) {
      setBootPhase(BootPhase::STA_CONNECTING, "继续等待 STA 连接");
      delay(10);
      return;
    }

    if (WiFiManager::isStationConnected()) {
      startRuntimeIfNeeded();
    }

    if (!gRuntimeInitialized) {
      delay(10);
      return;
    }
  }

  DisplayManager::lockRuntimeAccess();
  DisplayManager::refreshScheduledBrightness();
  finishClockRestoreIfReady();

  if (gTimeSyncRestorePending) {
    DisplayManager::updateLoadingAnimation();
    DisplayManager::unlockRuntimeAccess();
    WebSocketHandler::ws.cleanupClients();
    yield();
    return;
  }

  // 检查是否有待保存的二进制像素数据
  if (WebSocketHandler::binaryDataPending && millis() - WebSocketHandler::lastBinaryReceiveTime > 500) {
    WebSocketHandler::binaryDataPending = false;

    // 根据当前模式选择对应的像素数据
    PixelData*& imagePixels = (DisplayManager::currentMode == MODE_ANIMATION)
      ? ConfigManager::animImagePixels
      : ConfigManager::staticImagePixels;
    int& imagePixelCount = (DisplayManager::currentMode == MODE_ANIMATION)
      ? ConfigManager::animImagePixelCount
      : ConfigManager::staticImagePixelCount;

    if (imagePixels != nullptr && imagePixelCount > 0) {
      Serial.printf("=== 开始保存像素数据到 Preferences: %d 个像素 ===\n", imagePixelCount);

      if (DisplayManager::currentMode == MODE_ANIMATION) {
        ConfigManager::saveAnimImagePixels();
      } else {
        ConfigManager::saveStaticImagePixels();
      }

      // 只向当前上传连接返回确认，避免广播扰动其他客户端状态机
      WebSocketHandler::sendBinaryReceiveConfirmation(imagePixelCount);

      // 保存完成后恢复显示刷新
      DisplayManager::receivingPixels = false;
      WebSocketHandler::clearBinaryImageTransferBackup();

      // 保存完成后刷新显示
      if (DisplayManager::currentMode == MODE_CLOCK) {
        DisplayManager::displayClock(true);
      } else if (DisplayManager::currentMode == MODE_THEME) {
        DisplayManager::displayTheme(true);
      } else if (DisplayManager::currentMode == MODE_ANIMATION &&
                 AnimationManager::currentGIF == nullptr) {
        DisplayManager::displayClock(true);
      }
    }
  }

  // 连接保持策略：不因短时心跳抖动主动断开，只记录长时间空闲状态
  static bool wsIdleLogged = false;
  if (DisplayManager::clientConnected &&
      millis() - WebSocketHandler::lastMessageTime > kWebSocketIdleWarnMs) {
    if (!wsIdleLogged) {
      Serial.printf(
        "WebSocket 长时间空闲，保持连接不断开，空闲时长=%lu ms\n",
        millis() - WebSocketHandler::lastMessageTime
      );
      wsIdleLogged = true;
    }
  } else {
    wsIdleLogged = false;
  }

  // 根据当前模式执行不同的逻辑
  if (DisplayManager::currentMode == MODE_CANVAS || DisplayManager::currentMode == MODE_CLOCK) {
    if (DisplayManager::currentMode == MODE_CANVAS) {
      // 纯画板模式：不画时钟，不干预显示
    } else if (DisplayManager::clientConnected) {
      // 有客户端连接时，不自动刷新时钟
    } else {
      // 静态时钟模式，无客户端：每分钟更新一次时钟
      static unsigned long lastClockUpdate = 0;
      static int lastMinute = -1;

      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 0)) {
        if (timeinfo.tm_min != lastMinute) {
          lastMinute = timeinfo.tm_min;
          DisplayManager::displayClock();
        }
      } else {
        if (millis() - lastClockUpdate > 10000) {
          lastClockUpdate = millis();
          DisplayManager::displayClock();
        }
      }
    }
  } else if (DisplayManager::currentMode == MODE_THEME) {
    static unsigned long lastThemeUpdate = 0;
    uint16_t refreshInterval = getThemeRefreshIntervalMs(ConfigManager::themeConfig.themeId);
    if (millis() - lastThemeUpdate >= refreshInterval) {
      lastThemeUpdate = millis();
      DisplayManager::displayTheme();
    }
  } else if (DisplayManager::currentMode == MODE_ANIMATION) {
    // 动画模式
    if (DisplayManager::currentBusinessModeTag == ModeTags::GAME_SCREENSAVER &&
               GameScreensaverEffect::isActive()) {
      GameScreensaverEffect::update();
      GameScreensaverEffect::render();
    } else if ((DisplayManager::currentBusinessModeTag == ModeTags::TEXT_DISPLAY ||
                DisplayManager::currentBusinessModeTag == ModeTags::WEATHER ||
                DisplayManager::currentBusinessModeTag == ModeTags::COUNTDOWN ||
                DisplayManager::currentBusinessModeTag == ModeTags::STOPWATCH ||
                DisplayManager::currentBusinessModeTag == ModeTags::NOTIFICATION ||
                DisplayManager::currentBusinessModeTag == ModeTags::PLANET_SCREENSAVER) &&
               BoardNativeEffect::isActive()) {
      BoardNativeEffect::update();
      BoardNativeEffect::render();
    } else if (TetrisEffect::isActive) {
      // 俄罗斯方块屏保
      TetrisEffect::update();
      TetrisEffect::render(DisplayManager::dma_display);
    } else if (DisplayManager::nativeEffectType != NATIVE_EFFECT_NONE) {
      DisplayManager::renderNativeEffect();
    } else {
      // GIF 动画
      AnimationManager::updateGIFAnimation();
    }
  }

  // 更新 Loading 动画
  DisplayManager::updateLoadingAnimation();
  DisplayManager::unlockRuntimeAccess();

  // 清理WebSocket，防止内存泄漏
  WebSocketHandler::ws.cleanupClients();

  yield();
}
