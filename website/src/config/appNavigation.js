export const publicNavigation = [
  { key: "home", label: "首页", to: "/" },
  { key: "community", label: "社区", to: "/community" },
  { key: "templates", label: "边框", to: "/templates" },
  { key: "challenges", label: "挑战", to: "/challenges" },
  { key: "workspace", label: "工作台", to: "/workspace" },
  { key: "device", label: "设备", to: "/device-control" },
];

export const appNavigation = [
  { key: "workspace", label: "工作台", to: "/workspace" },
  { key: "create", label: "创作", to: "/create" },
  { key: "community", label: "社区", to: "/community" },
  { key: "device", label: "设备", to: "/device-control" },
  { key: "profile", label: "我的", to: "/profile" },
  { key: "settings", label: "设置", to: "/settings" },
];

export function resolveAppNavKey(path) {
  if (
    path === "/workspace" ||
    path.startsWith("/overview/") ||
    path.startsWith("/assist/") ||
    path.startsWith("/publish-project/")
  ) {
    return "workspace";
  }

  if (
    path.startsWith("/create") ||
    path.startsWith("/editor") ||
    path.startsWith("/pattern-workbench")
  ) {
    return "create";
  }

  if (
    path.startsWith("/community") ||
    path.startsWith("/gallery") ||
    path.startsWith("/artwork/") ||
    path.startsWith("/challenge") ||
    path.startsWith("/templates")
  ) {
    return "community";
  }

  if (
    path.startsWith("/device-control") ||
    path.startsWith("/device-params") ||
    path.startsWith("/ble-config") ||
    path.startsWith("/canvas-editor")
  ) {
    return "device";
  }

  if (
    path.startsWith("/profile") ||
    path.startsWith("/my-works") ||
    path.startsWith("/my-favorites") ||
    path.startsWith("/achievements") ||
    path.startsWith("/cloud-sync") ||
    path === "/followers" ||
    path === "/following"
  ) {
    return "profile";
  }

  if (path.startsWith("/settings")) {
    return "settings";
  }

  return "";
}
