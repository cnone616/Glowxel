#include <Arduino.h>
#include <time.h>
#include "config_manager.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "animation_manager.h"
#include "board_native_effect.h"
#include "mode_tags.h"
#include "websocket_handler.h"
#include "web_server.h"
#include "ota_manager.h"
#include "tetris_effect.h"
#include "eyes_effect.h"
#include "game_screensaver_effect.h"
#include "theme_renderer.h"

namespace {
constexpr unsigned long kWebSocketIdleWarnMs = 180000;
bool gStartupRestorePending = false;
bool gStartupRestoreDone = false;

void renderStartupRestoredFrame();

void finishStartupRestoreIfReady() {
  if (gStartupRestoreDone || !gStartupRestorePending) {
    return;
  }
  if (!WiFiManager::isTimeSynced()) {
    return;
  }

  if (!DisplayManager::doubleBufferEnabled) {
    DisplayManager::enableDoubleBuffer();
  }

  Serial.printf("[BOOT] 时间同步完成，开始恢复模式首帧，模式=%d，业务模式=%s\n",
                DisplayManager::currentMode,
                DisplayManager::currentBusinessModeTag.c_str());
  renderStartupRestoredFrame();
  Serial.printf("[BOOT] 恢复模式首帧已完成，耗时=%lu ms\n", millis());

  gStartupRestorePending = false;
  gStartupRestoreDone = true;
}

void renderStartupRestoredFrame() {
  auto renderTimeOrSyncHint = []() {
    if (WiFiManager::isTimeSynced()) {
      DisplayManager::displayClock(true);
      return;
    }
    WiFiManager::showTimeSyncScreen();
  };

  if (DisplayManager::currentMode == MODE_CLOCK) {
    DisplayManager::displayClock(true);
    return;
  }

  if (DisplayManager::currentMode == MODE_THEME) {
    DisplayManager::displayTheme(true);
    return;
  }

  if (DisplayManager::currentMode == MODE_CANVAS) {
    DisplayManager::renderCanvas();
    return;
  }

  if (DisplayManager::currentMode == MODE_ANIMATION &&
      DisplayManager::currentBusinessModeTag == ModeTags::GAME_SCREENSAVER) {
    GameScreensaverEffect::applyConfig(ConfigManager::gameScreensaverConfig);
    GameScreensaverEffect::render();
    return;
  }

  if (DisplayManager::currentMode == MODE_ANIMATION &&
      DisplayManager::currentBusinessModeTag == ModeTags::GIF_PLAYER) {
    if (AnimationManager::currentGIF != nullptr) {
      AnimationManager::currentGIF->isPlaying = true;
      AnimationManager::currentGIF->currentFrame = 0;
      AnimationManager::currentGIF->lastFrameTime = millis();
      AnimationManager::renderGIFFrame(0);
      return;
    }
    renderTimeOrSyncHint();
    return;
  }

  if (DisplayManager::currentMode == MODE_ANIMATION &&
      (DisplayManager::currentBusinessModeTag == ModeTags::TEXT_DISPLAY ||
       DisplayManager::currentBusinessModeTag == ModeTags::WEATHER ||
       DisplayManager::currentBusinessModeTag == ModeTags::COUNTDOWN ||
       DisplayManager::currentBusinessModeTag == ModeTags::STOPWATCH ||
       DisplayManager::currentBusinessModeTag == ModeTags::NOTIFICATION)) {
    if (BoardNativeEffect::isActive()) {
      BoardNativeEffect::render();
      return;
    }
  }

  if (DisplayManager::currentMode == MODE_ANIMATION &&
      DisplayManager::currentBusinessModeTag == ModeTags::PLANET_SCREENSAVER) {
    BoardNativeEffect::applyPlanetScreensaverConfig(
      BoardNativeEffect::getPlanetScreensaverConfig()
    );
    BoardNativeEffect::render();
    return;
  }

  if (DisplayManager::currentMode == MODE_ANIMATION &&
      DisplayManager::currentBusinessModeTag == ModeTags::EYES) {
    DisplayManager::activateEyesEffect(ConfigManager::eyesConfig);
    DisplayManager::renderNativeEffect();
    return;
  }

  if (DisplayManager::currentMode == MODE_ANIMATION &&
      (DisplayManager::currentBusinessModeTag == ModeTags::AMBIENT_EFFECT ||
       DisplayManager::currentBusinessModeTag == ModeTags::LED_MATRIX_SHOWCASE)) {
    DisplayManager::activateAmbientEffect(DisplayManager::ambientEffectConfig);
    DisplayManager::renderNativeEffect();
    return;
  }

  if (DisplayManager::currentMode == MODE_ANIMATION &&
      DisplayManager::currentBusinessModeTag == ModeTags::BREATH_EFFECT) {
    DisplayManager::activateBreathEffect(DisplayManager::breathEffectConfig);
    DisplayManager::renderNativeEffect();
    return;
  }

  if (DisplayManager::currentMode == MODE_ANIMATION &&
      DisplayManager::currentBusinessModeTag == ModeTags::RHYTHM_EFFECT) {
    DisplayManager::activateRhythmEffect(DisplayManager::rhythmEffectConfig);
    DisplayManager::renderNativeEffect();
    return;
  }

  if (DisplayManager::currentMode == MODE_ANIMATION &&
      AnimationManager::currentGIF != nullptr) {
    AnimationManager::currentGIF->isPlaying = true;
    AnimationManager::currentGIF->currentFrame = 0;
    AnimationManager::currentGIF->lastFrameTime = millis();
    AnimationManager::renderGIFFrame(0);
    return;
  }

  // 兜底：无论恢复到什么业务态，都保证启动时有可见画面，不出现黑屏等待。
  renderTimeOrSyncHint();
}
}

