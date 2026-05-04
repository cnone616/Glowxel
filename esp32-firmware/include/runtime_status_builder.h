#ifndef RUNTIME_STATUS_BUILDER_H
#define RUNTIME_STATUS_BUILDER_H

#include <ArduinoJson.h>

namespace RuntimeStatusBuilder {
constexpr size_t kStatusJsonCapacity = 2048;
constexpr size_t kCompactStatusJsonCapacity = 1024;
constexpr size_t kCompactStatusMessageBufferSize = 1024;
void fillStatus(JsonDocument& doc, bool includeSystemInfo, bool includeFreeHeap);
void fillCompactStatus(JsonDocument& doc);
}

#endif
