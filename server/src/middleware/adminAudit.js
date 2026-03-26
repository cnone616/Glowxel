const adminAuditService = require("../services/adminAuditService");
const serverLogger = require("../services/serverLogger");

function resolveAuditResource(req) {
  const segments = req.path.split("/").filter(Boolean);
  return segments[0] || "unknown";
}

function resolveAuditTargetId(req) {
  const segments = req.path.split("/").filter(Boolean);
  return segments[1] || null;
}

function adminAudit(req, res, next) {
  if (["GET", "HEAD", "OPTIONS"].includes(req.method)) {
    next();
    return;
  }

  res.on("finish", () => {
    if (!req.user?.id) {
      return;
    }

    const responseBody = res.locals.responseBody;
    const payload = {
      actorId: req.user.id,
      actorRole: req.user.role || "admin",
      method: req.method,
      path: req.originalUrl,
      statusCode: res.statusCode,
      responseCode:
        typeof responseBody?.code === "number" ? responseBody.code : res.statusCode,
      resource: resolveAuditResource(req),
      targetId: resolveAuditTargetId(req),
      ip: serverLogger.getClientIp(req),
      body: adminAuditService.sanitizeBody(req.body),
      file:
        req.file && req.file.originalname
          ? {
              originalname: req.file.originalname,
              mimetype: req.file.mimetype,
              size: req.file.size,
            }
          : null,
      message: responseBody?.message || null,
    };

    adminAuditService.writeAdminAuditLog(payload);
  });

  next();
}

module.exports = {
  adminAudit,
};
