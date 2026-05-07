<template>
  <view class="design-compare-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="goBack">
        <Icon name="arrow-left-bold" :size="28" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">样式双列对比</text>
      <view class="nav-right"></view>
    </view>

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content">
      <view class="hero-card glx-panel-card">
        <text class="hero-kicker">这次只做单页双列，不做台账混排</text>
        <text class="hero-title">{{ currentPage.shortLabel }}</text>
        <text class="hero-desc">
          左边保留当前页面真实搭配，右边展示重新设计后的实际预演。现在只看这一页，不把别的页面混进来。
        </text>
        <view class="hero-stats">
          <view class="hero-stat">
            <text class="hero-stat-value">{{ pageOptions.length }}</text>
            <text class="hero-stat-label">业务页</text>
          </view>
          <view class="hero-stat">
            <text class="hero-stat-value">{{ currentPage.entries.length }}</text>
            <text class="hero-stat-label">当前页条目</text>
          </view>
          <view class="hero-stat">
            <text class="hero-stat-value">{{ currentPage.path }}</text>
            <text class="hero-stat-label">页面路径</text>
          </view>
        </view>
      </view>

      <scroll-view scroll-x class="page-tabs-scroll">
        <view class="page-tabs">
          <view
            v-for="page in pageOptions"
            :key="page.path"
            class="page-tab"
            :class="{ active: page.path === selectedPagePath }"
            @click="selectPage(page.path)"
          >
            <text class="page-tab-title">{{ page.shortLabel }}</text>
            <text class="page-tab-meta">{{ page.entries.length }} 条</text>
          </view>
        </view>
      </scroll-view>

      <view class="compare-card glx-panel-card">
        <view class="compare-head">
          <view class="compare-head-main">
            <text class="compare-title">{{ currentPage.shortLabel }} · 左当前 / 右仅调图标</text>
            <text class="compare-path">{{ currentPage.path }}</text>
          </view>
          <text class="compare-summary">共 {{ currentPage.entries.length }} 条，不折叠</text>
        </view>

        <view class="compare-grid-labels">
          <view class="compare-grid-label current">
            <text class="compare-grid-label-text">当前</text>
          </view>
          <view class="compare-grid-label proposal">
            <text class="compare-grid-label-text">只调图标</text>
          </view>
        </view>

        <view
          v-for="entry in currentPage.entries"
          :key="entry.id"
          class="compare-row"
        >
          <view class="compare-row-head">
            <view class="compare-row-copy">
              <text class="compare-row-title">.{{ entry.groupKey }}</text>
              <text class="compare-row-selector">{{ entry.selector }}</text>
            </view>
            <view class="compare-row-tags">
              <text class="compare-tag role" :class="entry.role">{{ entry.roleLabel }}</text>
              <text v-if="entry.stateLabel" class="compare-tag state">{{ entry.stateLabel }}</text>
            </view>
          </view>

          <view class="compare-grid">
            <view class="preview-column">
              <view class="preview-frame current" :style="entry.current.canvasStyle">
                <view v-if="entry.variant === 'button'" class="preview-surface button" :style="entry.current.surfaceStyle">
                  <view class="preview-icon-shell" :style="entry.current.iconShellStyle">
                    <Icon :name="entry.currentIconName" :size="22" :color="entry.current.iconColor" />
                  </view>
                  <text class="preview-title-text" :style="entry.current.titleStyle">{{ entry.previewTitle }}</text>
                </view>

                <view v-else-if="entry.variant === 'field'" class="preview-field-wrap">
                  <view class="preview-surface field" :style="entry.current.surfaceStyle">
                    <Icon :name="entry.currentIconName" :size="20" :color="entry.current.iconColor" />
                    <text class="preview-title-text" :style="entry.current.titleStyle">{{ entry.previewTitle }}</text>
                  </view>
                  <view class="preview-field-action" :style="entry.current.iconShellStyle">
                    <Icon :name="entry.currentAuxIconName" :size="18" :color="entry.current.iconColor" />
                  </view>
                </view>

                <view v-else-if="entry.variant === 'badge'" class="preview-badge-stack">
                  <view class="preview-surface badge" :style="entry.current.surfaceStyle">
                    <view class="preview-badge-dot" :style="{ background: entry.current.iconColor }"></view>
                    <text class="preview-title-text" :style="entry.current.titleStyle">{{ entry.previewTitle }}</text>
                  </view>
                  <text class="preview-desc-text" :style="entry.current.descStyle">{{ entry.previewDesc }}</text>
                </view>

                <view v-else-if="entry.variant === 'icon'" class="preview-icon-stack">
                  <view class="preview-icon-shell large" :style="entry.current.iconShellStyle">
                    <Icon :name="entry.currentIconName" :size="24" :color="entry.current.iconColor" />
                  </view>
                  <text class="preview-title-text" :style="entry.current.titleStyle">{{ entry.previewTitle }}</text>
                  <text class="preview-desc-text" :style="entry.current.descStyle">{{ entry.previewDesc }}</text>
                </view>

                <view v-else-if="entry.variant === 'text'" class="preview-text-stack">
                  <view class="preview-text-marker" :style="entry.current.railStyle"></view>
                  <text class="preview-title-text" :style="entry.current.titleStyle">{{ entry.previewTitle }}</text>
                  <text class="preview-desc-text" :style="entry.current.descStyle">{{ entry.previewDesc }}</text>
                </view>

                <view v-else class="preview-surface container" :style="entry.current.surfaceStyle">
                  <view class="preview-container-head">
                    <text class="preview-title-text" :style="entry.current.titleStyle">{{ entry.previewTitle }}</text>
                  </view>
                  <view class="preview-container-body">
                    <view class="preview-icon-shell" :style="entry.current.iconShellStyle">
                      <Icon :name="entry.currentIconName" :size="20" :color="entry.current.iconColor" />
                    </view>
                    <view class="preview-container-copy">
                      <text class="preview-title-text" :style="entry.current.titleStyle">内容区</text>
                      <text class="preview-desc-text" :style="entry.current.descStyle">{{ entry.previewDesc }}</text>
                    </view>
                  </view>
                </view>

                <text class="preview-token">{{ entry.currentToken }}</text>
              </view>
            </view>

            <view class="preview-column">
              <view class="preview-frame proposal" :style="entry.proposal.canvasStyle">
                <view v-if="entry.variant === 'button'" class="preview-surface button" :style="entry.proposal.surfaceStyle">
                  <view class="preview-icon-shell" :style="entry.proposal.iconShellStyle">
                    <Icon :name="entry.proposalIconName" :size="24" :color="entry.proposal.iconColor" />
                  </view>
                  <text class="preview-title-text" :style="entry.proposal.titleStyle">{{ entry.previewTitle }}</text>
                </view>

                <view v-else-if="entry.variant === 'field'" class="preview-field-wrap">
                  <view class="preview-surface field" :style="entry.proposal.surfaceStyle">
                    <Icon :name="entry.proposalIconName" :size="22" :color="entry.proposal.iconColor" />
                    <text class="preview-title-text" :style="entry.proposal.titleStyle">{{ entry.previewTitle }}</text>
                  </view>
                  <view class="preview-field-action" :style="entry.proposal.iconShellStyle">
                    <Icon :name="entry.proposalAuxIconName" :size="20" :color="entry.proposal.iconColor" />
                  </view>
                </view>

                <view v-else-if="entry.variant === 'badge'" class="preview-badge-stack">
                  <view class="preview-surface badge" :style="entry.proposal.surfaceStyle">
                    <view class="preview-badge-dot" :style="{ background: entry.proposal.iconColor }"></view>
                    <text class="preview-title-text" :style="entry.proposal.titleStyle">{{ entry.previewTitle }}</text>
                  </view>
                  <text class="preview-desc-text" :style="entry.proposal.descStyle">{{ entry.previewDesc }}</text>
                </view>

                <view v-else-if="entry.variant === 'icon'" class="preview-icon-stack">
                  <view class="preview-icon-shell large" :style="entry.proposal.iconShellStyle">
                    <Icon :name="entry.proposalIconName" :size="28" :color="entry.proposal.iconColor" />
                  </view>
                  <text class="preview-title-text" :style="entry.proposal.titleStyle">{{ entry.previewTitle }}</text>
                  <text class="preview-desc-text" :style="entry.proposal.descStyle">{{ entry.previewDesc }}</text>
                </view>

                <view v-else-if="entry.variant === 'text'" class="preview-text-stack">
                  <view class="preview-text-marker" :style="entry.proposal.railStyle"></view>
                  <text class="preview-title-text" :style="entry.proposal.titleStyle">{{ entry.previewTitle }}</text>
                  <text class="preview-desc-text" :style="entry.proposal.descStyle">{{ entry.previewDesc }}</text>
                </view>

                <view v-else class="preview-surface container" :style="entry.proposal.surfaceStyle">
                  <view class="preview-container-head">
                    <text class="preview-title-text" :style="entry.proposal.titleStyle">{{ entry.previewTitle }}</text>
                  </view>
                  <view class="preview-container-body">
                    <view class="preview-icon-shell" :style="entry.proposal.iconShellStyle">
                      <Icon :name="entry.proposalIconName" :size="22" :color="entry.proposal.iconColor" />
                    </view>
                    <view class="preview-container-copy">
                      <text class="preview-title-text" :style="entry.proposal.titleStyle">内容区</text>
                      <text class="preview-desc-text" :style="entry.proposal.descStyle">{{ entry.previewDesc }}</text>
                    </view>
                  </view>
                </view>

                <text class="preview-token proposal">{{ entry.proposalToken }}</text>
              </view>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import {
  STYLE_AUDIT_GROUPS,
  STYLE_AUDIT_PAGES,
} from "../../utils/styleAuditData.js";

