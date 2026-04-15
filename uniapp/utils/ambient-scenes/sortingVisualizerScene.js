import { fillRect, setPixel } from "../led-matrix-demos/common.js";

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

function createInitialValues() {
  const values = [];
  for (let index = 1; index <= 64; index += 1) {
    values.push(index);
  }
  for (let index = values.length - 1; index > 0; index -= 1) {
    const swapIndex = (index * 13 + 17) % (index + 1);
    const temp = values[index];
    values[index] = values[swapIndex];
    values[swapIndex] = temp;
  }
  return values;
}

function cloneState() {
  return {
    values: createInitialValues(),
    algorithm: 0,
    phase: 1,
    wait: 0,
    accessIndices: [],
    i: 0,
    j: 0,
    minIndex: 0,
    keyValue: 0,
    quickStack: [[0, 63]],
    quickPartitioning: false,
    quickLow: 0,
    quickHigh: 0,
    quickPivot: 0,
    quickI: -1,
    quickJ: 0,
  };
}

function resetForAlgorithm(state) {
  state.values = createInitialValues();
  state.phase = 1;
  state.wait = 0;
  state.accessIndices = [];

  if (state.algorithm === 0) {
    state.i = 0;
    state.j = 0;
    return;
  }

  if (state.algorithm === 1) {
    state.i = 0;
    state.j = 1;
    state.minIndex = 0;
    return;
  }

  if (state.algorithm === 2) {
    state.i = 1;
    state.keyValue = state.values[1];
    state.j = 0;
    return;
  }

  state.quickStack = [[0, state.values.length - 1]];
  state.quickPartitioning = false;
}

function stepBubble(state) {
  const length = state.values.length;
  if (state.i >= length - 1) {
    return true;
  }
  if (state.j < length - state.i - 1) {
    state.accessIndices = [state.j, state.j + 1];
    if (state.values[state.j] > state.values[state.j + 1]) {
      const temp = state.values[state.j];
      state.values[state.j] = state.values[state.j + 1];
      state.values[state.j + 1] = temp;
    }
    state.j += 1;
    return false;
  }
  state.j = 0;
  state.i += 1;
  return false;
}

function stepSelection(state) {
  const length = state.values.length;
  if (state.i >= length - 1) {
    return true;
  }
  if (state.j < length) {
    state.accessIndices = [state.minIndex, state.j];
    if (state.values[state.j] < state.values[state.minIndex]) {
      state.minIndex = state.j;
    }
    state.j += 1;
    return false;
  }
  state.accessIndices = [state.i, state.minIndex];
  const temp = state.values[state.minIndex];
  state.values[state.minIndex] = state.values[state.i];
  state.values[state.i] = temp;
  state.i += 1;
  state.minIndex = state.i;
  state.j = state.i + 1;
  return false;
}

function stepInsertion(state) {
  const length = state.values.length;
  if (state.i >= length) {
    return true;
  }
  state.accessIndices = [Math.max(0, state.j), state.i];
  if (state.j >= 0 && state.values[state.j] > state.keyValue) {
    state.values[state.j + 1] = state.values[state.j];
    state.j -= 1;
    return false;
  }
  state.values[state.j + 1] = state.keyValue;
  state.i += 1;
  if (state.i < length) {
    state.keyValue = state.values[state.i];
    state.j = state.i - 1;
  }
  return false;
}

function stepQuickSort(state) {
  if (!state.quickPartitioning) {
    while (state.quickStack.length > 0) {
      const range = state.quickStack.pop();
      state.quickLow = range[0];
      state.quickHigh = range[1];
      if (state.quickLow < state.quickHigh) {
        state.quickPivot = state.values[state.quickHigh];
        state.quickI = state.quickLow - 1;
        state.quickJ = state.quickLow;
        state.quickPartitioning = true;
        break;
      }
    }
    if (!state.quickPartitioning) {
      return true;
    }
  }

  if (state.quickJ <= state.quickHigh - 1) {
    state.accessIndices = [state.quickJ, state.quickHigh];
    if (state.values[state.quickJ] < state.quickPivot) {
      state.quickI += 1;
      const temp = state.values[state.quickI];
      state.values[state.quickI] = state.values[state.quickJ];
      state.values[state.quickJ] = temp;
    }
    state.quickJ += 1;
    return false;
  }

  const pivotIndex = state.quickI + 1;
  const temp = state.values[pivotIndex];
  state.values[pivotIndex] = state.values[state.quickHigh];
  state.values[state.quickHigh] = temp;
  state.quickPartitioning = false;
  state.quickStack.push([state.quickLow, pivotIndex - 1]);
  state.quickStack.push([pivotIndex + 1, state.quickHigh]);
  state.accessIndices = [pivotIndex, state.quickHigh];
  return false;
}

function stepSort(state) {
  let done = false;
  for (let step = 0; step < 8 && !done; step += 1) {
    if (state.algorithm === 0) {
      done = stepBubble(state);
    } else if (state.algorithm === 1) {
      done = stepSelection(state);
    } else if (state.algorithm === 2) {
      done = stepInsertion(state);
    } else {
      done = stepQuickSort(state);
    }
  }

  if (done) {
    state.phase = 2;
    state.accessIndices = [];
  }
}

function stepToFrame(frameIndex) {
  const state = cloneState();
  resetForAlgorithm(state);

  for (let frame = 0; frame <= frameIndex; frame += 1) {
    if (state.phase === 1) {
      stepSort(state);
    } else {
      state.wait += 1;
      if (state.wait >= 18) {
        state.algorithm = (state.algorithm + 1) % 4;
        resetForAlgorithm(state);
      }
    }
  }

  return state;
}

function buildSortingVisualizerAmbientFrame(frameMap, frameIndex) {
  fillRect(frameMap, 0, 0, 64, 64, 0, 0, 0);
  const state = stepToFrame(frameIndex);
  const activeSet = new Set(state.accessIndices);

  for (let index = 0; index < state.values.length; index += 1) {
    const value = state.values[index];
    const barHeight = Math.max(1, Math.floor((value * 64) / state.values.length));
    let color;

    if (activeSet.has(index) && state.phase === 1) {
      color = { r: 255, g: 0, b: 0 };
    } else if (state.phase === 2) {
      color = { r: 0, g: 255, b: 0 };
    } else {
      color = hslToRgb((value / state.values.length) * 360, 1, 0.5);
    }

    for (let y = 63; y >= 64 - barHeight; y -= 1) {
      setPixel(frameMap, index, y, color.r, color.g, color.b);
    }
  }
}

export { buildSortingVisualizerAmbientFrame };
