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

function angleUnit(nx, ny) {
  return Math.atan2(ny, nx) / (Math.PI * 2);
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

function drawBrandLogo(frameMap, originX, originY, size) {
  const colors = [
    [{ r: 249, g: 115, b: 22 }, { r: 249, g: 115, b: 22 }, { r: 239, g: 68, b: 68 }],
    [{ r: 249, g: 115, b: 22 }, { r: 251, g: 191, b: 36 }, { r: 251, g: 191, b: 36 }],
    [{ r: 249, g: 115, b: 22 }, { r: 59, g: 130, b: 246 }, { r: 59, g: 130, b: 246 }],
  ];

  for (let row = 0; row < 3; row += 1) {
    for (let col = 0; col < 3; col += 1) {
      fillRect(frameMap, originX + col * size, originY + row * size, size - 1, size - 1, colors[row][col]);
    }
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

function renderStarfield(nx, ny, x, y, timeBase, intensityUnit) {
  const nebulaA = 0.5 + 0.5 * Math.sin(nx * 5 + timeBase * 0.45 + ny * 4);
  const nebulaB = 0.5 + 0.5 * Math.sin((nx - ny) * 7.2 - timeBase * 0.62);
  const backdrop = clampUnit(0.03 + nebulaA * 0.08 + nebulaB * 0.06);
  let rgb = {
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
  }

  return rgb;
}

function renderNeonTunnel(nx, ny, dist, timeBase, speedUnit, intensityUnit) {
  const angle = angleUnit(nx, ny);
  const tunnelDepth = fract(dist * 4.4 - timeBase * (1.8 + speedUnit * 1.1));
  const ringGlow = clampUnit(1 - Math.abs(tunnelDepth - 0.5) * 6.5);
  const laneGlow = clampUnit(
    1 - Math.abs(fract(angle * 6 + timeBase * 0.22) - 0.5) * 4.6,
  );
  const rimGlow = clampUnit(1 - Math.abs(dist - 0.88) * 8.5);
  const centerFade = clampUnit(1 - dist * 0.92);
  const pulse = 0.5 + 0.5 * Math.sin(timeBase * 3.2 + angle * 8);
  const glow = clampUnit(
    0.04 +
      ringGlow * (0.46 + intensityUnit * 0.2) +
      laneGlow * 0.24 +
      rimGlow * 0.18 +
      centerFade * 0.08,
  );

  return {
    r: (20 + 180 * laneGlow + 45 * pulse) * glow,
    g: (10 + 70 * pulse + 30 * rimGlow) * glow,
    b: (40 + 210 * ringGlow + 80 * laneGlow) * glow,
  };
}

function renderMetablob(nx, ny, timeBase, intensityUnit) {
  let field = 0;
  for (let index = 0; index < 4; index += 1) {
    const seedA = sparkSeed(index, 0.3);
    const seedB = sparkSeed(index, 1.6);
    const blobX =
      Math.sin(timeBase * (0.72 + seedA * 0.65) + seedB * Math.PI * 2) *
      (0.24 + seedA * 0.38);
    const blobY =
      Math.cos(timeBase * (0.94 + seedB * 0.52) + seedA * Math.PI * 2) *
      (0.22 + seedB * 0.36);
    const dx = nx - blobX;
    const dy = ny - blobY;
    field += (0.065 + intensityUnit * 0.032) / (dx * dx + dy * dy + 0.026);
  }

  const iso = clampUnit((field - 0.82) / (1.9 + intensityUnit * 0.4));
  const edge = smoothstep(1.8, 2.45 + intensityUnit * 0.3, field);
  const shimmer = 0.5 + 0.5 * Math.sin(field * 4.2 - timeBase * 2.1);
  const glow = clampUnit(iso * 0.78 + edge * 0.22);

  return {
    r: (8 + 46 * shimmer + 22 * edge) * glow,
    g: (24 + 150 * iso + 55 * shimmer) * glow,
    b: (42 + 150 * edge + 48 * (1 - shimmer)) * glow,
  };
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

function buildBoidsFrame(frameMap, elapsed, speedUnit, intensityUnit) {
  fillRect(frameMap, 0, 0, 64, 64, { r: 4, g: 10, b: 18 });
  for (let y = 8; y < 64; y += 9) {
    for (let x = 0; x < 64; x += 1) {
      setPixel(frameMap, x, y, 10, 18, 28);
    }
  }

  const timeBase = elapsed * (0.0012 + speedUnit * 0.0016);
  for (let index = 0; index < 11; index += 1) {
    const seedA = sparkSeed(index, 0.8);
    const seedB = sparkSeed(index, 2.4);
    const px =
      32 +
      Math.sin(timeBase * (0.9 + seedA * 0.8) + seedB * Math.PI * 2) * (10 + seedA * 18) +
      Math.cos(timeBase * (0.5 + seedB * 0.7) + index) * 6;
    const py =
      32 +
      Math.cos(timeBase * (0.7 + seedB * 0.9) + seedA * Math.PI * 2) * (8 + seedB * 16) +
      Math.sin(timeBase * (0.8 + seedA * 0.4) + index * 0.6) * 5;
    const tailX = px - Math.cos(timeBase + index) * 4;
    const tailY = py - Math.sin(timeBase * 1.1 + index) * 4;
    const body = addHighlight(
      mixRgb({ r: 44, g: 164, b: 255 }, { r: 80, g: 255, b: 196 }, seedA),
      0.12 + intensityUnit * 0.16,
    );
    drawGlow(frameMap, Math.round(px), Math.round(py), body, 0.9, 1);
    drawGlow(frameMap, Math.round((px + tailX) / 2), Math.round((py + tailY) / 2), scaleRgb(body, 0.45), 0.7, 1);
  }
}

function buildBouncingLogoFrame(frameMap, elapsed, speedUnit) {
  fillRect(frameMap, 0, 0, 64, 64, { r: 8, g: 11, b: 18 });
  const travelX = 8 + Math.abs(((elapsed * (0.02 + speedUnit * 0.02)) % 96) - 48);
  const travelY = 8 + Math.abs(((elapsed * (0.016 + speedUnit * 0.018) + 12) % 84) - 42);
  const x = Math.round(Math.min(64 - 18, travelX));
  const y = Math.round(Math.min(64 - 18, travelY));
  drawBrandLogo(frameMap, x, y, 6);
  drawGlow(frameMap, x + 8, y + 8, { r: 110, g: 180, b: 255 }, 0.32, 4);
}

function buildSortingVisualizerFrame(frameMap, frameIndex, intensityUnit) {
  fillRect(frameMap, 0, 0, 64, 64, { r: 6, g: 9, b: 16 });
  const heights = [13, 9, 5, 15, 7, 12, 3, 10, 16, 8, 14, 6, 11, 4, 2, 1];
  const compareCount = frameIndex * 3;
  let activeLeft = 0;
  let activeRight = 1;

  for (let step = 0; step < compareCount; step += 1) {
    const index = step % (heights.length - 1);
    activeLeft = index;
    activeRight = index + 1;
    if (heights[index] > heights[index + 1]) {
      const temp = heights[index];
      heights[index] = heights[index + 1];
      heights[index + 1] = temp;
    }
  }

  for (let index = 0; index < heights.length; index += 1) {
    const barHeight = heights[index];
    const x = index * 4;
    const base = index === activeLeft || index === activeRight
      ? { r: 255, g: 180, b: 68 }
      : mixRgb({ r: 78, g: 162, b: 255 }, { r: 96, g: 255, b: 188 }, index / 15);
    const color = addHighlight(base, intensityUnit * 0.14);
    for (let row = 0; row < barHeight; row += 1) {
      fillRect(frameMap, x + 1, 63 - row * 3, 3, 2, color);
    }
  }
}

function buildClockSceneFrame(frameMap, elapsed, intensityUnit) {
  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 64; x += 1) {
      const nx = (x - 31.5) / 31.5;
      const ny = (y - 31.5) / 31.5;
      const rgb = renderStarfield(nx, ny, x, y, elapsed * 0.0012, intensityUnit);
      setPixel(frameMap, x, y, rgb.r, rgb.g, rgb.b);
    }
  }

  const now = new Date();
  const text = `${String(now.getHours()).padStart(2, "0")}:${String(
    now.getMinutes(),
  ).padStart(2, "0")}`;
  fillRect(frameMap, 8, 18, 48, 28, { r: 6, g: 12, b: 20 });
  fillRect(frameMap, 8, 47, 48, 1, { r: 24, g: 60, b: 88 });
  drawGlyphText(frameMap, text, 10, 22, 3, { r: 142, g: 214, b: 255 });
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
  fillRect(frameMap, 7, 16, 50, 34, { r: 10, g: 16, b: 28 });
  fillRect(frameMap, 8, 50, Math.max(1, Math.round(progress * 48)), 2, { r: 255, g: 112, b: 64 });
  drawGlyphText(frameMap, text, 10, 23, 3, { r: 255, g: 212, b: 82 });
}

function buildWeatherSceneFrame(frameMap, elapsed, speedUnit) {
  fillRect(frameMap, 0, 0, 64, 64, { r: 6, g: 10, b: 18 });
  const phase = Math.floor((elapsed / 1000) * (0.28 + speedUnit * 0.2)) % 3;
  const temp = phase === 0 ? 26 : phase === 1 ? 18 : 8;
  const text = `${String(temp).padStart(2, "0")}`;
  fillRect(frameMap, 6, 10, 52, 40, { r: 10, g: 16, b: 28 });

  if (phase === 0) {
    drawGlow(frameMap, 18, 24, { r: 255, g: 206, b: 84 }, 0.95, 4);
  } else if (phase === 1) {
    drawGlow(frameMap, 16, 22, { r: 188, g: 212, b: 255 }, 0.8, 4);
    for (let row = 0; row < 5; row += 1) {
      fillRect(frameMap, 14 + row * 3, 30 + row, 1, 4, { r: 88, g: 176, b: 255 });
    }
  } else {
    drawGlow(frameMap, 18, 24, { r: 208, g: 232, b: 255 }, 0.72, 4);
    for (let index = 0; index < 6; index += 1) {
      setPixel(frameMap, 12 + index * 3, 32 + (index % 2), 240, 248, 255);
      setPixel(frameMap, 13 + index * 3, 33 + (index % 2), 240, 248, 255);
    }
  }

  drawGlyphText(frameMap, text, 32, 22, 3, { r: 224, g: 244, b: 255 });
  fillRect(frameMap, 48, 22, 2, 2, { r: 120, g: 196, b: 255 });
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

function buildSpecialSceneFrame(preset, frameMap, frameIndex, elapsed, speedUnit, intensityUnit) {
  if (preset === "boids") {
    buildBoidsFrame(frameMap, elapsed, speedUnit, intensityUnit);
    return true;
  }
  if (preset === "bouncing_logo") {
    buildBouncingLogoFrame(frameMap, elapsed, speedUnit);
    return true;
  }
  if (preset === "sorting_visualizer") {
    buildSortingVisualizerFrame(frameMap, frameIndex, intensityUnit);
    return true;
  }
  if (preset === "clock_scene") {
    buildClockSceneFrame(frameMap, elapsed, intensityUnit);
    return true;
  }
  if (preset === "countdown_scene") {
    buildCountdownSceneFrame(frameMap, elapsed, speedUnit);
    return true;
  }
  if (preset === "weather_scene") {
    buildWeatherSceneFrame(frameMap, elapsed, speedUnit);
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
  if (preset === "starfield") {
    return renderStarfield(nx, ny, x, y, timeBase, intensityUnit);
  }
  if (preset === "neon_tunnel") {
    return renderNeonTunnel(nx, ny, dist, timeBase, speedUnit, intensityUnit);
  }
  if (preset === "metablob") {
    return renderMetablob(nx, ny, timeBase, intensityUnit);
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
  if (preset === "rain_scene") {
    return renderRainScene(nx, ny, x, y, timeBase, speedUnit, intensityUnit);
  }
  return renderWatermelonPlasma(nx, ny, dist, timeBase, intensityUnit);
}

export function buildAmbientPreviewFrames(config) {
  const safeConfig = config || {};
  const preset = typeof safeConfig.preset === "string" ? safeConfig.preset : "digital_rain";
  const parsedSpeed = Number(safeConfig.speed);
  const parsedIntensity = Number(safeConfig.intensity);
  const speed = Number.isFinite(parsedSpeed) ? Math.max(1, Math.min(10, Math.round(parsedSpeed))) : 6;
  const intensity = Number.isFinite(parsedIntensity) ? Math.max(10, Math.min(100, Math.round(parsedIntensity))) : 72;
  const frameCount = safeConfig.loop === false ? 10 : 20;
  const frames = [];

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const frameMap = new Map();
    const elapsed = frameIndex * 120;
    const speedUnit = speed / 10;
    const intensityUnit = intensity / 100;
    const timeBase = elapsed * (0.00045 + speedUnit * 0.00135);

    if (buildSpecialSceneFrame(preset, frameMap, frameIndex, elapsed, speedUnit, intensityUnit)) {
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
