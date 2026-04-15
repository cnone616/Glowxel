<template>
  <view class="design-preview-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="goBack">
        <Icon name="arrow-left-bold" :size="28" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">样式审查总览</text>
      <view class="nav-right"></view>
    </view>

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content">
      <view class="preview-stack glx-scroll-stack">
        <view class="hero-card glx-panel-card">
          <view class="hero-topline">
            <text class="hero-tag">按 figma 审查流程收口</text>
            <text class="hero-meta">只做独立对比，不落正式页</text>
          </view>
          <text class="hero-title">这轮先不改正式页面，只把 36 个注册页的当前样子和 Figma Skill 重设计方案集中放到一页对比。</text>
          <text class="hero-desc">
            方向不是推翻你们现在的 `glx` 体系，而是先把图标、文字、背景的搭配问题完整摊开给你看。左列保留当前实际样式，右列展示按 Figma Skill 思路整理后的重设计方案，等你确认后再批量落正式页。
          </text>
          <view class="hero-chip-row">
            <text v-for="chip in heroChips" :key="chip" class="hero-chip">{{ chip }}</text>
          </view>
        </view>

        <view class="summary-grid">
          <view v-for="item in summaryCards" :key="item.label" class="summary-card glx-panel-card" :class="item.tone">
            <text class="summary-label">{{ item.label }}</text>
            <text class="summary-value">{{ item.value }}</text>
            <text class="summary-copy">{{ item.copy }}</text>
          </view>
        </view>

        <view class="section-block">
          <view class="section-header glx-section-head">
            <text class="section-title glx-section-title">整改语义色板</text>
            <text class="section-meta">不是全黑，而是按语义配色</text>
          </view>
          <view class="tone-grid">
            <view v-for="item in auditToneCards" :key="item.key" class="tone-card glx-panel-card" :class="item.tone">
              <view class="tone-card-preview" :class="item.tone">
                <view class="tone-card-icon-shell" :class="item.tone">
                  <Icon :name="item.icon" :size="24" :color="item.iconColor" />
                </view>
                <view class="tone-card-copy">
                  <text class="tone-card-title">{{ item.title }}</text>
                  <text class="tone-card-desc">{{ item.desc }}</text>
                </view>
              </view>
              <text class="tone-card-usage">{{ item.usage }}</text>
            </view>
          </view>
        </view>

        <view class="section-block">
          <view class="section-header glx-section-head">
            <text class="section-title glx-section-title">注册页面覆盖</text>
            <text class="section-meta">{{ styleAuditMeta.pageCount }} 页全量纳入</text>
          </view>
          <view class="page-audit-grid">
            <view v-for="page in styleAuditPages" :key="page.path" class="page-audit-card glx-panel-card">
              <view class="page-audit-topline">
                <text class="page-audit-label">{{ page.label }}</text>
                <text class="page-audit-count">{{ page.total }} 条</text>
              </view>
              <text class="page-audit-path">{{ page.path }}</text>
              <view class="page-audit-bg-row">
                <text class="page-audit-bg-label">页面底色</text>
                <view class="page-audit-bg-chip">
                  <view class="page-audit-bg-swatch" :style="{ background: page.bg }"></view>
                  <text class="page-audit-bg-value">{{ page.bg }}</text>
                </view>
              </view>
            </view>
          </view>
        </view>

        <view class="section-block">
          <view class="section-header glx-section-head">
            <text class="section-title glx-section-title">全量搭配台账</text>
            <text class="section-meta">左列当前，右列 Figma Skill 重设计，不漏页</text>
          </view>

          <view class="audit-toolbar glx-panel-card">
            <view class="audit-search-shell">
              <Icon name="search" :size="26" color="var(--nb-blue)" />
              <input
                v-model="auditQuery"
                class="audit-search-input"
                placeholder="按类名 / selector / 页面路径搜索"
              />
              <view v-if="auditQuery" class="audit-clear-btn" @click="auditQuery = ''">
                <Icon name="close" :size="24" color="var(--nb-ink)" />
              </view>
            </view>

            <scroll-view scroll-x class="audit-role-scroll">
              <view class="audit-role-list">
                <view
                  v-for="item in auditRoleOptions"
                  :key="item.value"
                  class="audit-role-chip"
                  :class="{ active: auditRole === item.value }"
                  @click="auditRole = item.value"
                >
                  <text class="audit-role-chip-text">{{ item.label }}</text>
                </view>
              </view>
            </scroll-view>

            <view class="audit-toolbar-foot">
              <text class="audit-toolbar-summary">
                当前展示 {{ filteredAuditGroups.length }} / {{ styleAuditGroups.length }} 个类名分组
              </text>
              <view class="audit-toolbar-actions">
                <view class="audit-toolbar-btn" @click="expandAllAuditGroups">
                  <text class="audit-toolbar-btn-text">展开全部</text>
                </view>
                <view class="audit-toolbar-btn secondary" @click="collapseAllAuditGroups">
                  <text class="audit-toolbar-btn-text">收起全部</text>
                </view>
              </view>
            </view>
          </view>

          <view class="audit-group-list">
            <view v-for="group in filteredAuditGroups" :key="group.key" class="audit-group-card glx-panel-card">
              <view class="audit-group-head" @click="toggleAuditGroup(group.key)">
                <view class="audit-group-main">
                  <text class="audit-group-title">.{{ group.key }}</text>
                  <text class="audit-group-meta">
                    {{ group.displayRole }} · {{ group.total }} 条 · {{ group.pageCount }} 页
                  </text>
                </view>
                <view class="audit-group-side">
                  <text class="audit-group-role" :class="group.role">{{ group.displayRole }}</text>
                  <text class="audit-group-toggle">{{ isAuditGroupExpanded(group.key) ? "收起" : "展开" }}</text>
                </view>
              </view>

              <view class="audit-group-pages">
                <text v-for="label in group.pageLabels.slice(0, 8)" :key="label" class="audit-page-chip">
                  {{ label }}
                </text>
                <text v-if="group.pageLabels.length > 8" class="audit-page-chip more">
                  +{{ group.pageLabels.length - 8 }}
                </text>
              </view>

              <view v-if="isAuditGroupExpanded(group.key)" class="audit-sample-list">
                <view v-for="sample in group.samples" :key="sample.id" class="audit-sample-card">
                  <view class="audit-sample-preview-grid">
                    <view class="audit-sample-preview-block">
                      <text class="audit-sample-stage-label">当前</text>
                      <view class="audit-sample-preview-shell" :style="sample.previewShellStyle">
                        <view class="audit-sample-icon-shell" :style="sample.iconShellStyle">
                          <Icon :name="sample.iconName" :size="24" :color="sample.previewColor" />
                        </view>
                        <view class="audit-sample-preview-copy">
                          <text class="audit-sample-preview-title" :style="{ color: sample.previewColor }">
                            Aa 文本 / 图标
                          </text>
                          <text class="audit-sample-preview-desc" :style="{ color: sample.previewMutedColor }">
                            当前类名的图标、文字、背景关系预演
                          </text>
                        </view>
                      </view>
                    </view>

                    <view class="audit-sample-preview-block">
                      <text class="audit-sample-stage-label proposal">Figma Skill 重设计</text>
                      <view class="audit-sample-preview-shell" :style="sample.proposalShellStyle">
                        <view class="audit-sample-icon-shell" :style="sample.proposalIconShellStyle">
                          <Icon :name="sample.iconName" :size="24" :color="sample.proposalColor" />
                        </view>
                        <view class="audit-sample-preview-copy">
                          <text class="audit-sample-preview-title" :style="{ color: sample.proposalColor }">
                            Aa 文本 / 图标
                          </text>
                          <text class="audit-sample-preview-desc" :style="{ color: sample.proposalMutedColor }">
                            {{ sample.proposalLabel }}
                          </text>
                        </view>
                      </view>
                    </view>
                  </view>

                  <view class="audit-sample-meta">
                    <text class="audit-sample-selector">{{ sample.selector }}</text>
                    <text class="audit-sample-page">{{ sample.page }}</text>
                    <view class="audit-token-row">
                      <text class="audit-token">字 {{ sample.displayColor }}</text>
                      <text class="audit-token">底 {{ sample.displayBackground }}</text>
                      <text class="audit-token">边 {{ sample.displayBorder }}</text>
                      <text v-if="sample.displayStates" class="audit-token">态 {{ sample.displayStates }}</text>
                      <text class="audit-token proposal">改 {{ sample.proposalSummary }}</text>
                    </view>
                  </view>
                </view>
              </view>
            </view>
          </view>
        </view>

        <view class="section-block">
          <view class="section-header glx-section-head">
            <text class="section-title glx-section-title">审查结论</text>
            <text class="section-meta">这轮先收 3 个重点</text>
          </view>
          <view class="finding-list">
            <view v-for="item in findings" :key="item.title" class="finding-card glx-panel-card">
              <view class="finding-topline">
                <text class="finding-title">{{ item.title }}</text>
                <text class="finding-level">{{ item.level }}</text>
              </view>
              <text class="finding-copy">{{ item.copy }}</text>
              <view class="finding-compare-row">
                <view class="finding-block current">
                  <text class="finding-block-label">当前容易出现</text>
                  <text class="finding-block-value">{{ item.current }}</text>
                </view>
                <view class="finding-block proposal">
                  <text class="finding-block-label">建议收口</text>
                  <text class="finding-block-value">{{ item.proposal }}</text>
                </view>
              </view>
            </view>
          </view>
        </view>

        <view class="section-block">
          <view class="section-header glx-section-head">
            <text class="section-title glx-section-title">设备控制预演</text>
            <text class="section-meta">主状态卡更稳，动作区更清楚</text>
          </view>
          <view class="demo-card glx-panel-card">
            <view class="demo-console-head">
              <view class="demo-device">
                <view class="demo-logo">
                  <view class="demo-logo-core"></view>
                </view>
                <view class="demo-device-copy">
                  <text class="demo-device-name">Glowxel Pro</text>
                  <view class="demo-device-status">
                    <view class="demo-status-dot"></view>
                    <text class="demo-status-text">设备在线</text>
                    <text class="demo-status-ip">192.168.31.84</text>
                  </view>
                </view>
              </view>
              <view class="demo-highlight-box">
                <text class="demo-highlight-label">当前模式</text>
                <text class="demo-highlight-value">主题时钟</text>
              </view>
            </view>

            <view class="metric-row">
              <view v-for="item in metrics" :key="item.label" class="metric-card" :class="item.tone">
                <text class="metric-label">{{ item.label }}</text>
                <text class="metric-value">{{ item.value }}</text>
              </view>
            </view>

            <view class="mode-grid">
              <view v-for="item in modeCards" :key="item.name" class="mode-card" :class="item.tone">
                <view class="mode-icon">
                  <Icon :name="item.icon" :size="26" color="var(--nb-ink)" />
                </view>
                <text class="mode-name">{{ item.name }}</text>
                <text class="mode-desc">{{ item.desc }}</text>
              </view>
            </view>

            <view class="action-list">
              <view v-for="item in actions" :key="item.title" class="action-row">
                <view class="action-icon" :class="item.tone">
                  <Icon :name="item.icon" :size="24" color="var(--nb-ink)" />
                </view>
                <view class="action-copy">
                  <text class="action-title">{{ item.title }}</text>
                  <text class="action-desc">{{ item.desc }}</text>
                </view>
                <text class="action-arrow">›</text>
              </view>
            </view>
          </view>
        </view>

        <view class="section-block">
          <view class="section-header glx-section-head">
            <text class="section-title glx-section-title">设备参数预演</text>
            <text class="section-meta">统一输入、选择、只读值盒的外轮廓</text>
          </view>
          <view class="demo-card glx-panel-card">
            <view class="param-list">
              <view v-for="item in paramRows" :key="item.label" class="param-row">
                <view class="param-copy">
                  <text class="param-label">{{ item.label }}</text>
                  <text class="param-desc">{{ item.desc }}</text>
                </view>
                <view class="param-field" :class="item.fieldType">
                  <text class="param-field-value">{{ item.value }}</text>
                  <text v-if="item.fieldType === 'select'" class="param-field-arrow">⌄</text>
                </view>
              </view>
            </view>

            <view class="field-rule-card">
              <view class="field-rule-head">
                <text class="field-rule-title">控件统一规则</text>
                <text class="field-rule-meta">同高、同边框、同内边距</text>
              </view>
              <view class="field-scale-row">
                <view class="field-scale-box input">ntp2.aliyun.com</view>
                <view class="field-scale-box select">左转 90°</view>
                <view class="field-scale-box readonly">v1.4.2</view>
              </view>
            </view>

            <view class="maintenance-list">
              <view v-for="item in maintenanceActions" :key="item.title" class="maintenance-row">
                <view class="maintenance-copy">
                  <text class="maintenance-title">{{ item.title }}</text>
                  <text class="maintenance-desc">{{ item.desc }}</text>
                </view>
                <view class="maintenance-button" :class="item.tone">
                  <text class="maintenance-button-text">{{ item.button }}</text>
                </view>
              </view>
            </view>
          </view>
        </view>

        <view class="section-block">
          <view class="section-header glx-section-head">
            <text class="section-title glx-section-title">控件预演</text>
            <text class="section-meta">滑块、开关、提示条、弹出框</text>
          </view>
          <view class="demo-card glx-panel-card">
            <view class="controls-preview-grid">
              <view class="controls-block slider-block">
                <view class="controls-block-head">
                  <text class="controls-block-title">滑块</text>
                  <text class="controls-block-meta">更像滑块，也要更顺滑</text>
                </view>
                <view class="slider-demo-row">
                  <view class="slider-demo-label-row">
                    <text class="slider-demo-label">亮度调节</text>
                    <text class="slider-demo-value">{{ previewBrightness }}%</text>
                  </view>
                  <GlxSlider
                    :value="previewBrightness"
                    :min="0"
                    :max="100"
                    @changing="handlePreviewSliderChanging('previewBrightness', $event)"
                    @change="handlePreviewSliderChange('previewBrightness', $event)"
                  />
                </view>
                <view class="slider-demo-row">
                  <view class="slider-demo-label-row">
                    <text class="slider-demo-label">夜间亮度</text>
                    <text class="slider-demo-value">{{ previewNightBrightness }}%</text>
                  </view>
                  <GlxSlider
                    :value="previewNightBrightness"
                    :min="0"
                    :max="100"
                    @changing="handlePreviewSliderChanging('previewNightBrightness', $event)"
                    @change="handlePreviewSliderChange('previewNightBrightness', $event)"
                  />
                </view>
              </view>

              <view class="controls-block switch-block">
                <view class="controls-block-head">
                  <text class="controls-block-title">开关</text>
                  <text class="controls-block-meta">开关也要和页面骨架同气质</text>
                </view>
                <view v-for="item in switchItems" :key="item.key" class="switch-row" @click="togglePreviewSwitch(item.key)">
                  <view class="switch-copy">
                    <text class="switch-label">{{ item.label }}</text>
                    <text class="switch-desc">{{ item.desc }}</text>
                  </view>
                  <GlxSwitch
                    class="glx-row-switch"
                    :checked="previewSwitches[item.key]"
                    @change="handlePreviewSwitchChange(item.key, $event)"
                  />
                </view>
              </view>
            </view>

            <view class="controls-preview-grid toast-modal-grid">
              <view class="controls-block toast-block">
                <view class="controls-block-head">
                  <text class="controls-block-title">提示条</text>
                  <text class="controls-block-meta">状态反馈要明显但不吵</text>
                </view>
                <view class="toast-preview-list">
                  <view v-for="item in toastItems" :key="item.type" class="toast-preview-item" :class="item.type">
                    <view class="toast-preview-main">
                      <text class="toast-preview-icon">{{ item.icon }}</text>
                      <text class="toast-preview-text">{{ item.text }}</text>
                    </view>
                    <text class="toast-preview-close">×</text>
                  </view>
                </view>
              </view>

              <view class="controls-block modal-block">
                <view class="controls-block-head">
                  <text class="controls-block-title">弹出框</text>
                  <text class="controls-block-meta">边框、按钮和输入框都一起看</text>
                </view>
                <view class="modal-preview-trigger" @click="showPreviewModal = true">
                  <text class="modal-preview-trigger-text">打开连接弹窗预演</text>
                </view>
                <view class="modal-preview-card">
                  <text class="modal-preview-note">点击上方按钮后，会弹出这一套连接弹窗预演。</text>
                </view>
              </view>
            </view>
          </view>
        </view>

        <view class="section-block">
          <view class="section-header glx-section-head">
            <text class="section-title glx-section-title">这轮明确不动</text>
            <text class="section-meta">防止样式调整扩大成重构</text>
          </view>
          <view class="freeze-card glx-panel-card">
            <view v-for="item in freezeItems" :key="item" class="freeze-row">
              <view class="freeze-dot"></view>
              <text class="freeze-text">{{ item }}</text>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <view v-if="showPreviewModal" class="preview-modal-overlay" @click="showPreviewModal = false">
      <view class="preview-modal-panel" @click.stop>
        <view class="preview-modal-header">
          <text class="preview-modal-title">连接设备</text>
          <view class="preview-modal-close" @click="showPreviewModal = false">
            <Icon name="close" :size="26" color="var(--nb-ink)" />
          </view>
        </view>
        <view class="preview-modal-body">
          <text class="preview-modal-desc">输入设备 IP 地址后开始连接，这里主要展示弹窗结构、输入框和按钮层级。</text>
          <view class="preview-modal-input">
            <text class="preview-modal-input-value">192.168.31.84</text>
          </view>
          <view class="preview-modal-status">
            <view class="preview-modal-status-dot"></view>
            <text class="preview-modal-status-text">等待连接</text>
          </view>
        </view>
        <view class="preview-modal-footer">
          <view class="preview-modal-button ghost" @click="showPreviewModal = false">
            <text class="preview-modal-button-text">取消</text>
          </view>
          <view class="preview-modal-button primary">
            <text class="preview-modal-button-text">连接</text>
          </view>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import GlxSlider from "../../components/GlxSlider.vue";