const ROLE_LABELS = {
  badge: "徽标",
  button: "按钮",
  container: "容器",
  field: "输入",
  icon: "图标",
  other: "其他",
  text: "文字",
};

const ROLE_ORDER = {
  button: 0,
  field: 1,
  icon: 2,
  badge: 3,
  container: 4,
  text: 5,
  other: 6,
};

const EXCLUDED_PAGE_PATHS = new Set(["pages/design-preview/index"]);

function shortPageLabel(pagePath) {
  const pageName = pagePath.split("/").pop() || pagePath;
  return pageName.replace(/-/g, " ");
}

function buildPageLabel(pagePath) {
  return pagePath.replace("pages/", "").replace(/\//g, " / ");
}

function pickAuditIcon(groupKey, selector, stateText, role) {
  const source = `${groupKey} ${selector} ${stateText}`.toLowerCase();
  if (/danger|delete|remove|ashbin|logout|clear/.test(source)) return "ashbin";
  if (/search|input|field/.test(source)) return "search";
  if (/download|export/.test(source)) return "download";
  if (/edit|rename/.test(source)) return "edit";
  if (/switch|toggle/.test(source)) return "switch";
  if (/badge|tag|status|follow/.test(source)) return "bookmark";
  if (/image|picture|empty/.test(source)) return "picture";
  if (/modal|panel|card|row|item/.test(source)) return "menu";
  if (role === "text") return "edit";
  return "help";
}

function pickProposalAuditIcon(iconName) {
  const proposalMap = {
    ashbin: "delete-filling",
    edit: "edit-filling",
    favorite: "favorite-filling",
    help: "help-filling",
    history: "history-filling",
    layout: "layout-filling",
    menu: "layout-filling",
    picture: "picture-filling",
    prompt: "prompt-filling",
    setting: "setting-filling",
    success: "success-filling",
    switch: "switch-filling",
    task: "task-filling",
    time: "clock-filling",
    user: "user-filling",
    work: "work-filling",
  };
  return proposalMap[iconName] || iconName;
}

function isLightTextColor(value) {
  if (!value) return false;
  const normalized = value.toLowerCase();
  return (
    normalized.includes("#fff") ||
    normalized.includes("white") ||
    normalized.includes("255,255,255") ||
    normalized.includes("255, 255, 255")
  );
}

function normalizeBackground(backgroundColor, background) {
  const value = backgroundColor || background;
  if (!value) return "var(--tone-paper-soft)";
  const normalized = value.trim().toLowerCase();
  if (normalized === "transparent" || normalized === "none") {
    return "var(--tone-paper-soft)";
  }
  return value;
}

function normalizeBorder(border, borderColor) {
  if (border) return border;
  if (borderColor) return `2rpx solid ${borderColor}`;
  return "2rpx solid var(--nb-ink)";
}

function normalizeRadius(value) {
  return value || "0";
}

function normalizeShadow(value) {
  if (!value) return "none";
  if (value.trim().toLowerCase() === "none") return "none";
  return value;
}

function deriveAccent(textColor, background) {
  const source = `${textColor} ${background}`.toLowerCase();
  if (/coral|danger|d92d20|ff6b6b|red/.test(source)) return "var(--nb-coral)";
  if (/green|22c55e|42c7a0|88d498/.test(source)) return "var(--nb-green)";
  if (/blue|74b9ff|4f7fff|2563eb|3b82f6/.test(source)) return "var(--nb-blue)";
  if (/yellow|gold|ffd23f|f59e0b/.test(source)) return "var(--nb-yellow)";
  return "var(--nb-ink)";
}

function resolveVariant(role) {
  if (role === "button") return "button";
  if (role === "field") return "field";
  if (role === "badge") return "badge";
  if (role === "icon") return "icon";
  if (role === "text") return "text";
  if (role === "container") return "container";
  return "container";
}

function resolvePreviewTitle(source, role) {
  if (role === "button") {
    if (/danger|delete|remove|ashbin|clear/.test(source)) return "危险操作";
    if (/edit|rename/.test(source)) return "编辑操作";
    if (/follow/.test(source)) return "关注按钮";
    return "操作按钮";
  }
  if (role === "field") {
    if (/search/.test(source)) return "搜索输入";
    if (/clear/.test(source)) return "清除动作";
    return "输入控件";
  }
  if (role === "badge") {
    if (/status|sync|complete/.test(source)) return "状态徽标";
    return "信息徽标";
  }
  if (role === "icon") {
    if (/empty/.test(source)) return "空状态图标";
    return "功能图标";
  }
  if (role === "text") {
    if (/title|name|label/.test(source)) return "标题层级";
    if (/desc|meta|subtitle|hint/.test(source)) return "辅助文字";
    return "文字层级";
  }
  if (role === "container") return "卡片容器";
  return "列表区块";
}

function resolvePreviewDesc(source, role) {
  if (role === "button") return "看图标、文字、底色是否同步";
  if (role === "field") return "看输入区和尾部动作是否统一";
  if (role === "badge") return "看标签色和彩度是否清楚";
  if (role === "icon") return "看图标壳存在感和对比度";
  if (role === "text") return "看标题和辅助信息的层级关系";
  if (role === "container") return "看容器、列表项和入口语法";
  return "看默认列表结构是否稳定";
}

function buildStage(spec) {
  return {
    canvasStyle: {
      background: spec.canvasBackground,
      border: "2rpx solid var(--nb-ink)",
      borderRadius: "0",
      boxShadow: "6rpx 6rpx 0 rgba(0, 0, 0, 0.08)",
    },
    surfaceStyle: {
      background: spec.elementBackground,
      border: spec.elementBorder,
      borderRadius: spec.elementRadius,
      boxShadow: spec.elementShadow,
    },
    iconShellStyle: {
      background: spec.iconShellBackground,
      border: spec.iconShellBorder,
      borderRadius: spec.elementRadius,
    },
    iconColor: spec.iconColor,
    titleStyle: {
      color: spec.textColor,
    },
    descStyle: {
      color: spec.mutedColor,
    },
  };
}

function buildEntry(page, group, sample, index) {
  const decls = {
    color: sample[2],
    background: sample[3],
    backgroundColor: sample[4],
    border: sample[5],
    borderColor: sample[6],
    borderRadius: sample[7],
    boxShadow: sample[8],
  };
  const textColor = decls.color || "var(--nb-ink)";
  const background = normalizeBackground(decls.backgroundColor, decls.background);
  const border = normalizeBorder(decls.border, decls.borderColor);
  const radius = normalizeRadius(decls.borderRadius);
  const shadow = normalizeShadow(decls.boxShadow);
  const currentIconName = pickAuditIcon(group.key, sample[1], sample[10], group.role);
  const proposalIconName = pickProposalAuditIcon(currentIconName);
  const currentAuxIconName = "close";
  const proposalAuxIconName = "close-bold";
  const currentMutedColor = isLightTextColor(textColor)
    ? "rgba(255,255,255,0.82)"
    : "var(--text-secondary)";
  const currentStage = buildStage({
    canvasBackground: "var(--nb-surface)",
    elementBackground: background,
    elementBorder: border,
    elementRadius: radius,
    elementShadow: shadow,
    iconShellBackground: isLightTextColor(textColor)
      ? "rgba(255,255,255,0.16)"
      : "rgba(255,255,255,0.82)",
    iconShellBorder: "2rpx solid var(--nb-ink)",
    iconColor: textColor,
    textColor,
    mutedColor: currentMutedColor,
  });
  const proposalStage = {
    ...currentStage,
    canvasStyle: {
      ...currentStage.canvasStyle,
    },
    surfaceStyle: {
      ...currentStage.surfaceStyle,
    },
    iconShellStyle: {
      ...currentStage.iconShellStyle,
    },
    titleStyle: {
      ...currentStage.titleStyle,
    },
    descStyle: {
      ...currentStage.descStyle,
    },
    iconColor: currentStage.iconColor,
  };
  const proposalToken =
    group.role === "text"
      ? "无图标，本项保持原样"
      : currentIconName === proposalIconName
        ? `只检查图标居中和清晰度：${proposalIconName}`
        : `只替换图标：${currentIconName} -> ${proposalIconName}`;

  return {
    id: `${page.path}-${group.key}-${index}`,
    groupKey: group.key,
    selector: sample[1],
    role: group.role,
    roleLabel: ROLE_LABELS[group.role] || ROLE_LABELS.other,
    stateLabel: sample[10] || "",
    variant: resolveVariant(group.role),
    previewTitle: resolvePreviewTitle(source, group.role),
    previewDesc: resolvePreviewDesc(source, group.role),
    currentIconName,
    proposalIconName,
    currentAuxIconName,
    proposalAuxIconName,
    current: currentStage,
    proposal: proposalStage,
    currentToken: `字 ${decls.color || "继承"} · 底 ${background} · 边 ${border}`,
    proposalToken,
  };
}

function buildPageOptions() {
  const pages = STYLE_AUDIT_PAGES.filter(
    (page) => !EXCLUDED_PAGE_PATHS.has(page.path)
  ).map((page) => ({
    ...page,
    shortLabel: shortPageLabel(page.path),
    label: buildPageLabel(page.path),
    entries: [],
  }));

  const pageMap = new Map(pages.map((page) => [page.index, page]));

  STYLE_AUDIT_GROUPS.forEach((group) => {
    group.samples.forEach((sample, index) => {
      const page = pageMap.get(sample[0]);
      if (!page) return;
      page.entries.push(buildEntry(page, group, sample, index));
    });
  });

  pages.forEach((page) => {
    page.entries.sort((left, right) => {
      const roleDelta =
        (ROLE_ORDER[left.role] || ROLE_ORDER.other) -
        (ROLE_ORDER[right.role] || ROLE_ORDER.other);
      if (roleDelta !== 0) return roleDelta;
      return left.groupKey.localeCompare(right.groupKey);
    });
  });

  return pages;
}

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
  },
  data() {
    const pageOptions = buildPageOptions();
    const defaultPage =
      pageOptions.find((page) => page.path === "pages/library/library") ||
      pageOptions[0];

    return {
      pageOptions,
      selectedPagePath: defaultPage ? defaultPage.path : "",
    };
  },
  computed: {
    currentPage() {
      return (
        this.pageOptions.find((page) => page.path === this.selectedPagePath) ||
        this.pageOptions[0] || {
          path: "",
          label: "",
          shortLabel: "",
          entries: [],
        }
      );
    },
  },
  methods: {
    goBack() {
      uni.navigateBack();
    },
    selectPage(pagePath) {
      this.selectedPagePath = pagePath;
    },
  },
};
</script>

