export function getLayoutMetrics() {
  const systemInfo = uni.getSystemInfoSync();
  const capsule =
    typeof uni.getMenuButtonBoundingClientRect === "function"
      ? uni.getMenuButtonBoundingClientRect()
      : null;

  let statusBarHeight = 0;
  if (typeof systemInfo.statusBarHeight === "number") {
    statusBarHeight = systemInfo.statusBarHeight;
  }

  let bottomSafeArea = 0;
  if (
    systemInfo.safeAreaInsets &&
    typeof systemInfo.safeAreaInsets.bottom === "number"
  ) {
    bottomSafeArea = systemInfo.safeAreaInsets.bottom;
  }

  let capsuleHeight = 32;
  if (capsule && typeof capsule.height === "number") {
    capsuleHeight = capsule.height;
  }

  let capsuleWidth = 108;
  if (capsule && typeof capsule.width === "number") {
    capsuleWidth = capsule.width;
  }

  let capsuleRight = 24;
  if (
    capsule &&
    typeof systemInfo.windowWidth === "number" &&
    typeof capsule.right === "number"
  ) {
    capsuleRight = systemInfo.windowWidth - capsule.right;
  }

  return {
    statusBarHeight,
    bottomSafeArea,
    capsuleHeight,
    capsuleWidth,
    capsuleRight,
    headerRowHeight: Math.max(88, capsuleHeight + 44),
    titleSideWidth: Math.max(96, capsuleWidth + capsuleRight + 12),
  };
}
