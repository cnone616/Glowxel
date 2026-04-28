import { setPixel } from "../led-matrix-demos/common.js";

const WIDTH = 64;
const HEIGHT = 64;
const CONTAINER_LEFT = 9;
const CONTAINER_RIGHT = 54;
const CONTAINER_TOP = 9;
const CONTAINER_BOTTOM = 57;
const MAX_PARTICLES = 72;
const DENSITY_SIZE = WIDTH * HEIGHT;

const liquidState = {
  key: "",
  lastElapsed: 0,
  simTime: 0,
  particles: [],
};

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

function mix(left, right, alpha) {
  return left * (1 - alpha) + right * alpha;
}

function seedValue(index, offset) {
  return (
    Math.sin((index + 1) * 12.9898 + offset * 78.233) * 43758.5453
  ) % 1;
}

function normalizedSeed(index, offset) {
  const seeded = seedValue(index, offset);
  return seeded < 0 ? seeded + 1 : seeded;
}

function resolveParticleCount(intensityUnit) {
  const particleCount = Math.round(28 + intensityUnit * 36);
  return clamp(particleCount, 28, MAX_PARTICLES);
}

function resetLiquidState(speedUnit, intensityUnit) {
  liquidState.lastElapsed = 0;
  liquidState.simTime = 0;
  liquidState.particles = [];

  const particleCount = resolveParticleCount(intensityUnit);
  const usableWidth = CONTAINER_RIGHT - CONTAINER_LEFT - 4;
  const usableHeight = CONTAINER_BOTTOM - CONTAINER_TOP - 6;

  for (let index = 0; index < particleCount; index += 1) {
    const px = CONTAINER_LEFT + 2 + normalizedSeed(index, 0.17) * usableWidth;
    const py =
      CONTAINER_TOP +
      usableHeight * (0.54 + normalizedSeed(index, 0.41) * 0.38);
    liquidState.particles.push({
      x: px,
      y: py,
      vx: (normalizedSeed(index, 0.63) - 0.5) * (0.8 + speedUnit * 0.6),
      vy: (normalizedSeed(index, 0.89) - 0.5) * 0.2,
    });
  }
}

function resolveStateKey(speedUnit, intensityUnit) {
  return `${speedUnit.toFixed(3)}:${intensityUnit.toFixed(3)}`;
}

function applyParticleCollisions(particles, minDist) {
  const minDistSq = minDist * minDist;

  for (let leftIndex = 0; leftIndex < particles.length; leftIndex += 1) {
    for (
      let rightIndex = leftIndex + 1;
      rightIndex < particles.length;
      rightIndex += 1
    ) {
      const left = particles[leftIndex];
      const right = particles[rightIndex];
      const dx = right.x - left.x;
      const dy = right.y - left.y;
      const distSq = dx * dx + dy * dy;

      if (distSq <= 0.0001 || distSq >= minDistSq) {
        continue;
      }

      const dist = Math.sqrt(distSq);
      const nx = dx / dist;
      const ny = dy / dist;
      const overlap = (minDist - dist) * 0.5;

      left.x -= nx * overlap;
      left.y -= ny * overlap;
      right.x += nx * overlap;
      right.y += ny * overlap;

      const leftNormal = left.vx * nx + left.vy * ny;
      const rightNormal = right.vx * nx + right.vy * ny;
      const sharedNormal = (leftNormal + rightNormal) * 0.5;

      left.vx += (sharedNormal - leftNormal) * nx;
      left.vy += (sharedNormal - leftNormal) * ny;
      right.vx += (sharedNormal - rightNormal) * nx;
      right.vy += (sharedNormal - rightNormal) * ny;
    }
  }
}

function simulateStep(stepMs, speedUnit, intensityUnit) {
  liquidState.simTime += stepMs;

  const particles = liquidState.particles;
  const dt = stepMs / 16;
  const timeSeconds = liquidState.simTime * 0.001;
  const swayA = Math.sin(timeSeconds * (1.6 + speedUnit * 1.5));
  const swayB = Math.sin(timeSeconds * (0.64 + speedUnit * 0.44) + 1.7);
  const swayC = Math.sin(timeSeconds * 0.31 + 0.9);
  const gravityX =
    (swayA * 0.08 + swayB * 0.05 + swayC * 0.03) *
    (0.48 + intensityUnit * 0.52);
  const gravityY =
    0.085 + 0.01 * (0.5 + 0.5 * Math.sin(timeSeconds * 0.73 + 0.35));
  const minDist = 2.35 + intensityUnit * 0.65;

  for (let index = 0; index < particles.length; index += 1) {
    const particle = particles[index];

    particle.vx += gravityX * dt;
    particle.vy += gravityY * dt;
    particle.vx *= 0.992;
    particle.vy *= 0.992;
    particle.x += particle.vx * dt;
    particle.y += particle.vy * dt;

    if (particle.x < CONTAINER_LEFT + 1) {
      particle.x = CONTAINER_LEFT + 1;
      particle.vx *= -0.72;
    }
    if (particle.x > CONTAINER_RIGHT - 1) {
      particle.x = CONTAINER_RIGHT - 1;
      particle.vx *= -0.72;
    }
    if (particle.y < CONTAINER_TOP + 1) {
      particle.y = CONTAINER_TOP + 1;
      particle.vy *= -0.58;
    }
    if (particle.y > CONTAINER_BOTTOM - 1) {
      particle.y = CONTAINER_BOTTOM - 1;
      particle.vy *= -0.46;
      particle.vx *= 0.985;
    }
  }

  applyParticleCollisions(particles, minDist);
}

