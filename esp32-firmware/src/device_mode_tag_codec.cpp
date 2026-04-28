#include "device_mode_tag_codec.h"
#include "mode_tags.h"

namespace {
const char* toTag(DeviceMode mode) {
  if (mode == MODE_CLOCK) {
    return ModeTags::CLOCK;
  }
  if (mode == MODE_CANVAS) {
    return ModeTags::CANVAS;
  }
  if (mode == MODE_ANIMATION) {
    return ModeTags::ANIMATION;
  }
  if (mode == MODE_TRANSFERRING) {
    return ModeTags::TRANSFERRING;
  }
  if (mode == MODE_THEME) {
    return ModeTags::THEME;
  }
  return nullptr;
}
}

namespace DeviceModeTagCodec {
const char* toTagOrUnknown(DeviceMode mode) {
  const char* tag = toTag(mode);
  if (tag != nullptr) {
    return tag;
  }
  return ModeTags::UNKNOWN;
}

const char* toTagOrEmpty(DeviceMode mode) {
  const char* tag = toTag(mode);
  if (tag != nullptr) {
    return tag;
  }
  return "";
}
}
