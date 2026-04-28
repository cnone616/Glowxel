// 闹钟 Canvas 绘制工具

// 5x7 点阵字体数据（列位图）
export const font5x7 = {
  "0": [0x3e, 0x51, 0x49, 0x45, 0x3e],
  "1": [0x00, 0x42, 0x7f, 0x40, 0x00],
  "2": [0x42, 0x61, 0x51, 0x49, 0x46],
  "3": [0x21, 0x41, 0x45, 0x4b, 0x31],
  "4": [0x18, 0x14, 0x12, 0x7f, 0x10],
  "5": [0x27, 0x45, 0x45, 0x45, 0x39],
  "6": [0x3c, 0x4a, 0x49, 0x49, 0x30],
  "7": [0x01, 0x71, 0x09, 0x05, 0x03],
  "8": [0x36, 0x49, 0x49, 0x49, 0x36],
  "9": [0x06, 0x49, 0x49, 0x29, 0x1e],
  ":": [0x00, 0x36, 0x36, 0x00, 0x00],
  "-": [0x08, 0x08, 0x08, 0x08, 0x08],
  "/": [0x20, 0x10, 0x08, 0x04, 0x02],
  " ": [0x00, 0x00, 0x00, 0x00, 0x00],

  A: [0x7e, 0x09, 0x09, 0x09, 0x7e],
  B: [0x7f, 0x49, 0x49, 0x49, 0x36],
  C: [0x3e, 0x41, 0x41, 0x41, 0x22],
  D: [0x7f, 0x41, 0x41, 0x22, 0x1c],
  E: [0x7f, 0x49, 0x49, 0x49, 0x41],
  F: [0x7f, 0x09, 0x09, 0x09, 0x01],
  G: [0x3e, 0x41, 0x49, 0x49, 0x7a],
  H: [0x7f, 0x08, 0x08, 0x08, 0x7f],
  I: [0x00, 0x41, 0x7f, 0x41, 0x00],
  J: [0x20, 0x40, 0x41, 0x3f, 0x01],
  K: [0x7f, 0x08, 0x14, 0x22, 0x41],
  L: [0x7f, 0x40, 0x40, 0x40, 0x40],
  M: [0x7f, 0x02, 0x0c, 0x02, 0x7f],
  N: [0x7f, 0x04, 0x08, 0x10, 0x7f],
  O: [0x3e, 0x41, 0x41, 0x41, 0x3e],
  P: [0x7f, 0x09, 0x09, 0x09, 0x06],
  Q: [0x3e, 0x41, 0x51, 0x21, 0x5e],
  R: [0x7f, 0x09, 0x19, 0x29, 0x46],
  S: [0x46, 0x49, 0x49, 0x49, 0x31],
  T: [0x01, 0x01, 0x7f, 0x01, 0x01],
  U: [0x3f, 0x40, 0x40, 0x40, 0x3f],
  V: [0x1f, 0x20, 0x40, 0x20, 0x1f],
  W: [0x3f, 0x40, 0x38, 0x40, 0x3f],
  X: [0x63, 0x14, 0x08, 0x14, 0x63],
  Y: [0x07, 0x08, 0x70, 0x08, 0x07],
  Z: [0x61, 0x51, 0x49, 0x45, 0x43],
};

