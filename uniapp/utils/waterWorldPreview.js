const WIDTH = 64;
const HEIGHT = 64;
const WATER_WORLD_PRESET_CONFIGS = Object.freeze({
  surface: Object.freeze({
    speed: 6,
    intensity: 72,
    density: 58,
    frameDelay: 48,
  }),
  current: Object.freeze({
    speed: 6,
    intensity: 72,
    density: 58,
    frameDelay: 48,
  }),
  caustics: Object.freeze({
    speed: 5,
    intensity: 74,
    density: 62,
    frameDelay: 52,
  }),
});
const WATER_WORLD_COLOR_THEME_OPTIONS = Object.freeze([
  Object.freeze({
    id: "ocean-blue",
    label: "海水蓝",
    swatches: Object.freeze(["#0a3d87", "#1d79d8", "#7ed8ff"]),
  }),
  Object.freeze({
    id: "deep-abyss",
    label: "深海蓝",
    swatches: Object.freeze(["#081f64", "#145fc0", "#6cbcff"]),
  }),
  Object.freeze({
    id: "lagoon-teal",
    label: "泻湖蓝",
    swatches: Object.freeze(["#0b466d", "#1f94b8", "#86e6ff"]),
  }),
  Object.freeze({
    id: "moon-tide",
    label: "夜潮蓝",
    swatches: Object.freeze(["#152562", "#2f6fd0", "#9fc8ff"]),
  }),
  Object.freeze({
    id: "reef-cyan",
    label: "礁湾蓝",
    swatches: Object.freeze(["#0d3559", "#1f88c7", "#78dfff"]),
  }),
]);
const WATER_WORLD_COLOR_THEME_MAP = Object.freeze({
  "ocean-blue": Object.freeze({
    deep: Object.freeze({ r: 6, g: 28, b: 86 }),
    mid: Object.freeze({ r: 20, g: 90, b: 176 }),
    light: Object.freeze({ r: 104, g: 204, b: 244 }),
    foam: Object.freeze({ r: 228, g: 246, b: 255 }),
  }),
  "deep-abyss": Object.freeze({
    deep: Object.freeze({ r: 4, g: 18, b: 74 }),
    mid: Object.freeze({ r: 14, g: 66, b: 162 }),
    light: Object.freeze({ r: 90, g: 182, b: 244 }),
    foam: Object.freeze({ r: 220, g: 238, b: 255 }),
  }),
  "lagoon-teal": Object.freeze({
    deep: Object.freeze({ r: 5, g: 34, b: 84 }),
    mid: Object.freeze({ r: 18, g: 112, b: 154 }),
    light: Object.freeze({ r: 98, g: 214, b: 238 }),
    foam: Object.freeze({ r: 230, g: 250, b: 255 }),
  }),
  "moon-tide": Object.freeze({
    deep: Object.freeze({ r: 12, g: 24, b: 88 }),
    mid: Object.freeze({ r: 38, g: 92, b: 188 }),
    light: Object.freeze({ r: 132, g: 202, b: 248 }),
    foam: Object.freeze({ r: 238, g: 246, b: 255 }),
  }),
  "reef-cyan": Object.freeze({
    deep: Object.freeze({ r: 8, g: 30, b: 74 }),
    mid: Object.freeze({ r: 22, g: 104, b: 176 }),
    light: Object.freeze({ r: 108, g: 216, b: 246 }),
    foam: Object.freeze({ r: 230, g: 248, b: 255 }),
  }),
});
const DEFAULT_WATER_WORLD_COLOR_THEME_ID = "ocean-blue";

