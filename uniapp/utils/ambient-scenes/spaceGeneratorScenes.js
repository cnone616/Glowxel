import { blendPixel, fillRect, setPixel } from "../led-matrix-demos/common.js";

const BAYER_MATRIX_4X4 = [
  0, 8, 2, 10,
  12, 4, 14, 6,
  3, 11, 1, 9,
  15, 7, 13, 5,
];

function clampUnit(value) {
  if (value < 0) {
    return 0;
  }
  if (value > 1) {
    return 1;
  }
  return value;
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

function fract(value) {
  return value - Math.floor(value);
}

function hexToRgb(hex) {
  const body = String(hex || "").replace(/^#/, "");
  if (!/^[0-9a-fA-F]{6}$/.test(body)) {
    return { r: 255, g: 255, b: 255 };
  }
  return {
    r: parseInt(body.slice(0, 2), 16),
    g: parseInt(body.slice(2, 4), 16),
    b: parseInt(body.slice(4, 6), 16),
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
  const safe = Math.max(0, scale);
  return {
    r: clampByte(rgb.r * safe),
    g: clampByte(rgb.g * safe),
    b: clampByte(rgb.b * safe),
  };
}

function addHighlight(rgb, amount) {
  const safe = clampUnit(amount);
  return {
    r: clampByte(rgb.r + (255 - rgb.r) * safe),
    g: clampByte(rgb.g + (255 - rgb.g) * safe),
    b: clampByte(rgb.b + (255 - rgb.b) * safe),
  };
}

function hash(x, y, seed) {
  return fract(Math.sin((x + 1) * 12.9898 + (y + 1) * 78.233 + seed * 37.719) * 43758.5453);
}

function bayerThreshold(x, y) {
  return BAYER_MATRIX_4X4[(x & 3) + ((y & 3) << 2)] / 16;
}

function sampleLayerField(x, y, elapsed, speedUnit, layer) {
  const nx = x / 63;
  const scroll =
    elapsed * 0.000011 * layer.speed * (0.45 + speedUnit * 0.85);
  const ny = fract(y / 63 + scroll);
  const densityScale = 2.6 + layer.density * 120;
  const warpX =
    nx +
    Math.sin((ny + layer.seed * 0.07) * (6.4 + densityScale * 0.32) + scroll * 3.1) * 0.12 +
    Math.cos((nx + layer.seed * 0.11) * (3.8 + densityScale * 0.18) - scroll * 2.4) * 0.05;
  const warpY =
    ny +
    Math.cos((nx + layer.seed * 0.13) * (5.8 + densityScale * 0.28) - scroll * 2.2) * 0.11 +
    Math.sin((ny + layer.seed * 0.19) * (4.2 + densityScale * 0.14) + scroll * 1.6) * 0.04;
  const fieldA =
    0.5 +
    0.5 *
      Math.sin(
        warpX * (8.4 + densityScale) +
          layer.seed * 0.91 +
          Math.sin(warpY * (5.8 + densityScale * 0.46) - scroll * 4.1),
      );
  const fieldB =
    0.5 +
    0.5 *
      Math.cos(
        warpY * (9.1 + densityScale * 0.82) -
          layer.seed * 0.63 +
          Math.sin(warpX * (4.9 + densityScale * 0.28) + scroll * 3.4),
      );
  const fieldC =
    0.5 +
    0.5 *
      Math.sin(
        (warpX + warpY) * (6.7 + densityScale * 0.58) +
          layer.seed * 1.37 -
          scroll * 2.1,
      );
  const grain = hash(x, y, layer.seed * 1.71);
  return clampUnit(fieldA * 0.42 + fieldB * 0.34 + fieldC * 0.18 + grain * 0.06);
}

function resolvePaletteColor(palette, value, ditherEnabled, x, y) {
  if (value <= 0 || palette.length === 0) {
    return null;
  }
  const scaled = clampUnit(value) * (palette.length - 1);
  const lowIndex = Math.floor(scaled);
  const highIndex = Math.min(palette.length - 1, lowIndex + 1);
  const mix = scaled - lowIndex;
  const index = ditherEnabled && mix > bayerThreshold(x, y) ? highIndex : lowIndex;
  return palette[index];
}

function blendNebulaLayer(frameMap, elapsed, speedUnit, intensityUnit, layer) {
  const thresholdShift = layer.oscillate
    ? Math.sin(elapsed * 0.001 * layer.oscillationRate + layer.oscillationOffset * Math.PI * 2) *
      layer.oscillationIntensity
    : 0;
  const threshold = Math.max(0, Math.min(0.92, layer.threshold + thresholdShift));
  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      const field = sampleLayerField(x, y, elapsed, speedUnit, layer);
      const value = (field - threshold) / Math.max(0.001, 1 - threshold);
      const paletteColor = resolvePaletteColor(layer.palette, value, layer.ditherEnabled, x, y);
      if (!paletteColor) {
        continue;
      }
      let finalColor = paletteColor;
      let alpha = layer.alpha * (0.58 + intensityUnit * 0.48);
      if (layer.modulationEnabled) {
        const modulationField = sampleLayerField(
          x,
          y,
          elapsed,
          speedUnit,
          {
            ...layer,
            density: layer.modulationDensity,
            seed: layer.modulationSeed,
            speed: layer.speed * 0.62 + 2.4,
          },
        );
        const quantizedLum =
          Math.floor(Math.min(modulationField, 0.999) * layer.modulationSteps) / layer.modulationSteps;
        const modulatedColor = scaleRgb(layer.modulationColor, quantizedLum);
        finalColor = mixRgb(finalColor, modulatedColor, layer.modulationIntensity);
        alpha *= 1 - (1 - quantizedLum) * layer.modulationAlphaIntensity;
      }
      blendPixel(frameMap, x, y, finalColor.r, finalColor.g, finalColor.b, clampUnit(alpha));
    }
  }
}

function drawStarShape(frameMap, centerX, centerY, sizeTier, color, alpha) {
  if (sizeTier === 0) {
    blendPixel(frameMap, centerX, centerY, color.r, color.g, color.b, alpha);
    return;
  }

  const core = addHighlight(color, 0.16);
  blendPixel(frameMap, centerX, centerY, core.r, core.g, core.b, alpha);
  blendPixel(frameMap, centerX - 1, centerY, color.r, color.g, color.b, alpha * 0.56);
  blendPixel(frameMap, centerX + 1, centerY, color.r, color.g, color.b, alpha * 0.56);
  blendPixel(frameMap, centerX, centerY - 1, color.r, color.g, color.b, alpha * 0.56);
  blendPixel(frameMap, centerX, centerY + 1, color.r, color.g, color.b, alpha * 0.56);

  if (sizeTier === 2) {
    blendPixel(frameMap, centerX - 2, centerY, color.r, color.g, color.b, alpha * 0.24);
    blendPixel(frameMap, centerX + 2, centerY, color.r, color.g, color.b, alpha * 0.24);
    blendPixel(frameMap, centerX, centerY - 2, color.r, color.g, color.b, alpha * 0.24);
    blendPixel(frameMap, centerX, centerY + 2, color.r, color.g, color.b, alpha * 0.24);
  }
}

function paintStarLayer(frameMap, elapsed, speedUnit, intensityUnit, layer) {
  const densityRatio = (64 * 64) / (360 * 240);
  const starCount = Math.max(8, Math.round(layer.sourceCount * densityRatio));
  const travel = elapsed * 0.000011 * layer.speed * (0.45 + speedUnit * 0.85);
  for (let index = 0; index < starCount; index += 1) {
    const seedOffset = layer.seedOffset + index * 0.73;
    const baseX = hash(index, 2, seedOffset);
    const baseY = hash(index, 7, seedOffset);
    const sizeSeed = hash(index, 13, seedOffset);
    const twinkleSeed = hash(index, 29, seedOffset);
    const swaySeed = hash(index, 37, seedOffset);
    const sizeTier = sizeSeed > 0.86 ? 2 : sizeSeed > 0.62 ? 1 : 0;
    const x =
      Math.round(
        baseX * 63 +
          Math.sin(elapsed * 0.00062 * (0.4 + swaySeed * 0.7) + seedOffset) * (sizeTier === 2 ? 1.2 : 0.7),
      );
    const y = Math.round(fract(baseY + travel + sizeSeed * 0.12) * 63);
    const twinkle =
      0.66 +
      0.34 *
        Math.sin(
          elapsed * 0.0012 * layer.flickerRate * (0.78 + twinkleSeed * 0.44) +
            seedOffset * 4.1,
        );
    const brightness = clampUnit((0.55 + intensityUnit * 0.35) * twinkle);
    const color = sizeTier === 2 ? addHighlight(layer.tint, 0.22) : layer.tint;
    drawStarShape(frameMap, x, y, sizeTier, color, brightness * (1 - layer.flickerDepth * 0.45));
    if (sizeTier >= 1) {
      const glow = scaleRgb(color, 0.32 + intensityUnit * 0.12);
      blendPixel(frameMap, x - 1, y - 1, glow.r, glow.g, glow.b, brightness * 0.16);
      blendPixel(frameMap, x + 1, y - 1, glow.r, glow.g, glow.b, brightness * 0.16);
      blendPixel(frameMap, x - 1, y + 1, glow.r, glow.g, glow.b, brightness * 0.16);
      blendPixel(frameMap, x + 1, y + 1, glow.r, glow.g, glow.b, brightness * 0.16);
    }
  }
}

const COSMIC_KALE_SCENE = {
  layers: [
    {
      type: "nebula",
      palette: [
        "#040c06", "#112318", "#1e3a29", "#305d42",
        "#4d8061", "#89a257", "#bedc7f", "#eeffcc",
      ].map(hexToRgb),
      threshold: 0.36,
      density: 0.01,
      alpha: 0.63,
      ditherEnabled: false,
      modulationEnabled: true,
      modulationColor: hexToRgb("#baa51a"),
      modulationIntensity: 0.5,
      modulationAlphaIntensity: 0.38,
      modulationDensity: 0.00275983119418487,
      modulationSteps: 13,
      oscillate: false,
      oscillationIntensity: 0.24,
      oscillationRate: 0.2,
      oscillationOffset: 0.2,
      speed: 11.5,
      seed: 1.31,
      modulationSeed: 3.67,
    },
    {
      type: "stars",
      sourceCount: 256,
      speed: 6,
      flickerRate: 1.12,
      flickerDepth: 0.16,
      seedOffset: 4.11,
      tint: hexToRgb("#f5ffd5"),
    },
    {
      type: "stars",
      sourceCount: 256,
      speed: 6,
      flickerRate: 1.12,
      flickerDepth: 0.16,
      seedOffset: 7.39,
      tint: hexToRgb("#e5ffd8"),
    },
    {
      type: "nebula",
      palette: [
        "#1e3a29", "#1e3a29", "#1e3a29", "#1e3a29",
        "#4d8061", "#4d8061", "#4d8061", "#bedc7f",
        "#4d8061", "#4d8061", "#4d8061", "#1e3a29",
        "#1e3a29", "#1e3a29", "#1e3a29",
      ].map(hexToRgb),
      threshold: 0,
      density: 0.01,
      alpha: 0.69,
      ditherEnabled: false,
      modulationEnabled: true,
      modulationColor: hexToRgb("#42ff1c"),
      modulationIntensity: 0.73,
      modulationAlphaIntensity: 0.6,
      modulationDensity: 0.0050061825498589,
      modulationSteps: 12,
      oscillate: true,
      oscillationIntensity: 0.0854537729173899,
      oscillationRate: 1.08593751378913,
      oscillationOffset: 0.5,
      speed: 29.2,
      seed: 9.27,
      modulationSeed: 12.81,
    },
  ],
};

const VOID_FIRE_SCENE = {
  layers: [
    {
      type: "stars",
      sourceCount: 360,
      speed: 6,
      flickerRate: 1.12,
      flickerDepth: 0.16,
      seedOffset: 2.17,
      tint: hexToRgb("#fff2d2"),
    },
    {
      type: "nebula",
      palette: [
        "#130208", "#130208", "#130208", "#130208", "#130208",
        "#1f0510", "#1f0510", "#1f0510", "#31051e", "#31051e",
        "#460e2b", "#7c183c", "#d53c6a", "#ff8274",
      ].map(hexToRgb),
      threshold: 0.25,
      density: 0.00532018091088416,
      alpha: 1,
      ditherEnabled: true,
      modulationEnabled: true,
      modulationColor: hexToRgb("#ad1818"),
      modulationIntensity: 0.56,
      modulationAlphaIntensity: 0.22,
      modulationDensity: 0.0052951897902873,
      modulationSteps: 10,
      oscillate: true,
      oscillationIntensity: 0.0261194029850746,
      oscillationRate: 0.8,
      oscillationOffset: 0.5,
      speed: 6,
      seed: 14.37,
      modulationSeed: 15.91,
    },
    {
      type: "nebula",
      palette: [
        "#130208", "#130208", "#130208", "#130208", "#130208",
        "#1f0510", "#1f0510", "#1f0510", "#31051e", "#31051e",
        "#460e2b", "#7c183c", "#d53c6a", "#ff8274",
      ].map(hexToRgb),
      threshold: 0.42,
      density: 0.00548032065799488,
      alpha: 0.89,
      ditherEnabled: false,
      modulationEnabled: true,
      modulationColor: hexToRgb("#ffbb00"),
      modulationIntensity: 1,
      modulationAlphaIntensity: 0.39,
      modulationDensity: 0.00548033786733446,
      modulationSteps: 16,
      oscillate: true,
      oscillationIntensity: 0.0186567164179104,
      oscillationRate: 0.626865671641791,
      oscillationOffset: 0.235,
      speed: 6,
      seed: 17.23,
      modulationSeed: 18.49,
    },
    {
      type: "stars",
      sourceCount: 360,
      speed: 6,
      flickerRate: 1.12,
      flickerDepth: 0.16,
      seedOffset: 6.83,
      tint: hexToRgb("#ffe0bf"),
    },
    {
      type: "nebula",
      palette: ["#130208", "#1f0510", "#31051e", "#460e2b", "#7c183c", "#d53c6a", "#ff8274"].map(hexToRgb),
      threshold: 0,
      density: 0.112781858847192,
      alpha: 0.56,
      ditherEnabled: false,
      modulationEnabled: true,
      modulationColor: hexToRgb("#ffc524"),
      modulationIntensity: 0.26,
      modulationAlphaIntensity: 1,
      modulationDensity: 0.00248612437373037,
      modulationSteps: 16,
      oscillate: false,
      oscillationIntensity: 0.04,
      oscillationRate: 0.8,
      oscillationOffset: 0.5,
      speed: 19.6,
      seed: 20.17,
      modulationSeed: 22.31,
    },
    {
      type: "nebula",
      palette: ["#130208", "#1f0510", "#31051e", "#460e2b", "#7c183c", "#d53c6a", "#ff8274"].map(hexToRgb),
      threshold: 0.03,
      density: 0.0324423671178528,
      alpha: 0.78,
      ditherEnabled: true,
      modulationEnabled: true,
      modulationColor: hexToRgb("#fc0004"),
      modulationIntensity: 0.44,
      modulationAlphaIntensity: 1,
      modulationDensity: 0.00266302079694548,
      modulationSteps: 10,
      oscillate: true,
      oscillationIntensity: 0.04,
      oscillationRate: 0.8,
      oscillationOffset: 0.5,
      speed: 29.6,
      seed: 24.11,
      modulationSeed: 26.73,
    },
  ],
};

const DEEP_SPACE_NEBULA_BODY_PALETTE = [
  "#050912", "#081020", "#0e1933", "#15264a",
  "#213b68", "#30598e", "#4478ad", "#69a2d8",
].map(hexToRgb);

const DEEP_SPACE_NEBULA_CORE_PALETTE = [
  "#0c1730", "#163056", "#214a7a", "#2f6ba3",
  "#4f97ca", "#8ec8ef", "#cdefff",
].map(hexToRgb);

const DEEP_SPACE_NEBULA_STAR_LAYER = {
  type: "stars",
  sourceCount: 54,
  speed: 2.8,
  flickerRate: 0.74,
  flickerDepth: 0.08,
  seedOffset: 1.37,
  tint: hexToRgb("#b7caee"),
};

function smoothstep(edge0, edge1, value) {
  if (edge0 === edge1) {
    return value < edge0 ? 0 : 1;
  }
  const safe = clampUnit((value - edge0) / (edge1 - edge0));
  return safe * safe * (3 - 2 * safe);
}

function ellipseMask(nx, ny, cx, cy, rx, ry) {
  const dx = (nx - cx) / Math.max(rx, 0.001);
  const dy = (ny - cy) / Math.max(ry, 0.001);
  return clampUnit(1 - dx * dx - dy * dy);
}

function resolveGradientPalette(palette, value) {
  if (palette.length === 0) {
    return { r: 0, g: 0, b: 0 };
  }
  const scaled = clampUnit(value) * (palette.length - 1);
  const lowIndex = Math.floor(scaled);
  const highIndex = Math.min(palette.length - 1, lowIndex + 1);
  return mixRgb(palette[lowIndex], palette[highIndex], scaled - lowIndex);
}

function sampleDeepSpaceNoise(nx, ny, timeBase) {
  const warpX =
    nx +
    Math.sin(ny * 7.1 + timeBase * 0.46 + Math.cos(nx * 3.8 - timeBase * 0.18)) * 0.085 +
    Math.cos(nx * 5.2 - timeBase * 0.22) * 0.032;
  const warpY =
    ny +
    Math.cos(nx * 6.4 - timeBase * 0.28 + Math.sin(ny * 4.2 + timeBase * 0.2)) * 0.082 +
    Math.sin(ny * 5.7 + timeBase * 0.16) * 0.028;
  const bandA = 0.5 + 0.5 * Math.sin((warpX * 6.8 + warpY * 3.2) + timeBase * 0.92);
  const bandB = 0.5 + 0.5 * Math.cos((warpY * 7.4 - warpX * 4.3) - timeBase * 0.68);
  const detail = 0.5 + 0.5 * Math.sin((warpX + warpY) * 12.4 - timeBase * 1.34);
  return clampUnit(bandA * 0.48 + bandB * 0.34 + detail * 0.18);
}

function buildDeepSpaceNebulaBase(frameMap, elapsed, speedUnit, intensityUnit) {
  const timeBase = elapsed * (0.00038 + speedUnit * 0.00072);
  const backgroundA = hexToRgb("#02040a");
  const backgroundB = hexToRgb("#050a15");
  const clusterColor = hexToRgb("#d9f4ff");

  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      const nx = x / 63;
      const ny = y / 63;
      const centerX = (x - 31.5) / 31.5;
      const centerY = (y - 31.5) / 31.5;
      const vignette = clampUnit(1 - Math.sqrt(centerX * centerX + centerY * centerY) * 0.58);

      const ridgeCenter =
        0.8 - nx * 0.47 + Math.sin(timeBase * 0.34 + nx * 4.6) * 0.032;
      const ridge = clampUnit(1 - Math.abs(ny - ridgeCenter) / 0.17);
      const cloudA = ellipseMask(nx, ny, 0.3, 0.67, 0.62, 0.34);
      const cloudB = ellipseMask(nx, ny, 0.66, 0.34, 0.34, 0.26);
      const coreMask = ellipseMask(nx, ny, 0.68, 0.3, 0.16, 0.12);
      const structure = clampUnit(cloudA * 0.84 + cloudB * 0.9 + ridge * 0.44);

      const largeNoise = sampleDeepSpaceNoise(nx, ny, timeBase);
      const fineNoise = sampleDeepSpaceNoise(nx + 0.19, ny - 0.13, timeBase * 1.62 + 2.4);
      const bodyField = clampUnit(
        (largeNoise * 0.7 + fineNoise * 0.3) * structure - 0.21 + cloudA * 0.08,
      );
      const coreField = clampUnit(
        (largeNoise * 0.38 + fineNoise * 0.62) * (coreMask * 1.34 + cloudB * 0.12) -
          0.48 +
          coreMask * 0.42,
      );

      const backgroundMix = clampUnit(0.18 + ridge * 0.18 + cloudA * 0.07);
      const baseBackground = mixRgb(backgroundA, backgroundB, backgroundMix);
      const background = scaleRgb(baseBackground, 0.68 + vignette * 0.32);
      setPixel(frameMap, x, y, background.r, background.g, background.b);

      if (bodyField > 0.01) {
        const bodyColor = resolveGradientPalette(DEEP_SPACE_NEBULA_BODY_PALETTE, bodyField);
        const bodyAlpha = clampUnit((0.16 + structure * 0.58) * (0.52 + intensityUnit * 0.34));
        blendPixel(frameMap, x, y, bodyColor.r, bodyColor.g, bodyColor.b, bodyAlpha);
      }

      if (coreField > 0.01) {
        const coreColor = resolveGradientPalette(DEEP_SPACE_NEBULA_CORE_PALETTE, coreField);
        const coreAlpha = clampUnit((0.12 + coreMask * 0.62) * (0.48 + intensityUnit * 0.38));
        blendPixel(frameMap, x, y, coreColor.r, coreColor.g, coreColor.b, coreAlpha);
      }

      const clusterGlow = smoothstep(0.34, 0.02, Math.abs(ny - (0.31 + Math.sin(nx * 8 + timeBase) * 0.012)));
      if (clusterGlow > 0.24 && cloudB > 0.18) {
        const glowAlpha = clampUnit(clusterGlow * cloudB * 0.14);
        blendPixel(frameMap, x, y, clusterColor.r, clusterColor.g, clusterColor.b, glowAlpha);
      }
    }
  }
}

