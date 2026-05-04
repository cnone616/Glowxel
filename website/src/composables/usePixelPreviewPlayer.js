import { onBeforeUnmount, ref } from "vue";

function clonePixelMap(map) {
  if (map instanceof Map) {
    return new Map(map);
  }
  return new Map();
}

export function usePixelPreviewPlayer() {
  const currentPixels = ref(new Map());
  const frameIndex = ref(0);
  const sequenceMaps = ref([]);
  const sequenceDelays = ref([]);
  let timerId = null;

  function stop() {
    if (timerId !== null) {
      window.clearTimeout(timerId);
      timerId = null;
    }
  }

  function scheduleNextFrame() {
    stop();

    if (!Array.isArray(sequenceMaps.value) || sequenceMaps.value.length <= 1) {
      return;
    }

    const safeDelays = Array.isArray(sequenceDelays.value) ? sequenceDelays.value : [];
    const delay =
      frameIndex.value < safeDelays.length && Number.isFinite(Number(safeDelays[frameIndex.value]))
        ? Math.max(16, Math.round(Number(safeDelays[frameIndex.value])))
        : 120;

    timerId = window.setTimeout(() => {
      if (!Array.isArray(sequenceMaps.value) || sequenceMaps.value.length === 0) {
        currentPixels.value = new Map();
        frameIndex.value = 0;
        stop();
        return;
      }

      const nextIndex =
        frameIndex.value + 1 >= sequenceMaps.value.length ? 0 : frameIndex.value + 1;
      frameIndex.value = nextIndex;
      currentPixels.value = clonePixelMap(sequenceMaps.value[nextIndex]);
      scheduleNextFrame();
    }, delay);
  }

  function playSequence(sequence) {
    const maps = Array.isArray(sequence?.maps) ? sequence.maps : [];
    const delays = Array.isArray(sequence?.delays) ? sequence.delays : [];

    sequenceMaps.value = maps;
    sequenceDelays.value = delays;
    frameIndex.value = 0;

    if (maps.length === 0) {
      currentPixels.value = new Map();
      stop();
      return;
    }

    currentPixels.value = clonePixelMap(maps[0]);
    scheduleNextFrame();
  }

  function setSingleFrame(map) {
    sequenceMaps.value = [];
    sequenceDelays.value = [];
    frameIndex.value = 0;
    currentPixels.value = clonePixelMap(map);
    stop();
  }

  function snapshot() {
    return clonePixelMap(currentPixels.value);
  }

  onBeforeUnmount(() => {
    stop();
  });

  return {
    currentPixels,
    frameIndex,
    playSequence,
    setSingleFrame,
    snapshot,
    stop,
  };
}
