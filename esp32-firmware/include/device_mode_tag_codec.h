#ifndef DEVICE_MODE_TAG_CODEC_H
#define DEVICE_MODE_TAG_CODEC_H

#include "display_manager.h"

namespace DeviceModeTagCodec {
const char* toTagOrUnknown(DeviceMode mode);
const char* toTagOrEmpty(DeviceMode mode);
}

#endif
