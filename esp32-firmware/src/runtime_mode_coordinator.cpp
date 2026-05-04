#include "runtime_mode_coordinator.h"

#include "animation_manager.h"
#include "board_native_effect.h"
#include "config_manager.h"
#include "maze_effect.h"
#include "mode_tags.h"
#include "snake_effect.h"
#include "tetris_clock_effect.h"
#include "tetris_effect.h"

namespace {
bool shouldClearScreenBeforeBusinessModeEntryInternal(const String& businessModeTag) {
  return businessModeTag == ModeTags::THEME ||
         businessModeTag == ModeTags::GIF_PLAYER ||
         businessModeTag == ModeTags::AMBIENT_EFFECT ||
         businessModeTag == ModeTags::LED_MATRIX_SHOWCASE ||
         businessModeTag == ModeTags::MAZE ||
         businessModeTag == ModeTags::SNAKE ||
         businessModeTag == ModeTags::PLANET_SCREENSAVER ||
         businessModeTag == ModeTags::TETRIS ||
         businessModeTag == ModeTags::TETRIS_CLOCK;
}

bool canRestoreGifBusinessMode() {
  if (AnimationManager::currentGIF == nullptr && !AnimationManager::loadAnimation()) {
    return false;
  }

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

  if (businessModeTag == ModeTags::TETRIS) {
    TetrisEffect::init(
      ConfigManager::tetrisConfig.clearMode,
      ConfigManager::tetrisConfig.cellSize,
      ConfigManager::tetrisConfig.speed,
      ConfigManager::tetrisConfig.showClock,
      ConfigManager::tetrisConfig.pieces
    );
    TetrisEffect::render(DisplayManager::dma_display);
    return true;
  }

  if (businessModeTag == ModeTags::TETRIS_CLOCK) {
    TetrisClockEffect::init(
      ConfigManager::tetrisClockConfig.speed,
      ConfigManager::tetrisClockConfig.cellSize,
      ConfigManager::tetrisClockConfig.hourFormat
    );
    TetrisClockEffect::render(DisplayManager::dma_display);
    return true;
  }

  if (businessModeTag == ModeTags::MAZE) {
    MazeEffect::applyConfig(ConfigManager::mazeConfig);
    if (!MazeEffect::isActive()) {
      return false;
    }
    return true;
  }

  if (businessModeTag == ModeTags::SNAKE) {
    SnakeEffect::applyConfig(ConfigManager::snakeConfig);
    if (!SnakeEffect::isActive()) {
      return false;
    }
    SnakeEffect::render();
    return true;
  }

  if (businessModeTag == ModeTags::EYES) {
    DisplayManager::clearScreen();
    DisplayManager::activateEyesEffect(ConfigManager::eyesConfig);
    DisplayManager::renderNativeEffect();
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
  return MODE_ANIMATION;
}

bool isRecoverableBusinessModeTag(const String& businessModeTag) {
  if (businessModeTag == ModeTags::CLOCK ||
      businessModeTag == ModeTags::THEME ||
      businessModeTag == ModeTags::CANVAS ||
      businessModeTag == ModeTags::ANIMATION ||
      businessModeTag == ModeTags::TETRIS ||
      businessModeTag == ModeTags::TETRIS_CLOCK ||
      businessModeTag == ModeTags::MAZE ||
      businessModeTag == ModeTags::SNAKE ||
      businessModeTag == ModeTags::EYES ||
      businessModeTag == ModeTags::AMBIENT_EFFECT ||
      businessModeTag == ModeTags::LED_MATRIX_SHOWCASE ||
      businessModeTag == ModeTags::PLANET_SCREENSAVER) {
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

  return businessModeTag != ModeTags::CANVAS;
}

bool isTransientRuntimeMode(DeviceMode mode) {
  return mode == MODE_CANVAS || mode == MODE_TRANSFERRING;
}

bool shouldClearScreenBeforeBusinessModeEntry(const String& businessModeTag) {
  return shouldClearScreenBeforeBusinessModeEntryInternal(businessModeTag);
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
  TetrisClockEffect::deactivate();
  MazeEffect::deactivate();
  SnakeEffect::deactivate();
  if (DisplayManager::currentBusinessModeTag == ModeTags::ANIMATION ||
      DisplayManager::currentBusinessModeTag == ModeTags::GIF_PLAYER) {
    AnimationManager::freeGIFAnimation();
  } else {
    AnimationManager::pauseAnimation();
  }
  BoardNativeEffect::deactivate();
  DisplayManager::setNativeEffectNone();
  DisplayManager::receivingPixels = false;
}

bool restoreCurrentModeFrame(bool syncBufferStrategy) {
  if (DisplayManager::isLoadingActive) {
    DisplayManager::stopLoadingAnimation();
  }

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
