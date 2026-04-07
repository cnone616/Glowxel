import { challengeAPI, templateAPI } from "../../../api/index.js";
import { ARTKAL_COLORS_FULL } from "../../../data/artkal-colors-full.js";
import { PERLER_BOARD_SIZE } from "../../../constants/perler.js";

export default {
  methods: {
    attachChallengeToProject(projectId) {
      if (!this.challengeId) {
        return;
      }

      this.projectStore.updateProject(projectId, {
        challengeId: this.challengeId,
      });
    },

    getPaddedSize(value) {
      return Math.ceil(value / PERLER_BOARD_SIZE) * PERLER_BOARD_SIZE;
    },

    parseTemplateSize(sizeText) {
      const matched = /^(\d+)x(\d+)$/i.exec(sizeText);
      if (!matched) {
        throw new Error("invalid template size");
      }

      return {
        width: Number(matched[1]),
        height: Number(matched[2]),
      };
    },

    buildPixelsMap(pixelObject) {
      if (
        !pixelObject ||
        Array.isArray(pixelObject) ||
        typeof pixelObject !== "object"
      ) {
        throw new Error("invalid template pixels");
      }

      return new Map(Object.entries(pixelObject));
    },

    buildPaletteCodes(pixels) {
      const hexColors = [...new Set(Array.from(pixels.values()))];

      return hexColors.map((hex) => {
        const matchedColor = ARTKAL_COLORS_FULL.find(
          (color) => color.hex.toUpperCase() === hex.toUpperCase(),
        );

        if (!matchedColor) {
          throw new Error(`unknown artkal color: ${hex}`);
        }

        return matchedColor.code;
      });
    },

    async loadTemplateProject() {
      uni.showLoading({ title: "加载模板..." });

      try {
        const res = await templateAPI.getTemplateById(this.templateId);
        if (!(res.success && res.data && res.data.template && res.data.pixels)) {
          throw new Error("template load failed");
        }

        const template = res.data.template;
        const size = this.parseTemplateSize(template.size);
        const pixels = this.buildPixelsMap(res.data.pixels);
        const palette = this.buildPaletteCodes(pixels);

        this.name = template.name;
        this.customWidth = size.width;
        this.customHeight = size.height;
        this.selectedColors = new Set(palette);
        this.previewPixels = pixels;
        this.previewUrl = template.image_url;

        const paddedWidth = this.getPaddedSize(size.width);
        const paddedHeight = this.getPaddedSize(size.height);
        const projectId = this.projectStore.addProject(
          template.name,
          size.width,
          size.height,
          palette.length,
          palette,
          paddedWidth,
          paddedHeight,
          template.image_url,
        );

        if (!projectId) {
          throw new Error("project create failed");
        }

        this.projectStore.saveProjectPixels(projectId, pixels);

        try {
          await templateAPI.useTemplate(this.templateId);
        } catch (error) {
          console.error("更新模板使用次数失败:", error);
        }

        uni.hideLoading();
        this.toast.showSuccess("模板已载入");
        uni.redirectTo({
          url: `/pages/overview/overview?id=${projectId}`,
        });
      } catch (error) {
        uni.hideLoading();
        console.error("加载模板失败:", error);
        this.toast.showError("模板加载失败");
        setTimeout(() => {
          uni.navigateBack();
        }, 500);
      }
    },

    async loadChallengeInfo() {
      uni.showLoading({ title: "加载挑战..." });

      try {
        const res = await challengeAPI.getChallengeById(this.challengeId);
        if (!(res.success && res.data && res.data.challenge)) {
          throw new Error("challenge load failed");
        }

        this.name = res.data.challenge.title;
        uni.hideLoading();
        this.toast.showSuccess("已载入挑战");
      } catch (error) {
        uni.hideLoading();
        console.error("加载挑战失败:", error);
        this.toast.showError("挑战加载失败");
        setTimeout(() => {
          uni.navigateBack();
        }, 500);
      }
    },
  },
};