<style scoped>
.design-compare-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--nb-paper);
  overflow: hidden;
}

.navbar {
  background: var(--nb-surface);
  border-bottom: 2rpx solid var(--nb-ink);
}

.content {
  flex: 1;
  padding: 16rpx 20rpx 28rpx;
  box-sizing: border-box;
  overflow-y: scroll;
}

.hero-card {
  margin-bottom: 20rpx;
  padding: 24rpx;
  background: #ffffff;
  border: 2rpx solid var(--nb-ink);
  box-shadow: none;
}

.hero-kicker {
  font-size: 22rpx;
  font-weight: 700;
  color: var(--nb-blue);
}

.hero-title {
  display: block;
  margin-top: 10rpx;
  font-size: 32rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.hero-desc {
  display: block;
  margin-top: 12rpx;
  font-size: 24rpx;
  line-height: 1.6;
  color: #4a4a4a;
}

.hero-stats {
  margin-top: 18rpx;
  display: flex;
  gap: 12rpx;
}

.hero-stat {
  flex: 1;
  min-width: 0;
  padding: 14rpx 16rpx;
  background: #f5f7fa;
  border: 2rpx solid var(--nb-ink);
}

.hero-stat-value {
  display: block;
  font-size: 22rpx;
  font-weight: 700;
  color: var(--nb-ink);
  word-break: break-all;
}

.hero-stat-label {
  display: block;
  margin-top: 6rpx;
  font-size: 20rpx;
  color: #4a4a4a;
}

.page-tabs-scroll {
  margin-bottom: 18rpx;
  white-space: nowrap;
}

.page-tabs {
  display: inline-flex;
  gap: 12rpx;
  padding-bottom: 6rpx;
}

.page-tab {
  min-width: 168rpx;
  padding: 16rpx 18rpx;
  background: #ffffff;
  border: 2rpx solid var(--nb-ink);
  box-sizing: border-box;
}

.page-tab.active {
  background: #eef5ff;
  box-shadow: inset 0 -6rpx 0 var(--nb-blue);
}

.page-tab-title {
  display: block;
  font-size: 24rpx;
  font-weight: 700;
  color: var(--nb-ink);
  text-transform: capitalize;
}

.page-tab-meta {
  display: block;
  margin-top: 6rpx;
  font-size: 20rpx;
  color: #4a4a4a;
}

.compare-card {
  padding: 24rpx;
  background: #ffffff;
  border: 2rpx solid var(--nb-ink);
  box-shadow: none;
}

.compare-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16rpx;
  margin-bottom: 18rpx;
}