// 3x5 点阵字体（与 ESP32 侧基础字模一致）
export const font3x5 = {
  "0": [7, 5, 5, 5, 7],
  "1": [2, 6, 2, 2, 7],
  "2": [7, 1, 7, 4, 7],
  "3": [7, 1, 7, 1, 7],
  "4": [5, 5, 7, 1, 1],
  "5": [7, 4, 7, 1, 7],
  "6": [7, 4, 7, 5, 7],
  "7": [7, 1, 1, 1, 1],
  "8": [7, 5, 7, 5, 7],
  "9": [7, 5, 7, 1, 7],
  ".": [0, 0, 0, 0, 2],
  ":": [0, 2, 0, 2, 0],
  "-": [0, 0, 7, 0, 0],
  " ": [0, 0, 0, 0, 0],
  A: [2, 5, 7, 5, 5],
  B: [6, 5, 6, 5, 6],
  C: [7, 4, 4, 4, 7],
  D: [6, 5, 5, 5, 6],
  E: [7, 4, 7, 4, 7],
  F: [7, 4, 7, 4, 4],
  G: [7, 4, 5, 5, 7],
  H: [5, 5, 7, 5, 5],
  I: [7, 2, 2, 2, 7],
  J: [3, 1, 1, 5, 7],
  K: [5, 6, 4, 6, 5],
  L: [4, 4, 4, 4, 7],
  M: [5, 7, 5, 5, 5],
  N: [5, 7, 7, 5, 5],
  O: [7, 5, 5, 5, 7],
  P: [7, 5, 7, 4, 4],
  Q: [7, 5, 5, 7, 1],
  R: [7, 5, 7, 6, 5],
  S: [7, 4, 7, 1, 7],
  T: [7, 2, 2, 2, 2],
  U: [5, 5, 5, 5, 7],
  V: [5, 5, 5, 2, 2],
  W: [5, 5, 5, 7, 5],
  X: [5, 5, 2, 5, 5],
  Y: [5, 5, 2, 2, 2],
  Z: [7, 1, 2, 4, 7],
};