function clamp(value, minValue, maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

function clampUnit(value) {
  return clamp(value, 0, 1);
}

function clampByte(value) {
  return clamp(Math.round(value), 0, 255);
}

function lerp(start, end, alpha) {
  return start + (end - start) * alpha;
}

function smoothstep(minValue, maxValue, value) {
  if (minValue === maxValue) {
    if (value < minValue) {
      return 0;
    }
    return 1;
  }
  const ratio = clampUnit((value - minValue) / (maxValue - minValue));
  return ratio * ratio * (3 - 2 * ratio);
}

function mixColor(left, right, alpha) {
  const ratio = clampUnit(alpha);
  return {
    r: lerp(left.r, right.r, ratio),
    g: lerp(left.g, right.g, ratio),
    b: lerp(left.b, right.b, ratio),
  };
}

function scaleColor(color, factor) {
  return {
    r: color.r * factor,
    g: color.g * factor,
    b: color.b * factor,
  };
}

function addColor(base, extra, alpha) {
  const ratio = clampUnit(alpha);
  return {
    r: base.r + extra.r * ratio,
    g: base.g + extra.g * ratio,
    b: base.b + extra.b * ratio,
  };
}

function setBufferPixel(buffer, x, y, r, g, b) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
    return;
  }
  const index = (y * WIDTH + x) * 4;
  buffer[index] = clampByte(r);
  buffer[index + 1] = clampByte(g);
  buffer[index + 2] = clampByte(b);
  buffer[index + 3] = 255;
}

function blendBufferPixel(buffer, x, y, color, alpha) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
    return;
  }
  const ratio = clampUnit(alpha);
  const index = (y * WIDTH + x) * 4;
  buffer[index] = clampByte(lerp(buffer[index], color.r, ratio));
  buffer[index + 1] = clampByte(lerp(buffer[index + 1], color.g, ratio));
  buffer[index + 2] = clampByte(lerp(buffer[index + 2], color.b, ratio));
  buffer[index + 3] = 255;
}

function clearBuffer(buffer, r, g, b) {
  for (let index = 0; index < buffer.length; index += 4) {
    buffer[index] = clampByte(r);
    buffer[index + 1] = clampByte(g);
    buffer[index + 2] = clampByte(b);
    buffer[index + 3] = 255;
  }
}

function seed(index, offset) {
  const raw =
    Math.sin((index + 1) * 12.9898 + offset * 78.233) * 43758.5453;
  const fract = raw - Math.floor(raw);
  if (fract < 0) {
    return fract + 1;
  }
  return fract;
}

function safeDivisor(value) {
  if (value > -0.0001 && value < 0.0001) {
    if (value < 0) {
      return -0.0001;
    }
    return 0.0001;
  }
  return value;
}

function resolveWaterWorldColorTheme(themeId) {
  const theme = WATER_WORLD_COLOR_THEME_MAP[themeId];
  if (!theme) {
    throw new Error(`Unsupported water world color theme: ${themeId}`);
  }
  return theme;
}

function createSurfacePalette(theme) {
  return {
    background: scaleColor(theme.deep, 0.24),
    deep: mixColor(theme.deep, theme.mid, 0.08),
    mid: theme.mid,
    shallow: mixColor(theme.mid, theme.light, 0.34),
    bright: mixColor(theme.light, theme.foam, 0.12),
    foam: theme.foam,
    trough: scaleColor(theme.deep, 0.56),
    edge: scaleColor(theme.deep, 0.38),
  };
}

function createCurrentPalette(theme) {
  return {
    background: scaleColor(theme.deep, 0.24),
    deep: scaleColor(theme.deep, 0.68),
    mid: mixColor(theme.deep, theme.mid, 0.52),
    bright: mixColor(theme.mid, theme.light, 0.7),
    glow: mixColor(theme.light, theme.foam, 0.26),
    vein: scaleColor(theme.deep, 0.42),
  };
}

function createCausticsPalette(theme) {
  return {
    background: scaleColor(theme.deep, 0.28),
    abyss: scaleColor(theme.deep, 0.58),
    deep: mixColor(theme.deep, theme.mid, 0.34),
    teal: theme.mid,
    aqua: theme.light,
    glint: theme.foam,
    shadeColor: scaleColor(theme.deep, 0.38),
    vignetteColor: scaleColor(theme.deep, 0.46),
  };
}

function createWaterWorldRenderPalette(themeId) {
  const theme = resolveWaterWorldColorTheme(themeId);
  return {
    surface: createSurfacePalette(theme),
    current: createCurrentPalette(theme),
    caustics: createCausticsPalette(theme),
  };
}

function bufferToFrameMap(buffer) {
  const frameMap = new Map();
  for (let y = 0; y < HEIGHT; y += 1) {
    for (let x = 0; x < WIDTH; x += 1) {
      const index = (y * WIDTH + x) * 4;
      const r = buffer[index];
      const g = buffer[index + 1];
      const b = buffer[index + 2];
      if (r === 0 && g === 0 && b === 0) {
        continue;
      }
      frameMap.set(`${x},${y}`, `rgb(${r}, ${g}, ${b})`);
    }
  }
  return frameMap;
}

