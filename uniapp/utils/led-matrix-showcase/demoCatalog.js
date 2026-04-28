const DEMO_ITEMS = [
  { id: "clock", label: "像素时钟", category: "经典屏保", type: "ambient", ambientPreset: "clock_scene", source: "AmbientScenes/Clock", hint: "上游 AmbientScenes 的 Clock 场景" },
  { id: "starfield", label: "星空", category: "经典屏保", type: "ambient", ambientPreset: "starfield", source: "AmbientScenes/Starfield", hint: "上游 AmbientScenes 的 Starfield 场景" },
  { id: "metablob", label: "液体模拟", category: "经典屏保", type: "ambient", ambientPreset: "metablob", source: "m0vi0/esp8266-liquid-sim", hint: "参考 m0vi0 的粒子液体模拟，并按 64×64 Glowxel PixelBoard 板载效果重写" },
  { id: "digital_rain", label: "数字雨", category: "经典屏保", type: "ambient", ambientPreset: "digital_rain", source: "AmbientScenes/DigitalRain", hint: "上游 AmbientScenes 的 DigitalRain 场景" },
  { id: "neon_tunnel", label: "霓虹隧道", category: "经典屏保", type: "ambient", ambientPreset: "neon_tunnel", source: "AmbientScenes/NeonTunnel", hint: "上游 AmbientScenes 的 NeonTunnel 场景" },
  { id: "boids", label: "群游粒子", category: "经典屏保", type: "ambient", ambientPreset: "boids", source: "AmbientScenes/Boids", hint: "上游 AmbientScenes 的 Boids 场景" },
  { id: "falling_sand", label: "流沙场", category: "经典屏保", type: "ambient", ambientPreset: "falling_sand", source: "AmbientScenes/FallingSand", hint: "上游 AmbientScenes 的 FallingSand 场景" },
  { id: "game_of_life", label: "生命演化", category: "分形屏保", type: "ambient", ambientPreset: "game_of_life", source: "FractalScenes/GameOfLife", hint: "上游 FractalScenes 的 GameOfLife 场景" },
  { id: "julia_set", label: "Julia 分形", category: "分形屏保", type: "ambient", ambientPreset: "julia_set", source: "FractalScenes/JuliaSet", hint: "上游 FractalScenes 的 JuliaSet 场景" },
  { id: "wave_pattern", label: "波纹分形", category: "分形屏保", type: "ambient", ambientPreset: "wave_pattern", source: "FractalScenes/WavePattern", hint: "上游 FractalScenes 的 WavePattern 场景" },
  { id: "maze", label: "迷宫漫游", category: "独立游戏", type: "animation", source: "GameScenes/Maze", hint: "上游 GameScenes 的 Maze 场景" },
  { id: "snake", label: "贪吃蛇", category: "独立游戏", type: "animation", source: "GameScenes/Snake", hint: "上游 GameScenes 的 Snake 场景" },
  { id: "sorting_visualizer", label: "排序矩阵", category: "游戏屏保", type: "animation", source: "AmbientScenes/SortingVisualizer", hint: "上游 AmbientScenes 的 SortingVisualizer 场景" },
  { id: "bouncing_logo", label: "弹跳图标", category: "游戏屏保", type: "ambient", ambientPreset: "bouncing_logo", source: "AmbientScenes/BouncingLogo", hint: "上游 AmbientScenes 的 BouncingLogo 场景" },
  { id: "watermelon_plasma", label: "西瓜等离子", category: "社区场景", type: "ambient", ambientPreset: "watermelon_plasma", source: "GithubScenes/WatermelonPlasma", hint: "上游 GithubScenes 的 WatermelonPlasma 场景" },
  { id: "github_wave", label: "社区波场", category: "社区场景", type: "ambient", ambientPreset: "reaction_diffusion", source: "GithubScenes/Wave", hint: "上游 GithubScenes 的 Wave 场景" },
  { id: "rain", label: "雨幕", category: "粒子场景", type: "ambient", ambientPreset: "rain_scene", source: "RGBMatrixAnimations/Rain", hint: "上游 RGBMatrixAnimations 的 Rain 场景" },
  { id: "sparks", label: "火花", category: "粒子场景", type: "ambient", ambientPreset: "sparks", source: "RGBMatrixAnimations/Sparks", hint: "上游 RGBMatrixAnimations 的 Sparks 场景" },
];

const LED_SHOWCASE_DEMO_IDS = [
  "clock",
  "starfield",
  "metablob",
  "digital_rain",
  "neon_tunnel",
  "boids",
  "falling_sand",
  "game_of_life",
  "julia_set",
  "wave_pattern",
  "watermelon_plasma",
  "github_wave",
  "rain",
  "sparks",
];

function resolveDemoById(demoId) {
  return DEMO_ITEMS.find((item) => item.id === demoId) || null;
}

function getLedMatrixDemoItems() {
  return LED_SHOWCASE_DEMO_IDS.map((id) => resolveDemoById(id)).filter(Boolean);
}

export {
  DEMO_ITEMS,
  LED_SHOWCASE_DEMO_IDS,
  resolveDemoById,
  getLedMatrixDemoItems,
};
