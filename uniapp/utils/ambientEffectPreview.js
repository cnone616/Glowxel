import { buildRainAmbientFrame } from "./ambient-scenes/rainScene.js";
import { buildStarfieldAmbientFrame } from "./ambient-scenes/starfieldScene.js";
import { buildDigitalRainAmbientFrames } from "./ambient-scenes/digitalRainScene.js";
import { buildMetaBlobAmbientFrame } from "./ambient-scenes/metaBlobScene.js";
import { buildNeonTunnelAmbientFrame } from "./ambient-scenes/neonTunnelScene.js";
import { buildBoidsAmbientFrame } from "./ambient-scenes/boidsScene.js";
import { buildSortingVisualizerAmbientFrame } from "./ambient-scenes/sortingVisualizerScene.js";
import { buildClockSceneAmbientFrame } from "./ambient-scenes/clockScene.js";

const LOCAL_ONLY_AMBIENT_PRESETS = new Set([
  "clock_scene",
  "starfield",
  "metablob",
  "digital_rain",
  "neon_tunnel",
  "boids",
  "falling_sand",
  "sorting_visualizer",
  "bouncing_logo",
  "game_of_life",
  "julia_set",
  "wave_pattern",
  "watermelon_plasma",
  "rain_scene",
  "sparks",
  "countdown_scene",
  "reaction_diffusion",
]);

const DIGIT_GLYPHS = {
  "0": ["111", "101", "101", "101", "111"],
  "1": ["010", "110", "010", "010", "111"],
  "2": ["111", "001", "111", "100", "111"],
  "3": ["111", "001", "111", "001", "111"],
  "4": ["101", "101", "111", "001", "001"],
  "5": ["111", "100", "111", "001", "111"],
  "6": ["111", "100", "111", "101", "111"],
  "7": ["111", "001", "010", "010", "010"],
  "8": ["111", "101", "111", "101", "111"],
  "9": ["111", "101", "111", "001", "111"],
  ":": ["000", "010", "000", "010", "000"],
  "-": ["000", "000", "111", "000", "000"],
  "A": ["010", "101", "111", "101", "101"],
  "C": ["111", "100", "100", "100", "111"],
  "D": ["110", "101", "101", "101", "110"],
  "E": ["111", "100", "110", "100", "111"],
  "I": ["111", "010", "010", "010", "111"],
  "M": ["101", "111", "111", "101", "101"],
  "N": ["101", "111", "111", "111", "101"],
  "O": ["111", "101", "101", "101", "111"],
  "R": ["110", "101", "110", "101", "101"],
  "S": ["111", "100", "111", "001", "111"],
  "T": ["111", "010", "010", "010", "010"],
  "U": ["101", "101", "101", "101", "111"],
  "W": ["101", "101", "111", "111", "101"],
  "Y": ["101", "101", "010", "010", "010"],
};

function clampUnit(value) {
  if (value < 0) {
    return 0;
  }
  if (value > 1) {
    return 1;
  }
  return value;
}

function fract(value) {
  return value - Math.floor(value);
}

function smoothstep(edge0, edge1, value) {
  if (edge0 === edge1) {
    return value < edge0 ? 0 : 1;
  }
  const x = clampUnit((value - edge0) / (edge1 - edge0));
  return x * x * (3 - 2 * x);
}

function clampByte(value) {
  if (value < 0) {
    return 0;
  }
  if (value > 255) {
    return 255;
  }
  return Math.round(value);
}

function normalizeHexColor(value) {
  if (typeof value !== "string") {
    return "";
  }
  const body = value.trim().replace(/^#/, "");
  if (!/^[0-9a-fA-F]{6}$/.test(body)) {
    return "";
  }
  return `#${body.toLowerCase()}`;
}

function hexToRgbColor(value) {
  const normalized = normalizeHexColor(value);
  if (normalized.length === 0) {
    return { r: 100, g: 200, b: 255 };
  }
  return {
    r: parseInt(normalized.slice(1, 3), 16),
    g: parseInt(normalized.slice(3, 5), 16),
    b: parseInt(normalized.slice(5, 7), 16),
  };
}

function setPixel(frameMap, x, y, r, g, b) {
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
    return;
  }
  frameMap.set(
    `${x},${y}`,
    `rgb(${clampByte(r)}, ${clampByte(g)}, ${clampByte(b)})`,
  );
}

function fillRect(frameMap, x, y, width, height, rgb) {
  for (let row = 0; row < height; row += 1) {
    for (let col = 0; col < width; col += 1) {
      setPixel(frameMap, x + col, y + row, rgb.r, rgb.g, rgb.b);
    }
  }
}

