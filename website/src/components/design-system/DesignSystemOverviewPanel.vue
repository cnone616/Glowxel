<template>
  <section class="ds-section">
    <div v-if="showPalette" class="ds-section__head">
      <div>
        <h2 class="ds-section__title">正式色板</h2>
        <p class="ds-section__meta">组件库色值直接跟当前项目 token 对齐。</p>
      </div>
    </div>

    <div v-if="showPalette" class="ds-grid ds-grid--three">
      <article v-for="tone in tones" :key="tone.name" class="ds-card">
        <div class="ds-tone">
          <span class="ds-tone__swatch" :style="{ background: tone.color }"></span>
          <div>
            <span class="ds-tone__name">{{ tone.name }}</span>
            <span class="ds-tone__value">{{ tone.color }}</span>
            <span class="ds-code-tag">{{ tone.token }}</span>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showSurfaces" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">面板与阴影</h3>
        <p class="ds-section__meta">先把基础壳层和阴影 modifier 摆完整。</p>
      </div>
    </div>

    <div v-if="showSurfaces" class="ds-grid ds-grid--two">
      <article class="ds-card">
        <span class="ds-card__label">卡片阴影</span>
        <div class="ds-demo-stack">
          <div v-for="item in surfaceLevels" :key="item.label" class="ds-demo-row">
            <div class="ds-surface-demo" :class="item.className">
              <strong>{{ item.label }}</strong>
            </div>
            <span class="ds-code-tag">{{ item.className }}</span>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">面板背景</span>
        <div class="ds-demo-stack">
          <div v-for="item in surfaceTones" :key="item.label" class="ds-demo-row">
            <div class="ds-surface-demo ds-shadow--sm" :class="item.className">
              <strong>{{ item.label }}</strong>
            </div>
            <span class="ds-code-tag">{{ item.className }}</span>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showIcons" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">图标语义</h3>
        <p class="ds-section__meta">图标底板和状态色也直接跟正式 token 走。</p>
      </div>
    </div>

    <div v-if="showIcons" class="ds-icon-grid">
      <article v-for="icon in icons" :key="icon.label" class="ds-card ds-icon-card">
        <span class="ds-icon-shell" :class="icon.toneClass">
          <DesignSystemGlyph :name="icon.name" />
        </span>
        <strong class="ds-card__title ds-card__title--compact">{{ icon.label }}</strong>
        <span class="ds-code-tag">{{ icon.toneClass || "default" }}</span>
      </article>
    </div>

    <div v-if="showIcons" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">运行态图标库</h3>
        <p class="ds-section__meta">来源固定为 `uniapp/components/Icon.vue + static/iconfont/*`，这里直接展示高频语义映射。</p>
      </div>
    </div>

    <div v-if="showIcons" class="ds-grid ds-grid--two">
      <article class="ds-card">
        <span class="ds-card__label">高频语义映射</span>
        <div class="ds-icon-audit">
          <div v-for="item in iconMappings" :key="item.semantic" class="ds-icon-audit__row">
            <div class="ds-icon-audit__main">
              <span class="ds-icon-shell" :class="item.toneClass">
                <DesignSystemGlyph :name="item.previewIcon" />
              </span>
              <div class="ds-icon-audit__copy">
                <strong>{{ item.semantic }}</strong>
                <span>{{ item.note }}</span>
              </div>
            </div>
            <span class="ds-code-tag">{{ item.fontToken }}</span>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">图标资源冻结规则</span>
        <div class="ds-demo-stack">
          <div v-for="item in iconRules" :key="item.title" class="ds-demo-row ds-demo-row--stack">
            <strong class="ds-card__title ds-card__title--compact">{{ item.title }}</strong>
            <span class="ds-card__copy">{{ item.copy }}</span>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showCoverage" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">覆盖盘点</h3>
        <p class="ds-section__meta">组件库先做成唯一基线，后面网站和 uniapp 都按这里替换。</p>
      </div>
    </div>

    <div v-if="showCoverage" class="ds-grid ds-grid--three">
      <article v-for="group in coverageGroups" :key="group.title" class="ds-card">
        <span class="ds-card__label">{{ group.label }}</span>
        <span class="ds-card__title">{{ group.title }}</span>
        <ul class="ds-list ds-list--tight">
          <li v-for="item in group.items" :key="item">{{ item }}</li>
        </ul>
      </article>
    </div>

    <div v-if="showCoverage" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">来源组件覆盖台账</h3>
        <p class="ds-section__meta">把已经做过风格改动的来源组件挂进组件库台账，后续页面只能照这里映射。</p>
      </div>
    </div>

    <div v-if="showCoverage" class="ds-grid ds-grid--two">
      <article v-for="item in sourceAudits" :key="item.title" class="ds-card">
        <span class="ds-card__label">{{ item.label }}</span>
        <span class="ds-card__title">{{ item.title }}</span>
        <div class="ds-source-audit">
          <div v-for="entry in item.items" :key="entry.name" class="ds-source-audit__row">
            <div class="ds-source-audit__copy">
              <strong>{{ entry.name }}</strong>
              <span>{{ entry.preview }}</span>
            </div>
            <span class="ds-code-tag">已展示</span>
          </div>
        </div>
      </article>
    </div>
  </section>
