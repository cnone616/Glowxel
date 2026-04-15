import { clamp, fillRect, normalizeSpeed } from "./common.js";

export function buildPingPongDemoMaps(speed, intensity) {
  const safeSpeed = normalizeSpeed(speed);
  const maps = [];

  const boardWidth = 64;
  const boardHeight = 64;
  const ballSize = 2;
  const paddleWidth = 2;
  const paddleHeight = 8;
  const targetFps = 60;
  const timeStep = 1 / targetFps;
  const maxFrames = 360;
  const ballSpeed = 0.18 + safeSpeed * 0.024;
  const paddleSpeed = 0.09 + safeSpeed * 0.012;
  const maxSpeedMultiplier = 4;
  let speedMultiplier = 1;
  let ballX = boardWidth / 2;
  let ballY = boardHeight / 2;
  let ballDX = ballSpeed;
  let ballDY = ballSpeed;
  let leftPaddleY = boardHeight / 2 - paddleHeight / 2;
  let rightPaddleY = boardHeight / 2 - paddleHeight / 2;

  while (maps.length < maxFrames) {
    const map = new Map();
    fillRect(map, 0, 0, boardWidth, boardHeight, 0, 0, 0);

    ballX += ballDX * timeStep * 60 * speedMultiplier;
    ballY += ballDY * timeStep * 60 * speedMultiplier;

    const targetY = ballY - paddleHeight / 2;
    if (ballDX < 0) {
      leftPaddleY += (targetY - leftPaddleY) * paddleSpeed * timeStep * 60;
    } else {
      rightPaddleY += (targetY - rightPaddleY) * paddleSpeed * timeStep * 60;
    }

    if (ballY <= 0 || ballY >= boardHeight - ballSize) {
      ballDY = -ballDY;
      ballY = clamp(ballY, 0, boardHeight - ballSize);
    }

    leftPaddleY = clamp(leftPaddleY, 0, boardHeight - paddleHeight);
    rightPaddleY = clamp(rightPaddleY, 0, boardHeight - paddleHeight);

    if (
      ballDX < 0 &&
      ballX <= paddleWidth &&
      ballY + ballSize >= leftPaddleY &&
      ballY <= leftPaddleY + paddleHeight
    ) {
      ballDX = -ballDX;
      ballX = paddleWidth;
      speedMultiplier = Math.min(speedMultiplier + 0.1, maxSpeedMultiplier);
    }

    if (
      ballDX > 0 &&
      ballX >= boardWidth - paddleWidth - ballSize &&
      ballY + ballSize >= rightPaddleY &&
      ballY <= rightPaddleY + paddleHeight
    ) {
      ballDX = -ballDX;
      ballX = boardWidth - paddleWidth - ballSize;
      speedMultiplier = Math.min(speedMultiplier + 0.1, maxSpeedMultiplier);
    }

    if (ballX < 0 || ballX > boardWidth) {
      ballX = boardWidth / 2;
      ballY = boardHeight / 2;
      ballDX = ballDX < 0 ? ballSpeed : -ballSpeed;
      ballDY = ballDY >= 0 ? ballSpeed : -ballSpeed;
      speedMultiplier = 1;
      leftPaddleY = boardHeight / 2 - paddleHeight / 2;
      rightPaddleY = boardHeight / 2 - paddleHeight / 2;
    }

    fillRect(map, 0, Math.round(leftPaddleY), paddleWidth, paddleHeight, 255, 255, 255);
    fillRect(
      map,
      boardWidth - paddleWidth,
      Math.round(rightPaddleY),
      paddleWidth,
      paddleHeight,
      255,
      255,
      255,
    );
    fillRect(map, Math.round(ballX), Math.round(ballY), ballSize, ballSize, 255, 255, 255);

    maps.push(map);
  }

  return maps;
}