.compare-head-main {
  flex: 1;
  min-width: 0;
}

.compare-title {
  display: block;
  font-size: 30rpx;
  font-weight: 700;
  color: var(--nb-ink);
  text-transform: capitalize;
}

.compare-path {
  display: block;
  margin-top: 8rpx;
  font-size: 20rpx;
  color: #4a4a4a;
  word-break: break-all;
}

.compare-summary {
  font-size: 22rpx;
  color: #4a4a4a;
}

.compare-grid-labels {
  display: flex;
  gap: 14rpx;
  margin-bottom: 16rpx;
}

.compare-grid-label {
  flex: 1;
  padding: 12rpx 14rpx;
  border: 2rpx solid var(--nb-ink);
  background: #f3f4f6;
}

.compare-grid-label.proposal {
  background: #edf4ff;
}

.compare-grid-label-text {
  font-size: 22rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.compare-row {
  padding: 18rpx 0 22rpx;
  border-top: 2rpx solid rgba(0, 0, 0, 0.08);
}

.compare-row:first-of-type {
  border-top: none;
}

.compare-row-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12rpx;
  margin-bottom: 14rpx;
}

.compare-row-copy {
  flex: 1;
  min-width: 0;
}

.compare-row-title {
  display: block;
  font-size: 24rpx;
  font-weight: 700;
  color: var(--nb-ink);
  word-break: break-all;
}