function ensureLiquidState(elapsed, speedUnit, intensityUnit) {
  const key = resolveStateKey(speedUnit, intensityUnit);
  if (liquidState.key !== key || elapsed < liquidState.lastElapsed) {
    liquidState.key = key;
    resetLiquidState(speedUnit, intensityUnit);
  }

  let remaining = elapsed - liquidState.lastElapsed;
  while (remaining > 0) {
    const stepMs = Math.min(16, remaining);
    simulateStep(stepMs, speedUnit, intensityUnit);
    liquidState.lastElapsed += stepMs;
    remaining -= stepMs;
  }
}

function buildDensityField(particles) {
  const density = new Float32Array(DENSITY_SIZE);
  const radius = 4;
  const radiusSq = radius * radius;

  for (let index = 0; index < particles.length; index += 1) {
    const particle = particles[index];
    const minX = Math.max(0, Math.floor(particle.x - radius));
    const maxX = Math.min(WIDTH - 1, Math.ceil(particle.x + radius));
    const minY = Math.max(0, Math.floor(particle.y - radius));
    const maxY = Math.min(HEIGHT - 1, Math.ceil(particle.y + radius));

    for (let y = minY; y <= maxY; y += 1) {
      for (let x = minX; x <= maxX; x += 1) {
        const dx = x + 0.5 - particle.x;
        const dy = y + 0.5 - particle.y;
        const distSq = dx * dx + dy * dy;
        if (distSq > radiusSq) {
          continue;
        }
        const weight = 1 - distSq / radiusSq;
        density[y * WIDTH + x] += weight * weight;
      }
    }
  }

  return density;
}

function drawContainer(frameMap) {
  const shell = { r: 10, g: 18, b: 30 };
  const floor = { r: 18, g: 32, b: 44 };

  for (let y = CONTAINER_TOP; y <= CONTAINER_BOTTOM; y += 1) {
    setPixel(frameMap, CONTAINER_LEFT - 1, y, shell.r, shell.g, shell.b);
    setPixel(frameMap, CONTAINER_RIGHT + 1, y, shell.r, shell.g, shell.b);
  }

  for (let x = CONTAINER_LEFT - 1; x <= CONTAINER_RIGHT + 1; x += 1) {
    setPixel(frameMap, x, CONTAINER_BOTTOM + 1, floor.r, floor.g, floor.b);
  }
}

function drawLiquid(frameMap, density, intensityUnit) {
  for (let y = CONTAINER_TOP; y <= CONTAINER_BOTTOM; y += 1) {
    for (let x = CONTAINER_LEFT; x <= CONTAINER_RIGHT; x += 1) {
      const index = y * WIDTH + x;
      const value = density[index];
      if (value <= 0.015) {
        continue;
      }

      const glow = clampUnit((value - 0.03) / 0.22);
      const body = clampUnit((value - 0.16) / 0.72);
      const above = y > 0 ? density[(y - 1) * WIDTH + x] : 0;
      const below = y < HEIGHT - 1 ? density[(y + 1) * WIDTH + x] : 0;
      const surface = clampUnit((value - above) * 1.5);
      const depth = clampUnit((y - CONTAINER_TOP) / (CONTAINER_BOTTOM - CONTAINER_TOP));

      let r = mix(2, 18, glow);
      let g = mix(10, 86, glow);
      let b = mix(28, 170, glow);

      r = mix(r, 22 + depth * 10, body);
      g = mix(g, 110 + depth * 18, body);
      b = mix(b, 205 + depth * 26, body);

      const whitecap = surface * surface * (0.38 + intensityUnit * 0.34);
      r += 90 * whitecap + below * 4;
      g += 110 * whitecap + below * 8;
      b += 120 * whitecap + below * 12;

      setPixel(frameMap, x, y, r, g, b);
    }
  }
}

function buildMetaBlobAmbientFrame(frameMap, elapsed, speedUnit, intensityUnit) {
  ensureLiquidState(elapsed, speedUnit, intensityUnit);
  const density = buildDensityField(liquidState.particles);
  drawLiquid(frameMap, density, intensityUnit);
  drawContainer(frameMap);
}

export { buildMetaBlobAmbientFrame };