function createRuntime() {
  return {
    surface: {
      lastElapsed: 0,
      lastSpawn: 0,
      pulses: [],
    },
  };
}

function spawnSurfacePulse(runtime, x, y, strength) {
  runtime.surface.pulses.push({
    x,
    y,
    strength,
    bornAt: runtime.surface.lastElapsed,
  });
}

function maybeSpawnSurfacePulse(
  runtime,
  elapsed,
  densityUnit,
  frequencyUnit,
  strengthUnit,
) {
  const interval = lerp(3600, 1850, frequencyUnit * 0.68 + densityUnit * 0.32);
  if (elapsed - runtime.surface.lastSpawn < interval) {
    return;
  }
  runtime.surface.lastSpawn = elapsed;
  const index =
    runtime.surface.pulses.length + Math.floor(elapsed / interval);
  spawnSurfacePulse(
    runtime,
    0.18 + seed(index, 0.31) * 0.64,
    0.2 + seed(index, 0.63) * 0.46,
    0.14 + strengthUnit * 0.16 + seed(index, 0.87) * 0.16,
  );
}

function sampleSurfaceBase(nx, ny, time, speedUnit, densityUnit) {
  const driftX = nx - 0.5;
  const driftY = ny - 0.5;
  const bandWarpA = Math.sin(
    driftX * 5.8 +
      time * 0.58 +
      Math.sin(driftY * 4.6 - time * 0.2) * 0.86,
  );
  const bandWarpB = Math.sin(
    driftX * 3.2 -
      time * 0.36 +
      Math.sin(driftY * 7.1 + time * 0.28) * 0.58,
  );
  const longSwell = Math.sin(
    (driftY * 1.72 + bandWarpA * 0.2 + driftX * 0.09) *
      (10.4 + densityUnit * 3.8) -
      time * (1.42 + speedUnit * 0.78),
  );
  const secondarySwell = Math.sin(
    (driftY * 1.18 - driftX * 0.26 + bandWarpB * 0.16) *
      (14.2 + densityUnit * 4.6) -
      time * (1.84 + speedUnit * 0.96),
  );
  const diagonalSwell = Math.sin(
    (driftY * 0.82 + driftX * 0.42 + bandWarpA * 0.08) * 8.6 -
      time * (1.08 + speedUnit * 0.54),
  );
  const backwash = Math.cos(
    (driftY * 1.36 - driftX * 0.92 + bandWarpB * 0.06) * 9.2 +
      time * (1.18 + densityUnit * 0.64),
  );
  const capillary = Math.sin(
    (driftY * 2.84 + driftX * 0.62 + bandWarpA * 0.04) * 22.8 -
      time * (2.46 + speedUnit * 0.84),
  );
  const trough = Math.cos(
    (driftY * 0.66 - driftX * 0.14) * 6.4 -
      time * (0.82 + speedUnit * 0.3),
  );

  return clampUnit(
    0.5 +
      0.5 *
        (longSwell * 0.34 +
          secondarySwell * 0.24 +
          diagonalSwell * 0.16 +
          backwash * 0.1 +
          capillary * 0.08 +
          trough * 0.08),
  );
}

function sampleSurfaceRipple(runtime, nx, ny, elapsed, speedUnit) {
  let ripple = 0;
  for (let index = 0; index < runtime.surface.pulses.length; index += 1) {
    const pulse = runtime.surface.pulses[index];
    const age = (elapsed - pulse.bornAt) * 0.001;
    const dx = nx - pulse.x;
    const dy = ny - pulse.y;
    const dist = Math.sqrt(dx * dx + dy * dy);
    const phase = dist * 54 - age * (7.4 + speedUnit * 2.8);
    ripple +=
      Math.sin(phase) *
      Math.exp(-dist * 11.4) *
      Math.exp(-age * 0.68) *
      pulse.strength;
  }
  return ripple;
}

