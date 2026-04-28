import { buildAmbientPreviewFrames } from "../ambientEffectPreview.js";
import { buildAnimationDemoMaps, resolveAnimationFrameDelay } from "../led-matrix-demos/animationRegistry.js";
import { clamp, normalizeIntensity, normalizeSpeed } from "../led-matrix-demos/common.js";
import { mapsToFrames } from "../led-matrix-demos/frameCodec.js";
import { buildMazePreviewSequence } from "../led-matrix-demos/mazeDemo.js";
import { resolveDemoById } from "./demoCatalog.js";

function normalizeDensity(value) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) {
    return 72;
  }
  return clamp(Math.round(parsed), 10, 100);
}

function normalizeHexColor(value) {
  if (typeof value !== "string") {
    return "#64c8ff";
  }
  const body = value.trim().replace(/^#/, "");
  if (!/^[0-9a-fA-F]{6}$/.test(body)) {
    return "#64c8ff";
  }
  return `#${body.toLowerCase()}`;
}

function buildAnimationFramesForDemo(demoId, speed, intensity, options) {
  const maps = buildAnimationDemoMaps(demoId, speed, intensity, options);
  const delay = resolveAnimationFrameDelay(demoId, speed);
  return mapsToFrames(maps, delay);
}

function resolveMazePreviewDelays(phaseTags, speed, mazeSizeMode) {
  const effectiveSpeed = Math.min(speed, 6);
  const wideBaseDelays = [138, 126, 114, 102, 90, 80];
  const denseBaseDelays = [164, 152, 140, 128, 116, 104];
  const delayTable = mazeSizeMode === "dense" ? denseBaseDelays : wideBaseDelays;
  const baseDelay = delayTable[Math.max(0, Math.min(delayTable.length - 1, effectiveSpeed - 1))];
  const multiplierMap = {
    generation: 1.24,
    generation_hold: 1,
    solve: 1.02,
    solve_hold: 1,
    reveal: 0.98,
    reveal_hold: 1,
    travel: 0.92,
    done_hold: 1,
  };
  return phaseTags.map((phase) => {
    if (phase === "generation_hold" || phase === "solve_hold" || phase === "reveal_hold") {
      return 500;
    }
    if (phase === "done_hold") {
      return 125;
    }
    const multiplier = multiplierMap[phase] || 0.96;
    return clamp(Math.round(baseDelay * multiplier), 42, 168);
  });
}

function buildPreviewSequence(options) {
  const safeOptions = options || {};
  const demo = resolveDemoById(safeOptions.demoId);
  if (!demo) {
    return {
      maps: [],
      delays: [],
    };
  }

  const speed = normalizeSpeed(safeOptions.speed);
  const intensity = normalizeIntensity(safeOptions.intensity);
  const density = normalizeDensity(safeOptions.density);
  const color = normalizeHexColor(safeOptions.color);

  if (demo.type === "ambient") {
    const maps = buildAmbientPreviewFrames({
      preset: demo.ambientPreset,
      speed,
      intensity,
      density,
      color,
      loop: true,
      bouncingLogoScale: safeOptions.bouncingLogoScale,
    });
    const delay = clamp(172 - speed * 9, 60, 180);
    return {
      maps,
      delays: maps.map(() => delay),
    };
  }

  if (demo.id === "maze") {
    const sequence = buildMazePreviewSequence(speed, intensity, safeOptions);
    return {
      maps: sequence.maps,
      delays: resolveMazePreviewDelays(sequence.phaseTags, speed, safeOptions.mazeSizeMode),
    };
  }

  const maps = buildAnimationDemoMaps(demo.id, speed, intensity, {
    ...safeOptions,
    snakeWidth: safeOptions.snakeWidth,
    snakeColor: safeOptions.snakeColor,
    foodColor: safeOptions.foodColor,
    previewMode: true,
  });
  const delay = resolveAnimationFrameDelay(demo.id, speed);
  return {
    maps,
    delays: maps.map(() => delay),
  };
}

function buildLedMatrixPreviewMaps(options) {
  return buildPreviewSequence(options).maps;
}

function buildLedMatrixPreviewSequence(options) {
  return buildPreviewSequence(options);
}

function buildLedMatrixDemoFrames(options) {
  const safeOptions = options || {};
  const demo = resolveDemoById(safeOptions.demoId);
  if (!demo) {
    return [];
  }

  const speed = normalizeSpeed(safeOptions.speed);
  const intensity = normalizeIntensity(safeOptions.intensity);
  const density = normalizeDensity(safeOptions.density);
  const color = normalizeHexColor(safeOptions.color);

  if (demo.type === "ambient") {
    const maps = buildAmbientPreviewFrames({
      preset: demo.ambientPreset,
      speed,
      intensity,
      density,
      color,
      loop: true,
      bouncingLogoScale: safeOptions.bouncingLogoScale,
    });
    const delay = clamp(172 - speed * 9, 60, 180);
    return mapsToFrames(maps, delay);
  }

  return buildAnimationFramesForDemo(demo.id, speed, intensity, safeOptions);
}

export {
  buildAnimationFramesForDemo,
  buildLedMatrixDemoFrames,
  buildLedMatrixPreviewMaps,
  buildLedMatrixPreviewSequence,
  resolveMazePreviewDelays,
};