function paintDeepSpaceNebulaCluster(frameMap, elapsed, intensityUnit) {
  const centerX = 44;
  const centerY = 20;
  const tint = hexToRgb("#f5fbff");
  for (let index = 0; index < 16; index += 1) {
    const angle = hash(index, 5, 0.37) * Math.PI * 2;
    const radius = Math.pow(hash(index, 9, 0.91), 1.65) * 10;
    const orbitX = centerX + Math.cos(angle) * radius;
    const orbitY = centerY + Math.sin(angle) * radius * 0.58;
    const twinkle =
      0.64 +
      0.36 * Math.sin(elapsed * 0.0011 * (0.8 + hash(index, 3, 0.61) * 0.4) + index * 1.7);
    const alpha = clampUnit((0.36 + intensityUnit * 0.24) * twinkle);
    const sizeTier = hash(index, 7, 0.44) > 0.82 ? 1 : 0;
    drawStarShape(frameMap, Math.round(orbitX), Math.round(orbitY), sizeTier, tint, alpha);
  }
}

function buildSpaceGeneratorFrame(frameMap, elapsed, speedUnit, intensityUnit, scene) {
  fillRect(frameMap, 0, 0, 64, 64, 0, 0, 0);
  scene.layers.forEach((layer) => {
    if (layer.type === "nebula") {
      blendNebulaLayer(frameMap, elapsed, speedUnit, intensityUnit, layer);
      return;
    }
    paintStarLayer(frameMap, elapsed, speedUnit, intensityUnit, layer);
  });
}

function buildCosmicKaleAmbientFrame(frameMap, elapsed, speedUnit, intensityUnit) {
  buildSpaceGeneratorFrame(frameMap, elapsed, speedUnit, intensityUnit, COSMIC_KALE_SCENE);
}

function buildVoidFireAmbientFrame(frameMap, elapsed, speedUnit, intensityUnit) {
  buildSpaceGeneratorFrame(frameMap, elapsed, speedUnit, intensityUnit, VOID_FIRE_SCENE);
}

function buildDeepSpaceNebulaAmbientFrame(frameMap, elapsed, speedUnit, intensityUnit) {
  fillRect(frameMap, 0, 0, 64, 64, 0, 0, 0);
  buildDeepSpaceNebulaBase(frameMap, elapsed, speedUnit, intensityUnit);
  paintStarLayer(frameMap, elapsed, speedUnit, intensityUnit, DEEP_SPACE_NEBULA_STAR_LAYER);
  paintDeepSpaceNebulaCluster(frameMap, elapsed, intensityUnit);
}

export {
  buildCosmicKaleAmbientFrame,
  buildDeepSpaceNebulaAmbientFrame,
  buildVoidFireAmbientFrame,
};
