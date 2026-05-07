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
#include "maze_effect.h"
#include "tetris_clock_effect.h"
#include "tetris_effect.h"
#include "eyes_effect.h"
#include "snake_effect.h"
#include "theme_renderer.h"

namespace {
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
bool gRuntimeInteractiveSettingsMode = false;
bool gDeferredRuntimeModulesInitialized = false;
bool gRuntimeDisplayOutputPaused = false;

void setBootPhase(BootPhase nextPhase, const char* reason) {
  if (gBootPhase == nextPhase) {
    return;
  }
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

void clearScreenBeforeStartupRestoreIfNeeded() {
  if (!RuntimeModeCoordinator::shouldClearScreenBeforeBusinessModeEntry(
        DisplayManager::currentBusinessModeTag)) {
    return;
  }

  DisplayManager::clearScreen();
}

void initDeferredRuntimeModulesIfNeeded() {
  if (gDeferredRuntimeModulesInitialized) {
    return;
  }

  AnimationManager::init();
  EyesEffect::init();
  MazeEffect::init();
  SnakeEffect::init();
  BoardNativeEffect::init();
  ConfigManager::loadPlanetScreensaverConfig();
  OTAManager::init();
  OTAManager::checkUpdate();
  gDeferredRuntimeModulesInitialized = true;
}

bool isRuntimeSettingsSessionActive() {
  return WiFiManager::isRuntimeSettingsWindowActive() ||
         WebServer::isSettingsSessionActive();
}

void pauseRuntimeDisplayOutputIfNeeded() {
  if (gRuntimeDisplayOutputPaused || DisplayManager::dma_display == nullptr) {
    return;
  }

  DisplayManager::dma_display->stopDMAoutput();
  gRuntimeDisplayOutputPaused = true;
}

void resumeRuntimeDisplayOutputIfNeeded() {
  if (!gRuntimeDisplayOutputPaused) {
    return;
  }

  DisplayManager::applyDeviceParams(false);
  DisplayManager::refreshScheduledBrightness();
  gRuntimeDisplayOutputPaused = false;
}

void restoreRuntimeFrame() {
  gTimeSyncRestorePending = !WiFiManager::isTimeSynced();
  if (gTimeSyncRestorePending) {
    WiFiManager::showStationConnectedScreen();
    return;
  }

  clearScreenBeforeStartupRestoreIfNeeded();
  if (renderStartupRestoredFrame()) {
    return;
  }

  gTimeSyncRestorePending = false;
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

  clearScreenBeforeStartupRestoreIfNeeded();
  if (renderStartupRestoredFrame()) {
    return;
  }

  showRestoreWaitScreen();
}

void printPortalReadyBanner() {
}

void printRuntimeReadyBanner() {
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
  WebServer::initRuntime();

  gRuntimeInitialized = true;
  gDeferredRuntimeModulesInitialized = false;
  initDeferredRuntimeModulesIfNeeded();
  restoreRuntimeFrame();

  setBootPhase(BootPhase::RUNTIME_ACTIVE, "运行态初始化完成");
  printRuntimeReadyBanner();
}
}

void setup() {
  blankMatrixOutputsEarly();
  Serial.begin(115200);
  delay(2000);

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
    }
  }
}

void loop() {
  WiFiManager::tick();
  WebServer::handleLoop();
  WebSocketHandler::tick();
  RuntimeCommandBus::tick();
  if (RuntimeCommandBus::consumePostTransactionYieldHint()) {
    delay(2);
    return;
  }

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

  if (isRuntimeSettingsSessionActive()) {
    if (!gRuntimeInteractiveSettingsMode) {
      gRuntimeInteractiveSettingsMode = true;
      pauseRuntimeDisplayOutputIfNeeded();
    }
    delay(5);
    return;
  }

  if (gRuntimeInteractiveSettingsMode) {
    gRuntimeInteractiveSettingsMode = false;
    resumeRuntimeDisplayOutputIfNeeded();
    initDeferredRuntimeModulesIfNeeded();
    restoreRuntimeFrame();
  }

  const bool shouldUpdateMazeOutsideLock =
    !gTimeSyncRestorePending &&
    DisplayManager::currentMode == MODE_ANIMATION &&
    DisplayManager::currentBusinessModeTag == ModeTags::MAZE &&
    MazeEffect::isActive();
  if (shouldUpdateMazeOutsideLock) {
    MazeEffect::update();
  }

  DisplayManager::lockRuntimeAccess();
  DisplayManager::refreshScheduledBrightness();
  finishClockRestoreIfReady();

  if (gTimeSyncRestorePending) {
    DisplayManager::updateLoadingAnimation();
    DisplayManager::unlockRuntimeAccess();
    if (WebSocketHandler::ws.count() > 0) {
      WebSocketHandler::ws.cleanupClients();
    }
    yield();
    return;
  }

  // 根据当前模式执行不同的逻辑
  if (DisplayManager::currentMode == MODE_CANVAS || DisplayManager::currentMode == MODE_CLOCK) {
    if (DisplayManager::currentMode == MODE_CANVAS) {
      // 纯画板模式：不画时钟，不干预显示
    } else {
      // 静态时钟模式：按当前时间显示粒度刷新，避免秒显停住。
      static unsigned long lastClockUpdate = 0;
      static int lastClockTick = -1;
      static bool lastShowSeconds = false;

      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 0)) {
        bool showSeconds = ConfigManager::clockConfig.showSeconds;
        int tickValue = showSeconds ? timeinfo.tm_sec : timeinfo.tm_min;
        if (tickValue != lastClockTick || showSeconds != lastShowSeconds) {
          lastClockTick = tickValue;
          lastShowSeconds = showSeconds;
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
    if (DisplayManager::currentBusinessModeTag == ModeTags::MAZE &&
               MazeEffect::isActive()) {
      MazeEffect::render();
    } else if (DisplayManager::currentBusinessModeTag == ModeTags::SNAKE &&
               SnakeEffect::isActive()) {
      SnakeEffect::update();
      SnakeEffect::render();
    } else if ((DisplayManager::currentBusinessModeTag == ModeTags::TEXT_DISPLAY ||
                DisplayManager::currentBusinessModeTag == ModeTags::PLANET_SCREENSAVER) &&
               BoardNativeEffect::isActive()) {
      BoardNativeEffect::update();
      BoardNativeEffect::render();
    } else if (DisplayManager::currentBusinessModeTag == ModeTags::TETRIS_CLOCK &&
               TetrisClockEffect::isActive) {
      TetrisClockEffect::update();
      TetrisClockEffect::render(DisplayManager::dma_display);
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
  if (WebSocketHandler::ws.count() > 0) {
    WebSocketHandler::ws.cleanupClients();
  }

  yield();
}
