function isSuccessStatus(resp) {
  return resp && (resp.status === "ok" || resp.status === "success");
}

function sendAndWait(ws, data, matcher, timeoutMs) {
  return new Promise((resolve, reject) => {
    let done = false;

    const handler = (msg) => {
      if (done) {
        return;
      }
      if (!matcher(msg)) {
        return;
      }
      done = true;
      ws.offMessage(handler);
      clearTimeout(timer);
      resolve(msg);
    };

    ws.onMessage(handler);

    const timer = setTimeout(() => {
      if (done) {
        return;
      }
      done = true;
      ws.offMessage(handler);
      reject(new Error("设备响应超时"));
    }, timeoutMs);

    ws.send(data).catch((err) => {
      if (done) {
        return;
      }
      done = true;
      ws.offMessage(handler);
      clearTimeout(timer);
      reject(err);
    });
  });
}

async function sendFrameBinary(ws, frameIndex, pixelBytes, totalPixels) {
  if (!(pixelBytes instanceof Uint8Array)) {
    throw new Error("帧像素数据类型错误");
  }
  if (totalPixels * 5 !== pixelBytes.length) {
    throw new Error("帧像素长度不匹配");
  }

  if (totalPixels === 0) {
    return;
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
    (msg) => {
      if (!msg || msg.index !== frameIndex) {
        return false;
      }
      if (msg.message !== "frame status") {
        return false;
      }
      return true;
    },
    10000,
  );

  if (!isSuccessStatus(statusResp)) {
    throw new Error("帧状态确认失败");
  }
}

export async function uploadAnimationFrames(ws, frames, targetMode) {
  if (!Array.isArray(frames) || frames.length === 0) {
    throw new Error("动画帧不能为空");
  }
  if (typeof targetMode !== "string" || targetMode.length === 0) {
    throw new Error("目标模式不能为空");
  }

  const enterResp = await sendAndWait(
    ws,
    { cmd: "set_mode", mode: "transferring" },
    (msg) => msg && msg.message === "entered transferring mode",
    10000,
  );
  if (!isSuccessStatus(enterResp)) {
    throw new Error("进入传输模式失败");
  }

  const beginResp = await sendAndWait(
    ws,
    { cmd: "animation_begin", frameCount: frames.length },
    (msg) => msg && msg.message === "ready to receive frames",
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
      (msg) => msg && msg.index === i && msg.message === "frame initialized",
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
    (msg) => msg && msg.message === "animation loaded and playing",
    10000,
  );
  if (!isSuccessStatus(endResp)) {
    throw new Error("动画结束确认失败");
  }

  const modeResp = await sendAndWait(
    ws,
    { cmd: "set_mode", mode: targetMode },
    (msg) =>
      msg &&
      typeof msg.message === "string" &&
      msg.message.indexOf("switched to") === 0,
    10000,
  );
  if (!isSuccessStatus(modeResp)) {
    throw new Error("切换目标模式失败");
  }
}