.compare-row-selector {
  display: block;
  margin-top: 6rpx;
  font-size: 20rpx;
  color: #4a4a4a;
  word-break: break-all;
}

.compare-row-tags {
  display: flex;
  flex-wrap: wrap;
  justify-content: flex-end;
  gap: 8rpx;
}

.compare-tag {
  padding: 6rpx 10rpx;
  border: 2rpx solid var(--nb-ink);
  font-size: 18rpx;
  color: var(--nb-ink);
  background: #ffffff;
}

.compare-tag.state {
  background: #f2f2f2;
}

.compare-grid {
  display: flex;
  gap: 14rpx;
}

.preview-column {
  flex: 1;
  min-width: 0;
}

.preview-frame {
  min-height: 236rpx;
  padding: 16rpx;
  box-sizing: border-box;
}

.preview-surface {
  width: 100%;
  box-sizing: border-box;
}

.preview-surface.button {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 18rpx 16rpx;
}

.preview-field-wrap {
  display: flex;
  gap: 10rpx;
}

.preview-surface.field {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 10rpx;
  padding: 16rpx 14rpx;
}

.preview-field-action {
  width: 70rpx;
  height: 70rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
}

.preview-badge-stack {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.preview-surface.badge {
  display: inline-flex;
  align-items: center;
  gap: 10rpx;
  width: auto;
  padding: 14rpx 16rpx;
  max-width: 100%;
}

.preview-badge-dot {
  width: 14rpx;
  height: 14rpx;
  flex-shrink: 0;
}

.preview-icon-stack {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 12rpx;
}

.preview-icon-shell {
  width: 62rpx;
  height: 62rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
}

.preview-icon-shell.large {
  width: 92rpx;
  height: 92rpx;
}

.preview-text-stack {
  display: flex;
  flex-direction: column;
  gap: 10rpx;
}

.preview-text-marker {
  width: 28rpx;
  height: 8rpx;
}

.preview-surface.container {
  padding: 14rpx;
}

.preview-container-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 10rpx;
}

.preview-container-body {
  display: flex;
  gap: 12rpx;
  margin-top: 14rpx;
}

.preview-container-copy {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.preview-title-text {
  font-size: 24rpx;
  font-weight: 700;
  line-height: 1.4;
}

.preview-desc-text {
  font-size: 20rpx;
  line-height: 1.5;
}

.preview-token {
  display: block;
  margin-top: 14rpx;
  font-size: 18rpx;
  line-height: 1.5;
  color: #4a4a4a;
  word-break: break-all;
}

.preview-token.proposal {
  color: var(--nb-blue);
}
</style>