function drawGlow(frameMap, centerX, centerY, rgb, alpha, radius = 1) {
  for (let offsetY = -radius; offsetY <= radius; offsetY += 1) {
    for (let offsetX = -radius; offsetX <= radius; offsetX += 1) {
      const distance = Math.abs(offsetX) + Math.abs(offsetY);
      const fade = clampUnit(1 - distance / (radius + 1));
      if (fade <= 0) {
        continue;
      }
      const amount = alpha * fade;
      setPixel(
        frameMap,
        centerX + offsetX,
        centerY + offsetY,
        rgb.r * amount,
        rgb.g * amount,
        rgb.b * amount,
      );
    }
  }
}

function addHighlight(rgb, amount) {
  const safe = clampUnit(amount);
  return {
    r: clampByte(rgb.r + (255 - rgb.r) * safe),
    g: clampByte(rgb.g + (255 - rgb.g) * safe),
    b: clampByte(rgb.b + (255 - rgb.b) * safe),
  };
}

function mixRgb(left, right, alpha) {
  const safe = clampUnit(alpha);
  return {
    r: clampByte(left.r * (1 - safe) + right.r * safe),
    g: clampByte(left.g * (1 - safe) + right.g * safe),
    b: clampByte(left.b * (1 - safe) + right.b * safe),
  };
}

function scaleRgb(rgb, scale) {
  const safe = clampUnit(scale);
  return {
    r: clampByte(rgb.r * safe),
    g: clampByte(rgb.g * safe),
    b: clampByte(rgb.b * safe),
  };
}

function columnSeed(index) {
  return fract(Math.sin((index + 1) * 12.9898) * 43758.5453);
}

function glowPulse(timeValue, offset) {
  return 0.5 + 0.5 * Math.sin(timeValue + offset);
}

function sparkSeed(index, shift) {
  return fract(Math.sin((index + 1) * 17.371 + shift * 3.147) * 24634.6345);
}

function drawGlyphText(frameMap, text, x, y, scale, rgb) {
  let cursorX = x;
  for (let index = 0; index < text.length; index += 1) {
    const glyph = DIGIT_GLYPHS[text[index]];
    if (!glyph) {
      cursorX += 4 * scale;
      continue;
    }
    for (let row = 0; row < glyph.length; row += 1) {
      for (let col = 0; col < glyph[row].length; col += 1) {
        if (glyph[row][col] !== "1") {
          continue;
        }
        fillRect(
          frameMap,
          cursorX + col * scale,
          y + row * scale,
          scale,
          scale,
          rgb,
        );
      }
    }
    cursorX += 4 * scale;
  }
}

function measureGlyphTextWidth(text, scale) {
  if (!text || text.length === 0) {
    return 0;
  }
  return text.length * 4 * scale - scale;
}

function drawGlyphTextCentered(frameMap, text, y, scale, rgb) {
  const width = measureGlyphTextWidth(text, scale);
  const x = Math.max(0, Math.floor((64 - width) / 2));
  drawGlyphText(frameMap, text, x, y, scale, rgb);
}

function strokeRect(frameMap, x, y, width, height, rgb) {
  for (let col = 0; col < width; col += 1) {
    setPixel(frameMap, x + col, y, rgb.r, rgb.g, rgb.b);
    setPixel(frameMap, x + col, y + height - 1, rgb.r, rgb.g, rgb.b);
  }
  for (let row = 0; row < height; row += 1) {
    setPixel(frameMap, x, y + row, rgb.r, rgb.g, rgb.b);
    setPixel(frameMap, x + width - 1, y + row, rgb.r, rgb.g, rgb.b);
  }
}

function drawChip(frameMap, x, y, width, rgb) {
  fillRect(frameMap, x, y, width, 2, rgb);
}

function drawPanelFrame(frameMap, x, y, width, height, background, border, accent) {
  fillRect(frameMap, x, y, width, height, background);
  strokeRect(frameMap, x, y, width, height, border);
  fillRect(frameMap, x + 2, y + 2, width - 4, 1, accent);
  fillRect(frameMap, x + 2, y + height - 3, width - 4, 1, accent);
}

const BRAND_LOGO_MAIN_PALETTE = {
  bg: { r: 10, g: 10, b: 14 },
  border: { r: 54, g: 64, b: 84 },
  cells: [
    [
      { r: 249, g: 115, b: 22 },
      { r: 249, g: 115, b: 22 },
      { r: 239, g: 68, b: 68 },
    ],
    [
      { r: 249, g: 115, b: 22 },
      { r: 251, g: 191, b: 36 },
      { r: 251, g: 191, b: 36 },
    ],
    [
      { r: 249, g: 115, b: 22 },
      { r: 59, g: 130, b: 246 },
      { r: 59, g: 130, b: 246 },
    ],
  ],
};

