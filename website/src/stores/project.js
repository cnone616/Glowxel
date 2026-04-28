import { defineStore } from "pinia";
import { projectAPI } from "@/api/index.js";

export const useProjectStore = defineStore("project", {
  state: () => ({
    projects: [],
    currentProject: null,
    currentPixels: null,
    syncStatus: null,
    loading: false,
    detailLoading: false,
  }),
  getters: {
    totalProjects(state) {
      return state.projects.length;
    },
    draftProjects(state) {
      return state.projects.filter((item) => item.status === "draft");
    },
    reviewingProjects(state) {
      return state.projects.filter((item) => item.status === "reviewing");
    },
    publishedProjects(state) {
      return state.projects.filter((item) => item.status === "published");
    },
  },
  actions: {
    async loadProjects(params = { page: 1, limit: 50 }) {
      this.loading = true;
      try {
        const response = await projectAPI.getList(params);
        if (response.success && response.data != null && Array.isArray(response.data.list)) {
          this.projects = response.data.list;
          return response;
        }
        this.projects = [];
        return response;
      } finally {
        this.loading = false;
      }
    },
    async loadProjectDetail(id) {
      this.detailLoading = true;
      try {
        const response = await projectAPI.getDetail(id);
        if (response.success && response.data != null) {
          this.currentProject = response.data.project || null;
          this.currentPixels = response.data.pixels || null;
        } else {
          this.currentProject = null;
          this.currentPixels = null;
        }
        return response;
      } finally {
        this.detailLoading = false;
      }
    },
    async removeProject(id) {
      const response = await projectAPI.remove(id);
      if (response.success) {
        this.projects = this.projects.filter((item) => item.id !== id);
        if (this.currentProject != null && this.currentProject.id === id) {
          this.currentProject = null;
          this.currentPixels = null;
        }
      }
      return response;
    },
    async loadSyncStatus() {
      const response = await projectAPI.getSyncStatus();
      if (response.success) {
        this.syncStatus = response.data || null;
      }
      return response;
    },
    async batchSyncProjects(projects) {
      const response = await projectAPI.batchSync(projects);
      if (response.success) {
        await this.loadSyncStatus();
      }
      return response;
    },
  },
});
