<template>
  <section class="ds-section">
    <div class="ds-section__head">
      <div>
        <h2 class="ds-section__title">导航与布局</h2>
        <p class="ds-section__meta">先把官网导航、组件库顶部 tabs、页头和列表入口全部摆成正式语法。</p>
      </div>
    </div>

    <article v-if="showHeaders" class="ds-card">
      <span class="ds-card__label">官网顶层导航</span>
      <div class="ds-nav-preview">
        <span
          v-for="item in navItems"
          :key="item.label"
          class="ds-nav-preview__item"
          :class="{ 'is-active': item.active }"
        >
          {{ item.label }}
        </span>
      </div>
      <p class="ds-card__copy ds-card__copy--top-spaced">
        官网最顶层导航已经和组件库顶部 tabs 统一成同一套黄底按钮选中态。
      </p>
    </article>

    <article v-if="showHeaders" class="ds-card ds-card--top-spaced">
      <span class="ds-card__label">组件库顶部 Tabs</span>
      <div class="ds-top-tabs">
        <button
          v-for="item in topTabs"
          :key="item.label"
          type="button"
          class="ds-top-tab"
          :class="{ 'is-active': item.active }"
        >
          {{ item.label }}
        </button>
      </div>
      <p class="ds-card__copy ds-card__copy--top-spaced">
        组件库内部 tabs 继续保留黄底按钮态，官网最顶层导航也跟这套语法对齐。
      </p>
    </article>

    <div v-if="showHeaders" class="ds-grid ds-grid--two">
      <article class="ds-example">
        <div class="ds-card ds-card--plain">
          <span class="ds-card__label">一级页头部</span>
          <div class="ds-header-preview">
            <div class="ds-header-preview__main">
              <span class="ds-icon-shell ds-icon-shell--primary">
                <DesignSystemGlyph name="home" />
              </span>
              <div class="ds-title-stack">
                <span class="ds-title-stack__title">控件库 / 组件规范</span>
                <span class="ds-title-stack__desc">这是官网里的正式页面，但视觉继续按上一版硬朗规范站收口</span>
              </div>
            </div>
            <span class="ds-inline-note">官网内正式页头</span>
          </div>
          <div class="ds-toolbar">
            <span class="ds-inline-note">搜索和筛选继续留在工具层</span>
            <span class="ds-inline-note">不再额外堆标签框说明自己是什么页</span>
          </div>
        </div>
      </article>

      <article class="ds-example">
        <div class="ds-card ds-card--plain">
          <span class="ds-card__label">二级页头部</span>
          <div class="ds-header-preview">
            <div class="ds-header-preview__main">
              <span class="ds-icon-shell">
                <DesignSystemGlyph name="arrow-left" />
              </span>
              <div class="ds-title-stack">
                <span class="ds-title-stack__title">设备参数</span>
                <span class="ds-title-stack__desc">左返回 / 中标题 / 右侧尽量保持干净</span>
              </div>
            </div>
            <span class="ds-inline-note">二级页头部</span>
          </div>
          <div class="ds-inline-list">
            <span>基础参数</span>
            <span>亮度与时间</span>
            <span>网络</span>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showHeaders" class="ds-grid ds-grid--two ds-grid--spaced">
      <article class="ds-card">
        <span class="ds-card__label">Logo.vue / BrandLogo</span>
        <div class="ds-brand-showcase">
          <BrandLogo variant="page" />
          <div class="ds-brand-showcase__meta">
            <span class="ds-inline-note">中文主标 + 黄底英文标签继续保留</span>
            <span class="ds-inline-note">官网与 uniapp 的品牌壳统一照这一版走</span>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">LegacyTabBar.vue</span>
        <div class="ds-legacy-tabbar">
          <div
            v-for="item in legacyTabItems"
            :key="item.label"
            class="ds-legacy-tabbar__item"
          >
            <span class="ds-legacy-tabbar__icon" :class="{ 'is-active': item.active }">
              <DesignSystemGlyph :name="item.icon" />
            </span>
            <span class="ds-legacy-tabbar__label" :class="{ 'is-active': item.active }">
              {{ item.label }}
            </span>
          </div>
        </div>
        <p class="ds-card__copy ds-card__copy--top-spaced">
          小程序底部 tab 先对齐旧原生观感，只切图标和字色，不做黄底整块选中。
        </p>
      </article>
    </div>

    <div v-if="showLists" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">列表与卡片语法</h3>
        <p class="ds-section__meta">白底黑边、统一硬阴影、清楚分区，不再回到柔和圆角专题页语法</p>
      </div>
    </div>

    <div v-if="showLists" class="ds-grid ds-grid--two">
      <div class="ds-showcase">
        <div class="ds-list-card">
          <div class="ds-list-card__main">
            <span class="ds-icon-shell ds-icon-shell--blue">
              <DesignSystemGlyph name="setting" />
            </span>
            <div class="ds-list-card__copy">
              <span class="ds-list-card__title">设备参数</span>
              <span class="ds-list-card__desc">说明文字收进副标题，不在标题区堆满额外标签。</span>
            </div>
          </div>
          <div class="ds-list-card__meta">
            <span>查看</span>
            <DesignSystemGlyph name="arrow-right" />
          </div>
        </div>

        <div class="ds-list-card ds-list-card--spaced">
          <div class="ds-list-card__main">
            <span class="ds-icon-shell ds-icon-shell--info">
              <DesignSystemGlyph name="profile" />
            </span>
            <div class="ds-list-card__copy">
              <span class="ds-list-card__title">用户中心入口</span>
              <span class="ds-list-card__desc">图标壳仍然清楚，但大小、阴影、边框全部跟同组统一。</span>
            </div>
          </div>
          <div class="ds-list-card__meta">
            <span>进入</span>
            <DesignSystemGlyph name="arrow-right" />
          </div>
        </div>
      </div>

      <div class="ds-showcase">
        <div class="ds-empty">
          <span class="ds-icon-shell ds-icon-shell--primary">
            <DesignSystemGlyph name="picture" />
          </span>
          <strong class="ds-card__title ds-card__title--compact">还没有内容</strong>
          <span class="ds-card__copy">空状态继续跟随黑边硬壳语法，不做默认灰大壳，也不堆大段说明。</span>
        </div>
      </div>
    </div>

    <div v-if="showLists" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">业务卡片家族</h3>
        <p class="ds-section__meta">这些是 uniapp 里已经改过风格的实际壳层，必须在组件库里定板后再映射回页面。</p>
      </div>
    </div>

    <div v-if="showLists" class="ds-grid ds-grid--three">
      <article class="ds-card">
        <span class="ds-card__label">ProjectCard.vue</span>
        <div class="ds-project-card">
          <div class="ds-project-card__media">
            <div class="ds-project-card__grid"></div>
            <div class="ds-project-card__thumb">
              <div class="ds-project-card__pixels">
                <span v-for="cell in projectPixels" :key="cell" :style="{ backgroundColor: cell }"></span>
              </div>
            </div>
            <span class="ds-project-card__badge ds-project-card__badge--published">已发布</span>
            <span class="ds-project-card__progress"><span></span></span>
          </div>
          <div class="ds-project-card__body">
            <strong class="ds-project-card__title">像素拼豆图纸</strong>
            <div class="ds-project-card__meta">
              <span>64×64</span>
              <span>4/18</span>
            </div>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">ArtworkCard.vue</span>
        <div class="ds-artwork-card">
          <div class="ds-artwork-card__media">
            <div class="ds-artwork-card__art"></div>
            <span class="ds-artwork-card__pill">社区作品</span>
          </div>
          <div class="ds-artwork-card__body">
            <strong class="ds-card__title ds-card__title--compact">霓虹像素街区</strong>
            <div class="ds-artwork-card__author">
              <span class="ds-avatar-shell ds-avatar-shell--sm">GX</span>
              <span>光格像素工坊用户</span>
            </div>
            <div class="ds-artwork-card__stats">
              <span>1.2k 喜欢</span>
              <span>84 评论</span>
              <span>4.6k 浏览</span>
            </div>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">TemplateCard.vue</span>
        <div class="ds-template-card">
          <div class="ds-template-card__media">
            <div class="ds-template-card__preview"></div>
            <span class="ds-status-pill ds-status-pill--warning">中等</span>
          </div>
          <div class="ds-template-card__body">
            <strong class="ds-card__title ds-card__title--compact">霓虹天气模板</strong>
            <div class="ds-template-card__facts">
              <span>64×64</span>
              <span>12 色</span>
              <span>2.3k 使用</span>
            </div>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showLists" class="ds-grid ds-grid--two ds-grid--spaced">
      <article class="ds-card">
        <span class="ds-card__label">ChallengeCard.vue</span>
        <div class="ds-challenge-card">
          <div class="ds-challenge-card__banner">
            <div class="ds-challenge-card__overlay"></div>
            <span class="ds-challenge-card__status">进行中</span>
            <span class="ds-icon-shell ds-icon-shell--warning ds-challenge-card__icon">
              <DesignSystemGlyph name="work" />
            </span>
          </div>
          <div class="ds-challenge-card__body">
            <strong class="ds-card__title ds-card__title--compact">像素春日挑战</strong>
            <p class="ds-card__copy">横幅、状态角标、说明和 CTA 继续按硬边结构收口。</p>
            <div class="ds-challenge-card__meta">
              <span>3.2k 人参与</span>
              <span>还剩 6 天</span>
              <span>奖励模板包</span>
            </div>
          </div>
          <div class="ds-challenge-card__foot">
            <button type="button" class="ds-btn ds-btn--primary ds-btn--sm ds-shadow--sm">立即参与</button>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">Avatar / Comment.vue</span>
        <div class="ds-comment-card">
          <span class="ds-avatar-shell">GX</span>
          <div class="ds-comment-card__body">
            <div class="ds-comment-card__head">
              <strong>光格像素工坊用户</strong>
              <span>刚刚</span>
            </div>
            <p class="ds-card__copy">头像允许保留圆形，但外围信息块、评论块和操作行继续按白底黑边直角体系收口。</p>
            <div class="ds-comment-card__actions">
              <span class="ds-inline-note">点赞</span>
              <span class="ds-inline-note">回复</span>
            </div>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showLists" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">预览与编辑壳层</h3>
        <p class="ds-section__meta">PixelPreviewBoard、ImageCropper、时钟设置卡这些特殊壳层也要先在组件库里定板。</p>
      </div>
    </div>

    <div v-if="showLists" class="ds-grid ds-grid--two">
      <article class="ds-card">
        <span class="ds-card__label">PixelPreviewBoard / PixelCanvas</span>
        <div class="ds-preview-shell">
          <div class="ds-preview-shell__screen">
            <div class="ds-preview-shell__pixels">
              <span v-for="cell in previewPixels" :key="cell.id" :style="{ backgroundColor: cell.color }"></span>
            </div>
          </div>
          <div class="ds-preview-shell__meta">
            <strong>64 × 64 预览板</strong>
            <span>黑底预览、统一留边、像素居中。</span>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">ImageCropper / Clock Settings</span>
        <div class="ds-editor-stack">
          <div class="ds-cropper-shell">
            <div class="ds-cropper-shell__image"></div>
            <div class="ds-cropper-shell__frame"></div>
          </div>
          <div class="ds-setting-card">
            <strong>ClockTextSettingsCard.vue</strong>
            <span>字号、位置、配色和文本字段仍然按统一设置卡语法收口。</span>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">ClockThemePanel.vue</span>
        <div class="ds-theme-panel">
          <div class="ds-theme-panel__head">
            <strong>主题库</strong>
            <span>3 个主题</span>
          </div>
          <div class="ds-theme-panel__grid">
            <div
              v-for="item in themeCards"
              :key="item.name"
              class="ds-theme-card"
              :class="{ 'is-active': item.active }"
            >
              <div class="ds-theme-card__preview" :class="item.previewClass"></div>
              <span class="ds-theme-card__name">{{ item.name }}</span>
              <span v-if="item.badge" class="ds-theme-card__badge">{{ item.badge }}</span>
            </div>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">ClockFontPanel.vue</span>
        <div class="ds-font-panel">
          <div class="ds-font-panel__list">
            <div
              v-for="item in fontCards"
              :key="item.name"
              class="ds-font-card"
              :class="{ 'is-active': item.active }"
            >
              <div class="ds-font-card__preview" :class="item.previewClass">{{ item.sample }}</div>
              <span class="ds-font-card__name">{{ item.name }}</span>
            </div>
          </div>
          <div class="ds-font-panel__settings">
            <div class="ds-font-panel__row">
              <span>显示秒钟</span>
              <span class="ds-font-panel__toggle is-active">开启</span>
            </div>
            <div class="ds-choice-row">
              <button type="button" class="ds-choice-chip is-active">24 小时</button>
              <button type="button" class="ds-choice-chip">12 小时</button>
            </div>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showHeaders" class="ds-grid ds-grid--three ds-grid--spaced">
      <article v-for="item in rules" :key="item.title" class="ds-card">
        <span class="ds-card__label">{{ item.label }}</span>
        <span class="ds-card__title">{{ item.title }}</span>
        <p class="ds-card__copy">{{ item.copy }}</p>
      </article>
    </div>

    <div v-if="showHeaders" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">Uniapp 风格硬约束</h3>
        <p class="ds-section__meta">网页组件库直接承接小程序的正式规则，不另起一套解释口径。</p>
      </div>
    </div>

    <div v-if="showHeaders" class="ds-grid ds-grid--three">
      <article v-for="item in guardrails" :key="item.title" class="ds-card">
        <span class="ds-card__label">{{ item.label }}</span>
        <span class="ds-card__title">{{ item.title }}</span>
        <p class="ds-card__copy">{{ item.copy }}</p>
      </article>
    </div>
  </section>