function resolveBouncingLogoScale(value) {
  const parsed = Number(value);
  if (parsed === 1 || parsed === 2 || parsed === 3) {
    return parsed;
  }
  return 2;
}

function measureBrandLogo(scale) {
  const pixelScale = resolveBouncingLogoScale(scale);
  const padding = 0;
  const gap = 1;
  const blockSize = pixelScale;
  const iconSize = padding * 2 + blockSize * 3 + gap * 2;
  return {
    pixelScale,
    padding,
    gap,
    blockSize,
    iconSize,
  };
}

function drawBrandLogo(frameMap, originX, originY, scale, palette) {
  const metrics = measureBrandLogo(scale);
  const cells = palette.cells;
  const startX = originX + metrics.padding;
  const startY = originY + metrics.padding;

  for (let row = 0; row < 3; row += 1) {
    for (let col = 0; col < 3; col += 1) {
      const x = startX + col * (metrics.blockSize + metrics.gap);
      const y = startY + row * (metrics.blockSize + metrics.gap);
      const color = cells[row][col];
      fillRect(frameMap, x, y, metrics.blockSize, metrics.blockSize, color);
    }
  }
}

function drawImpactBurst(frameMap, side, anchorX, anchorY, color) {
  if (side === "left") {
    setPixel(frameMap, anchorX, anchorY, color.r, color.g, color.b);
    setPixel(frameMap, anchorX + 1, anchorY - 1, color.r, color.g, color.b);
    setPixel(frameMap, anchorX + 1, anchorY + 1, color.r, color.g, color.b);
    setPixel(frameMap, anchorX + 2, anchorY, color.r, color.g, color.b);
    return;
  }
  if (side === "right") {
    setPixel(frameMap, anchorX, anchorY, color.r, color.g, color.b);
    setPixel(frameMap, anchorX - 1, anchorY - 1, color.r, color.g, color.b);
    setPixel(frameMap, anchorX - 1, anchorY + 1, color.r, color.g, color.b);
    setPixel(frameMap, anchorX - 2, anchorY, color.r, color.g, color.b);
    return;
  }
  if (side === "top") {
    setPixel(frameMap, anchorX, anchorY, color.r, color.g, color.b);
    setPixel(frameMap, anchorX - 1, anchorY + 1, color.r, color.g, color.b);
    setPixel(frameMap, anchorX + 1, anchorY + 1, color.r, color.g, color.b);
    setPixel(frameMap, anchorX, anchorY + 2, color.r, color.g, color.b);
    return;
  }
  if (side === "bottom") {
    setPixel(frameMap, anchorX, anchorY, color.r, color.g, color.b);
    setPixel(frameMap, anchorX - 1, anchorY - 1, color.r, color.g, color.b);
    setPixel(frameMap, anchorX + 1, anchorY - 1, color.r, color.g, color.b);
    setPixel(frameMap, anchorX, anchorY - 2, color.r, color.g, color.b);
  }
}

function createEmptyGrid(size, seedOffset) {
  const grid = [];
  for (let y = 0; y < size; y += 1) {
    const row = [];
    for (let x = 0; x < size; x += 1) {
      const seed = sparkSeed(y * size + x, seedOffset);
      row.push(seed > 0.58 ? 1 : 0);
    }
    grid.push(row);
  }
  return grid;
}

function evolveLifeGrid(grid) {
  const size = grid.length;
  const next = [];
  for (let y = 0; y < size; y += 1) {
    const row = [];
    for (let x = 0; x < size; x += 1) {
      let neighbors = 0;
      for (let offsetY = -1; offsetY <= 1; offsetY += 1) {
        for (let offsetX = -1; offsetX <= 1; offsetX += 1) {
          if (offsetX === 0 && offsetY === 0) {
            continue;
          }
          const px = (x + offsetX + size) % size;
          const py = (y + offsetY + size) % size;
          neighbors += grid[py][px];
        }
      }
      const alive = grid[y][x] === 1;
      row.push(alive ? (neighbors === 2 || neighbors === 3 ? 1 : 0) : neighbors === 3 ? 1 : 0);
    }
    next.push(row);
  }
  return next;
}

