#include "runtime_mode_coordinator.h"

#include "animation_manager.h"
#include "board_native_effect.h"
#include "config_manager.h"
#include "game_screensaver_effect.h"
#include "mode_tags.h"
#include "tetris_effect.h"

namespace {
bool canRestoreGifBusinessMode() {
  return AnimationManager::currentGIF != nullptr &&
         AnimationManager::currentGIF->frameCount > 0;
}

bool canRestoreBoardNativeMode(const String& businessModeTag) {
  if (BoardNativeEffect::isActive()) {
    return true;
  }

  if (businessModeTag == ModeTags::TEXT_DISPLAY) {
    BoardNativeEffect::applyTextDisplayConfig(BoardNativeEffect::getTextDisplayConfig());
    return BoardNativeEffect::isActive();
  }
  if (businessModeTag == ModeTags::WEATHER) {
    BoardNativeEffect::applyWeatherConfig(BoardNativeEffect::getWeatherConfig());
    return BoardNativeEffect::isActive();
  }
  if (businessModeTag == ModeTags::COUNTDOWN) {
    BoardNativeEffect::applyCountdownConfig(BoardNativeEffect::getCountdownConfig());
    return BoardNativeEffect::isActive();
  }
  if (businessModeTag == ModeTags::STOPWATCH) {
    BoardNativeEffect::applyStopwatchConfig(BoardNativeEffect::getStopwatchConfig());
    return BoardNativeEffect::isActive();
  }
  if (businessModeTag == ModeTags::NOTIFICATION) {
    BoardNativeEffect::applyNotificationConfig(BoardNativeEffect::getNotificationConfig());
    return BoardNativeEffect::isActive();
  }
  if (businessModeTag == ModeTags::PLANET_SCREENSAVER) {
    BoardNativeEffect::applyPlanetScreensaverConfig(
      BoardNativeEffect::getPlanetScreensaverConfig()
    );
    return BoardNativeEffect::isActive();
  }

  return false;
}

void playGifFromFirstFrame() {
  if (AnimationManager::currentGIF == nullptr) {
    return;
  }

  AnimationManager::currentGIF->isPlaying = true;
  AnimationManager::currentGIF->currentFrame = 0;
  AnimationManager::currentGIF->lastFrameTime = millis();
  AnimationManager::renderGIFFrame(0);
}

bool renderAnimationBusinessFrame(const String& businessModeTag) {
  if (businessModeTag == ModeTags::ANIMATION) {
    if (canRestoreGifBusinessMode()) {
      playGifFromFirstFrame();
      return true;
    }
    DisplayManager::displayClock(true);
    return true;
  }

  if (businessModeTag == ModeTags::GIF_PLAYER) {
    if (!canRestoreGifBusinessMode()) {
      return false;
    }
    playGifFromFirstFrame();
    return true;
  }

  if (businessModeTag == ModeTags::TETRIS ||
      businessModeTag == ModeTags::TETRIS_CLOCK) {
    TetrisEffect::init(
      TetrisEffect::doClearLines,
      TetrisEffect::cellSize,
      TetrisEffect::dropSpeed,
      TetrisEffect::showClock,
      TetrisEffect::piecesMask
    );
    TetrisEffect::render(DisplayManager::dma_display);
    return true;
  }

  if (businessModeTag == ModeTags::GAME_SCREENSAVER) {
    GameScreensaverEffect::applyConfig(ConfigManager::gameScreensaverConfig);
    if (!GameScreensaverEffect::isActive()) {
      return false;
    }
    GameScreensaverEffect::render();
    return true;
  }

  if (businessModeTag == ModeTags::EYES) {
    DisplayManager::activateEyesEffect(ConfigManager::eyesConfig);
    return true;
  }

  if (businessModeTag == ModeTags::BREATH_EFFECT) {
    DisplayManager::activateBreathEffect(DisplayManager::breathEffectConfig);
    DisplayManager::renderNativeEffect();
    return true;
  }

  if (businessModeTag == ModeTags::RHYTHM_EFFECT) {
    DisplayManager::activateRhythmEffect(DisplayManager::rhythmEffectConfig);
    DisplayManager::renderNativeEffect();
    return true;
  }

  if (businessModeTag == ModeTags::AMBIENT_EFFECT ||
      businessModeTag == ModeTags::LED_MATRIX_SHOWCASE) {
    DisplayManager::activateAmbientEffect(DisplayManager::ambientEffectConfig);
    DisplayManager::renderNativeEffect();
    return true;
  }

  if (ModeTags::isBoardNativeModeTag(businessModeTag) ||
      businessModeTag == ModeTags::PLANET_SCREENSAVER) {
    if (!canRestoreBoardNativeMode(businessModeTag)) {
      return false;
    }
    BoardNativeEffect::render();
    return true;
  }

  return false;
}

void applyModeState(
  DeviceMode mode,
  const String& businessModeTag,
  bool updateLastBusinessTarget
) {
  DisplayManager::currentMode = mode;
  DisplayManager::currentBusinessModeTag = businessModeTag;

  if (updateLastBusinessTarget) {
    DisplayManager::lastBusinessMode = mode;
    DisplayManager::lastBusinessModeTag = businessModeTag;
  }
}
}