import GlxSwitch from "../../components/GlxSwitch.vue";
import {
  STYLE_AUDIT_GROUPS,
  STYLE_AUDIT_META,
  STYLE_AUDIT_PAGES,
} from "../../utils/styleAuditData.js";

const AUDIT_ROLE_LABELS = {
  all: "全部",
  badge: "徽标状态",
  button: "按钮操作",
  container: "卡片容器",
  field: "输入搜索",
  icon: "图标外壳",
  other: "其他",
  text: "文字层级",
};

function shortPageLabel(pagePath) {
  const segments = pagePath.split("/");
  return segments.slice(-2).join("/");
}

function pickAuditIcon(groupKey, selector, stateText, role) {
  const source = `${groupKey} ${selector} ${stateText}`.toLowerCase();
  if (/danger|delete|remove|ashbin|logout/.test(source)) return "ashbin";
  if (/search|input|field|clear/.test(source)) return "search";
  if (/download|export/.test(source)) return "download";
  if (/edit|rename/.test(source)) return "edit";
  if (/switch|toggle/.test(source)) return "switch";
  if (/badge|tag|status|follow/.test(source)) return "bookmark";
  if (/modal|panel|card|row|item/.test(source)) return "menu";
  if (role === "text") return "edit";
  if (role === "icon") return "picture";
  return "help";
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

function normalizePreviewBackground(backgroundValue) {
  if (!backgroundValue) return "var(--tone-paper-soft)";
  const normalized = backgroundValue.trim().toLowerCase();
  if (normalized === "transparent" || normalized === "none") {
    return "var(--tone-paper-soft)";
  }
  return backgroundValue;
}

function buildPreviewBorder(decls) {
  if (decls.border) return decls.border;
  if (decls.borderColor) return `2rpx solid ${decls.borderColor}`;
  return "2rpx solid var(--nb-ink)";
}

function buildPreviewShadow(boxShadow) {
  if (!boxShadow) return "none";
  if (boxShadow.trim().toLowerCase() === "none") return "none";
  return boxShadow;
}

function buildProposalTone(source, role) {
  if (/danger|delete|remove|ashbin|logout/.test(source)) {
    return {
      label: "危险动作统一为红底白字白图标",
      background: "var(--nb-coral)",
      color: "#ffffff",
    };
  }

  if (/follow-btn/.test(source) && /following/.test(source)) {
    return {
      label: "已关注统一为绿底系状态块",
      background: "var(--tone-green-soft)",
      color: "var(--nb-green)",
    };
  }

  if (
    /footer-mini-btn--edit|rename-btn\\.confirm|action-btn\\.primary|action-button\\.primary|image-action-btn|row-btn\\.primary|page-action-btn/.test(
      source
    )
  ) {
    return {
      label: "编辑 / 调整类统一为蓝底白字白图标",
      background: "var(--nb-blue)",
      color: "#ffffff",
    };
  }

  if (
    /search-btn|clear-btn|icon-btn|menu-icon|setting-icon|preview-entry-icon|panel-action-icon|action-icon|modal-icon|step-badge/.test(
      source
    )
  ) {
    return {
      label: "工具图标壳统一为蓝系工具语义",
      background: "var(--tone-blue-soft)",
      color: "var(--nb-blue)",
    };
  }

  if (/empty-icon/.test(source)) {
    return {
      label: "空状态图标壳统一为黄底橙图标",
      background: "var(--tone-yellow-soft)",
      color: "var(--nb-orange)",
    };
  }

  if (/complete-badge|status-active|sync-status-card|device-status-dot/.test(source)) {
    return {
      label: "完成 / 成功状态统一为绿色语义",
      background: "var(--nb-green)",
      color: "var(--nb-ink)",
    };
  }

  if (/primary|confirm|export-button|glx-cta-button|glx-primary-action/.test(source)) {
    return {
      label: "主操作统一为黄底黑字黑图标",
      background: "var(--nb-yellow)",
      color: "var(--nb-ink)",
    };
  }

  if (role === "icon" || role === "button") {
    return {
      label: "高频交互默认收口到蓝系工具语义",
      background: "var(--tone-blue-soft)",
      color: "var(--nb-blue)",
    };
  }

  return {
    label: "文字层级保持当前，优先先修图标与背景关系",
    background: "var(--tone-paper-soft)",
    color: "var(--nb-ink)",
  };
}

function buildAuditPages() {
  return STYLE_AUDIT_PAGES.map((page) => ({
    ...page,
    label: shortPageLabel(page.path),
  }));
}

function buildAuditGroups(pages) {
  const pageMap = new Map(pages.map((page) => [page.index, page]));
  return STYLE_AUDIT_GROUPS.map((group) => {
    const samples = group.samples.map((sample, index) => {
      const page = pageMap.get(sample[0]);
      const decls = {
        color: sample[2],
        background: sample[3],
        backgroundColor: sample[4],
        border: sample[5],
        borderColor: sample[6],
        borderRadius: sample[7],
        boxShadow: sample[8],
      };
      const previewColor = decls.color || "var(--nb-ink)";
      const previewRadius = decls.borderRadius || "0";
      const source = `${group.key} ${sample[1]} ${sample[10] || ""}`.toLowerCase();
      const proposal = buildProposalTone(source, group.role);
      return {
        id: `${group.key}-${page?.path || "page"}-${index}`,
        page: page?.path || "",
        pageLabel: page?.label || "",
        selector: sample[1],
        previewColor,
        previewMutedColor: isLightTextColor(previewColor)
          ? "rgba(255,255,255,0.78)"
          : "var(--text-secondary)",
        previewShellStyle: {
          background: normalizePreviewBackground(
            decls.backgroundColor || decls.background
          ),
          border: buildPreviewBorder(decls),
          borderRadius: previewRadius,
          boxShadow: buildPreviewShadow(decls.boxShadow),
        },
        iconShellStyle: {
          background: isLightTextColor(previewColor)
            ? "rgba(255,255,255,0.18)"
            : "rgba(255,255,255,0.74)",
          border: "2rpx solid var(--nb-ink)",
          borderRadius: previewRadius,
        },
        proposalColor: proposal.color,
        proposalMutedColor: isLightTextColor(proposal.color)
          ? "rgba(255,255,255,0.78)"
          : "var(--text-secondary)",
        proposalShellStyle: {
          background: proposal.background,
          border: "2rpx solid var(--nb-ink)",
          borderRadius: previewRadius,
          boxShadow: "none",
        },
        proposalIconShellStyle: {
          background: isLightTextColor(proposal.color)
            ? "rgba(255,255,255,0.18)"
            : "rgba(255,255,255,0.74)",
          border: "2rpx solid var(--nb-ink)",
          borderRadius: previewRadius,
        },
        proposalLabel: proposal.label,
        proposalSummary: `${proposal.background} / ${proposal.color}`,
        iconName: pickAuditIcon(group.key, sample[1], sample[10], group.role),
        displayColor: decls.color || "继承",
        displayBackground:
          decls.backgroundColor || decls.background || "透明",
        displayBorder: decls.border || decls.borderColor || "继承",
        displayStates: sample[10] || "",
      };
    });

    return {
      ...group,
      displayRole: AUDIT_ROLE_LABELS[group.role] || AUDIT_ROLE_LABELS.other,
      pageLabels: group.pages.map(shortPageLabel),
      samples,
    };
  });
}

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    GlxSlider,
    GlxSwitch,
  },
  data() {
    const styleAuditPages = buildAuditPages();
    const styleAuditGroups = buildAuditGroups(styleAuditPages);
    const expandedAuditGroups = styleAuditGroups
      .slice()
      .sort((a, b) => b.total - a.total)
      .slice(0, 12)
      .reduce((accumulator, group) => {
        accumulator[group.key] = true;
        return accumulator;
      }, {});

    return {
      heroChips: ["36 页全量审计", "573 个类名分组", "1191 条当前样式", "只做独立对比页"],
      summaryCards: [
        {
          label: "注册页面",
          value: `${STYLE_AUDIT_META.pageCount}`,
          copy: "按 pages.json 当前注册页全量扫描",
          tone: "yellow",
        },
        {
          label: "样式组合",
          value: `${STYLE_AUDIT_META.entryCount}`,
          copy: "图标、文字、背景声明全部入账",
          tone: "blue",
        },
        {
          label: "类名分组",
          value: `${STYLE_AUDIT_META.groupCount}`,
          copy: "同类型类名归并到一张审计台账",
          tone: "green",
        },
      ],
      auditToneCards: [
        {
          key: "primary",
          tone: "yellow",
          icon: "add",
          iconColor: "var(--nb-ink)",
          title: "主操作",
          desc: "黄底黑字黑图标",
          usage: "用于发送、确认、主入口和当前页最重要动作",
        },
        {
          key: "edit",
          tone: "blue",
          icon: "edit",
          iconColor: "#ffffff",
          title: "编辑操作",
          desc: "蓝底白字白图标",
          usage: "用于编辑、修改、重命名、跳转到编辑链路",
        },
        {
          key: "danger",
          tone: "danger",
          icon: "ashbin",
          iconColor: "#ffffff",
          title: "危险操作",
          desc: "红底白字白图标",
          usage: "用于删除、退出、清空、覆盖等不可逆动作",
        },
        {
          key: "status",
          tone: "green",
          icon: "check-item",
          iconColor: "var(--nb-ink)",
          title: "成功状态",
          desc: "绿底黑字黑图标",
          usage: "用于已连接、已同步、完成、通过这类正向状态",
        },
      ],
      findings: [
        {
          title: "图标不能再偷懒成一片黑",
          level: "高优先",
          copy: "用户已经明确允许彩色，而且要求按语义重新搭配。后续不能再用“全部收黑”回避设计判断。",
          current: "大量入口、图标壳、搜索按钮、空状态图标默认回到黑图标方案。",
          proposal: "主操作、编辑、危险、成功、列表入口分别建立稳定色板，不再只剩黑图标方案。",
        },
        {
          title: "同类控件跨页仍在漂移",
          level: "高优先",
          copy: "同样叫 `action-btn`、`clear-btn`、`empty-icon`、`modal-btn` 的类，在不同页面里配色关系还不统一。",
          current: "同名类跨页会出现全黑、浅色片、红底黑字、蓝底黑字等混搭。",
          proposal: "先按类名分组审计，再统一收口成语义色板，避免每页各写各的。",
        },
        {
          title: "审计必须可视化，不靠口头总结",
          level: "建议",
          copy: "这次不是只修截图页，而是把 36 个注册页都展开成台账给你看，先看得见，再逐组改。",
          current: "过去只能靠截图点名，很难确认是否真做到全量覆盖。",
          proposal: "在一页里直接按类名展开，逐组查看当前样式、页面路径和色值声明。",
        },
      ],
      metrics: [
        { label: "亮度", value: "74%", tone: "yellow" },
        { label: "网络", value: "已连接", tone: "blue" },
        { label: "参数", value: "已同步", tone: "white" },
      ],
      modeCards: [
        { name: "主题", desc: "时钟主题", icon: "clock-filling", tone: "yellow" },
        { name: "画板", desc: "像素编辑", icon: "picture-filling", tone: "blue" },
        { name: "特效", desc: "动态效果", icon: "dynamic-filling", tone: "green" },
        { name: "通知", desc: "消息提醒", icon: "notification-filling", tone: "orange" },
      ],
      actions: [
        { title: "打开设备参数", desc: "兼容参数、时间服务器和设备信息统一放这里", icon: "setting", tone: "blue" },
        { title: "同步网络时间", desc: "手动触发时间更新，不需要用户猜测后台状态", icon: "history", tone: "yellow" },
        { title: "重置网络", desc: "危险动作独立强调，不和普通设置混放", icon: "close", tone: "danger" },
      ],
      paramRows: [
        { label: "画面方向", desc: "如果屏幕是侧着的，就在这里调整方向", value: "左转 90°", fieldType: "select" },
        { label: "灯珠颜色顺序", desc: "如果红蓝绿显示不对，在这里切换", value: "蓝绿交换", fieldType: "select" },
        { label: "自定义时间服务器", desc: "默认优先国内地址，也允许手动填写", value: "ntp2.aliyun.com", fieldType: "input" },
        { label: "固件版本", desc: "只读信息也保持同一套轮廓语言", value: "v1.4.2", fieldType: "readonly" },
      ],
      maintenanceActions: [
        { title: "保存并应用", desc: "把本页参数统一写入设备", button: "立即下发", tone: "yellow" },
        { title: "重新读取", desc: "重新拉取设备当前实际参数", button: "重新读取", tone: "blue" },
      ],
      previewBrightness: 74,
      previewNightBrightness: 36,
      previewSwitches: {
        clkphase: true,
        ntpAuto: false,
        wifiKeep: true,
      },
      switchItems: [
        { key: "clkphase", label: "错位兼容开关", desc: "遇到分裂或残影时可以尝试打开" },
        { key: "ntpAuto", label: "自动校时", desc: "连接网络后自动同步时间" },
        { key: "wifiKeep", label: "保持网络连接", desc: "避免设备长时间待机后掉线" },
      ],
      toastItems: [
        { type: "success", icon: "✓", text: "参数已保存并下发到设备" },
        { type: "info", icon: "i", text: "正在同步网络时间，请稍候" },
        { type: "warning", icon: "!", text: "该操作会覆盖当前配置" },
      ],
      styleAuditMeta: STYLE_AUDIT_META,
      styleAuditPages,
      styleAuditGroups,
      auditQuery: "",
      auditRole: "all",
      expandedAuditGroups,
      showPreviewModal: false,
      freezeItems: [
        "不在这轮样式页里改设备协议、参数字段和板载程序逻辑。",
        "不顺手重排正式业务页面结构，只先看视觉收口效果。",
        "不再保留旧的概念预览子页，后续只看这一张总览页。",
      ],
    };
  },
  computed: {
    auditRoleOptions() {
      const counts = this.styleAuditGroups.reduce((accumulator, group) => {
        accumulator[group.role] = (accumulator[group.role] || 0) + 1;
        return accumulator;
      }, {});

      return [
        { value: "all", label: `全部 ${this.styleAuditGroups.length}` },
        ...Object.keys(AUDIT_ROLE_LABELS)
          .filter((key) => key !== "all" && counts[key])
          .map((key) => ({
            value: key,
            label: `${AUDIT_ROLE_LABELS[key]} ${counts[key]}`,
          })),
      ];
    },
    filteredAuditGroups() {
      const query = this.auditQuery.trim().toLowerCase();
      return this.styleAuditGroups.filter((group) => {
        if (this.auditRole !== "all" && group.role !== this.auditRole) {
          return false;
        }
        if (!query) return true;
        if (group.key.toLowerCase().includes(query)) return true;
        if (group.pages.some((page) => page.toLowerCase().includes(query))) {
          return true;
        }
        return group.samples.some(
          (sample) =>
            sample.selector.toLowerCase().includes(query) ||
            sample.page.toLowerCase().includes(query)
        );
      });
    },
  },
  methods: {
    goBack() {
      uni.navigateBack();
    },
    isAuditGroupExpanded(groupKey) {
      return !!this.expandedAuditGroups[groupKey];
    },
    toggleAuditGroup(groupKey) {
      this.expandedAuditGroups = {
        ...this.expandedAuditGroups,
        [groupKey]: !this.expandedAuditGroups[groupKey],
      };
    },
    expandAllAuditGroups() {
      this.expandedAuditGroups = this.filteredAuditGroups.reduce(
        (accumulator, group) => {
          accumulator[group.key] = true;
          return accumulator;
        },
        {}
      );
    },
    collapseAllAuditGroups() {
      this.expandedAuditGroups = {};
    },
    readSliderValue(event) {
      const nextValue = event?.detail?.value;
      return typeof nextValue === "number" ? nextValue : 0;
    },
    handlePreviewSliderChanging(key, event) {
      this[key] = this.readSliderValue(event);
    },
    handlePreviewSliderChange(key, event) {
      this[key] = this.readSliderValue(event);
    },
    handlePreviewSwitchChange(key, event) {
      this.previewSwitches = {
        ...this.previewSwitches,
        [key]: !!event?.detail?.value,
      };
    },
    togglePreviewSwitch(key) {
      this.previewSwitches = {
        ...this.previewSwitches,
        [key]: !this.previewSwitches[key],
      };
    },
  },
};
</script>

