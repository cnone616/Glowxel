<template>
  <div class="design-system-page">
    <div class="container">
      <div class="ds-shell">
        <section class="ds-page-head">
          <div class="ds-page-head__brand">
            <BrandLogo variant="page" />
            <div class="ds-page-head__copy">
              <span class="ds-page-head__eyebrow">光格像素工坊组件规范站</span>
              <h1 class="ds-page-head__title">官网内正式组件页</h1>
              <p class="ds-page-head__desc">只保留正式组件覆盖、状态和规则，不再堆多余解释块。</p>
            </div>
          </div>

          <div class="ds-page-head__meta">
            <span class="ds-page-head__meta-label">当前基线</span>
            <strong class="ds-page-head__meta-title">上一版真实结构 + 更轻阴影</strong>
            <p class="ds-page-head__meta-copy">左侧切组，右侧直接看正式组件与 uniapp 约束。</p>
          </div>
        </section>

        <div class="ds-workspace">
          <aside class="ds-sidebar">
            <div class="ds-sidebar__head">
              <span class="ds-sidebar__eyebrow">分组导航</span>
              <h2 class="ds-sidebar__title">组件目录</h2>
              <p class="ds-sidebar__desc">优先补覆盖，不讲空话。</p>
            </div>

            <DesignSystemTabs v-model="activeTab" :tabs="tabs" />
          </aside>

          <div class="ds-content">
            <section class="ds-content__intro">
              <span class="ds-content__eyebrow">当前分组</span>
              <h2 class="ds-content__title">{{ currentTab.label }}</h2>
              <p class="ds-content__desc">{{ currentTab.description }}</p>
            </section>

            <DesignSystemOverviewPanel v-if="isOverviewPanel" :section-key="activeTab" />
            <DesignSystemLayoutPanel v-else-if="isLayoutPanel" :section-key="activeTab" />
            <DesignSystemControlsPanel v-else-if="isControlsPanel" :section-key="activeTab" />
            <DesignSystemFeedbackPanel v-else :section-key="activeTab" />
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, ref } from "vue";
import BrandLogo from "@/components/BrandLogo.vue";
import DesignSystemTabs from "@/components/design-system/DesignSystemTabs.vue";
import DesignSystemOverviewPanel from "@/components/design-system/DesignSystemOverviewPanel.vue";
import DesignSystemLayoutPanel from "@/components/design-system/DesignSystemLayoutPanel.vue";
import DesignSystemControlsPanel from "@/components/design-system/DesignSystemControlsPanel.vue";
import DesignSystemFeedbackPanel from "@/components/design-system/DesignSystemFeedbackPanel.vue";
import "@/assets/styles/design-system.css";

const tabs = [
  { key: "overview", label: "总览", description: "先看正式色、图标和壳层语法。" },
  { key: "colors", label: "色彩与风格", description: "主操作黄、编辑蓝和四类状态色统一收口。" },
  { key: "icons", label: "图标与底板", description: "图标、底板和品牌语法在这里集中看。" },
  { key: "headers", label: "头部与导航", description: "官网导航、顶部 tabs、页头和列表入口都在这一组。" },
  { key: "buttons", label: "按钮系统", description: "按钮尺寸、阴影和语义 modifier 全部可复用。" },
  { key: "controls", label: "控件系统", description: "输入、选择、开关、滑块、步进和勾选组直接可操作。" },
  { key: "lists", label: "列表与卡片", description: "入口卡、列表行和空状态继续沿用旧结构。" },
  { key: "skeletons", label: "骨架与加载", description: "骨架、彩色 loader 和动作轻遮罩统一看。" },
  { key: "feedback", label: "弹窗与反馈", description: "Toast、Dialog 和状态反馈全部回到标准语义。" },
  { key: "rules", label: "统一规则", description: "集中看 uniapp 已锁定的样式硬约束。" },
];

const activeTab = ref("overview");
const overviewTabKeys = ["overview", "colors", "icons"];
const layoutTabKeys = ["headers", "lists"];
const controlsTabKeys = ["buttons", "controls"];

const isOverviewPanel = computed(() => overviewTabKeys.includes(activeTab.value));
const isLayoutPanel = computed(() => layoutTabKeys.includes(activeTab.value));
const isControlsPanel = computed(() => controlsTabKeys.includes(activeTab.value));
const currentTab = computed(() => {
  const matchedTab = tabs.find((item) => item.key === activeTab.value);
  if (matchedTab) {
    return matchedTab;
  }
  return tabs[0];
});
</script>

<style scoped>
.design-system-page {
  padding: 24px 0 48px;
}
</style>
