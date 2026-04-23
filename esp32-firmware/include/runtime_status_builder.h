#ifndef RUNTIME_STATUS_BUILDER_H
#define RUNTIME_STATUS_BUILDER_H

#include <ArduinoJson.h>

namespace RuntimeStatusBuilder {
constexpr size_t kStatusJsonCapacity = 2048;
void fillStatus(JsonDocument& doc, bool includeSystemInfo, bool includeFreeHeap);
}

#endif
