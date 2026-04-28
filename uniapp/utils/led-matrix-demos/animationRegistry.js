import { buildMazeDemoMaps } from "./mazeDemo.js";
import { buildSnakeDemoMaps } from "./snakeDemo.js";
import { buildSortingVisualizerDemoMaps } from "./sortingVisualizerDemo.js";
import { buildTetrisGameDemoMaps } from "./tetrisGameDemo.js";
import { buildWaveSceneDemoMaps } from "./waveSceneDemo.js";
import { clamp, normalizeSpeed } from "./common.js";

const ANIMATION_BUILDERS = {
  github_wave: buildWaveSceneDemoMaps,
  maze: buildMazeDemoMaps,
  snake: buildSnakeDemoMaps,
  sorting_visualizer: buildSortingVisualizerDemoMaps,
  tetris_game: buildTetrisGameDemoMaps,
};

function resolveAnimationBuilder(demoId) {
  const builder = ANIMATION_BUILDERS[demoId];
  if (!builder) {
    throw new Error(`未注册的动画演示：${demoId}`);
  }
  return builder;
}

function buildAnimationDemoMaps(demoId, speed, intensity, options) {
  const builder = resolveAnimationBuilder(demoId);
  return builder(speed, intensity, options);
}

function resolveAnimationFrameDelay(demoId, speed) {
  const safeSpeed = normalizeSpeed(speed);
  if (demoId === "maze") {
    return clamp(120 - safeSpeed * 8, 38, 120);
  }
  if (demoId === "tetris_game") {
    return clamp(110 - safeSpeed * 5, 48, 110);
  }
  return clamp(156 - safeSpeed * 9, 50, 170);
}

export { buildAnimationDemoMaps, resolveAnimationFrameDelay };