function renderDigitalRain(x, y, timeBase, speedUnit, intensityUnit, elapsed) {
  const seed = columnSeed(x);
  const companionSeed = columnSeed(x + 19);
  const travel = fract(timeBase * (0.34 + speedUnit * 0.32) + seed);
  const headY = travel * 86 - 11;
  const headYSecondary =
    fract(timeBase * (0.22 + speedUnit * 0.18) + companionSeed * 0.83) * 80 -
    8;
  const tailLength = 10 + intensityUnit * 26;
  const tailDistance = headY - y;
  const secondaryDistance = headYSecondary - y;
  let tail = 0;
  let tailSecondary = 0;

  if (tailDistance >= 0 && tailDistance <= tailLength) {
    tail = 1 - tailDistance / Math.max(tailLength, 1);
  }
  if (secondaryDistance >= 0 && secondaryDistance <= tailLength * 0.56) {
    tailSecondary = 1 - secondaryDistance / Math.max(tailLength * 0.56, 1);
  }

  const headGlow = smoothstep(4.5, 0, Math.abs(headY - y));
  const secondaryHeadGlow = smoothstep(3.2, 0, Math.abs(headYSecondary - y));
  const glyph =
    (y + Math.floor(seed * 11) + Math.floor(elapsed / 65)) % 6 === 0 ? 1 : 0;
  const ambient = 0.03 + glowPulse(timeBase * 0.9, seed * 6) * 0.04;
  const brightness = clampUnit(
    ambient +
      tail * (0.52 + intensityUnit * 0.28) +
      tailSecondary * 0.28 +
      headGlow * 0.55 +
      secondaryHeadGlow * 0.26 +
      glyph * tail * 0.18,
  );

  let rgb = {
    r: 8 + 38 * headGlow + 16 * secondaryHeadGlow,
    g: 26 + 220 * brightness,
    b: 8 + 92 * (tail * 0.55 + headGlow * 0.35 + tailSecondary * 0.22),
  };

  if ((x + Math.floor(elapsed / 42)) % 9 === 0 && tail > 0.16) {
    rgb = addHighlight(rgb, 0.1 + intensityUnit * 0.12);
  }

  return rgb;
}

function renderFallingSand(nx, y, x, timeBase, speedUnit, intensityUnit) {
  const depth = y / 63;
  const duneBase =
    0.74 +
    Math.sin(nx * 5.4 + timeBase * 0.22) * 0.045 +
    Math.sin(nx * 10.8 - timeBase * 0.14) * 0.018;
  const duneGlow = clampUnit((depth - duneBase) / 0.18);
  const driftA = fract(timeBase * (0.62 + speedUnit * 0.68) + x * 0.071);
  const driftB = fract(timeBase * (0.48 + speedUnit * 0.44) + x * 0.127 + 0.31);
  const grainA =
    smoothstep(0.018, 0, Math.abs(depth - driftA * 0.76)) *
    smoothstep(0.08, 0.01, Math.abs(fract(x * 0.19) - 0.5));
  const grainB =
    smoothstep(0.016, 0, Math.abs(depth - driftB * 0.7)) *
    smoothstep(0.12, 0.02, Math.abs(fract(x * 0.11 + 0.4) - 0.5));
  const sparkle = clampUnit((grainA + grainB) * (0.8 + intensityUnit * 0.4));
  const haze = clampUnit(0.04 + duneGlow * 0.36 + sparkle * 0.32);
  let rgb = {
    r: (18 + 170 * duneGlow + 120 * sparkle) * haze,
    g: (10 + 104 * duneGlow + 76 * sparkle) * haze,
    b: (4 + 34 * duneGlow + 16 * sparkle) * haze,
  };

  if (sparkle > 0.48) {
    rgb = addHighlight(rgb, 0.08 + intensityUnit * 0.12);
  }

  return rgb;
}

function renderSparks(nx, ny, dist, timeBase, speedUnit, intensityUnit) {
  let rgb = {
    r: 2,
    g: 1,
    b: 1,
  };
  const launchX = 0;
  const launchY = 0.55;
  const ember = clampUnit(1 - dist * 1.1) * 0.08;
  rgb.r += 24 * ember;
  rgb.g += 12 * ember;
  rgb.b += 6 * ember;

  for (let index = 0; index < 8; index += 1) {
    const seedA = sparkSeed(index, 0.4);
    const seedB = sparkSeed(index, 2.1);
    const angle = -1.9 + seedA * 0.95;
    const progress = fract(timeBase * (0.34 + speedUnit * 0.64) + seedB);
    const radius = progress * (0.65 + seedA * 0.35);
    const sx = launchX + Math.cos(angle) * radius;
    const sy = launchY + Math.sin(angle) * radius * 1.2;
    const dx = nx - sx;
    const dy = ny - sy;
    const tip = clampUnit(1 - Math.sqrt(dx * dx + dy * dy) / 0.085);
    const tail = clampUnit(
      1 - Math.sqrt((nx - sx * 0.78) ** 2 + (ny - (sy * 0.78 + 0.12)) ** 2) / 0.15,
    );
    rgb.r += 180 * tip + 96 * tail;
    rgb.g += 110 * tip + 54 * tail;
    rgb.b += 26 * tip + 10 * tail;
  }

  if (ny > 0.55) {
    const floorGlow = clampUnit((ny - 0.55) / 0.45) * 0.12;
    rgb.r += 38 * floorGlow;
    rgb.g += 12 * floorGlow;
  }

  if (intensityUnit > 0.4 && Math.abs(nx) < 0.04 && ny > 0.18) {
    rgb.r += 30;
    rgb.g += 10;
  }

  return rgb;
}