const CLOCK_FONT_OPTIONS = [
  {
    id: "classic_5x7",
    name: "5x7 经典",
    width: 5,
    height: 7,
    spacing: 1,
    previewColor: "#00ff9d",
    previewBg: "#0f221b",
    glyphs: {
      "0": [14, 17, 19, 21, 25, 17, 14],
      "1": [4, 12, 4, 4, 4, 4, 14],
      "2": [14, 17, 1, 2, 4, 8, 31],
      "3": [30, 1, 1, 14, 1, 1, 30],
      "4": [2, 6, 10, 18, 31, 2, 2],
      "5": [31, 16, 30, 1, 1, 17, 14],
      "6": [6, 8, 16, 30, 17, 17, 14],
      "7": [31, 1, 2, 4, 8, 8, 8],
      "8": [14, 17, 17, 14, 17, 17, 14],
      "9": [14, 17, 17, 15, 1, 2, 12],
      ":": [0, 12, 12, 0, 12, 12, 0],
    },
  },
  {
    id: "minimal_3x5",
    name: "3x5 极简",
    width: 3,
    height: 5,
    spacing: 1,
    previewColor: "#ff6464",
    previewBg: "#2a1212",
    glyphs: {
      "0": [7, 5, 5, 5, 7],
      "1": [2, 6, 2, 2, 7],
      "2": [7, 1, 7, 4, 7],
      "3": [7, 1, 7, 1, 7],
      "4": [5, 5, 7, 1, 1],
      "5": [7, 4, 7, 1, 7],
      "6": [7, 4, 7, 5, 7],
      "7": [7, 1, 2, 2, 2],
      "8": [7, 5, 7, 5, 7],
      "9": [7, 5, 7, 1, 7],
      ":": [0, 2, 0, 2, 0],
    },
  },
  {
    id: "rounded_4x6",
    name: "4x6 圆润",
    width: 4,
    height: 6,
    spacing: 1,
    previewColor: "#64c8ff",
    previewBg: "#10202a",
    glyphs: {
      "0": [6, 9, 9, 9, 9, 6],
      "1": [4, 12, 4, 4, 4, 14],
      "2": [6, 9, 1, 2, 4, 15],
      "3": [14, 1, 6, 1, 1, 14],
      "4": [2, 6, 10, 15, 2, 2],
      "5": [15, 8, 14, 1, 9, 6],
      "6": [6, 8, 14, 9, 9, 6],
      "7": [15, 1, 2, 4, 4, 4],
      "8": [6, 9, 6, 9, 9, 6],
      "9": [6, 9, 9, 7, 1, 6],
      ":": [0, 4, 0, 0, 4, 0],
    },
  },
  {
    id: "lcd_6x8",
    name: "6x8 LCD",
    width: 6,
    height: 8,
    spacing: 1,
    previewColor: "#ffb020",
    previewBg: "#261b08",
    glyphs: {
      "0": [30, 51, 33, 33, 33, 33, 51, 30],
      "1": [12, 28, 12, 12, 12, 12, 12, 63],
      "2": [30, 51, 3, 6, 12, 24, 48, 63],
      "3": [62, 3, 3, 14, 3, 3, 3, 62],
      "4": [6, 14, 22, 38, 63, 6, 6, 6],
      "5": [63, 48, 48, 62, 3, 3, 51, 30],
      "6": [14, 24, 48, 62, 51, 51, 51, 30],
      "7": [63, 3, 6, 12, 24, 24, 24, 24],
      "8": [30, 51, 51, 30, 51, 51, 51, 30],
      "9": [30, 51, 51, 31, 3, 6, 12, 56],
      ":": [0, 12, 12, 0, 0, 12, 12, 0],
    },
  },
  {
    id: "hollow_5x7",
    name: "5x7 空心",
    width: 5,
    height: 7,
    spacing: 1,
    previewColor: "#5ba8ff",
    previewBg: "#101a2b",
    glyphs: {
      "0": [14, 17, 17, 17, 17, 17, 14],
      "1": [4, 12, 4, 4, 4, 4, 4],
      "2": [14, 17, 1, 2, 4, 8, 31],
      "3": [30, 1, 1, 14, 1, 1, 30],
      "4": [2, 6, 10, 18, 31, 2, 2],
      "5": [31, 16, 30, 1, 1, 17, 14],
      "6": [6, 8, 16, 30, 17, 17, 14],
      "7": [31, 1, 2, 4, 8, 8, 8],
      "8": [14, 17, 17, 14, 17, 17, 14],
      "9": [14, 17, 17, 15, 1, 2, 12],
      ":": [0, 8, 0, 0, 0, 8, 0],
    },
  },
  {
    id: "seven_seg_5x7",
    name: "5x7 七段",
    width: 5,
    height: 7,
    spacing: 1,
    previewColor: "#ff4f88",
    previewBg: "#2b101b",
    glyphs: {
      "0": [31, 17, 17, 17, 17, 17, 31],
      "1": [1, 1, 1, 1, 1, 1, 1],
      "2": [31, 1, 1, 31, 16, 16, 31],
      "3": [31, 1, 1, 31, 1, 1, 31],
      "4": [17, 17, 17, 31, 1, 1, 1],
      "5": [31, 16, 16, 31, 1, 1, 31],
      "6": [31, 16, 16, 31, 17, 17, 31],
      "7": [31, 1, 1, 1, 1, 1, 1],
      "8": [31, 17, 17, 31, 17, 17, 31],
      "9": [31, 17, 17, 31, 1, 1, 31],
      ":": [0, 4, 0, 0, 0, 4, 0],
    },
  },
  {
    id: "italic_6x7",
    name: "6x7 斜体",
    width: 6,
    height: 7,
    spacing: 1,
    previewColor: "#7cff7c",
    previewBg: "#102612",
    glyphs: {
      "0": [15, 17, 18, 18, 34, 60, 0],
      "1": [4, 12, 4, 4, 8, 8, 28],
      "2": [14, 17, 2, 4, 8, 16, 62],
      "3": [30, 1, 2, 12, 2, 1, 30],
      "4": [2, 6, 10, 18, 31, 4, 4],
      "5": [31, 16, 30, 1, 1, 18, 12],
      "6": [6, 8, 16, 30, 17, 18, 12],
      "7": [31, 1, 2, 4, 8, 8, 16],
      "8": [14, 17, 18, 12, 18, 33, 30],
      "9": [14, 17, 18, 15, 2, 4, 24],
      ":": [0, 8, 0, 0, 16, 0, 0],
    },
  },
  {
    id: "retro_5x7",
    name: "5x7 终端",
    width: 5,
    height: 7,
    spacing: 1,
    previewColor: "#8ef2ff",
    previewBg: "#102728",
    glyphs: {
      "0": [31, 17, 19, 21, 25, 17, 31],
      "1": [6, 14, 6, 6, 6, 6, 31],
      "2": [31, 1, 1, 31, 16, 16, 31],
      "3": [31, 1, 1, 31, 1, 1, 31],
      "4": [17, 17, 17, 31, 1, 1, 1],
      "5": [31, 16, 16, 31, 1, 1, 31],
      "6": [31, 16, 16, 31, 17, 17, 31],
      "7": [31, 1, 1, 1, 1, 1, 1],
      "8": [31, 17, 17, 31, 17, 17, 31],
      "9": [31, 17, 17, 31, 1, 1, 31],
      ":": [0, 12, 0, 0, 0, 12, 0],
    },
  },
  {
    id: "neo_bold_6x8",
    name: "6x8 Neo Bold",
    width: 6,
    height: 8,
    spacing: 1,
    previewColor: "#f5ff72",
    previewBg: "#24280e",
    glyphs: {
      "0": [30, 63, 55, 63, 59, 51, 30, 28],
      "1": [12, 28, 12, 12, 12, 12, 30, 28],
      "2": [30, 63, 3, 6, 12, 24, 63, 62],
      "3": [62, 63, 3, 30, 3, 3, 62, 60],
      "4": [6, 14, 30, 54, 63, 62, 6, 4],
      "5": [63, 62, 62, 63, 3, 51, 30, 28],
      "6": [14, 28, 48, 62, 51, 51, 30, 28],
      "7": [63, 63, 6, 12, 24, 24, 24, 16],
      "8": [30, 63, 51, 62, 51, 51, 30, 28],
      "9": [30, 63, 51, 63, 3, 6, 28, 24],
      ":": [0, 12, 0, 0, 0, 0, 12, 0],
    },
  },
  {
    id: "stencil_classic_6x8",
    name: "6x8 Stencil",
    width: 6,
    height: 8,
    spacing: 1,
    previewColor: "#f2f4f7",
    previewBg: "#1f2228",
    glyphs: {
      "0": [30, 35, 51, 63, 59, 51, 26, 0],
      "1": [12, 24, 12, 8, 12, 8, 30, 0],
      "2": [30, 51, 3, 6, 8, 24, 59, 0],
      "3": [62, 3, 3, 26, 3, 3, 62, 0],
      "4": [6, 14, 26, 54, 59, 6, 2, 0],
      "5": [63, 48, 62, 3, 3, 51, 30, 0],
      "6": [14, 8, 48, 62, 51, 51, 26, 0],
      "7": [63, 3, 6, 8, 24, 24, 24, 0],
      "8": [30, 51, 51, 30, 51, 51, 26, 0],
      "9": [30, 35, 51, 27, 3, 2, 28, 0],
      ":": [0, 12, 0, 0, 0, 0, 12, 0],
    },
  },
  {
    id: "wire_lcd_6x8",
    name: "6x8 Wire LCD",
    width: 6,
    height: 8,
    spacing: 1,
    previewColor: "#87ebff",
    previewBg: "#0f2430",
    glyphs: {
      "0": [30, 51, 33, 33, 33, 33, 51, 30],
      "1": [12, 20, 12, 12, 12, 12, 12, 63],
      "2": [30, 51, 3, 6, 12, 24, 48, 63],
      "3": [62, 3, 3, 14, 3, 3, 3, 62],
      "4": [6, 10, 22, 38, 57, 6, 6, 6],
      "5": [63, 48, 48, 62, 3, 3, 51, 30],
      "6": [14, 24, 48, 46, 51, 51, 51, 30],
      "7": [63, 3, 6, 12, 24, 24, 24, 24],
      "8": [30, 51, 51, 30, 51, 51, 51, 30],
      "9": [30, 51, 51, 29, 3, 6, 12, 56],
      ":": [0, 12, 0, 0, 0, 0, 12, 0],
    },
  },
  {
    id: "frame_lcd_7x9",
    name: "7x9 Frame LCD",
    width: 7,
    height: 9,
    spacing: 1,
    previewColor: "#caff73",
    previewBg: "#18240f",
    glyphs: {
      "0": [30, 30, 115, 97, 117, 97, 97, 115, 30],
      "1": [12, 12, 20, 12, 46, 12, 12, 12, 127],
      "2": [30, 30, 115, 3, 22, 12, 24, 112, 127],
      "3": [126, 126, 3, 3, 46, 3, 3, 3, 126],
      "4": [6, 6, 10, 22, 118, 121, 6, 6, 6],
      "5": [127, 79, 80, 80, 126, 3, 3, 115, 30],
      "6": [14, 14, 24, 112, 94, 83, 83, 115, 30],
      "7": [127, 125, 3, 6, 46, 24, 24, 24, 24],
      "8": [30, 30, 115, 115, 30, 115, 83, 115, 30],
      "9": [30, 30, 115, 115, 63, 3, 6, 12, 120],
      ":": [0, 24, 0, 0, 0, 0, 0, 24, 0],
    },
  },
  {
    id: "slant_retro_7x7",
    name: "7x7 Slant Retro",
    width: 7,
    height: 7,
    spacing: 1,
    previewColor: "#ffa46b",
    previewBg: "#2a160d",
    glyphs: {
      "0": [31, 34, 38, 42, 100, 68, 124],
      "1": [6, 28, 12, 12, 24, 24, 124],
      "2": [31, 2, 2, 62, 64, 64, 124],
      "3": [31, 2, 2, 62, 4, 4, 124],
      "4": [17, 34, 34, 62, 4, 4, 4],
      "5": [31, 32, 32, 62, 4, 4, 124],
      "6": [31, 32, 32, 62, 68, 68, 124],
      "7": [31, 2, 2, 2, 4, 4, 4],
      "8": [31, 34, 34, 62, 68, 68, 124],
      "9": [31, 34, 34, 62, 4, 4, 124],
      ":": [0, 24, 0, 0, 0, 24, 0],
    },
  },
  {
    id: "prism_seg_6x8",
    name: "6x8 Prism Seg",
    width: 6,
    height: 8,
    spacing: 1,
    previewColor: "#dba2ff",
    previewBg: "#241037",
    glyphs: {
      "0": [31, 31, 17, 34, 42, 34, 62, 28],
      "1": [1, 1, 1, 2, 10, 2, 2, 0],
      "2": [31, 15, 1, 62, 56, 32, 62, 28],
      "3": [31, 15, 1, 54, 26, 2, 62, 28],
      "4": [17, 17, 17, 62, 26, 2, 2, 0],
      "5": [31, 30, 16, 54, 26, 2, 62, 28],
      "6": [31, 30, 16, 62, 58, 34, 62, 28],
      "7": [31, 15, 1, 2, 10, 2, 2, 0],
      "8": [31, 31, 17, 62, 58, 34, 62, 28],
      "9": [31, 31, 17, 54, 26, 2, 62, 28],
      ":": [0, 12, 0, 0, 0, 0, 12, 0],
    },
  },
  {
    id: "arc_round_5x7",
    name: "5x7 Arc Round",
    width: 5,
    height: 7,
    spacing: 1,
    previewColor: "#72ffd2",
    previewBg: "#0d241d",
    glyphs: {
      "0": [14, 18, 18, 18, 18, 30, 12],
      "1": [12, 24, 24, 8, 8, 30, 28],
      "2": [14, 18, 18, 6, 8, 31, 30],
      "3": [30, 18, 14, 2, 2, 30, 28],
      "4": [6, 12, 20, 30, 22, 6, 4],
      "5": [31, 18, 28, 18, 18, 30, 12],
      "6": [14, 16, 28, 18, 18, 30, 12],
      "7": [31, 18, 6, 8, 8, 12, 8],
      "8": [14, 18, 30, 18, 18, 30, 12],
      "9": [14, 18, 18, 30, 2, 14, 12],
      ":": [0, 4, 0, 0, 0, 4, 0],
    },
  },
  {
    id: "compact_minimal_4x6",
    name: "4x6 Compact",
    width: 4,
    height: 6,
    spacing: 1,
    previewColor: "#ffd86b",
    previewBg: "#241a08",
    glyphs: {
      "0": [15, 15, 13, 13, 13, 15],
      "1": [2, 2, 14, 2, 2, 15],
      "2": [15, 15, 1, 15, 12, 15],
      "3": [15, 15, 1, 15, 1, 15],
      "4": [13, 13, 13, 15, 1, 1],
      "5": [15, 15, 12, 15, 1, 15],
      "6": [15, 15, 12, 15, 13, 15],
      "7": [15, 15, 1, 2, 2, 2],
      "8": [15, 15, 13, 15, 13, 15],
      "9": [15, 15, 13, 15, 1, 15],
      ":": [0, 4, 0, 0, 4, 0],
    },
  },
  {
    id: "pixel_rail_5x8",
    name: "5x8 Pixel Rail",
    width: 5,
    height: 8,
    spacing: 1,
    previewColor: "#ff799c",
    previewBg: "#2a0f18",
    glyphs: {
      "0": [31, 31, 31, 25, 25, 25, 25, 31],
      "1": [6, 6, 30, 30, 22, 6, 6, 31],
      "2": [31, 31, 17, 1, 31, 25, 24, 31],
      "3": [31, 31, 31, 1, 31, 17, 1, 31],
      "4": [25, 25, 25, 31, 31, 17, 1, 1],
      "5": [31, 31, 25, 24, 31, 17, 1, 31],
      "6": [31, 31, 31, 24, 31, 25, 25, 31],
      "7": [31, 31, 17, 15, 7, 6, 6, 6],
      "8": [31, 31, 25, 25, 31, 25, 25, 31],
      "9": [31, 31, 31, 25, 31, 17, 1, 31],
      ":": [0, 4, 0, 0, 0, 0, 4, 0],
    },
  },
];