function sampleSurfaceHeight(
  runtime,
  nx,
  ny,
  elapsed,
  time,
  speedUnit,
  densityUnit,
) {
  const base = sampleSurfaceBase(nx, ny, time, speedUnit, densityUnit);
  const ripple = sampleSurfaceRipple(runtime, nx, ny, elapsed, speedUnit);
  return clampUnit(base + ripple * (0.045 + densityUnit * 0.018));
}

function renderSurface(buffer, elapsed, params, runtime) {
  const palette = params.palette.surface;
  clearBuffer(
    buffer,
    palette.background.r,
    palette.background.g,
    palette.background.b,
  );

  const speedUnit = params.speed / 10;
  const intensityUnit = params.intensity / 100;
  const densityUnit = params.density / 100;
  const waterLevelUnit = params.waterLevel / 100;
  const frequencyUnit = params.frequency / 100;
  const strengthUnit = params.strength / 100;
  const surfaceDensityUnit = clampUnit(densityUnit * 0.44 + frequencyUnit * 0.56);
  const time =
    elapsed *
    (0.00038 + speedUnit * 0.0011 + frequencyUnit * 0.00072);
  const sampleStep = 1.4 / (WIDTH - 1);

  maybeSpawnSurfacePulse(
    runtime,
    elapsed,
    densityUnit,
    frequencyUnit,
    strengthUnit,
  );
  runtime.surface.lastElapsed = elapsed;
  runtime.surface.pulses = runtime.surface.pulses.filter((pulse) => {
    return elapsed - pulse.bornAt < 6200;
  });

  const deep = palette.deep;
  const mid = palette.mid;
  const shallow = palette.shallow;
  const bright = palette.bright;
  const foamColor = palette.foam;
  const troughColor = palette.trough;
  const edgeColor = palette.edge;
  const lightDir = { x: -0.46, y: 0.36, z: 0.82 };

  for (let y = 0; y < HEIGHT; y += 1) {
    for (let x = 0; x < WIDTH; x += 1) {
      const nx = x / (WIDTH - 1);
      const ny = y / (HEIGHT - 1);

      const field = sampleSurfaceHeight(
        runtime,
        nx,
        ny,
        elapsed,
        time,
        speedUnit,
        surfaceDensityUnit,
      );
      const fieldL = sampleSurfaceHeight(
        runtime,
        Math.max(0, nx - sampleStep),
        ny,
        elapsed,
        time,
        speedUnit,
        surfaceDensityUnit,
      );
      const fieldX = sampleSurfaceHeight(
        runtime,
        Math.min(1, nx + sampleStep),
        ny,
        elapsed,
        time,
        speedUnit,
        surfaceDensityUnit,
      );
      const fieldU = sampleSurfaceHeight(
        runtime,
        nx,
        Math.max(0, ny - sampleStep),
        elapsed,
        time,
        speedUnit,
        surfaceDensityUnit,
      );
      const fieldY = sampleSurfaceHeight(
        runtime,
        nx,
        Math.min(1, ny + sampleStep),
        elapsed,
        time,
        speedUnit,
        surfaceDensityUnit,
      );

      const fieldSmooth = clampUnit(
        field * 0.44 + (fieldL + fieldX + fieldU + fieldY) * 0.14,
      );
      const gradientX = (fieldX - fieldL) / (sampleStep * 2);
      const gradientY = (fieldY - fieldU) / (sampleStep * 2);
      const curvature =
        fieldX + fieldL + fieldY + fieldU - fieldSmooth * 4;
      const slope =
        Math.abs(gradientX) * 0.44 + Math.abs(gradientY) * 0.96;
      const normalX = -gradientX * (0.2 + surfaceDensityUnit * 0.08 + strengthUnit * 0.06);
      const normalY = -gradientY * (0.88 + surfaceDensityUnit * 0.18 + strengthUnit * 0.12);
      const normalLength = Math.sqrt(
        normalX * normalX + normalY * normalY + 1,
      );
      const lightAmount = clampUnit(
        (normalX * lightDir.x + normalY * lightDir.y + lightDir.z) /
          normalLength,
      );
      const specular = Math.pow(lightAmount, 14 - intensityUnit * 5.5);
      const longGlint = Math.pow(
        clampUnit(
          1 - Math.abs(gradientX * 2.6 - gradientY * 0.22 - 0.12),
        ),
        7,
      );
      const undercurrent =
        0.5 +
        0.5 *
          Math.sin(
            (nx * (0.72 + frequencyUnit * 0.24) - ny * 1.14) *
              (5.8 + frequencyUnit * 2.2) -
              time * (1.1 + surfaceDensityUnit * 0.36),
          );
      const crestAmount =
        smoothstep(0.58 - strengthUnit * 0.05, 0.86, fieldSmooth) *
        smoothstep(0.05, 0.34 + strengthUnit * 0.04, slope);
      const ridgeHighlight =
        crestAmount *
        Math.pow(
          clampUnit(
            0.5 +
              0.5 *
                Math.sin(
                  (ny * 2.04 + nx * 0.16) * 15.6 -
                    time * (1.12 + speedUnit * 0.22),
                ),
          ),
          3.2,
        ) *
        (0.05 + intensityUnit * 0.09 + strengthUnit * 0.05);
      const foamAmount =
        crestAmount *
        smoothstep(-0.2, 0.05, -curvature) *
        (0.08 + intensityUnit * 0.14 + strengthUnit * 0.1);
      const highlightAmount =
        crestAmount * longGlint * (0.12 + intensityUnit * 0.1);
      const sparkle =
        smoothstep(0.52, 0.9, fieldSmooth) *
        Math.pow(
          clampUnit(1 - Math.abs(gradientY * 0.7 + gradientX * 1.34 - 0.1)),
          4,
        ) *
        (0.03 + intensityUnit * 0.05);
      const foamGrain =
        smoothstep(0.8 - strengthUnit * 0.06, 1, fieldSmooth) *
        Math.pow(
          clampUnit(
            0.5 +
              0.5 *
                Math.sin(
                  nx * (26 + frequencyUnit * 12) +
                    ny * 17 -
                    time * (5.2 + frequencyUnit * 1.7) +
                    Math.sin(ny * 11 + time * 1.2) * 0.6,
                ),
          ),
          4.6,
        ) *
        (0.02 + strengthUnit * 0.05);
      const subsurface =
        smoothstep(0.36, 0.8, fieldSmooth) *
        (0.04 + (1 - ny) * (0.04 + waterLevelUnit * 0.08) + undercurrent * 0.035);
      const troughShadow =
        smoothstep(0.08, 0.52, 1 - fieldSmooth) *
        smoothstep(0.035, 0.28, slope) *
        0.24;

      const depthMix = clampUnit(
        0.04 +
          ny * (0.5 + (1 - waterLevelUnit) * 0.18) +
          fieldSmooth * (0.16 + waterLevelUnit * 0.08),
      );
      let color = mixColor(deep, mid, depthMix);
      color = mixColor(
        color,
        shallow,
        smoothstep(0.5, 0.88, fieldSmooth) * 0.32 + highlightAmount * 0.08,
      );
      color = mixColor(color, bright, subsurface);
      color = mixColor(
        color,
        bright,
        clampUnit(
          highlightAmount * 0.78 +
            ridgeHighlight * 0.74 +
            specular * 0.28 +
            sparkle,
        ),
      );
      color = mixColor(color, foamColor, foamAmount * 0.74);
      color = mixColor(color, foamColor, foamGrain);
      color = mixColor(color, troughColor, troughShadow);

      const edgeShade = smoothstep(
        1.06,
        0.22,
        Math.abs(nx - 0.5) * 1.18 + Math.abs(ny - 0.48) * 0.54,
      );
      color = mixColor(
        edgeColor,
        color,
        0.42 + edgeShade * 0.58,
      );

      setBufferPixel(buffer, x, y, color.r, color.g, color.b);
    }
  }
}