void setup() {
  Serial.begin(115200);
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
  
  // 初始化各个模块
  ConfigManager::preloadDeviceParamsConfig();
  DisplayManager::init();
  ConfigManager::init();
  WiFiManager::init();

  WebSocketHandler::init();
  WebServer::init();

  // 联网成功后恢复业务显示与 OTA；热点配网模式仅保留 AP 门户
  if (!WiFiManager::isConfigMode()) {
    AnimationManager::init();
    EyesEffect::init();
    GameScreensaverEffect::init();
    BoardNativeEffect::init();

    if (!DisplayManager::doubleBufferEnabled) {
      DisplayManager::enableDoubleBuffer();
    }

    if (WiFiManager::isTimeSynced()) {
      Serial.printf("[BOOT] 准备渲染恢复模式首帧，模式=%d，业务模式=%s\n",
                    DisplayManager::currentMode,
                    DisplayManager::currentBusinessModeTag.c_str());
      renderStartupRestoredFrame();
      Serial.printf("[BOOT] 恢复模式首帧已完成，耗时=%lu ms\n", millis());
      gStartupRestoreDone = true;
      gStartupRestorePending = false;
    } else {
      gStartupRestorePending = true;
      gStartupRestoreDone = false;
      Serial.println("[BOOT] 时间尚未同步，保持 WiFi 成功页，等待同步后再进入恢复模式");
    }

    // OTA 检查更新
    OTAManager::init();
    OTAManager::checkUpdate();
  }
  
  Serial.println("\n=================================");
  Serial.println("系统就绪！");
  if (WiFiManager::isConfigMode()) {
    Serial.println("WiFi 热点配网模式已启动");
    Serial.print("热点名称: ");
    Serial.println(WiFiManager::getConfigPortalSSID());
    Serial.print("配网页地址: http://");
    Serial.println(WiFiManager::getConfigPortalIP());
  } else {
    Serial.print("STA模式访问地址: http://");
    Serial.println(WiFiManager::getDeviceIP());
    Serial.println("WebSocket 路径: ws://" + WiFiManager::getDeviceIP() + "/ws");
  }
  Serial.println("=================================\n");
}

void loop() {
  // 配网模式下维护 AP 门户与 DNS，不进入业务显示循环
  if (WiFiManager::isConfigMode()) {
    WiFiManager::tick();
    delay(10);
    return;
  }

  WiFiManager::tick();
  DisplayManager::refreshScheduledBrightness();
  finishStartupRestoreIfReady();

  if (gStartupRestorePending && !gStartupRestoreDone) {
    WebSocketHandler::ws.cleanupClients();
    DisplayManager::updateLoadingAnimation();
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

      // 保存完成后刷新显示
      if (DisplayManager::currentMode == MODE_CLOCK) {
        DisplayManager::drawBackground();
        DisplayManager::drawClockOverlay();
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

  // 清理WebSocket，防止内存泄漏
  WebSocketHandler::ws.cleanupClients();

  // 更新 Loading 动画
  DisplayManager::updateLoadingAnimation();

  yield();
}