</template>

<script setup>
import { computed } from "vue";
import DesignSystemGlyph from "./DesignSystemGlyph.vue";

const props = defineProps({
  sectionKey: {
    type: String,
    required: true,
  },
});

const showPalette = computed(() => props.sectionKey === "overview" || props.sectionKey === "colors");
const showSurfaces = computed(() => props.sectionKey === "overview" || props.sectionKey === "colors");
const showIcons = computed(() => props.sectionKey === "overview" || props.sectionKey === "icons");
const showCoverage = computed(() => props.sectionKey === "overview");

const tones = [
  { name: "主操作黄", color: "#FFCB45", token: "--nb-yellow" },
  { name: "编辑蓝", color: "#5EA8FF", token: "--nb-blue" },
  { name: "Info", color: "#909399", token: "--ds-info" },
  { name: "Success", color: "#67C23A", token: "--ds-success" },
  { name: "Warning", color: "#FFCB45", token: "--ds-warning" },
  { name: "Danger", color: "#F56C6C", token: "--ds-danger" },
];

const surfaceLevels = [
  { label: "无阴影", className: "ds-shadow--none" },
  { label: "轻阴影", className: "ds-shadow--sm" },
  { label: "标准阴影", className: "ds-shadow--md" },
  { label: "强调阴影", className: "ds-shadow--lg" },
];

const surfaceTones = [
  { label: "默认白底", className: "ds-surface--default" },
  { label: "轻黄面板", className: "ds-surface--yellow" },
  { label: "轻蓝面板", className: "ds-surface--blue" },
  { label: "浅灰面板", className: "ds-surface--muted" },
];

const icons = [
  { name: "home", label: "默认入口", toneClass: "" },
  { name: "picture", label: "主强调", toneClass: "ds-icon-shell--primary" },
  { name: "edit", label: "编辑语义", toneClass: "ds-icon-shell--blue" },
  { name: "sync", label: "Info", toneClass: "ds-icon-shell--info" },
  { name: "check", label: "Success", toneClass: "ds-icon-shell--success" },
  { name: "warning", label: "Warning", toneClass: "ds-icon-shell--warning" },
  { name: "trash", label: "Danger", toneClass: "ds-icon-shell--danger" },
];

const iconMappings = [
  { semantic: "picture", previewIcon: "picture", fontToken: "picture", toneClass: "ds-icon-shell--primary", note: "作品、缩略图、空状态" },
  { semantic: "palette", previewIcon: "palette", fontToken: "adjust", toneClass: "ds-icon-shell--warning", note: "调色板、颜色选择器" },
  { semantic: "check", previewIcon: "check", fontToken: "check-item", toneClass: "ds-icon-shell--success", note: "确认、多选、成功态" },
  { semantic: "trash", previewIcon: "trash", fontToken: "ashbin", toneClass: "ds-icon-shell--danger", note: "删除、清理、危险动作" },
  { semantic: "favorite", previewIcon: "favorite", fontToken: "favorite", toneClass: "ds-icon-shell--info", note: "收藏、点赞、作品状态" },
  { semantic: "link", previewIcon: "link", fontToken: "link", toneClass: "", note: "设备连接、网络状态" },
  { semantic: "notification", previewIcon: "notification", fontToken: "notification", toneClass: "", note: "消息、提醒、更新" },
  { semantic: "work", previewIcon: "work", fontToken: "work", toneClass: "", note: "成就、任务、工作台入口" },
];

const iconRules = [
  {
    title: "语义先映射，再进页面",
    copy: "页面要用新图标时，先补组件库和 Icon 语义映射，不再在业务页里直接临时换一个图。",
  },
  {
    title: "图标库单独展示",
    copy: "组件库必须能看见当前正式图标族和语义映射，不允许只在页面里零散出现几个图标。",
  },
  {
    title: "定板后禁止自由发挥",
    copy: "一旦语义、颜色和底板在组件库里定下来，后续 uniapp 页面只能按这套映射使用。",
  },
];