<style scoped>
.design-preview-page {
  background: var(--nb-paper);
}

.navbar {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 28rpx;
}

.nav-left,
.nav-right {
  width: 64rpx;
  height: 64rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  background: var(--nb-surface);
  box-sizing: border-box;
}

.nav-title {
  flex: 1;
  text-align: center;
  font-size: 32rpx;
}

.preview-stack {
  padding: 24rpx;
  gap: 24rpx;
}

.hero-card,
.summary-card,
.finding-card,
.demo-card,
.freeze-card {
  padding: 24rpx;
  box-shadow: var(--shadow-card);
}

.hero-card {
  background: #ffd85a;
}

.hero-topline,
.finding-topline,
.section-header,
.field-rule-head,
.demo-console-head,
.demo-device-status,
.metric-row {
  display: flex;
  align-items: center;
}

.hero-topline,
.finding-topline,
.section-header,
.field-rule-head {
  justify-content: space-between;
}

.hero-tag,
.hero-meta,
.summary-label,
.summary-copy,
.finding-level,
.section-meta,
.field-rule-meta,
.demo-highlight-label,
.metric-label,
.mode-desc,
.action-desc,
.param-desc,
.freeze-text,
.maintenance-desc {
  color: #4a4a4a;
  font-size: 22rpx;
  line-height: 1.5;
}