const CLOCK_FONT_MAP = CLOCK_FONT_OPTIONS.reduce((result, option) => {
  result[option.id] = option;
  return result;
}, {});

function getBase5x7Rows(char) {
  const columns = font5x7[char];
  if (!columns) {
    return null;
  }
  const rows = [];
  for (let row = 0; row < 7; row++) {
    let bits = 0;
    for (let col = 0; col < columns.length; col++) {
      bits <<= 1;
      if (columns[col] & (1 << row)) {
        bits |= 1;
      }
    }
    rows.push(bits);
  }
  return rows;
}

function rowsToMatrix(rows, width) {
  return rows.map((rowBits) => {
    const row = [];
    for (let col = 0; col < width; col++) {
      const mask = 1 << (width - col - 1);
      row.push(rowBits & mask ? 1 : 0);
    }
    return row;
  });
}

function scaleMatrix(matrix, width, height) {
  const sourceHeight = matrix.length;
  const sourceWidth = matrix[0].length;
  const scaled = [];

  for (let y = 0; y < height; y++) {
    const sourceY = Math.floor((y * sourceHeight) / height);
    const row = [];
    for (let x = 0; x < width; x++) {
      const sourceX = Math.floor((x * sourceWidth) / width);
      row.push(matrix[sourceY][sourceX] ? 1 : 0);
    }
    scaled.push(row);
  }

  return scaled;
}