const coverageGroups = [
  {
    label: "来源范围",
    title: "本轮冻结的视觉资产来源",
    items: [
      "uniapp/components",
      "uniapp/pages/*",
      "uniapp-rebuild/components",
      "uniapp/static/iconfont/*",
    ],
  },
  {
    label: "正式覆盖",
    title: "组件库已经锁定的展示域",
    items: [
      "色板、阴影、状态色、图标底板",
      "官网导航、LegacyTabBar、品牌 Logo",
      "输入、开关、滑块、步进器、调色板、色彩拾取器",
      "Project / Artwork / Template / Challenge 卡片家族",
      "PixelPreviewBoard、PixelCanvas、ImageCropper、设备控制面板",
      "Toast、Modal、ConnectModal、JsonImportModal、LoadingOverlay",
      "九宫格 loader 与动作轻遮罩",
    ],
  },
  {
    label: "冻结规则",
    title: "组件库成为唯一定板源",
    items: [
      "凡是已经做过风格改动的控件和图标，都必须先在这里挂出正式展示位",
      "后续官网、uniapp、uniapp-rebuild 只能照组件库映射，不允许页面自己再发明一版",
      "新增图标或新控件先补组件库语义，再进入业务页",
    ],
  },
];

const sourceAudits = [
  {
    label: "控件族",
    title: "特殊控件与输入组件",
    items: [
      { name: "GlxSlider.vue", preview: "控件系统 / 滑块" },
      { name: "GlxStepper.vue", preview: "控件系统 / 步进器" },
      { name: "GlxSwitch.vue", preview: "控件系统 / 开关" },
      { name: "Input.vue", preview: "控件系统 / 搜索框、输入框、多行输入" },
      { name: "ColorPalette.vue", preview: "控件系统 / 调色板" },
      { name: "ColorPanelPicker.vue", preview: "控件系统 / 颜色面板拾取器" },
      { name: "Icon.vue", preview: "图标与底板 / 运行态图标库" },
      { name: "static/iconfont/*", preview: "图标与底板 / glyph 语义映射" },
    ],
  },
  {
    label: "卡片与预览",
    title: "内容卡片与编辑壳层",
    items: [
      { name: "ProjectCard.vue", preview: "列表与卡片 / ProjectCard" },
      { name: "ArtworkCard.vue", preview: "列表与卡片 / ArtworkCard" },
      { name: "TemplateCard.vue", preview: "列表与卡片 / TemplateCard" },
      { name: "ChallengeCard.vue", preview: "列表与卡片 / ChallengeCard" },
      { name: "Avatar.vue", preview: "列表与卡片 / Avatar 与评论头部" },
      { name: "Comment.vue", preview: "列表与卡片 / 评论卡" },
      { name: "PixelPreviewBoard.vue", preview: "列表与卡片 / 像素预览板" },
      { name: "PixelCanvas.vue", preview: "列表与卡片 / 像素画布壳层" },
      { name: "ImageCropper.vue", preview: "列表与卡片 / 裁剪器壳层" },
      { name: "ClockTextSettingsCard.vue", preview: "列表与卡片 / 时钟文字设置卡" },
      { name: "ClockThemePanel.vue", preview: "列表与卡片 / 时钟主题面板" },
      { name: "ClockFontPanel.vue", preview: "列表与卡片 / 时钟字体面板" },
    ],
  },
  {
    label: "弹窗与反馈",
    title: "反馈、导入与加载组件",
    items: [
      { name: "Modal.vue", preview: "弹窗与反馈 / 标准 Dialog" },
      { name: "Toast.vue", preview: "弹窗与反馈 / Toast" },
      { name: "ConfirmDialogHost.vue", preview: "弹窗与反馈 / Confirm host" },
      { name: "ConnectModal.vue", preview: "弹窗与反馈 / 连接设备弹窗" },
      { name: "ConfirmModal.vue", preview: "弹窗与反馈 / 危险确认卡" },
      { name: "HelpModal.vue", preview: "弹窗与反馈 / 帮助说明卡" },
      { name: "JsonImportModal.vue", preview: "弹窗与反馈 / JSON 导入弹窗" },
      { name: "LoadingOverlay.vue", preview: "骨架与加载 / 动作轻遮罩" },
      { name: "GlxInlineLoader.vue", preview: "骨架与加载 / 内容区局部 loader" },
      { name: "GlxLogoLoader.vue", preview: "骨架与加载 / 品牌九宫格 loader" },
    ],
  },
  {
    label: "导航与品牌",
    title: "导航壳与品牌资产",
    items: [
      { name: "Logo.vue", preview: "头部与导航 / 品牌 Logo" },
      { name: "LegacyTabBar.vue", preview: "头部与导航 / 小程序底部导航" },
    ],
  },
];
</script>