function renderCurrent(buffer, elapsed, params) {
  const palette = params.palette.current;
  clearBuffer(
    buffer,
    palette.background.r,
    palette.background.g,
    palette.background.b,
  );

  const speedUnit = params.speed / 10;
  const intensityUnit = params.intensity / 100;
  const densityUnit = params.density / 100;
  const waterLevelUnit = params.waterLevel / 100;
  const frequencyUnit = params.frequency / 100;
  const strengthUnit = params.strength / 100;
  const currentDensityUnit = clampUnit(densityUnit * 0.4 + frequencyUnit * 0.6);
  const time =
    elapsed *
    (0.00026 + speedUnit * 0.00096 + frequencyUnit * 0.00072);

  const deep = palette.deep;
  const mid = palette.mid;
  const bright = palette.bright;
  const glow = palette.glow;
  const vein = palette.vein;

  for (let y = 0; y < HEIGHT; y += 1) {
    for (let x = 0; x < WIDTH; x += 1) {
      const nx = x / (WIDTH - 1);
      const ny = y / (HEIGHT - 1);

      const flowA = Math.sin(
        nx * (6.8 + frequencyUnit * 4.4) +
          time * (2 + speedUnit * 1.16 + frequencyUnit * 0.7) +
          Math.sin(ny * (4.4 + frequencyUnit * 2.6) + time * 0.8) * 1.35,
      );
      const flowB = Math.sin(
        (nx + ny) * (6.2 + frequencyUnit * 3.8) -
          time * (1.56 + speedUnit * 0.88 + frequencyUnit * 0.46),
      );
      const flowC = Math.cos(
        ny * (7.4 + currentDensityUnit * 3.6) -
          nx * (4 + frequencyUnit * 1.8) +
          time * (1.2 + currentDensityUnit * 0.72),
      );
      const currentField =
        0.5 + 0.5 * (flowA * 0.42 + flowB * 0.34 + flowC * 0.24);

      const px = nx * Math.PI * 2 - 20;
      const py = ny * Math.PI * 2 - 20;
      let ix = px;
      let iy = py;
      let causticField = 1;

      let timeSeed = time * (3.2 - speedUnit * 1.4);
      let temp = px + Math.cos(timeSeed - ix) + Math.sin(timeSeed + iy);
      iy = py + Math.sin(timeSeed - iy) + Math.cos(timeSeed + ix);
      ix = temp;
      causticField +=
        1 /
        Math.hypot(
          (px / safeDivisor(Math.sin(timeSeed + ix))) *
            (0.03 + currentDensityUnit * 0.032),
          (py / safeDivisor(Math.cos(timeSeed + iy))) *
            (0.03 + currentDensityUnit * 0.032),
        );

      timeSeed = time * (2.0 - speedUnit * 0.6);
      temp = px + Math.cos(timeSeed - ix) + Math.sin(timeSeed + iy);
      iy = py + Math.sin(timeSeed - iy) + Math.cos(timeSeed + ix);
      ix = temp;
      causticField +=
        1 /
        Math.hypot(
          (px / safeDivisor(Math.sin(timeSeed + ix))) *
            (0.03 + currentDensityUnit * 0.032),
          (py / safeDivisor(Math.cos(timeSeed + iy))) *
            (0.03 + currentDensityUnit * 0.032),
        );

      let caustic = 1.65 - Math.sqrt(Math.max(0.0001, causticField / 2));
      caustic = clampUnit(Math.pow(caustic, 4));
      const suspendedGlow =
        Math.pow(
          clampUnit(
            0.5 +
              0.5 *
                Math.sin(
                  nx * (30 + frequencyUnit * 12) -
                    ny * 14 +
                    time * (8 + frequencyUnit * 3.4) +
                    Math.sin(ny * 12 + time * 2) * 0.7,
                ),
          ),
          8,
        ) *
        (0.04 + strengthUnit * 0.12) *
        (0.34 + (1 - ny) * 0.32);

      const depthMix = clampUnit(
        0.05 +
          ny * (0.5 + waterLevelUnit * 0.3) +
          currentField * (0.14 + waterLevelUnit * 0.06),
      );
      let color = mixColor(deep, mid, depthMix);
      color = mixColor(
        color,
        bright,
        smoothstep(0.46, 0.94, currentField) * (0.52 + strengthUnit * 0.24),
      );
      color = mixColor(
        color,
        glow,
        caustic * (0.32 + intensityUnit * 0.18 + strengthUnit * 0.18),
      );
      color = mixColor(color, glow, suspendedGlow);

      const darkVein =
        smoothstep(0.68, 0.9, currentField) * 0.18 * (1 - caustic);
      color = mixColor(color, vein, darkVein);

      setBufferPixel(buffer, x, y, color.r, color.g, color.b);
    }
  }
}

