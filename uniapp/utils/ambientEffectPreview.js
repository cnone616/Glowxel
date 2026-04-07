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

function setPixel(frameMap, x, y, r, g, b) {
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
    return;
  }
  frameMap.set(`${x},${y}`, `rgb(${clampByte(r)}, ${clampByte(g)}, ${clampByte(b)})`);
}

function addHighlight(rgb, amount) {
  const safe = clampUnit(amount);
  return {
    r: clampByte(rgb.r + (255 - rgb.r) * safe),
    g: clampByte(rgb.g + (255 - rgb.g) * safe),
    b: clampByte(rgb.b + (255 - rgb.b) * safe),
  };
}

function columnSeed(index) {
  return fract(Math.sin((index + 1) * 12.9898) * 43758.5453);
}

function glowPulse(timeValue, offset) {
  return 0.5 + 0.5 * Math.sin(timeValue + offset);
}

function fireflySeed(index, shift) {
  return fract(Math.sin((index + 1) * 17.371 + shift * 3.147) * 24634.6345);
}

function renderPresetPixel(preset, x, y, timeBase, speedUnit, intensityUnit, elapsed) {
  const nx = (x - 31.5) / 31.5;
  const ny = (y - 31.5) / 31.5;
  const dist = Math.sqrt(nx * nx + ny * ny);
  let rgb = { r: 0, g: 0, b: 0 };

  if (preset === "aurora_flow") {
    const curtainA =
      0.5 +
      0.5 *
        Math.sin(nx * 5.2 + timeBase * 1.9 + Math.sin(ny * 3.6 - timeBase * 1.1));
    const curtainB = 0.5 + 0.5 * Math.sin(nx * 7.4 - timeBase * 1.5 + ny * 5.0);
    const horizon = clampUnit(1 - (y / 63) * 1.18);
    const veil = clampUnit(curtainA * 0.55 + curtainB * 0.45);
    const glow = clampUnit(horizon * (0.35 + veil * (0.5 + intensityUnit * 0.35)));
    const accent = clampUnit(0.24 + 0.76 * glowPulse(timeBase * 1.1, nx * 2.5 + ny * 1.7));
    rgb = {
      r: (10 + 70 * veil + 24 * accent) * glow,
      g: (30 + 180 * veil + 28 * accent) * glow,
      b: (50 + 210 * accent + 18 * veil) * glow,
    };
    if (((x + y + Math.floor(elapsed / 180)) % 29) === 0) {
      rgb = addHighlight(rgb, 0.18 + intensityUnit * 0.16);
    }
  } else if (preset === "plasma_wave") {
    const plasmaA = Math.sin(nx * 5.8 + timeBase * 1.4 + Math.sin(ny * 4.0 - timeBase * 0.9));
    const plasmaB = Math.sin((nx + ny) * 6.4 - timeBase * 1.8);
    const plasmaC = Math.sin(dist * 12.0 - timeBase * 3.0);
    const field = clampUnit((plasmaA + plasmaB + plasmaC + 3) / 6);
    const heat = clampUnit(0.5 + 0.5 * Math.sin(field * 8 + timeBase * 2.2));
    const cool = clampUnit(0.5 + 0.5 * Math.cos(field * 7 - timeBase * 1.3));
    const glow = 0.18 + field * (0.56 + intensityUnit * 0.22);
    rgb = {
      r: (40 + 205 * heat) * glow,
      g: (20 + 150 * field + 45 * cool) * glow,
      b: (60 + 185 * cool) * glow,
    };
    rgb = addHighlight(rgb, field * 0.12);
  } else if (preset === "matrix_rain") {
    const seed = columnSeed(x);
    const companionSeed = columnSeed(x + 19);
    const travel = fract(timeBase * (0.34 + speedUnit * 0.32) + seed);
    const headY = travel * (64 + 22) - 11;
    const headYSecondary =
      fract(timeBase * (0.22 + speedUnit * 0.18) + companionSeed * 0.83) * (64 + 16) - 8;
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
    const glyph = ((y + Math.floor(seed * 11) + Math.floor(elapsed / 65)) % 6) === 0 ? 1 : 0;
    const ambient = 0.03 + glowPulse(timeBase * 0.9, seed * 6) * 0.04;
    const brightness = clampUnit(
      ambient +
      tail * (0.52 + intensityUnit * 0.28) +
      tailSecondary * 0.28 +
      headGlow * 0.55 +
      secondaryHeadGlow * 0.26 +
      glyph * tail * 0.18,
    );
    rgb = {
      r: 8 + 38 * headGlow + 16 * secondaryHeadGlow,
      g: 26 + 220 * brightness,
      b: 8 + 92 * (tail * 0.55 + headGlow * 0.35 + tailSecondary * 0.22),
    };
    if (((x + Math.floor(elapsed / 42)) % 9) === 0 && tail > 0.16) {
      rgb = addHighlight(rgb, 0.1 + intensityUnit * 0.12);
    }
  } else if (preset === "firefly_swarm") {
    const dusk = clampUnit(1 - (y / 63) * 0.78);
    const backgroundWave = 0.5 + 0.5 * Math.sin(ny * 6 - timeBase * 0.85);
    rgb = {
      r: 2 + 6 * backgroundWave,
      g: 10 + 18 * dusk,
      b: 16 + 42 * dusk,
    };
    for (let i = 0; i < 9; i += 1) {
      const seedX = fireflySeed(i, 0);
      const seedY = fireflySeed(i, 1.3);
      const orbit = fireflySeed(i, 2.7);
      const fx =
        6 +
        seedX * 52 +
        Math.sin(timeBase * (0.7 + orbit * 0.8) + seedY * Math.PI * 2) * (5 + orbit * 10);
      const fy =
        8 +
        seedY * 44 +
        Math.cos(timeBase * (0.9 + seedX * 0.7) + orbit * 4.1) * (4 + seedX * 9);
      const dx = x - fx;
      const dy = y - fy;
      const glow = clampUnit(1 - Math.sqrt(dx * dx + dy * dy) / (4.5 + intensityUnit * 4));
      if (glow > 0) {
        const twinkle = glowPulse(timeBase * (1.5 + seedX), seedY * 8);
        rgb.r += 50 * glow * twinkle;
        rgb.g += (140 + 70 * intensityUnit) * glow * twinkle;
        rgb.b += 30 * glow * twinkle;
      }
    }
  } else if (preset === "meteor_shower") {
    const diag = fract((x + y) / 96 - timeBase * (0.7 + speedUnit * 0.9));
    const diagB = fract((x * 0.84 + y * 1.18) / 88 - timeBase * (0.48 + speedUnit * 0.66) + 0.37);
    const streak = clampUnit(1 - Math.abs(diag - 0.5) * 7.6);
    const streakB = clampUnit(1 - Math.abs(diagB - 0.5) * 8.4);
    const sparkle = ((x * 13 + y * 7 + Math.floor(elapsed / 60)) % 43) === 0 ? 1 : 0;
    const haze = clampUnit(0.08 + (1 - dist) * 0.14);
    rgb = {
      r: 10 + 80 * streak + 52 * streakB + 45 * sparkle + 20 * haze,
      g: 18 + 120 * streak + 78 * streakB + 60 * sparkle + 35 * haze,
      b: 28 + 180 * streak + 110 * streakB + 90 * sparkle + 60 * haze,
    };
    if (sparkle > 0) {
      rgb = addHighlight(rgb, 0.2 + intensityUnit * 0.15);
    }
  } else if (preset === "ocean_current") {
    const tideA = 0.5 + 0.5 * Math.sin(nx * 4.6 - timeBase * 1.1 + Math.sin(ny * 5 + timeBase * 0.7));
    const tideB = 0.5 + 0.5 * Math.sin((nx - ny) * 6.2 + timeBase * 1.6);
    const ripple = 0.5 + 0.5 * Math.sin(dist * 14 - timeBase * 2.4);
    const depth = clampUnit(0.18 + (y / 63) * 0.52);
    const foam = clampUnit(1 - Math.abs(ripple - 0.82) * 5.5);
    const glow = clampUnit(0.16 + depth * 0.34 + tideA * 0.24 + tideB * 0.2);
    rgb = {
      r: (6 + 28 * tideB + 42 * foam) * glow,
      g: (36 + 110 * tideA + 40 * foam) * glow,
      b: (70 + 150 * tideA + 36 * ripple) * glow,
    };
    rgb = addHighlight(rgb, foam * (0.1 + intensityUnit * 0.12));
  } else if (preset === "neon_grid") {
    const scanX = clampUnit(1 - Math.abs(fract(x / 64 + timeBase * (0.28 + speedUnit * 0.18)) - 0.5) * 5.6);
    const scanY = clampUnit(1 - Math.abs(fract(y / 64 - timeBase * (0.22 + speedUnit * 0.14)) - 0.5) * 5.6);
    const gridLine = x % 8 === 0 || y % 8 === 0 ? 1 : 0;
    const junction = x % 8 === 0 && y % 8 === 0 ? 1 : 0;
    const cellPulse = 0.5 + 0.5 * Math.sin((x + y) * 0.25 + timeBase * 3.2);
    const brightness = clampUnit(
      0.06 +
      gridLine * 0.24 +
      junction * 0.12 +
      scanX * 0.32 +
      scanY * 0.26 +
      cellPulse * 0.14,
    );
    rgb = {
      r: (18 + 120 * scanX + 40 * gridLine + 40 * junction) * brightness,
      g: (10 + 70 * cellPulse + 30 * gridLine + 12 * junction) * brightness,
      b: (30 + 180 * scanY + 100 * gridLine + 60 * junction) * brightness,
    };
    if (gridLine > 0.5) {
      rgb = addHighlight(rgb, 0.08 + intensityUnit * 0.1);
    }
  } else if (preset === "sunset_blush") {
    const sky = 1 - y / 63;
    const warmBand = 0.5 + 0.5 * Math.sin(nx * 4.2 + timeBase * 0.9 + ny * 2);
    const cloud = 0.5 + 0.5 * Math.sin(nx * 8 - timeBase * 0.6 + Math.sin(ny * 6 + timeBase * 0.5));
    const sunGlow = clampUnit(1 - Math.sqrt((nx * 0.92) ** 2 + ((ny + 0.18) * 1.25) ** 2) * 1.55);
    const haze = clampUnit(0.1 + sky * 0.35 + warmBand * 0.22 + cloud * 0.12);
    rgb = {
      r: (40 + 185 * sky + 55 * sunGlow) * haze,
      g: (16 + 88 * warmBand + 76 * sunGlow) * haze,
      b: (20 + 105 * cloud + 70 * (1 - sky)) * haze,
    };
    rgb = addHighlight(rgb, sunGlow * (0.16 + intensityUnit * 0.14));
  } else if (preset === "starfield_drift") {
    const nebulaA = 0.5 + 0.5 * Math.sin(nx * 5 + timeBase * 0.45 + ny * 4);
    const nebulaB = 0.5 + 0.5 * Math.sin((nx - ny) * 7.2 - timeBase * 0.62);
    const backdrop = clampUnit(0.03 + nebulaA * 0.08 + nebulaB * 0.06);
    rgb = {
      r: 4 + 16 * nebulaB * backdrop,
      g: 6 + 22 * nebulaA * backdrop,
      b: 12 + 46 * (nebulaA * 0.55 + nebulaB * 0.45) * backdrop,
    };
    const starIndex = (x * 31 + y * 17) % 97;
    if (starIndex < 3) {
      const twinkle = glowPulse(timeBase * (1.1 + starIndex * 0.35), (x + y) * 0.21);
      const shine = 0.28 + twinkle * (0.42 + intensityUnit * 0.18);
      rgb.r += 140 * shine;
      rgb.g += 150 * shine;
      rgb.b += 185 * shine;
      if (x > 0 && starIndex === 0 && twinkle > 0.72) {
        rgb.r += 32;
        rgb.g += 34;
        rgb.b += 40;
      }
    }
  }

  return {
    r: clampByte(rgb.r),
    g: clampByte(rgb.g),
    b: clampByte(rgb.b),
  };
}

export function buildAmbientPreviewFrames(config) {
  const safeConfig = config || {};
  const preset =
    typeof safeConfig.preset === "string" ? safeConfig.preset : "aurora_flow";
  const parsedSpeed = Number(safeConfig.speed);
  const parsedIntensity = Number(safeConfig.intensity);
  const speed = Number.isFinite(parsedSpeed)
    ? Math.max(1, Math.min(10, Math.round(parsedSpeed)))
    : 6;
  const intensity = Number.isFinite(parsedIntensity)
    ? Math.max(10, Math.min(100, Math.round(parsedIntensity)))
    : 72;
  const frameCount = safeConfig.loop === false ? 10 : 20;
  const frames = [];

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const frameMap = new Map();
    const elapsed = frameIndex * 120;
    const speedUnit = speed / 10;
    const intensityUnit = intensity / 100;
    const timeBase = elapsed * (0.00045 + speedUnit * 0.00135);

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