function renderWavePattern(nx, ny, timeBase, intensityUnit) {
  const waveA = 0.5 + 0.5 * Math.sin(nx * 7.2 - timeBase * 1.6 + Math.sin(ny * 4.6));
  const waveB = 0.5 + 0.5 * Math.sin(ny * 11.4 + timeBase * 1.1 + nx * 3.6);
  const stripe = clampUnit(1 - Math.abs(fract(waveA * 1.8 + waveB * 1.2) - 0.5) * 5.8);
  const glow = clampUnit(0.06 + stripe * (0.58 + intensityUnit * 0.14) + waveB * 0.16);

  return {
    r: (8 + 44 * waveB + 16 * stripe) * glow,
    g: (18 + 110 * stripe + 40 * waveA) * glow,
    b: (40 + 170 * waveA + 72 * stripe) * glow,
  };
}

function renderRainScene(nx, ny, x, y, timeBase, speedUnit, intensityUnit) {
  const cloud = 0.5 + 0.5 * Math.sin(nx * 4.6 + timeBase * 0.24 + Math.sin(ny * 6.2));
  let rgb = {
    r: 3 + 8 * cloud,
    g: 8 + 12 * cloud,
    b: 14 + 26 * cloud,
  };

  const band = x % 5;
  const trail = fract(timeBase * (0.72 + speedUnit * 0.66) + x * 0.041);
  const headY = trail * 1.22 - 0.12;
  const distance = headY - y / 63;
  if (band < 2 && distance >= 0 && distance <= 0.24 + intensityUnit * 0.2) {
    const drop = 1 - distance / (0.24 + intensityUnit * 0.2);
    rgb.r += 34 * drop;
    rgb.g += 88 * drop;
    rgb.b += 150 * drop;
  }

  if (y > 47) {
    const puddle = 0.5 + 0.5 * Math.sin(nx * 10.2 - timeBase * 1.4);
    const floor = clampUnit((y - 47) / 16);
    rgb.r += 6 * puddle * floor;
    rgb.g += 20 * puddle * floor;
    rgb.b += 52 * puddle * floor;
  }

  return rgb;
}

function renderWatermelonPlasma(nx, ny, dist, timeBase, intensityUnit) {
  const plasmaA = 0.5 + 0.5 * Math.sin(nx * 5.8 + timeBase * 1.2 + Math.sin(ny * 4.2));
  const plasmaB = 0.5 + 0.5 * Math.sin((nx + ny) * 6.6 - timeBase * 1.9);
  const rind = 0.5 + 0.5 * Math.sin(dist * 10.4 - timeBase * 1.5);
  const field = clampUnit(plasmaA * 0.48 + plasmaB * 0.34 + rind * 0.18);
  const flesh = clampUnit(0.3 + plasmaA * 0.7);
  const glow = clampUnit(0.16 + field * (0.52 + intensityUnit * 0.2));
  let rgb = {
    r: (22 + 215 * flesh) * glow,
    g: (14 + 145 * (1 - flesh) + 42 * rind) * glow,
    b: (12 + 54 * (1 - field)) * glow,
  };

  if ((Math.floor((nx + 1) * 18) + Math.floor((ny + 1) * 18)) % 19 === 0) {
    rgb.r *= 0.62;
    rgb.g *= 0.42;
    rgb.b *= 0.38;
  }

  return addHighlight(rgb, field * 0.08);
}