function sampleCausticsField(nx, ny, time, speedUnit, densityUnit) {
  const px = (nx - 0.5) * (11.6 + densityUnit * 2.2);
  const py = (ny - 0.46) * (13.4 + densityUnit * 2.8);
  let ix = px;
  let iy = py;
  let energy = 0;

  for (let index = 0; index < 3; index += 1) {
    const phase =
      time * (1.2 + index * 0.46 + speedUnit * 0.3) + index * 1.7;
    const warp =
      Math.sin(iy * (1.7 + index * 0.22) + phase) * (0.78 - index * 0.12);
    const twist =
      Math.cos(ix * (1.4 + index * 0.28) - phase * 0.86) *
      (0.66 - index * 0.1);
    const nextX = px + warp + Math.sin(phase + ix * 0.72 - iy * 0.38) * 0.34;
    iy = py + twist + Math.cos(phase * 0.92 + iy * 0.66 + ix * 0.24) * 0.3;
    ix = nextX;
    const ridge =
      1 -
      Math.abs(
        Math.sin(
          ix * (2.2 + index * 0.42) +
            iy * (1.8 + index * 0.36) +
            phase * 1.6,
        ),
      );
    energy += Math.pow(clampUnit(ridge), 2.3 - index * 0.3);
  }

  const swell =
    0.5 +
    0.5 *
      Math.sin(
        (nx * 1.12 - ny * 0.84) * 6.6 - time * (0.56 + speedUnit * 0.18),
      );
  const focus = smoothstep(0.16, 0.98, ny);
  return clampUnit(
    Math.pow(clampUnit(energy / 2.55), 1.42) * (0.72 + swell * 0.28) * focus,
  );
}