</template>

<script setup>
import { computed } from "vue";
import BrandLogo from "@/components/BrandLogo.vue";
import DesignSystemGlyph from "./DesignSystemGlyph.vue";

const props = defineProps({
  sectionKey: {
    type: String,
    required: true,
  },
});

const showHeaders = computed(() => props.sectionKey === "headers");
const showLists = computed(() => props.sectionKey === "lists");

const rules = [
  {
    label: "官网基线",
    title: "整站与控件库一起回到上一版基线",
    copy: "规范站虽然挂在官网里，但整站都继续沿用白底黑边硬阴影，不再分成两种产品气质。",
  },
  {
    label: "阴影规则",
    title: "阴影统一分三级",
    copy: "导航、tabs、卡片、弹窗分别对应固定 shadow token，不再同层级忽大忽小。",
  },
  {
    label: "布局规则",
    title: "结构清楚，但不做柔和专题页",
    copy: "不再堆玻璃感背景和大圆角卡片，重点回到可查阅、可扩展、可维护的规范站结构。",
  },
];

const navItems = [
  { label: "首页" },
  { label: "工作台" },
  { label: "设备" },
  { label: "社区" },
  { label: "控件库", active: true },
  { label: "模板" },
  { label: "挑战" },
];

const topTabs = [
  { label: "总览" },
  { label: "控件", active: true },
  { label: "反馈" },
  { label: "规则" },
];