function matrixToRows(matrix) {
  return matrix.map((row) =>
    row.reduce((bits, pixel) => (bits << 1) | (pixel ? 1 : 0), 0),
  );
}

function createDashRows(width, height) {
  const rows = new Array(height).fill(0);
  rows[Math.floor(height / 2)] = (1 << width) - 1;
  return rows;
}

function createEmptyRows(height) {
  return new Array(height).fill(0);
}

function getGeneratedRows(font, char) {
  if (char === " ") {
    return createEmptyRows(font.height);
  }
  if (char === "-") {
    return createDashRows(font.width, font.height);
  }

  const normalizedChar = /^[a-z]$/.test(char) ? char.toUpperCase() : char;
  let baseRows = null;
  let baseWidth = 5;

  if (font.width === 3 && font.height === 5 && font3x5[normalizedChar]) {
    baseRows = font3x5[normalizedChar];
    baseWidth = 3;
  } else {
    baseRows = getBase5x7Rows(normalizedChar);
    baseWidth = 5;
  }

  if (!baseRows) {
    return createEmptyRows(font.height);
  }

  const matrix = rowsToMatrix(baseRows, baseWidth);
  const scaledMatrix = scaleMatrix(matrix, font.width, font.height);
  return matrixToRows(scaledMatrix);
}

