#ifndef AMBIENT_PRESET_CODEC_H
#define AMBIENT_PRESET_CODEC_H

#include <Arduino.h>

namespace AmbientPresetCodec {
const char* toString(uint8_t preset);
bool fromString(const String& value, uint8_t& preset);
}

#endif