namespace RuntimeModeCoordinator {
DeviceMode resolveTopLevelMode(const String& businessModeTag) {
  if (businessModeTag == ModeTags::CLOCK) {
    return MODE_CLOCK;
  }
  if (businessModeTag == ModeTags::THEME) {
    return MODE_THEME;
  }
  if (businessModeTag == ModeTags::CANVAS) {
    return MODE_CANVAS;
  }
  if (businessModeTag == ModeTags::TRANSFERRING) {
    return MODE_TRANSFERRING;
  }
  return MODE_ANIMATION;
}

bool isRecoverableBusinessModeTag(const String& businessModeTag) {
  if (businessModeTag == ModeTags::CLOCK ||
      businessModeTag == ModeTags::THEME ||
      businessModeTag == ModeTags::CANVAS ||
      businessModeTag == ModeTags::ANIMATION ||
      businessModeTag == ModeTags::TETRIS ||
      businessModeTag == ModeTags::TETRIS_CLOCK ||
      businessModeTag == ModeTags::EYES ||
      businessModeTag == ModeTags::AMBIENT_EFFECT ||
      businessModeTag == ModeTags::LED_MATRIX_SHOWCASE ||
      businessModeTag == ModeTags::GAME_SCREENSAVER) {
    return true;
  }

  if (businessModeTag == ModeTags::GIF_PLAYER) {
    return canRestoreGifBusinessMode();
  }

  return false;
}

bool isReturnTargetBusinessModeTag(const String& businessModeTag) {
  if (businessModeTag.length() == 0) {
    return false;
  }

  return businessModeTag != ModeTags::CANVAS &&
         businessModeTag != ModeTags::TRANSFERRING;
}

bool isTransientRuntimeMode(DeviceMode mode) {
  return mode == MODE_CANVAS || mode == MODE_TRANSFERRING;
}

void captureModeState(ModeStateSnapshot& snapshot) {
  snapshot.currentMode = DisplayManager::currentMode;
  snapshot.lastBusinessMode = DisplayManager::lastBusinessMode;
  snapshot.currentBusinessModeTag = DisplayManager::currentBusinessModeTag;
  snapshot.lastBusinessModeTag = DisplayManager::lastBusinessModeTag;
}

void restoreModeState(const ModeStateSnapshot& snapshot, bool renderFrame) {
  DisplayManager::currentMode = snapshot.currentMode;
  DisplayManager::lastBusinessMode = snapshot.lastBusinessMode;
  DisplayManager::currentBusinessModeTag = snapshot.currentBusinessModeTag;
  DisplayManager::lastBusinessModeTag = snapshot.lastBusinessModeTag;

  if (renderFrame) {
    restoreCurrentModeFrame();
  }
}

void deactivateRuntimeContent() {
  TetrisEffect::deactivate();
  GameScreensaverEffect::deactivate();
  AnimationManager::pauseAnimation();
  BoardNativeEffect::deactivate();
  DisplayManager::setNativeEffectNone();
  DisplayManager::receivingPixels = false;
}

bool restoreCurrentModeFrame(bool syncBufferStrategy) {
  if (syncBufferStrategy && !DisplayManager::syncBufferStrategyForCurrentMode()) {
    return false;
  }

  if (DisplayManager::dma_display == nullptr) {
    return false;
  }

  if (DisplayManager::currentMode == MODE_CLOCK) {
    DisplayManager::displayClock(true);
    return true;
  }

  if (DisplayManager::currentMode == MODE_THEME) {
    DisplayManager::displayTheme(true);
    return true;
  }

  if (DisplayManager::currentMode == MODE_CANVAS) {
    DisplayManager::renderCanvas();
    return true;
  }

  if (DisplayManager::currentMode == MODE_TRANSFERRING) {
    return true;
  }

  if (DisplayManager::currentMode != MODE_ANIMATION) {
    return false;
  }

  return renderAnimationBusinessFrame(DisplayManager::currentBusinessModeTag);
}

bool restoreAfterTransientDisconnect(bool persistState, bool syncBufferStrategy) {
  if (isTransientRuntimeMode(DisplayManager::currentMode)) {
    return restoreLastBusinessModeFrame(persistState, syncBufferStrategy);
  }

  return restoreCurrentModeFrame(syncBufferStrategy);
}

bool switchToMode(
  DeviceMode mode,
  const String& businessModeTag,
  bool updateLastBusinessTarget,
  bool persistState,
  bool syncBufferStrategy
) {
  ModeStateSnapshot previousState;
  captureModeState(previousState);

  applyModeState(mode, businessModeTag, updateLastBusinessTarget);

  if (!restoreCurrentModeFrame(syncBufferStrategy)) {
    restoreModeState(previousState, true);
    return false;
  }

  if (persistState) {
    ConfigManager::saveClockConfig();
  }

  return true;
}

bool restoreLastBusinessModeFrame(bool persistState, bool syncBufferStrategy) {
  String targetBusinessModeTag = DisplayManager::lastBusinessModeTag;
  if (!isReturnTargetBusinessModeTag(targetBusinessModeTag)) {
    targetBusinessModeTag = ModeTags::CLOCK;
  }

  return switchToMode(
    resolveTopLevelMode(targetBusinessModeTag),
    targetBusinessModeTag,
    true,
    persistState,
    syncBufferStrategy
  );
}
}