const legacyTabItems = [
  { label: "首页", icon: "home" },
  { label: "社区", icon: "community" },
  { label: "创作", icon: "edit" },
  { label: "设备", icon: "device" },
  { label: "我的", icon: "profile", active: true },
];

const projectPixels = [
  "#111111",
  "#111111",
  "#ffcb45",
  "#ffcb45",
  "#111111",
  "#111111",
  "#111111",
  "#5ea8ff",
  "#5ea8ff",
  "#111111",
  "#67c23a",
  "#67c23a",
  "#f56c6c",
  "#f56c6c",
  "#111111",
  "#111111",
];

const previewPixels = [
  { id: "p1", color: "#111111" },
  { id: "p2", color: "#ffcb45" },
  { id: "p3", color: "#5ea8ff" },
  { id: "p4", color: "#111111" },
  { id: "p5", color: "#67c23a" },
  { id: "p6", color: "#111111" },
  { id: "p7", color: "#f56c6c" },
  { id: "p8", color: "#111111" },
  { id: "p9", color: "#111111" },
  { id: "p10", color: "#5ea8ff" },
  { id: "p11", color: "#ffcb45" },
  { id: "p12", color: "#111111" },
  { id: "p13", color: "#111111" },
  { id: "p14", color: "#67c23a" },
  { id: "p15", color: "#f56c6c" },
  { id: "p16", color: "#111111" },
];