function buildBouncingLogoStates(scale) {
  const logoScale = resolveBouncingLogoScale(scale);
  const logoMetrics = measureBrandLogo(logoScale);
  const logoSize = logoMetrics.iconSize;
  const minX = 2;
  const minY = 2;
  const maxX = 64 - logoSize - 2;
  const maxY = 64 - logoSize - 2;
  let x = 7;
  let y = 9;
  let vx = 1;
  let vy = 1;
  let hitLeft = false;
  let hitRight = false;
  let hitTop = false;
  let hitBottom = false;
  const states = [];
  const seen = new Set();

  while (true) {
    const key = `${x},${y},${vx},${vy}`;
    if (seen.has(key)) {
      break;
    }
    seen.add(key);
    states.push({
      x,
      y,
      hitLeft,
      hitRight,
      hitTop,
      hitBottom,
    });

    hitLeft = false;
    hitRight = false;
    hitTop = false;
    hitBottom = false;
    x += vx;
    y += vy;

    if (x <= minX) {
      x = minX;
      vx = Math.abs(vx);
      hitLeft = true;
    } else if (x >= maxX) {
      x = maxX;
      vx = -Math.abs(vx);
      hitRight = true;
    }

    if (y <= minY) {
      y = minY;
      vy = Math.abs(vy);
      hitTop = true;
    } else if (y >= maxY) {
      y = maxY;
      vy = -Math.abs(vy);
      hitBottom = true;
    }
  }

  return states;
}

function buildBouncingLogoFrame(frameMap, frameIndex, scale) {
  const logoScale = resolveBouncingLogoScale(scale);
  const logoMetrics = measureBrandLogo(logoScale);
  const logoSize = logoMetrics.iconSize;
  const states = buildBouncingLogoStates(logoScale);
  const current = states[frameIndex % states.length];

  fillRect(frameMap, 0, 0, 64, 64, { r: 5, g: 6, b: 10 });

  if (current.hitLeft) {
    drawImpactBurst(frameMap, "left", 1, current.y + Math.floor(logoSize / 2), {
      r: 249,
      g: 115,
      b: 22,
    });
  }
  if (current.hitRight) {
    drawImpactBurst(frameMap, "right", 62, current.y + Math.floor(logoSize / 2), {
      r: 59,
      g: 130,
      b: 246,
    });
  }
  if (current.hitTop) {
    drawImpactBurst(frameMap, "top", current.x + Math.floor(logoSize / 2), 1, {
      r: 239,
      g: 68,
      b: 68,
    });
  }
  if (current.hitBottom) {
    drawImpactBurst(
      frameMap,
      "bottom",
      current.x + Math.floor(logoSize / 2),
      62,
      { r: 251, g: 191, b: 36 },
    );
  }

  drawBrandLogo(frameMap, current.x, current.y, logoScale, BRAND_LOGO_MAIN_PALETTE);
}

