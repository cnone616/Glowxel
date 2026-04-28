export const SESSION_TOKEN_KEY = "auth_token";
export const SESSION_CURRENT_USER_KEY = "currentUser";
export const SESSION_LOGIN_TIME_KEY = "loginTime";

export function writeStoredAuthToken(token) {
  localStorage.setItem(SESSION_TOKEN_KEY, token);
}

export function getStoredAuthToken() {
  const token = localStorage.getItem(SESSION_TOKEN_KEY);
  if (typeof token === "string") {
    return token;
  }
  return "";
}

export function readStoredCurrentUser() {
  const raw = localStorage.getItem(SESSION_CURRENT_USER_KEY);
  if (!(typeof raw === "string" && raw.length > 0)) {
    return null;
  }

  try {
    const parsed = JSON.parse(raw);
    if (parsed != null && typeof parsed === "object") {
      return parsed;
    }
  } catch (error) {
    return null;
  }

  return null;
}

export function writeStoredCurrentUser(user) {
  localStorage.setItem(SESSION_CURRENT_USER_KEY, JSON.stringify(user));
}

export function getStoredLoginTime() {
  const raw = localStorage.getItem(SESSION_LOGIN_TIME_KEY);
  if (!(typeof raw === "string" && raw.length > 0)) {
    return null;
  }

  const parsed = Number(raw);
  if (Number.isFinite(parsed)) {
    return parsed;
  }
  return null;
}

export function markLoginTime(timestamp = Date.now()) {
  localStorage.setItem(SESSION_LOGIN_TIME_KEY, String(timestamp));
}

export function clearStoredSession() {
  localStorage.removeItem(SESSION_TOKEN_KEY);
  localStorage.removeItem(SESSION_CURRENT_USER_KEY);
  localStorage.removeItem(SESSION_LOGIN_TIME_KEY);
}
