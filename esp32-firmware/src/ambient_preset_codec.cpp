#include "ambient_preset_codec.h"
#include "display_manager.h"

namespace AmbientPresetCodec {
const char* toString(uint8_t preset) {
  if (preset == AMBIENT_PRESET_AURORA) {
    return "digital_rain";
  }
  if (preset == AMBIENT_PRESET_PLASMA) {
    return "starfield";
  }
  if (preset == AMBIENT_PRESET_MATRIX_RAIN) {
    return "neon_tunnel";
  }
  if (preset == AMBIENT_PRESET_FIREFLY_SWARM) {
    return "metablob";
  }
  if (preset == AMBIENT_PRESET_METEOR_SHOWER) {
    return "falling_sand";
  }
  if (preset == AMBIENT_PRESET_OCEAN_CURRENT) {
    return "sparks";
  }
  if (preset == AMBIENT_PRESET_NEON_GRID) {
    return "wave_pattern";
  }
  if (preset == AMBIENT_PRESET_SUNSET_BLUSH) {
    return "rain_scene";
  }
  if (preset == AMBIENT_PRESET_STARFIELD_DRIFT) {
    return "watermelon_plasma";
  }
  if (preset == AMBIENT_PRESET_BOIDS) {
    return "boids";
  }
  if (preset == AMBIENT_PRESET_BOUNCING_LOGO) {
    return "bouncing_logo";
  }
  if (preset == AMBIENT_PRESET_SORTING_VISUALIZER) {
    return "sorting_visualizer";
  }
  if (preset == AMBIENT_PRESET_CLOCK_SCENE) {
    return "clock_scene";
  }
  if (preset == AMBIENT_PRESET_COUNTDOWN_SCENE) {
    return "countdown_scene";
  }
  if (preset == AMBIENT_PRESET_WEATHER_SCENE) {
    return "weather_scene";
  }
  if (preset == AMBIENT_PRESET_GAME_OF_LIFE) {
    return "game_of_life";
  }
  if (preset == AMBIENT_PRESET_JULIA_SET) {
    return "julia_set";
  }
  if (preset == AMBIENT_PRESET_REACTION_DIFFUSION) {
    return "reaction_diffusion";
  }
  if (preset == AMBIENT_PRESET_COSMIC_KALE) {
    return "cosmic_kale";
  }
  if (preset == AMBIENT_PRESET_VOID_FIRE) {
    return "void_fire";
  }
  if (preset == AMBIENT_PRESET_DEEP_SPACE_NEBULA) {
    return "deep_space_nebula";
  }
  return "";
}

bool fromString(const String& value, uint8_t& preset) {
  if (value == "digital_rain") {
    preset = AMBIENT_PRESET_AURORA;
    return true;
  }
  if (value == "starfield") {
    preset = AMBIENT_PRESET_PLASMA;
    return true;
  }
  if (value == "neon_tunnel") {
    preset = AMBIENT_PRESET_MATRIX_RAIN;
    return true;
  }
  if (value == "metablob") {
    preset = AMBIENT_PRESET_FIREFLY_SWARM;
    return true;
  }
  if (value == "falling_sand") {
    preset = AMBIENT_PRESET_METEOR_SHOWER;
    return true;
  }
  if (value == "sparks") {
    preset = AMBIENT_PRESET_OCEAN_CURRENT;
    return true;
  }
  if (value == "wave_pattern") {
    preset = AMBIENT_PRESET_NEON_GRID;
    return true;
  }
  if (value == "rain_scene") {
    preset = AMBIENT_PRESET_SUNSET_BLUSH;
    return true;
  }
  if (value == "watermelon_plasma") {
    preset = AMBIENT_PRESET_STARFIELD_DRIFT;
    return true;
  }
  if (value == "boids") {
    preset = AMBIENT_PRESET_BOIDS;
    return true;
  }
  if (value == "bouncing_logo") {
    preset = AMBIENT_PRESET_BOUNCING_LOGO;
    return true;
  }
  if (value == "sorting_visualizer") {
    preset = AMBIENT_PRESET_SORTING_VISUALIZER;
    return true;
  }
  if (value == "clock_scene") {
    preset = AMBIENT_PRESET_CLOCK_SCENE;
    return true;
  }
  if (value == "countdown_scene") {
    preset = AMBIENT_PRESET_COUNTDOWN_SCENE;
    return true;
  }
  if (value == "weather_scene") {
    preset = AMBIENT_PRESET_WEATHER_SCENE;
    return true;
  }
  if (value == "game_of_life") {
    preset = AMBIENT_PRESET_GAME_OF_LIFE;
    return true;
  }
  if (value == "julia_set") {
    preset = AMBIENT_PRESET_JULIA_SET;
    return true;
  }
  if (value == "reaction_diffusion") {
    preset = AMBIENT_PRESET_REACTION_DIFFUSION;
    return true;
  }
  if (value == "cosmic_kale") {
    preset = AMBIENT_PRESET_COSMIC_KALE;
    return true;
  }
  if (value == "void_fire") {
    preset = AMBIENT_PRESET_VOID_FIRE;
    return true;
  }
  if (value == "deep_space_nebula") {
    preset = AMBIENT_PRESET_DEEP_SPACE_NEBULA;
    return true;
  }
  return false;
}
}
