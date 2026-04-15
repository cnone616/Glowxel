import { fillRect, setPixel } from "../led-matrix-demos/common.js";

function fract(value) {
  return value - Math.floor(value);
}

function seed(index, shift) {
  return fract(Math.sin((index + 1) * 17.371 + shift * 3.147) * 24634.6345);
}

function hslToRgb(hue, saturation, lightness) {
  const chroma = (1 - Math.abs(2 * lightness - 1)) * saturation;
  const huePrime = ((hue % 360) + 360) % 360 / 60;
  const x = chroma * (1 - Math.abs((huePrime % 2) - 1));
  const match = lightness - chroma / 2;
  let r1 = 0;
  let g1 = 0;
  let b1 = 0;

  if (huePrime < 1) {
    r1 = chroma;
    g1 = x;
  } else if (huePrime < 2) {
    r1 = x;
    g1 = chroma;
  } else if (huePrime < 3) {
    g1 = chroma;
    b1 = x;
  } else if (huePrime < 4) {
    g1 = x;
    b1 = chroma;
  } else if (huePrime < 5) {
    r1 = x;
    b1 = chroma;
  } else {
    r1 = chroma;
    b1 = x;
  }

  return {
    r: Math.round((r1 + match) * 255),
    g: Math.round((g1 + match) * 255),
    b: Math.round((b1 + match) * 255),
  };
}

function clampMagnitude(vector, maxMagnitude) {
  const magnitude = Math.sqrt(vector.x * vector.x + vector.y * vector.y);
  if (magnitude <= 0 || magnitude <= maxMagnitude) {
    return vector;
  }
  const scale = maxMagnitude / magnitude;
  return {
    x: vector.x * scale,
    y: vector.y * scale,
  };
}

function normalize(vector) {
  const magnitude = Math.sqrt(vector.x * vector.x + vector.y * vector.y);
  if (magnitude <= 0.0001) {
    return { x: 0, y: 0 };
  }
  return {
    x: vector.x / magnitude,
    y: vector.y / magnitude,
  };
}

function cloneBoids() {
  const boids = [];
  for (let index = 0; index < 18; index += 1) {
    const hue = seed(index, 0.31) * 360;
    const color = hslToRgb(hue, 1, 0.5);
    boids.push({
      position: {
        x: seed(index, 1.11) * 63,
        y: seed(index, 2.07) * 63,
      },
      velocity: {
        x: seed(index, 3.11) * 2 - 1,
        y: seed(index, 4.23) * 2 - 1,
      },
      acceleration: { x: 0, y: 0 },
      color,
    });
  }
  return boids;
}

function separate(boids, target, maxSpeed, maxForce, separationDistance) {
  let count = 0;
  let steer = { x: 0, y: 0 };

  for (let index = 0; index < boids.length; index += 1) {
    const other = boids[index];
    if (other === target) {
      continue;
    }
    const dx = target.position.x - other.position.x;
    const dy = target.position.y - other.position.y;
    const distance = Math.sqrt(dx * dx + dy * dy);
    if (distance <= 0 || distance >= separationDistance) {
      continue;
    }
    const diff = normalize({ x: dx, y: dy });
    steer.x += diff.x / distance;
    steer.y += diff.y / distance;
    count += 1;
  }

  if (count <= 0) {
    return { x: 0, y: 0 };
  }

  steer.x /= count;
  steer.y /= count;
  steer = normalize(steer);
  steer.x *= maxSpeed;
  steer.y *= maxSpeed;
  steer.x -= target.velocity.x;
  steer.y -= target.velocity.y;
  return clampMagnitude(steer, maxForce);
}

function align(boids, target, maxSpeed, maxForce, neighborDistance) {
  let count = 0;
  let sum = { x: 0, y: 0 };

  for (let index = 0; index < boids.length; index += 1) {
    const other = boids[index];
    if (other === target) {
      continue;
    }
    const dx = target.position.x - other.position.x;
    const dy = target.position.y - other.position.y;
    const distance = Math.sqrt(dx * dx + dy * dy);
    if (distance <= 0 || distance >= neighborDistance) {
      continue;
    }
    sum.x += other.velocity.x;
    sum.y += other.velocity.y;
    count += 1;
  }

  if (count <= 0) {
    return { x: 0, y: 0 };
  }

  sum.x /= count;
  sum.y /= count;
  sum = normalize(sum);
  sum.x *= maxSpeed;
  sum.y *= maxSpeed;
  sum.x -= target.velocity.x;
  sum.y -= target.velocity.y;
  return clampMagnitude(sum, maxForce);
}