export function getClockFontOptions() {
  return CLOCK_FONT_OPTIONS;
}

export function getClockFont(fontId) {
  return CLOCK_FONT_MAP[fontId];
}

export function getClockGlyphRows(fontId, char) {
  const font = getClockFont(fontId);
  const directGlyph = font.glyphs[char];
  if (directGlyph) {
    return directGlyph;
  }
  return getGeneratedRows(font, char);
}

export function drawClockCharToPixels(
  char,
  x,
  y,
  color,
  pixelMap,
  fontId,
  size = 1,
) {
  const font = getClockFont(fontId);
  const rows = getClockGlyphRows(fontId, char);

  for (let row = 0; row < font.height; row++) {
    const bits = rows[row];
    for (let col = 0; col < font.width; col++) {
      const mask = 1 << (font.width - col - 1);
      if (!(bits & mask)) {
        continue;
      }
      for (let sy = 0; sy < size; sy++) {
        for (let sx = 0; sx < size; sx++) {
          const px = x + col * size + sx;
          const py = y + row * size + sy;
          if (px >= 0 && px < 64 && py >= 0 && py < 64) {
            pixelMap.set(`${px},${py}`, color);
          }
        }
      }
    }
  }

  return (font.width + font.spacing) * size;
}

export function getClockTextWidth(text, fontId, size = 1) {
  const font = getClockFont(fontId);
  if (!text || text.length === 0) {
    return 0;
  }
  return text.length * (font.width + font.spacing) * size - font.spacing * size;
}

