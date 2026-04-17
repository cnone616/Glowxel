#ifndef MODE_TAGS_H
#define MODE_TAGS_H

#include <Arduino.h>

namespace ModeTags {
static constexpr const char* CLOCK = "clock";
static constexpr const char* CANVAS = "canvas";
static constexpr const char* ANIMATION = "animation";
static constexpr const char* TRANSFERRING = "transferring";
static constexpr const char* THEME = "theme";
static constexpr const char* UNKNOWN = "unknown";

static constexpr const char* BREATH_EFFECT = "breath_effect";
static constexpr const char* RHYTHM_EFFECT = "rhythm_effect";
static constexpr const char* AMBIENT_EFFECT = "ambient_effect";
static constexpr const char* LED_MATRIX_SHOWCASE = "led_matrix_showcase";

static constexpr const char* TETRIS = "tetris";
static constexpr const char* EYES = "eyes";
static constexpr const char* GAME_SCREENSAVER = "game_screensaver";
static constexpr const char* GIF_PLAYER = "gif_player";

static constexpr const char* TEXT_DISPLAY = "text_display";
static constexpr const char* WEATHER = "weather";
static constexpr const char* COUNTDOWN = "countdown";
static constexpr const char* STOPWATCH = "stopwatch";
static constexpr const char* NOTIFICATION = "notification";
static constexpr const char* PLANET_SCREENSAVER = "planet_screensaver";

inline bool isBoardNativeModeTag(const String& modeTag) {
  return modeTag == TEXT_DISPLAY ||
         modeTag == WEATHER ||
         modeTag == COUNTDOWN ||
         modeTag == STOPWATCH ||
         modeTag == NOTIFICATION;
}

inline bool isAmbientModeTag(const String& modeTag) {
  return modeTag == AMBIENT_EFFECT || modeTag == LED_MATRIX_SHOWCASE;
}
}

#endif