function buildCountdownSceneFrame(frameMap, elapsed, speedUnit) {
  fillRect(frameMap, 0, 0, 64, 64, { r: 8, g: 10, b: 18 });
  const totalSeconds = 5 * 60;
  const spent = Math.floor((elapsed / 1000) * (0.4 + speedUnit * 1.2)) % totalSeconds;
  const remaining = totalSeconds - spent;
  const minutes = Math.floor(remaining / 60);
  const seconds = remaining % 60;
  const text = `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
  const progress = remaining / totalSeconds;
  const warning = progress < 0.25;
  const accent = warning ? { r: 255, g: 108, b: 82 } : { r: 255, g: 212, b: 82 };
  const border = warning ? { r: 124, g: 48, b: 44 } : { r: 108, g: 84, b: 40 };
  drawPanelFrame(frameMap, 5, 13, 54, 38, { r: 10, g: 16, b: 28 }, border, scaleRgb(accent, 0.55));
  drawGlyphText(frameMap, "TIMER", 10, 17, 1, scaleRgb(accent, 0.86));
  drawChip(frameMap, 42, 17, 10, scaleRgb(accent, 0.7));
  drawGlyphTextCentered(frameMap, text, 26, 2, accent);
  strokeRect(frameMap, 10, 43, 44, 5, border);
  fillRect(
    frameMap,
    11,
    44,
    Math.max(2, Math.round(progress * 42)),
    3,
    accent,
  );
  drawGlow(frameMap, 11 + Math.round(progress * 42), 45, accent, 0.28, 1);
}

function buildGameOfLifeFrame(frameMap, frameIndex, intensityUnit) {
  fillRect(frameMap, 0, 0, 64, 64, { r: 4, g: 9, b: 13 });
  let grid = createEmptyGrid(16, 0.42);
  for (let step = 0; step < frameIndex + 2; step += 1) {
    grid = evolveLifeGrid(grid);
  }

  for (let y = 0; y < 16; y += 1) {
    for (let x = 0; x < 16; x += 1) {
      if (grid[y][x] !== 1) {
        continue;
      }
      const color = addHighlight(
        mixRgb({ r: 68, g: 255, b: 144 }, { r: 82, g: 178, b: 255 }, (x + y) / 30),
        intensityUnit * 0.16,
      );
      fillRect(frameMap, x * 4, y * 4, 3, 3, color);
    }
  }
}

function buildJuliaSetFrame(frameMap, elapsed, intensityUnit) {
  const timeBase = elapsed * 0.0012;
  const cx = -0.7 + Math.sin(timeBase * 0.6) * 0.12;
  const cy = 0.27015 + Math.cos(timeBase * 0.8) * 0.08;

  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      let zx = (x - 32) / 22;
      let zy = (y - 32) / 22;
      let iteration = 0;
      while (zx * zx + zy * zy < 4 && iteration < 18) {
        const nextX = zx * zx - zy * zy + cx;
        zy = 2 * zx * zy + cy;
        zx = nextX;
        iteration += 1;
      }
      const ratio = iteration / 18;
      const color = addHighlight(
        mixRgb({ r: 34, g: 18, b: 92 }, { r: 82, g: 212, b: 255 }, ratio),
        intensityUnit * 0.12,
      );
      setPixel(frameMap, x, y, color.r * ratio, color.g * ratio, color.b * ratio);
    }
  }
}

function buildReactionDiffusionFrame(frameMap, elapsed, speedUnit, intensityUnit) {
  const timeBase = elapsed * (0.0011 + speedUnit * 0.0011);
  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      const nx = (x - 31.5) / 31.5;
      const ny = (y - 31.5) / 31.5;
      const fieldA = 0.5 + 0.5 * Math.sin(nx * 8.2 + timeBase * 1.6 + Math.sin(ny * 6.4));
      const fieldB = 0.5 + 0.5 * Math.cos(ny * 8.8 - timeBase * 1.2 + Math.sin(nx * 5.6));
      const contour = smoothstep(0.46, 0.62 + intensityUnit * 0.08, fieldA * 0.58 + fieldB * 0.42);
      const color = mixRgb(
        { r: 22, g: 38, b: 88 },
        { r: 96, g: 255, b: 204 },
        contour,
      );
      setPixel(frameMap, x, y, color.r * contour, color.g * contour, color.b * contour);
    }
  }
}

function buildSpecialSceneFrame(
  preset,
  frameMap,
  frameIndex,
  elapsed,
  speedUnit,
  intensityUnit,
  specialConfig,
) {
  if (preset === "starfield") {
    buildStarfieldAmbientFrame(frameMap, elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset === "digital_rain") {
    const frames = buildDigitalRainAmbientFrames(frameIndex + 1, speedUnit);
    const target = frames[frameIndex];
    if (target instanceof Map) {
      target.forEach((value, key) => {
        frameMap.set(key, value);
      });
    }
    return true;
  }
  if (preset === "metablob") {
    buildMetaBlobAmbientFrame(frameMap, elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset === "neon_tunnel") {
    buildNeonTunnelAmbientFrame(frameMap, elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset === "rain_scene") {
    buildRainAmbientFrame(
      frameMap,
      elapsed,
      speedUnit,
      specialConfig.rainDensity,
      specialConfig.rainColor,
    );
    return true;
  }
  if (preset === "boids") {
    buildBoidsAmbientFrame(frameMap, elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset === "bouncing_logo") {
    buildBouncingLogoFrame(
      frameMap,
      frameIndex,
      specialConfig.bouncingLogoScale,
    );
    return true;
  }
  if (preset === "sorting_visualizer") {
    buildSortingVisualizerAmbientFrame(frameMap, frameIndex, intensityUnit);
    return true;
  }
  if (preset === "clock_scene") {
    buildClockSceneAmbientFrame(frameMap, elapsed, intensityUnit);
    return true;
  }
  if (preset === "countdown_scene") {
    buildCountdownSceneFrame(frameMap, elapsed, speedUnit);
    return true;
  }
  if (preset === "game_of_life") {
    buildGameOfLifeFrame(frameMap, frameIndex, intensityUnit);
    return true;
  }
  if (preset === "julia_set") {
    buildJuliaSetFrame(frameMap, elapsed, intensityUnit);
    return true;
  }
  if (preset === "reaction_diffusion") {
    buildReactionDiffusionFrame(frameMap, elapsed, speedUnit, intensityUnit);
    return true;
  }
  return false;
}

function renderPresetPixel(preset, x, y, timeBase, speedUnit, intensityUnit, elapsed) {
  const nx = (x - 31.5) / 31.5;
  const ny = (y - 31.5) / 31.5;
  const dist = Math.sqrt(nx * nx + ny * ny);

  if (preset === "digital_rain") {
    return renderDigitalRain(x, y, timeBase, speedUnit, intensityUnit, elapsed);
  }
  if (preset === "falling_sand") {
    return renderFallingSand(nx, y, x, timeBase, speedUnit, intensityUnit);
  }
  if (preset === "sparks") {
    return renderSparks(nx, ny, dist, timeBase, speedUnit, intensityUnit);
  }
  if (preset === "wave_pattern") {
    return renderWavePattern(nx, ny, timeBase, intensityUnit);
  }
  return renderWatermelonPlasma(nx, ny, dist, timeBase, intensityUnit);
}

function resolveAmbientPreviewFramePlan(preset, loop, bouncingLogoScale) {
  if (preset === "bouncing_logo") {
    const bouncingStates = buildBouncingLogoStates(bouncingLogoScale);
    return {
      frameCount: loop === false ? Math.min(36, bouncingStates.length) : bouncingStates.length,
      elapsedStep: 120,
    };
  }

  if (loop === false) {
    return {
      frameCount: 24,
      elapsedStep: 120,
    };
  }

  const plans = {
    starfield: { frameCount: 96, elapsedStep: 90 },
    digital_rain: { frameCount: 108, elapsedStep: 80 },
    metablob: { frameCount: 96, elapsedStep: 100 },
    neon_tunnel: { frameCount: 96, elapsedStep: 90 },
    rain_scene: { frameCount: 108, elapsedStep: 85 },
    boids: { frameCount: 120, elapsedStep: 85 },
    clock_scene: { frameCount: 120, elapsedStep: 100 },
    countdown_scene: { frameCount: 90, elapsedStep: 100 },
    falling_sand: { frameCount: 108, elapsedStep: 85 },
    sparks: { frameCount: 96, elapsedStep: 80 },
    game_of_life: { frameCount: 84, elapsedStep: 95 },
    julia_set: { frameCount: 84, elapsedStep: 95 },
    reaction_diffusion: { frameCount: 96, elapsedStep: 90 },
    wave_pattern: { frameCount: 84, elapsedStep: 95 },
    watermelon_plasma: { frameCount: 84, elapsedStep: 95 },
  };

  return plans[preset] || { frameCount: 72, elapsedStep: 90 };
}

export function buildAmbientPreviewFrames(config) {
  const safeConfig = config || {};
  const requestedPreset =
    typeof safeConfig.preset === "string" ? safeConfig.preset : "digital_rain";
  const preset = LOCAL_ONLY_AMBIENT_PRESETS.has(requestedPreset)
    ? requestedPreset
    : "digital_rain";
  const parsedSpeed = Number(safeConfig.speed);
  const parsedIntensity = Number(safeConfig.intensity);
  const parsedDensity = Number(safeConfig.density);
  const speed = Number.isFinite(parsedSpeed) ? Math.max(1, Math.min(10, Math.round(parsedSpeed))) : 6;
  const intensity = Number.isFinite(parsedIntensity) ? Math.max(10, Math.min(100, Math.round(parsedIntensity))) : 72;
  const density = Number.isFinite(parsedDensity) ? Math.max(10, Math.min(100, Math.round(parsedDensity))) : 72;
  const rainColor = hexToRgbColor(safeConfig.color);
  const bouncingLogoScale = resolveBouncingLogoScale(safeConfig.bouncingLogoScale);
  const framePlan = resolveAmbientPreviewFramePlan(
    preset,
    safeConfig.loop === false ? false : true,
    bouncingLogoScale,
  );
  const frameCount = framePlan.frameCount;
  const frames = [];

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const frameMap = new Map();
    const elapsed = frameIndex * framePlan.elapsedStep;
    const speedUnit = speed / 10;
    const intensityUnit = intensity / 100;
    const timeBase = elapsed * (0.00045 + speedUnit * 0.00135);

    if (
      buildSpecialSceneFrame(
        preset,
        frameMap,
        frameIndex,
        elapsed,
        speedUnit,
        intensityUnit,
        {
          bouncingLogoScale,
          rainDensity: density,
          rainColor,
        },
      )
    ) {
      frames.push(frameMap);
      continue;
    }

    for (let y = 0; y < 64; y += 1) {
      for (let x = 0; x < 64; x += 1) {
        const rgb = renderPresetPixel(preset, x, y, timeBase, speedUnit, intensityUnit, elapsed);
        setPixel(frameMap, x, y, rgb.r, rgb.g, rgb.b);
      }
    }

    frames.push(frameMap);
  }

  return frames;
}
