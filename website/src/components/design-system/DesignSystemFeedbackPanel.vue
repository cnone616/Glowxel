<template>
  <section class="ds-section">
    <div class="ds-section__head">
      <div>
        <h2 class="ds-section__title">骨架、Loading 与反馈</h2>
        <p class="ds-section__meta">页面内容用骨架或局部 loader，动作阻塞再上轻遮罩；反馈组件也继续沿用上一版硬边块面</p>
      </div>
    </div>

    <div v-if="showLoading" class="ds-skeleton-grid">
      <article v-for="item in skeletons" :key="item.title" class="ds-skeleton-card">
        <div class="ds-skeleton-card__head">
          <span class="ds-icon-shell" :class="item.tone">
            <DesignSystemGlyph :name="item.icon" />
          </span>
          <strong>{{ item.title }}</strong>
        </div>
        <div class="ds-skeleton-card__body">
          <div class="ds-skeleton-block" :class="{ 'ds-skeleton-block--poster': item.poster }"></div>
          <div class="ds-skeleton-bar"></div>
          <div class="ds-skeleton-block"></div>
          <div class="ds-skeleton-bar ds-skeleton-bar--short"></div>
        </div>
      </article>
    </div>

    <div v-if="showLoading" class="ds-grid ds-grid--two ds-grid--spaced">
      <article class="ds-example">
        <span class="ds-card__label">正式 Loader</span>
        <div class="ds-loader-stack">
          <div v-for="item in officialLoaders" :key="item.title" class="ds-loader-box">
            <div class="ds-loader-grid" :class="item.className" aria-hidden="true">
              <span v-for="index in 9" :key="item.key + index"></span>
            </div>
            <div>
              <strong class="ds-card__title ds-card__title--tight">{{ item.title }}</strong>
              <p class="ds-card__copy">{{ item.copy }}</p>
            </div>
          </div>
        </div>
      </article>

      <article class="ds-example">
        <span class="ds-card__label">动作轻遮罩</span>
        <div class="ds-showcase">
          <div class="ds-loader-box ds-loader-box--emphasis">
            <div class="ds-loader-grid ds-loader-grid--chase" aria-hidden="true">
              <span v-for="index in 9" :key="'mask-' + index"></span>
            </div>
            <div>
              <strong class="ds-card__title ds-card__title--tight">发送中…</strong>
              <p class="ds-card__copy">传输流程固定使用 5 号（Chase），并且只锁当前内容区，不做厚重全屏模态层。</p>
            </div>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showLoading" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">九宫格 Loading 家族</h3>
        <p class="ds-section__meta">现存来源方案 10 款，当前正式采用 3 款。</p>
      </div>
    </div>

    <article v-if="showLoading" class="ds-card">
      <span class="ds-card__label">来源: logo-loading-showcase.html</span>
      <div class="ds-loader-family">
        <div v-for="item in loaderFamily" :key="item.key" class="ds-loader-family__item">
          <div class="ds-loader-grid" :class="item.className" aria-hidden="true">
            <span v-for="index in 9" :key="item.key + '-family-' + index"></span>
          </div>
          <div class="ds-loader-family__meta">
            <strong>{{ item.title }}</strong>
            <span class="ds-loader-chip" :class="'ds-loader-chip--' + item.statusTone">{{ item.statusLabel }}</span>
          </div>
        </div>
      </div>
    </article>

    <div v-if="showLoading" class="ds-grid ds-grid--two ds-grid--spaced">
      <article class="ds-card">
        <span class="ds-card__label">GlxLogoLoader.vue</span>
        <div class="ds-logo-loader-showcase">
          <div v-for="item in officialLoaders" :key="'logo-' + item.key" class="ds-logo-loader-showcase__item">
            <div class="ds-logo-loader-showcase__stage">
              <div class="ds-loader-grid" :class="item.className" aria-hidden="true">
                <span v-for="index in 9" :key="'logo-grid-' + item.key + index"></span>
              </div>
            </div>
            <strong>{{ item.title }}</strong>
            <span>{{ item.copy }}</span>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">GlxInlineLoader.vue / LoadingOverlay.vue</span>
        <div class="ds-inline-loader-preview">
          <div class="ds-loader-box">
            <div class="ds-loader-grid ds-loader-grid--flip" aria-hidden="true">
              <span v-for="index in 9" :key="'inline-' + index"></span>
            </div>
            <div>
              <strong class="ds-card__title ds-card__title--tight">内容加载中</strong>
              <p class="ds-card__copy">列表尾部、局部刷新和面板内容继续走内容区 loader。</p>
            </div>
          </div>
          <div class="ds-light-mask-preview">
            <div class="ds-light-mask-preview__body">
              <span class="ds-light-mask-preview__bar"></span>
              <span class="ds-light-mask-preview__bar ds-light-mask-preview__bar--short"></span>
              <span class="ds-light-mask-preview__panel"></span>
            </div>
            <div class="ds-light-mask-preview__card">
              <div class="ds-loader-grid ds-loader-grid--chase" aria-hidden="true">
                <span v-for="index in 9" :key="'light-mask-' + index"></span>
              </div>
              <span>发送中…</span>
            </div>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showFeedback" class="ds-grid ds-grid--two ds-grid--spaced">
      <article class="ds-example">
        <span class="ds-card__label">Toast.vue</span>
        <div class="ds-toast-stack">
          <div class="ds-toast ds-toast--info">
            <span class="ds-icon-shell ds-icon-shell--info">
              <DesignSystemGlyph name="sync" />
            </span>
            <span class="ds-card__copy">正在重新读取设备参数，请保持网络稳定。</span>
          </div>
          <div class="ds-toast ds-toast--success">
            <span class="ds-icon-shell ds-icon-shell--success">
              <DesignSystemGlyph name="check" />
            </span>
            <span class="ds-card__copy">同步成功，最新项目已经写入云端。</span>
          </div>
          <div class="ds-toast ds-toast--warning">
            <span class="ds-icon-shell ds-icon-shell--warning">
              <DesignSystemGlyph name="warning" />
            </span>
            <span class="ds-card__copy">当前操作需要确认后继续执行。</span>
          </div>
          <div class="ds-toast ds-toast--danger">
            <span class="ds-icon-shell ds-icon-shell--danger">
              <DesignSystemGlyph name="trash" />
            </span>
            <span class="ds-card__copy">删除失败，请稍后再试。</span>
          </div>
        </div>
      </article>

      <article class="ds-example">
        <span class="ds-card__label">Modal.vue / ConfirmDialogHost.vue</span>
        <div class="ds-dialog">
          <div class="ds-dialog__head">
            <span class="ds-icon-shell ds-icon-shell--danger">
              <DesignSystemGlyph name="trash" />
            </span>
            <div class="ds-title-stack">
              <span class="ds-title-stack__title ds-title-stack__title--dialog">删除作品</span>
              <span class="ds-title-stack__desc">Danger 固定走更接近 Element 的标准红色。</span>
            </div>
          </div>
          <div class="ds-dialog__body">
            危险确认不再用偏粉的旧红色，也不再单独做一套柔和专题页标题语言。它只是官网体系里的一个标准反馈组件。
          </div>
          <div class="ds-dialog__foot">
            <button type="button" class="ds-btn">再想想</button>
            <button type="button" class="ds-btn ds-btn--danger">
              <DesignSystemGlyph name="trash" />
              <span>确认删除</span>
            </button>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showFeedback" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">来源弹窗壳层</h3>
        <p class="ds-section__meta">ConnectModal / HelpModal / ConfirmModal 这类 uniapp 弹窗也要回收到组件库，不再只靠通用对话框代替。</p>
      </div>
    </div>

    <div v-if="showFeedback" class="ds-grid ds-grid--three">
      <article class="ds-card">
        <span class="ds-card__label">ConnectModal.vue</span>
        <div class="ds-connect-modal">
          <div class="ds-connect-modal__head">
            <strong>连接设备</strong>
            <button type="button" class="ds-connect-modal__close">
              <DesignSystemGlyph name="close" />
            </button>
          </div>
          <div class="ds-connect-modal__body">
            <p class="ds-card__copy">请输入 Glowxel PixelBoard 的 IP 地址。</p>
            <div class="ds-connect-modal__input">
              <input value="192.168.31.84" class="ds-input ds-input--bare" readonly />
            </div>
            <div class="ds-connect-modal__status ds-connect-modal__status--connecting">
              <div class="ds-loader-grid ds-loader-grid--flip" aria-hidden="true">
                <span v-for="index in 9" :key="'connecting-' + index"></span>
              </div>
              <span>连接中...</span>
            </div>
            <div class="ds-connect-modal__status ds-connect-modal__status--error">
              <span class="ds-icon-shell ds-icon-shell--danger">
                <DesignSystemGlyph name="close" />
              </span>
              <span>连接超时，请重试</span>
            </div>
          </div>
          <div class="ds-connect-modal__foot">
            <button type="button" class="ds-btn">取消</button>
            <button type="button" class="ds-btn ds-btn--primary">连接</button>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">Help / Confirm 家族</span>
        <div class="ds-help-stack">
          <div class="ds-help-card">
            <span class="ds-icon-shell ds-icon-shell--warning">
              <DesignSystemGlyph name="help" />
            </span>
            <div class="ds-help-card__copy">
              <strong>HelpModal.vue</strong>
              <span>说明卡片、图标底板和正文块继续沿用直角硬边语法。</span>
            </div>
          </div>
          <div class="ds-help-card">
            <span class="ds-icon-shell ds-icon-shell--danger">
              <DesignSystemGlyph name="trash" />
            </span>
            <div class="ds-help-card__copy">
              <strong>ConfirmModal.vue</strong>
              <span>危险确认继续走 Element 风格 danger 红，不再另造一套粉红确认按钮。</span>
            </div>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">JsonImportModal.vue</span>
        <div class="ds-json-modal">
          <div class="ds-json-modal__head">
            <strong>导入配置</strong>
            <button type="button" class="ds-connect-modal__close">
              <DesignSystemGlyph name="close" />
            </button>
          </div>
          <div class="ds-json-modal__body">
            <p class="ds-card__copy">JSON 导入弹窗继续沿用直角边框、代码输入框和底部双按钮结构。</p>
            <div class="ds-json-modal__textarea">
              <span>{</span>
              <span>&nbsp;&nbsp;"brightness": 72,</span>
              <span>&nbsp;&nbsp;"mode": "clock_theme"</span>
              <span>}</span>
            </div>
          </div>
          <div class="ds-json-modal__foot">
            <button type="button" class="ds-btn">取消</button>
            <button type="button" class="ds-btn ds-btn--primary">确定</button>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showFeedback" class="ds-grid ds-grid--three ds-grid--spaced">
      <article v-for="rule in rules" :key="rule.title" class="ds-card">
        <span class="ds-card__label">{{ rule.label }}</span>
        <span class="ds-card__title">{{ rule.title }}</span>
        <p class="ds-card__copy">{{ rule.copy }}</p>
      </article>
    </div>

    <div v-if="showRules" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">Uniapp 加载与反馈约束</h3>
        <p class="ds-section__meta">把已经锁定的 loading 与反馈语义直接收成规范块，后面继续扩页时不再猜。</p>
      </div>
    </div>

    <div v-if="showRules" class="ds-grid ds-grid--three">
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
import DesignSystemGlyph from "./DesignSystemGlyph.vue";