function cohesion(boids, target, maxSpeed, maxForce, neighborDistance) {
  let count = 0;
  let center = { x: 0, y: 0 };

  for (let index = 0; index < boids.length; index += 1) {
    const other = boids[index];
    if (other === target) {
      continue;
    }
    const dx = target.position.x - other.position.x;
    const dy = target.position.y - other.position.y;
    const distance = Math.sqrt(dx * dx + dy * dy);
    if (distance <= 0 || distance >= neighborDistance) {
      continue;
    }
    center.x += other.position.x;
    center.y += other.position.y;
    count += 1;
  }

  if (count <= 0) {
    return { x: 0, y: 0 };
  }

  center.x /= count;
  center.y /= count;

  let desired = {
    x: center.x - target.position.x,
    y: center.y - target.position.y,
  };
  desired = normalize(desired);
  desired.x *= maxSpeed;
  desired.y *= maxSpeed;
  desired.x -= target.velocity.x;
  desired.y -= target.velocity.y;
  return clampMagnitude(desired, maxForce);
}

function runBoids(boids, steps, speedUnit, intensityUnit) {
  const maxSpeed = 0.75 + speedUnit * 1.25;
  const maxForce = 0.03 + intensityUnit * 0.05;
  const separationDistance = 5 + intensityUnit * 7;
  const alignmentDistance = 10 + intensityUnit * 12;
  const cohesionDistance = 12 + intensityUnit * 16;
  const separationWeight = 1.4;
  const alignmentWeight = 1.0;
  const cohesionWeight = 1.0;

  for (let step = 0; step < steps; step += 1) {
    for (let index = 0; index < boids.length; index += 1) {
      const boid = boids[index];
      const sep = separate(boids, boid, maxSpeed, maxForce, separationDistance);
      const ali = align(boids, boid, maxSpeed, maxForce, alignmentDistance);
      const coh = cohesion(boids, boid, maxSpeed, maxForce, cohesionDistance);

      boid.acceleration.x =
        sep.x * separationWeight + ali.x * alignmentWeight + coh.x * cohesionWeight;
      boid.acceleration.y =
        sep.y * separationWeight + ali.y * alignmentWeight + coh.y * cohesionWeight;
    }

    for (let index = 0; index < boids.length; index += 1) {
      const boid = boids[index];
      boid.velocity.x += boid.acceleration.x;
      boid.velocity.y += boid.acceleration.y;
      const limited = clampMagnitude(boid.velocity, maxSpeed);
      boid.velocity.x = limited.x;
      boid.velocity.y = limited.y;
      boid.position.x += boid.velocity.x;
      boid.position.y += boid.velocity.y;
      boid.acceleration.x = 0;
      boid.acceleration.y = 0;

      if (boid.position.x > 64) {
        boid.position.x = 0;
      } else if (boid.position.x < 0) {
        boid.position.x = 64;
      }

      if (boid.position.y > 64) {
        boid.position.y = 0;
      } else if (boid.position.y < 0) {
        boid.position.y = 64;
      }
    }
  }
}

function buildBoidsAmbientFrame(frameMap, elapsed, speedUnit, intensityUnit) {
  fillRect(frameMap, 0, 0, 64, 64, 0, 0, 0);

  const boids = cloneBoids();
  const stepCount = Math.max(1, Math.floor(elapsed / 120) + 1);
  runBoids(boids, stepCount, speedUnit, intensityUnit);

  for (let index = 0; index < boids.length; index += 1) {
    const boid = boids[index];
    const x = Math.round(boid.position.x);
    const y = Math.round(boid.position.y);
    setPixel(frameMap, x, y, boid.color.r, boid.color.g, boid.color.b);
  }
}

export { buildBoidsAmbientFrame };