.hero-title,
.section-title,
.finding-title,
.summary-value,
.demo-device-name,
.demo-highlight-value,
.mode-name,
.action-title,
.param-label,
.field-rule-title,
.maintenance-title {
  color: var(--nb-ink);
  font-weight: 900;
}

.hero-title {
  display: block;
  margin-top: 18rpx;
  font-size: 42rpx;
  line-height: 1.2;
}

.hero-desc,
.finding-copy,
.finding-block-value {
  display: block;
  margin-top: 18rpx;
  color: #4a4a4a;
  font-size: 24rpx;
  line-height: 1.6;
}

.hero-chip-row,
.summary-grid,
.mode-grid,
.field-scale-row {
  display: flex;
  flex-wrap: wrap;
}

.hero-chip-row {
  gap: 12rpx;
  margin-top: 22rpx;
}

.hero-chip {
  padding: 10rpx 18rpx;
  border: 3rpx solid var(--nb-ink);
  background: var(--nb-surface);
  color: var(--nb-ink);
  font-size: 22rpx;
  line-height: 1.2;
  font-weight: 900;
}

.summary-grid {
  gap: 16rpx;
}

.summary-card {
  width: calc((100% - 32rpx) / 3);
  min-height: 172rpx;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  box-sizing: border-box;
}