const props = defineProps({
  sectionKey: {
    type: String,
    required: true,
  },
});

const showLoading = computed(() => props.sectionKey === "skeletons");
const showFeedback = computed(() => props.sectionKey === "feedback");
const showRules = computed(() => props.sectionKey === "rules");

const skeletons = [
  { title: "首页骨架", icon: "home", tone: "ds-icon-shell--primary", poster: true },
  { title: "社区骨架", icon: "community", tone: "ds-icon-shell--blue" },
  { title: "设备骨架", icon: "device", tone: "ds-icon-shell--success" },
  { title: "我的骨架", icon: "profile", tone: "ds-icon-shell--info" },
];

const rules = [
  {
    label: "内容加载",
    title: "优先骨架或局部 loader",
    copy: "页面首屏、列表和主题内容优先显示骨架，不再把 showLoading 当整页默认入口。",
  },
  {
    label: "动作阻塞",
    title: "动作遮罩只锁当前内容区",
    copy: "保存、发送、同步、删除可以锁动作，但视觉停留在内容区，不盖官网导航。",
  },
  {
    label: "反馈语义",
    title: "Toast / Dialog 全部回到四类状态色",
    copy: "Info、Success、Warning、Danger 在官网和 uniapp 里都用同一套定义，不再一页一个口径。",
  },
];

