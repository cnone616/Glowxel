#ifndef WEBSOCKET_EFFECT_COMMON_H
#define WEBSOCKET_EFFECT_COMMON_H

#include <ArduinoJson.h>

inline void wsSetErrorResponse(StaticJsonDocument<768>& response, const char* message) {
  response["status"] = "error";
  response["message"] = message;
}

inline bool wsParseRequiredColorObject(
  JsonDocument& doc,
  const char* fieldName,
  uint8_t& outR,
  uint8_t& outG,
  uint8_t& outB
) {
  if (!doc.containsKey(fieldName)) {
    return false;
  }
  JsonObject color = doc[fieldName].as<JsonObject>();
  if (!color.containsKey("r") || !color.containsKey("g") || !color.containsKey("b")) {
    return false;
  }
  outR = color["r"].as<uint8_t>();
  outG = color["g"].as<uint8_t>();
  outB = color["b"].as<uint8_t>();
  return true;
}

inline bool wsEnsureColorObject(JsonObject color) {
  return color.containsKey("r") && color.containsKey("g") && color.containsKey("b");
}

inline int wsClampInt(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

#endif