export function getClockTextHeight(fontId, size = 1) {
  const font = getClockFont(fontId);
  return font.height * size;
}

export function drawClockTextToPixels(
  text,
  x,
  y,
  color,
  pixelMap,
  fontId,
  size = 1,
  align = "left",
) {
  const totalWidth = getClockTextWidth(text, fontId, size);
  let startX = x;
  if (align === "center") {
    startX = x - Math.floor(totalWidth / 2);
  } else if (align === "right") {
    startX = x - totalWidth;
  }

  let currentX = startX;
  for (let i = 0; i < text.length; i++) {
    currentX += drawClockCharToPixels(
      text[i],
      currentX,
      y,
      color,
      pixelMap,
      fontId,
      size,
    );
  }
}

export function drawTinyCharToPixels(char, x, y, color, pixelMap, size = 1) {
  return drawClockCharToPixels(
    char,
    x,
    y,
    color,
    pixelMap,
    "minimal_3x5",
    size,
  );
}

export function drawTinyTextToPixels(
  text,
  x,
  y,
  color,
  pixelMap,
  size = 1,
  align = "left",
) {
  drawClockTextToPixels(
    text,
    x,
    y,
    color,
    pixelMap,
    "minimal_3x5",
    size,
    align,
  );
}

export function getTinyTextWidth(text, size = 1) {
  return getClockTextWidth(text, "minimal_3x5", size);
}

export function hexToRgb(hex) {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result
    ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16),
      }
    : { r: 255, g: 255, b: 255 };
}

export function drawCharToPixels(char, x, y, color, size, pixelMap) {
  return drawClockCharToPixels(
    char,
    x,
    y,
    color,
    pixelMap,
    "classic_5x7",
    size,
  );
}

export function getTextWidth(text, size) {
  return getClockTextWidth(text, "classic_5x7", size);
}

export function drawTextToPixels(
  text,
  x,
  y,
  color,
  size,
  pixelMap,
  align = "left",
) {
  drawClockTextToPixels(
    text,
    x,
    y,
    color,
    pixelMap,
    "classic_5x7",
    size,
    align,
  );
}

export function getCurrentTimeText(showSeconds, hourFormat) {
  const now = new Date();
  let hours = now.getHours();

  if (hourFormat === 12) {
    hours = hours % 12;
    if (hours === 0) {
      hours = 12;
    }
  }

  const hourText = String(hours).padStart(2, "0");
  const minuteText = String(now.getMinutes()).padStart(2, "0");

  if (showSeconds) {
    const secondText = String(now.getSeconds()).padStart(2, "0");
    return `${hourText}:${minuteText}:${secondText}`;
  }

  return `${hourText}:${minuteText}`;
}

export function getCurrentDateText() {
  const now = new Date();
  const month = String(now.getMonth() + 1).padStart(2, "0");
  const date = String(now.getDate()).padStart(2, "0");
  return `${month}-${date}`;
}

export function getCurrentWeekText() {
  const weekDays = ["SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"];
  const now = new Date();
  return weekDays[now.getDay()];
}

export function drawImageToPixels(imageData, x, y, width, height, pixelMap) {
  return Promise.resolve();
}
