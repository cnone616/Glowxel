require("dotenv").config();
const express = require("express");
const cors = require("cors");
const path = require("path");

const { rateLimit } = require("./middleware/auth");
const {
  captureResponseBody,
  responseLogger,
} = require("./middleware/responseLogger");
const serverLogger = require("./services/serverLogger");

const app = express();

// 全局限流：每个 IP 每分钟最多 120 次请求
app.use(rateLimit(120, 60000));

// 中间件
app.use(cors());
app.use(express.json({ limit: "50mb" }));
app.use(express.urlencoded({ extended: true, limit: "50mb" }));
app.use(captureResponseBody);
app.use(responseLogger);
app.use("/uploads", express.static(path.join(__dirname, "../uploads")));

// 路由
app.use("/api/admin", require("./routes/admin"));
app.use("/api/user", require("./routes/user"));
app.use("/api/follow", require("./routes/follow"));
app.use("/api/project", require("./routes/project"));
app.use("/api/artwork", require("./routes/artwork"));
app.use("/api/like", require("./routes/like"));
app.use("/api/collect", require("./routes/collect"));
app.use("/api/comment", require("./routes/comment"));
app.use("/api/template", require("./routes/template"));
app.use("/api/challenge", require("./routes/challenge"));
app.use("/api/firmware", require("./routes/firmware"));
app.use("/api/upload", require("./routes/upload"));
app.use("/api/notification", require("./routes/notification"));

// 健康检查
app.get("/api/health", (req, res) => {
  res.json({ code: 0, message: "ok", time: new Date().toISOString() });
});

// 全局错误处理
app.use((err, req, res, next) => {
  serverLogger.error("unhandled_server_error", {
    method: req.method,
    path: req.originalUrl,
    ip: serverLogger.getClientIp(req),
    userId: req.user?.id || null,
    name: err.name,
    message: err.message,
    stack: err.stack,
  });
  res.status(500).json({ code: 500, message: "服务器内部错误" });
});

module.exports = app;

if (require.main === module) {
  const PORT = process.env.PORT || 3000;
  app.listen(PORT, "0.0.0.0", () => {
    console.log(`RenLight 后端服务运行在 http://0.0.0.0:${PORT}`);
  });
}
