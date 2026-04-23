#ifndef RUNTIME_MODE_COORDINATOR_H
#define RUNTIME_MODE_COORDINATOR_H

#include <Arduino.h>
#include "display_manager.h"

namespace RuntimeModeCoordinator {
struct ModeStateSnapshot {
  DeviceMode currentMode;
  DeviceMode lastBusinessMode;
  String currentBusinessModeTag;
  String lastBusinessModeTag;
};

DeviceMode resolveTopLevelMode(const String& businessModeTag);
bool isRecoverableBusinessModeTag(const String& businessModeTag);
bool isReturnTargetBusinessModeTag(const String& businessModeTag);
bool isTransientRuntimeMode(DeviceMode mode);

void captureModeState(ModeStateSnapshot& snapshot);
void restoreModeState(const ModeStateSnapshot& snapshot, bool renderFrame = true);

void deactivateRuntimeContent();
bool restoreCurrentModeFrame(bool syncBufferStrategy = true);
bool restoreAfterTransientDisconnect(bool persistState = true, bool syncBufferStrategy = true);
bool switchToMode(
  DeviceMode mode,
  const String& businessModeTag,
  bool updateLastBusinessTarget,
  bool persistState,
  bool syncBufferStrategy = true
);
bool restoreLastBusinessModeFrame(bool persistState = true, bool syncBufferStrategy = true);
}

#endif
