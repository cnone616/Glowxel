import {
  applyClockThemePreset,
  findClockThemePreset,
  getClockThemePresets,
  getMatchingClockThemeId,
} from "../../../uniapp/utils/clockThemePresets.js";

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function mapPreviewImagePath(previewImage) {
  if (typeof previewImage !== "string") {
    return "";
  }
  if (!previewImage.startsWith("/static/clockwise-themes/")) {
    return previewImage;
  }
  return previewImage.replace("/static/clockwise-themes/", "/clock-themes/");
}

function sanitizePreset(preset) {
  const next = {
    ...clone(preset),
    previewImage: mapPreviewImagePath(preset.previewImage),
  };
  return next;
}

export function getWebsiteClockThemePresets() {
  return getClockThemePresets().map(sanitizePreset);
}

export function findWebsiteClockThemePreset(themeId) {
  const preset = findClockThemePreset(themeId);
  return preset ? sanitizePreset(preset) : null;
}

export function applyWebsiteClockThemePreset(currentConfig, themeId) {
  return clone(applyClockThemePreset(clone(currentConfig), themeId));
}

export function getWebsiteMatchingClockThemeId(config) {
  return getMatchingClockThemeId(clone(config));
}