.summary-card.blue { background: #bcd6ff; }
.summary-card.green { background: #cbe7bb; }
.summary-card.yellow { background: #f2cf4a; }

.tone-grid,
.page-audit-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16rpx;
}

.tone-card {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.tone-card-preview {
  min-height: 124rpx;
  padding: 18rpx;
  display: flex;
  align-items: center;
  gap: 16rpx;
  border: 3rpx solid var(--nb-ink);
}

.tone-card.yellow,
.tone-card-preview.yellow,
.tone-card-icon-shell.yellow {
  background: #f2cf4a;
}

.tone-card.blue,
.tone-card-preview.blue,
.tone-card-icon-shell.blue {
  background: #bcd6ff;
}

.tone-card.green,
.tone-card-preview.green,
.tone-card-icon-shell.green {
  background: #cbe7bb;
}

.tone-card.danger,
.tone-card-preview.danger,
.tone-card-icon-shell.danger {
  background: #efb8bf;
}

.tone-card-icon-shell {
  width: 56rpx;
  height: 56rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  background: rgba(255, 255, 255, 0.74);
}

.tone-card-copy {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.tone-card-title,
.page-audit-label,
.audit-group-title,
.audit-sample-selector,
.audit-toolbar-summary,
.page-audit-bg-label {
  color: var(--nb-ink);
  font-size: 24rpx;
  line-height: 1.3;
  font-weight: 900;
}

.tone-card-desc,
.tone-card-usage,
.page-audit-path,
.audit-group-meta,
.audit-sample-page,
.page-audit-bg-value {
  color: #4a4a4a;
  font-size: 22rpx;
  line-height: 1.5;
}

.page-audit-card {
  display: flex;
  flex-direction: column;
  gap: 14rpx;
}

.page-audit-topline,
.page-audit-bg-row,
.audit-group-head,
.audit-toolbar-foot {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
}

.page-audit-count {
  padding: 8rpx 14rpx;
  border: 3rpx solid var(--nb-ink);
  background: #f2cf4a;
  color: var(--nb-ink);
  font-size: 22rpx;
  font-weight: 900;
}

.page-audit-bg-chip {
  display: inline-flex;
  align-items: center;
  gap: 10rpx;
  min-height: 48rpx;
  padding: 0 14rpx;
  border: 3rpx solid var(--nb-ink);
  background: var(--nb-surface);
}

.page-audit-bg-swatch {
  width: 24rpx;
  height: 24rpx;
  border: 2rpx solid var(--nb-ink);
  background: #ffffff;
}

.audit-toolbar {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.audit-search-shell {
  display: flex;
  align-items: center;
  gap: 12rpx;
  min-height: 88rpx;
  padding: 0 18rpx;
  border: 3rpx solid var(--nb-ink);
  background: var(--tone-paper-soft);
}

.audit-search-input {
  flex: 1;
  min-height: 80rpx;
  color: var(--nb-ink);
  font-size: 26rpx;
  background: transparent;
  border: none;
  outline: none;
}

.audit-search-input::placeholder {
  color: var(--text-tertiary);
}

.audit-clear-btn {
  width: 44rpx;
  height: 44rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 2rpx solid var(--nb-ink);
  background: #ffffff;
}

.audit-role-scroll {
  width: 100%;
  white-space: nowrap;
}

.audit-role-list {
  display: inline-flex;
  gap: 12rpx;
}

.audit-role-chip {
  min-height: 60rpx;
  padding: 0 18rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  background: var(--nb-surface);
}

.audit-role-chip.active {
  background: #f2cf4a;
}

.audit-role-chip-text,
.audit-toolbar-btn-text,
.audit-group-role,
.audit-group-toggle,
.audit-page-chip,
.audit-token {
  color: var(--nb-ink);
  font-size: 22rpx;
  line-height: 1.2;
  font-weight: 900;
}

.audit-toolbar-actions {
  display: flex;
  gap: 12rpx;
}

.audit-toolbar-btn {
  min-height: 56rpx;
  padding: 0 18rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  background: #f2cf4a;
}

.audit-toolbar-btn.secondary {
  background: #dce9ff;
}

.audit-group-list {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.audit-group-card {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.audit-group-main,
.audit-sample-preview-copy,
.audit-sample-meta {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.audit-group-main {
  flex: 1;
}

.audit-group-side {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.audit-group-role {
  min-width: 116rpx;
  padding: 10rpx 14rpx;
  text-align: center;
  border: 3rpx solid var(--nb-ink);
  background: #ffffff;
}

.audit-group-role.button { background: #f2cf4a; }
.audit-group-role.icon { background: #dce9ff; }
.audit-group-role.text { background: #fff1b8; }
.audit-group-role.field { background: #eee8ff; }
.audit-group-role.badge { background: #e2f8ee; }
.audit-group-role.container { background: #ffe6cc; }

.audit-group-pages {
  display: flex;
  flex-wrap: wrap;
  gap: 10rpx;
}

.audit-page-chip {
  padding: 8rpx 14rpx;
  border: 2rpx solid var(--nb-ink);
  background: var(--nb-surface);
}

.audit-page-chip.more {
  background: #f3f1ee;
}

.audit-sample-list {
  display: flex;
  flex-direction: column;
  gap: 14rpx;
}

.audit-sample-card {
  display: grid;
  grid-template-columns: minmax(0, 1.4fr) minmax(0, 1fr);
  gap: 16rpx;
  padding: 16rpx;
  border: 3rpx solid var(--nb-ink);
  background: rgba(255, 255, 255, 0.82);
}

.audit-sample-preview-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12rpx;
}

.audit-sample-preview-block {
  display: flex;
  flex-direction: column;
  gap: 10rpx;
}

.audit-sample-preview-shell {
  min-height: 110rpx;
  padding: 16rpx;
  display: flex;
  align-items: center;
  gap: 14rpx;
  box-sizing: border-box;
}

.audit-sample-stage-label {
  color: var(--nb-ink);
  font-size: 20rpx;
  line-height: 1.2;
  font-weight: 900;
}

.audit-sample-stage-label.proposal,
.audit-token.proposal {
  background: #dce9ff;
}

.audit-sample-icon-shell {
  width: 52rpx;
  height: 52rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
}

.audit-sample-preview-title,
.audit-sample-preview-desc {
  font-size: 22rpx;
  line-height: 1.5;
}

.audit-token-row {
  display: flex;
  flex-wrap: wrap;
  gap: 10rpx;
}

.audit-token {
  padding: 8rpx 12rpx;
  border: 2rpx solid var(--nb-ink);
  background: #fffdf8;
}

.summary-value {
  font-size: 34rpx;
  line-height: 1.2;
}

.section-block {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.section-title {
  font-size: 30rpx;
}

.section-meta {
  text-align: right;
}

.finding-list,
.action-list,
.param-list,
.maintenance-list,
.freeze-card {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.finding-level {
  padding: 8rpx 16rpx;
  border: 3rpx solid var(--nb-ink);
  background: #efc93f;
  color: var(--nb-ink);
  font-weight: 900;
}

.finding-compare-row {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16rpx;
  margin-top: 18rpx;
}

.finding-block {
  padding: 18rpx;
  border: 3rpx solid var(--nb-ink);
  background: var(--nb-surface);
}

.finding-block.proposal {
  background: #d6e5ff;
}

.finding-block-label,
.demo-status-text,
.demo-status-ip,
.demo-highlight-label,
.metric-label,
.mode-name,
.action-title,
.param-label,
.param-field-value,
.field-scale-box,
.maintenance-button-text {
  color: var(--nb-ink);
  font-size: 22rpx;
  line-height: 1.3;
  font-weight: 900;
}

.demo-console-head {
  justify-content: space-between;
  gap: 16rpx;
}

.demo-device {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.demo-logo {
  width: 88rpx;
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  background: var(--nb-ink);
}

.demo-logo-core {
  width: 42rpx;
  height: 42rpx;
  background: var(--nb-yellow);
}

.demo-device-copy {
  display: flex;
  flex-direction: column;
  gap: 10rpx;
}

.demo-device-name {
  font-size: 30rpx;
}

.demo-device-status {
  gap: 10rpx;
  flex-wrap: wrap;
}

.demo-status-dot {
  width: 16rpx;
  height: 16rpx;
  background: var(--color-success);
  border: 2rpx solid var(--nb-ink);
}

.demo-highlight-box {
  min-width: 180rpx;
  padding: 16rpx 18rpx;
  border: 3rpx solid var(--nb-ink);
  background: #c4d9ff;
  box-sizing: border-box;
}

.demo-highlight-value {
  display: block;
  margin-top: 10rpx;
  font-size: 28rpx;
}

.metric-row {
  gap: 12rpx;
  margin-top: 18rpx;
}

.metric-card {
  flex: 1;
  min-height: 112rpx;
  padding: 16rpx;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  border: 3rpx solid var(--nb-ink);
  box-sizing: border-box;
}

.metric-card.yellow { background: #f2cf4a; }
.metric-card.blue { background: #bcd6ff; }
.metric-card.white { background: var(--nb-surface); }

.metric-value {
  color: var(--nb-ink);
  font-size: 32rpx;
  line-height: 1.2;
  font-weight: 900;
}

.mode-grid {
  gap: 12rpx;
  margin-top: 18rpx;
}

.mode-card {
  width: calc((100% - 12rpx) / 2);
  min-height: 144rpx;
  padding: 18rpx;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  border: 3rpx solid var(--nb-ink);
  box-sizing: border-box;
}

.mode-card.yellow { background: #f2cf4a; }
.mode-card.blue { background: #bcd6ff; }
.mode-card.green { background: #cbe7bb; }
.mode-card.orange { background: #f1c39a; }

.mode-icon {
  width: 52rpx;
  height: 52rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  background: rgba(255, 255, 255, 0.82);
}

.action-row,
.param-row,
.maintenance-row,
.freeze-row {
  display: flex;
  align-items: center;
}

.action-row,
.param-row,
.maintenance-row {
  gap: 16rpx;
}

.action-row,
.maintenance-row {
  padding: 18rpx;
  border: 3rpx solid var(--nb-ink);
  background: var(--nb-surface);
}

.action-icon {
  width: 56rpx;
  height: 56rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  flex: 0 0 56rpx;
}

.action-icon.blue { background: #bcd6ff; }
.action-icon.yellow { background: #f2cf4a; }
.action-icon.danger { background: #efb8bf; }

.action-copy,
.param-copy,
.maintenance-copy {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.action-arrow,
.param-field-arrow {
  color: var(--nb-ink);
  font-size: 32rpx;
  font-weight: 900;
}

.param-list {
  margin-top: 4rpx;
}

.param-row {
  align-items: flex-start;
}

.param-field {
  min-width: 208rpx;
  min-height: 88rpx;
  padding: 0 18rpx;
  display: inline-flex;
  align-items: center;
  justify-content: space-between;
  gap: 12rpx;
  border: 3rpx solid var(--nb-ink);
  background: var(--nb-surface);
  box-sizing: border-box;
}

.param-field.select { background: #dce9ff; }
.param-field.readonly { background: #f3f1ee; }

.field-rule-card {
  margin-top: 20rpx;
  padding: 20rpx;
  border: 3rpx solid var(--nb-ink);
  background: #ffe79b;
}

.field-scale-row {
  gap: 12rpx;
  margin-top: 16rpx;
}

.field-scale-box {
  flex: 1;
  min-height: 84rpx;
  padding: 0 18rpx;
  display: inline-flex;
  align-items: center;
  border: 3rpx solid var(--nb-ink);
  background: var(--nb-surface);
  box-sizing: border-box;
}

.field-scale-box.select { background: #dce9ff; }
.field-scale-box.readonly { background: #f3f1ee; }

.maintenance-button {
  min-width: 132rpx;
  min-height: 72rpx;
  padding: 0 18rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  box-sizing: border-box;
}

.maintenance-button.yellow { background: #f2cf4a; }
.maintenance-button.blue { background: #bcd6ff; }

.controls-preview-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16rpx;
}

.toast-modal-grid {
  margin-top: 16rpx;
}

.controls-block {
  padding: 20rpx;
  border: 3rpx solid var(--nb-ink);
  background: var(--nb-surface);
}

.slider-block {
  background: #fff7db;
}

.switch-block {
  background: #eef4ff;
}

.toast-block {
  background: #fff2d9;
}

.modal-block {
  background: #f4f1ff;
}

.controls-block-head {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  margin-bottom: 18rpx;
}

.controls-block-title,
.slider-demo-label,
.switch-label,
.preview-modal-title,
.toast-preview-text,
.preview-modal-button-text {
  color: var(--nb-ink);
  font-size: 24rpx;
  line-height: 1.3;
  font-weight: 900;
}

.controls-block-meta,
.switch-desc,
.modal-preview-note,
.preview-modal-desc,
.preview-modal-status-text {
  color: #4a4a4a;
  font-size: 22rpx;
  line-height: 1.5;
}

.slider-demo-row {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.slider-demo-row + .slider-demo-row {
  margin-top: 18rpx;
}

.slider-demo-label-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
}

.slider-demo-value {
  color: var(--nb-ink);
  font-size: 28rpx;
  line-height: 1.2;
  font-weight: 900;
}

.switch-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 18rpx;
  border: 3rpx solid var(--nb-ink);
  background: rgba(255, 255, 255, 0.72);
}

.switch-row + .switch-row {
  margin-top: 12rpx;
}

.switch-copy {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.toast-preview-list {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.toast-preview-item {
  min-height: 92rpx;
  padding: 0 18rpx 0 20rpx;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  border: 3rpx solid var(--nb-ink);
  box-sizing: border-box;
}

.toast-preview-item.success {
  background: #b9dc96;
}

.toast-preview-item.info {
  background: #bcd6ff;
}

.toast-preview-item.warning {
  background: #f2cf4a;
}

.toast-preview-main {
  flex: 1;
  min-width: 0;
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.toast-preview-icon,
.toast-preview-close {
  color: var(--nb-ink);
  font-size: 30rpx;
  line-height: 1;
  font-weight: 900;
}

.modal-preview-trigger {
  min-height: 84rpx;
  padding: 0 20rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  background: #d7c9ff;
  box-sizing: border-box;
}

.modal-preview-trigger-text,
.preview-modal-input-value {
  color: var(--nb-ink);
  font-size: 24rpx;
  line-height: 1.3;
  font-weight: 900;
}

.modal-preview-card {
  margin-top: 16rpx;
  padding: 18rpx;
  border: 3rpx solid var(--nb-ink);
  background: rgba(255, 255, 255, 0.78);
}

.preview-modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 1200;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 40rpx;
  background: rgba(0, 0, 0, 0.28);
  box-sizing: border-box;
}

.preview-modal-panel {
  width: 100%;
  max-width: 620rpx;
  border: 4rpx solid var(--nb-ink);
  background: var(--nb-surface);
  box-shadow: 8rpx 8rpx 0 #000000;
}

.preview-modal-header,
.preview-modal-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 24rpx;
}

.preview-modal-header {
  border-bottom: 3rpx solid var(--nb-ink);
}

.preview-modal-close {
  width: 52rpx;
  height: 52rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  background: #f6f1ff;
  box-sizing: border-box;
}

.preview-modal-body {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
  padding: 24rpx;
}

.preview-modal-input {
  min-height: 84rpx;
  padding: 0 20rpx;
  display: flex;
  align-items: center;
  border: 3rpx solid var(--nb-ink);
  background: #fff8e2;
  box-sizing: border-box;
}

.preview-modal-status {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 16rpx 18rpx;
  border: 3rpx solid var(--nb-ink);
  background: #eef4ff;
}

.preview-modal-status-dot {
  width: 14rpx;
  height: 14rpx;
  border: 2rpx solid var(--nb-ink);
  background: #f2cf4a;
}

.preview-modal-footer {
  border-top: 3rpx solid var(--nb-ink);
}

.preview-modal-button {
  flex: 1;
  min-height: 84rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid var(--nb-ink);
  box-sizing: border-box;
}

.preview-modal-button.ghost {
  background: var(--nb-surface);
}

.preview-modal-button.primary {
  background: #f2cf4a;
}

.freeze-row {
  align-items: flex-start;
  gap: 12rpx;
}

.freeze-dot {
  width: 14rpx;
  height: 14rpx;
  margin-top: 10rpx;
  flex: 0 0 14rpx;
  border: 2rpx solid var(--nb-ink);
  background: var(--nb-yellow);
}

@media (max-width: 720rpx) {
  .summary-card,
  .mode-card {
    width: 100%;
  }

  .tone-grid,
  .page-audit-grid,
  .controls-preview-grid,
  .audit-sample-card,
  .audit-sample-preview-grid,
  .finding-compare-row {
    grid-template-columns: 1fr;
  }

  .controls-preview-grid {
    gap: 16rpx;
  }

  .demo-console-head,
  .metric-row,
  .field-scale-row {
    flex-direction: column;
    align-items: stretch;
  }

  .param-row,
  .maintenance-row {
    flex-direction: column;
    align-items: stretch;
  }

  .param-field,
  .maintenance-button {
    width: 100%;
  }

  .audit-toolbar-foot,
  .audit-group-head {
    flex-direction: column;
    align-items: stretch;
  }

  .audit-toolbar-actions,
  .audit-group-side {
    width: 100%;
  }

  .audit-toolbar-btn,
  .audit-group-role {
    flex: 1;
  }
}
</style>