function renderCaustics(buffer, elapsed, params) {
  const palette = params.palette.caustics;
  clearBuffer(
    buffer,
    palette.background.r,
    palette.background.g,
    palette.background.b,
  );

  const speedUnit = params.speed / 10;
  const intensityUnit = params.intensity / 100;
  const densityUnit = params.density / 100;
  const waterLevelUnit = params.waterLevel / 100;
  const frequencyUnit = params.frequency / 100;
  const strengthUnit = params.strength / 100;
  const causticDensityUnit = clampUnit(densityUnit * 0.42 + frequencyUnit * 0.58);
  const time =
    elapsed *
    (0.00028 + speedUnit * 0.00082 + frequencyUnit * 0.00056);

  const abyss = palette.abyss;
  const deep = palette.deep;
  const teal = palette.teal;
  const aqua = palette.aqua;
  const glint = palette.glint;
  const shadeColor = palette.shadeColor;
  const vignetteColor = palette.vignetteColor;

  for (let y = 0; y < HEIGHT; y += 1) {
    for (let x = 0; x < WIDTH; x += 1) {
      const nx = x / (WIDTH - 1);
      const ny = y / (HEIGHT - 1);

      let caustic = sampleCausticsField(
        nx,
        ny,
        time,
        speedUnit,
        causticDensityUnit,
      );
      const lowWave =
        0.5 +
        0.5 *
          Math.sin(
            (nx * 0.82 + ny * (1 + waterLevelUnit * 0.3)) *
              (4.6 + frequencyUnit * 1.6) -
              time * (0.72 + speedUnit * 0.2),
          );
      const crossWave =
        0.5 +
        0.5 *
          Math.cos(
            (nx * 1.08 - ny * 0.48) * (6.4 + causticDensityUnit * 2.2) +
              time * (0.94 + causticDensityUnit * 0.24),
          );
      const haze =
        0.5 +
        0.5 *
          Math.sin(
            (nx * 1.3 + ny * 0.64) * 3.8 +
              time * (0.48 + densityUnit * 0.16) +
                Math.sin(ny * 5.2 - time * 0.64) * 0.42,
          );
      caustic = clampUnit(caustic * 0.76 + lowWave * 0.12 + crossWave * 0.12);
      const bottomMix = smoothstep(0.18, 1, ny);
      const brightBand =
        caustic *
        Math.pow(
          clampUnit(
            1 -
              Math.abs(
                Math.sin(
                  (nx * 1.7 - ny * 1.18) * 9.4 +
                    time * (1.18 + speedUnit * 0.24),
                ),
              ),
          ),
          3.4,
        );
      const dustFlash =
        Math.pow(
          clampUnit(
            0.5 +
              0.5 *
                Math.sin(
                  nx * (26 + frequencyUnit * 12) +
                    ny * 18 -
                    time * (4.4 + frequencyUnit * 1.2),
                ),
          ),
          5,
        ) *
        (0.012 + strengthUnit * 0.04) *
        smoothstep(0.18, 0.84, ny);
      const shade = smoothstep(0.46, 0.96, 1 - caustic) * 0.2;

      let color = mixColor(abyss, deep, 0.18 + bottomMix * 0.44 + lowWave * 0.1);
      color = mixColor(color, teal, 0.16 + haze * 0.22 + crossWave * 0.14);
      color = mixColor(
        color,
        aqua,
        clampUnit(
          caustic * (0.22 + intensityUnit * 0.16 + strengthUnit * 0.12) +
            brightBand * 0.42,
        ),
      );
      color = mixColor(
        color,
        glint,
        clampUnit(
          brightBand * (0.1 + intensityUnit * 0.12 + strengthUnit * 0.08) +
            dustFlash,
        ),
      );
      color = mixColor(color, shadeColor, shade);

      const vignette = smoothstep(
        1.12,
        0.16,
        Math.abs(nx - 0.5) * 1.08 + Math.abs(ny - 0.62) * 0.78,
      );
      color = mixColor(vignetteColor, color, 0.46 + vignette * 0.54);

      setBufferPixel(buffer, x, y, color.r, color.g, color.b);
    }
  }
}

