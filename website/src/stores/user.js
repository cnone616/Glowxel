import { defineStore } from "pinia";
import { userAPI } from "@/api/index.js";
import {
  clearStoredSession,
  getStoredAuthToken,
  getStoredLoginTime,
  markLoginTime,
  readStoredCurrentUser,
  writeStoredAuthToken,
  writeStoredCurrentUser,
} from "@/utils/session.js";

export const useUserStore = defineStore("user", {
  state: () => ({
    initialized: false,
    isLoggedIn: false,
    currentUser: null,
    loading: false,
  }),
  getters: {
    userId(state) {
      if (state.currentUser != null && typeof state.currentUser.id !== "undefined") {
        return state.currentUser.id;
      }
      return null;
    },
    displayName(state) {
      if (
        state.currentUser != null &&
        typeof state.currentUser.name === "string" &&
        state.currentUser.name.length > 0
      ) {
        return state.currentUser.name;
      }
      return "未登录用户";
    },
  },
  actions: {
    loadFromStorage() {
      const storedToken = getStoredAuthToken();
      this.isLoggedIn = typeof storedToken === "string" && storedToken.length > 0;

      this.currentUser = readStoredCurrentUser();

      if (this.isLoggedIn && getStoredLoginTime() == null) {
        markLoginTime();
      }
    },
    async init() {
      if (this.initialized) {
        return;
      }
      this.loadFromStorage();
      if (this.isLoggedIn) {
        await this.fetchProfile();
      }
      this.initialized = true;
    },
    applySession(token, user) {
      writeStoredAuthToken(token);
      writeStoredCurrentUser(user);
      markLoginTime();
      this.isLoggedIn = true;
      this.currentUser = user;
      this.initialized = true;
    },
    async fetchProfile() {
      if (!this.isLoggedIn) {
        return;
      }
      this.loading = true;
      try {
        const response = await userAPI.getProfile();
        if (response.success) {
          if (response.data != null && response.data.user != null) {
            this.currentUser = response.data.user;
          } else if (response.data != null) {
            this.currentUser = response.data;
          }
          if (this.currentUser != null) {
            writeStoredCurrentUser(this.currentUser);
            markLoginTime();
          }
          return;
        }
        if (response.code === 401) {
          this.logout();
        }
      } finally {
        this.loading = false;
      }
    },
    async loginWithAdmin(payload) {
      this.loading = true;
      try {
        const response = await userAPI.adminLogin(payload);
        if (
          response.success &&
          response.data != null &&
          typeof response.data.token === "string" &&
          response.data.user != null
        ) {
          this.applySession(response.data.token, response.data.user);
        }
        return response;
      } finally {
        this.loading = false;
      }
    },
    async updateProfile(payload) {
      const response = await userAPI.updateProfile(payload);
      if (response.success) {
        if (response.data != null && response.data.user != null) {
          this.currentUser = response.data.user;
        } else if (response.data != null) {
          this.currentUser = response.data;
        }
        if (this.currentUser != null) {
          writeStoredCurrentUser(this.currentUser);
          markLoginTime();
        }
      }
      return response;
    },
    logout() {
      this.isLoggedIn = false;
      this.currentUser = null;
      clearStoredSession();
    },
  },
});
