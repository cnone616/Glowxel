import { buildTetrisPreviewFrames } from "../tetrisClockPreview.js";
import { normalizeSpeed } from "../led-matrix-demos/common.js";

function buildTetrisDemoMaps(config) {
  const safeSpeed = normalizeSpeed(config.speed);
  let speedLabel = "normal";
  if (safeSpeed <= 3) {
    speedLabel = "slow";
  } else if (safeSpeed >= 8) {
    speedLabel = "fast";
  }
  return buildTetrisPreviewFrames({
    sceneKey: "led_matrix_tetris_scene",
    clearMode: true,
    cellSize: config.cellSize,
    speed: speedLabel,
    showClock: config.showClock,
    pieces: [0, 1, 2, 3, 4, 5, 6],
  });
}

function resolveTetrisDropSpeed(speed) {
  const safeSpeed = normalizeSpeed(speed);
  if (safeSpeed <= 3) {
    return 300;
  }
  if (safeSpeed >= 8) {
    return 80;
  }
  return 150;
}

export { buildTetrisDemoMaps, resolveTetrisDropSpeed };