function renderMode(mode, buffer, elapsed, params, runtime) {
  if (mode === "surface") {
    renderSurface(buffer, elapsed, params, runtime);
    return;
  }
  if (mode === "current") {
    renderCurrent(buffer, elapsed, params);
    return;
  }
  if (mode === "caustics") {
    renderCaustics(buffer, elapsed, params);
    return;
  }
  throw new Error(`Unsupported water world mode: ${mode}`);
}

function getPresetConfig(preset) {
  if (preset === "surface") {
    return WATER_WORLD_PRESET_CONFIGS.surface;
  }
  if (preset === "current") {
    return WATER_WORLD_PRESET_CONFIGS.current;
  }
  if (preset === "caustics") {
    return WATER_WORLD_PRESET_CONFIGS.caustics;
  }
  throw new Error(`Unsupported water world preset: ${preset}`);
}

function assertPreviewField(config, fieldName) {
  if (!Number.isFinite(config[fieldName])) {
    throw new Error(`Missing water world preview field: ${fieldName}`);
  }
}

function createWaterWorldPreviewState(config, colorThemeId) {
  const preset = getPresetConfig(config.preset);
  assertPreviewField(config, "waterLevel");
  assertPreviewField(config, "frequency");
  assertPreviewField(config, "strength");
  return {
    preset: config.preset,
    elapsed: 0,
    frameDelay: preset.frameDelay,
    runtime: createRuntime(),
    params: {
      speed: preset.speed,
      intensity: preset.intensity,
      density: preset.density,
      waterLevel: config.waterLevel,
      frequency: config.frequency,
      strength: config.strength,
      palette: createWaterWorldRenderPalette(colorThemeId),
    },
  };
}

function stepWaterWorldPreviewState(state) {
  state.elapsed += state.frameDelay;
}

function renderWaterWorldPreviewState(state) {
  const buffer = new Uint8ClampedArray(WIDTH * HEIGHT * 4);
  renderMode(state.preset, buffer, state.elapsed, state.params, state.runtime);
  return bufferToFrameMap(buffer);
}

function buildWaterWorldPreviewFrames(config, frameCount = 56, colorThemeId) {
  const state = createWaterWorldPreviewState(config, colorThemeId);
  const frames = [];

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    frames.push(renderWaterWorldPreviewState(state));
    stepWaterWorldPreviewState(state);
  }

  return frames;
}

export {
  DEFAULT_WATER_WORLD_COLOR_THEME_ID,
  WATER_WORLD_COLOR_THEME_OPTIONS,
  buildWaterWorldPreviewFrames,
  createWaterWorldPreviewState,
  renderWaterWorldPreviewState,
  stepWaterWorldPreviewState,
};
