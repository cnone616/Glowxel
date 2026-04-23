function isSuccessStatus(resp) {
  return resp && (resp.status === "ok" || resp.status === "success");
}

function sendAndWait(ws, data, matcher, timeoutMs) {
  return ws.sendAndWait(data, timeoutMs, matcher);
}

function matchMessageOrError(expectedMessage, extraMatcher = null) {
  return (msg) => {
    if (!msg || typeof msg !== "object") {
      return false;
    }
    if (msg.status === "error" || msg.error) {
      return true;
    }
    if (typeof extraMatcher === "function" && extraMatcher(msg)) {
      return true;
    }
    return msg.message === expectedMessage;
  };
}

async function sendFrameBinary(ws, frameIndex, pixelBytes, totalPixels) {
  if (!(pixelBytes instanceof Uint8Array)) {
    throw new Error("帧像素数据类型错误");
  }
  if (totalPixels * 5 !== pixelBytes.length) {
    throw new Error("帧像素长度不匹配");
  }

  const chunkPixels = 200;
  for (let offsetPixels = 0; offsetPixels < totalPixels; offsetPixels += chunkPixels) {
    const endPixels = Math.min(offsetPixels + chunkPixels, totalPixels);
    const startByte = offsetPixels * 5;
    const endByte = endPixels * 5;
    const chunk = pixelBytes.slice(startByte, endByte);

    await new Promise((resolve, reject) => {
      ws.socket.send({
        data: chunk.buffer,
        success: resolve,
        fail: reject,
      });
    });

    if (endPixels < totalPixels) {
      await new Promise((resolve) => setTimeout(resolve, 80));
    }
  }

  const statusResp = await sendAndWait(
    ws,
    { cmd: "frame_status", index: frameIndex },
    matchMessageOrError("frame status", (msg) => {
      if (!msg || msg.index !== frameIndex) {
        return false;
      }
      return msg.message === "frame status";
    }),
    10000,
  );

  if (!isSuccessStatus(statusResp)) {
    throw new Error("帧状态确认失败");
  }
  if (statusResp.count !== totalPixels) {
    throw new Error(`帧状态数量不匹配: 期望 ${totalPixels}，实际 ${statusResp.count}`);
  }
}

export async function uploadAnimationFrames(ws, frames, targetMode) {
  if (!Array.isArray(frames) || frames.length === 0) {
    throw new Error("动画帧不能为空");
  }
  if (typeof targetMode !== "string" || targetMode.length === 0) {
    throw new Error("目标模式不能为空");
  }

  let loadingStarted = false;

  try {
    const loadingResp = await ws.startLoading();
    if (!isSuccessStatus(loadingResp)) {
      throw new Error("启动加载动画失败");
    }
    loadingStarted = true;

    const enterResp = await ws.setMode("transferring");
    if (!isSuccessStatus(enterResp)) {
      throw new Error("进入传输模式失败");
    }

    const beginResp = await sendAndWait(
      ws,
      { cmd: "animation_begin", frameCount: frames.length },
      matchMessageOrError("ready to receive frames"),
      10000,
    );
    if (!isSuccessStatus(beginResp)) {
      throw new Error("动画初始化失败");
    }

    for (let i = 0; i < frames.length; i++) {
      const frame = frames[i];
      if (
        !frame ||
        frame.type === undefined ||
        frame.delay === undefined ||
        frame.totalPixels === undefined ||
        frame.pixels === undefined
      ) {
        throw new Error(`第 ${i + 1} 帧字段不完整`);
      }

      const beginFrameResp = await sendAndWait(
        ws,
        {
          cmd: "frame_begin",
          index: i,
          type: frame.type,
          delay: frame.delay,
          totalPixels: frame.totalPixels,
        },
        matchMessageOrError("frame initialized", (msg) => {
          return msg && msg.index === i && msg.message === "frame initialized";
        }),
        10000,
      );

      if (!isSuccessStatus(beginFrameResp)) {
        throw new Error(`第 ${i + 1} 帧初始化失败`);
      }

      await sendFrameBinary(ws, i, frame.pixels, frame.totalPixels);
    }

    const endResp = await sendAndWait(
      ws,
      { cmd: "animation_end" },
      matchMessageOrError("animation upload ready"),
      10000,
    );
    if (!isSuccessStatus(endResp)) {
      throw new Error("动画结束确认失败");
    }

    const modeResp = await ws.setMode(targetMode);
    if (!isSuccessStatus(modeResp)) {
      throw new Error("切换目标模式失败");
    }
  } finally {
    if (loadingStarted) {
      try {
        await ws.stopLoading();
      } catch (err) {
        console.error("停止设备 loading 失败:", err);
      }
    }
  }
}