const officialLoaders = [
  {
    key: "chase",
    title: "Chase",
    copy: "用于发送/传输流程，固定为 5 号正式动效。",
    className: "ds-loader-grid--chase",
  },
  {
    key: "flip",
    title: "Flip",
    copy: "用于切换内容区、卡片块和小范围刷新。",
    className: "ds-loader-grid--flip",
  },
  {
    key: "stamp",
    title: "Stamp",
    copy: "用于常规等待态，与 Flip 一起参与随机池。",
    className: "ds-loader-grid--stamp",
  },
];

const loaderFamily = [
  { key: "wave", title: "Wave", className: "ds-loader-grid--wave", statusLabel: "保留", statusTone: "info" },
  { key: "column", title: "Column", className: "ds-loader-grid--column", statusLabel: "保留", statusTone: "info" },
  { key: "glitch", title: "Glitch", className: "ds-loader-grid--glitch", statusLabel: "保留", statusTone: "info" },
  { key: "collapse", title: "Collapse", className: "ds-loader-grid--collapse", statusLabel: "保留", statusTone: "info" },
  { key: "chase", title: "Chase", className: "ds-loader-grid--chase", statusLabel: "正式", statusTone: "success" },
  { key: "flip", title: "Flip", className: "ds-loader-grid--flip", statusLabel: "正式", statusTone: "success" },
  { key: "slide", title: "Slide", className: "ds-loader-grid--slide", statusLabel: "保留", statusTone: "info" },
  { key: "liquid", title: "Liquid", className: "ds-loader-grid--liquid", statusLabel: "保留", statusTone: "info" },
  { key: "stamp", title: "Stamp", className: "ds-loader-grid--stamp", statusLabel: "正式", statusTone: "success" },
  { key: "link", title: "Link", className: "ds-loader-grid--link", statusLabel: "保留", statusTone: "info" },
];

const guardrails = [
  {
    label: "内容加载",
    title: "首屏和列表优先骨架",
    copy: "页面进入、tab 切换和首屏内容加载不再默认整屏 loading，先看骨架和局部彩色 loader。",
  },
  {
    label: "动作反馈",
    title: "showLoading 收窄为动作轻遮罩",
    copy: "保存、发送、同步、删除、读取参数这类阻塞动作才允许上轻遮罩；其中发送/传输统一固定 Chase。",
  },
  {
    label: "品牌语言",
    title: "只用品牌九宫格 loader",
    copy: "黑色 spinner、默认系统 loading 和无语义小圆圈都不再进入正式页面。",
  },
];
</script>
