const serverLogger = require("../services/serverLogger");

function captureResponseBody(req, res, next) {
  const originalJson = res.json.bind(res);

  res.json = (body) => {
    res.locals.responseBody = body;
    return originalJson(body);
  };

  next();
}

function responseLogger(req, res, next) {
  const startedAt = Date.now();

  res.on("finish", () => {
    const responseBody = res.locals.responseBody;
    const responseCode =
      typeof responseBody?.code === "number" ? responseBody.code : res.statusCode;
    const payload = {
      method: req.method,
      path: req.originalUrl,
      statusCode: res.statusCode,
      responseCode,
      durationMs: Date.now() - startedAt,
      ip: serverLogger.getClientIp(req),
      userId: req.user?.id || null,
      message: responseBody?.message || null,
    };

    if (res.statusCode >= 500 || responseCode >= 500) {
      serverLogger.error("api_response_failed", payload);
      return;
    }

    if (res.statusCode >= 400 || responseCode >= 400) {
      serverLogger.warn("api_response_warn", payload);
    }
  });

  next();
}

module.exports = {
  captureResponseBody,
  responseLogger,
};