const themeCards = [
  { name: "经典", previewClass: "ds-theme-card__preview--classic", active: true, badge: "当前" },
  { name: "图纸", previewClass: "ds-theme-card__preview--planet", badge: "图片" },
  { name: "霓虹", previewClass: "ds-theme-card__preview--neon" },
];

const fontCards = [
  { name: "像素方正", sample: "12:34", previewClass: "ds-font-card__preview--mono", active: true },
  { name: "圆点屏", sample: "12:34", previewClass: "ds-font-card__preview--dot" },
  { name: "窄体屏", sample: "12:34", previewClass: "ds-font-card__preview--condensed" },
];

const guardrails = [
  {
    label: "直角语法",
    title: "不做圆角胶囊和漂浮圆壳",
    copy: "网页组件库里的按钮、tabs、卡片、列表项默认都保持直角块面，后续映射到 uniapp 也不再临时圆角化。",
  },
  {
    label: "点击反馈",
    title: "禁止按压位移、缩放、抖动",
    copy: "所有入口、按钮、返回键、顶部导航和 tabs 都只改颜色或字重，不再靠 transform 制造点击感。",
  },
  {
    label: "分组语义",
    title: "功能页底部 tabs 不是黄底整块选中",
    copy: "官网组件库必须明确区分组件库内部 tabs 与 uniapp 功能页底部 tabs，避免后续迁回小程序时把语法混掉。",
  },
];
</script>
