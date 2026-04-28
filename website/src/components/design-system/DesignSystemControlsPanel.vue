<template>
  <section class="ds-section">
    <div class="ds-section__head">
      <div>
        <h2 class="ds-section__title">按钮与控件</h2>
        <p class="ds-section__meta">先把语义、尺寸、阴影和基础控件摆完整，解释文字只保留最少量。</p>
      </div>
    </div>

    <div v-if="showButtons" class="ds-grid ds-grid--three">
      <article v-for="button in buttons" :key="button.title" class="ds-card">
        <span class="ds-card__label">{{ button.label }}</span>
        <div class="ds-button-preview">
          <button type="button" class="ds-btn ds-btn--md ds-shadow--sm" :class="button.tone">
            <DesignSystemGlyph :name="button.icon" />
            <span>{{ button.title }}</span>
          </button>
          <span class="ds-code-tag">{{ button.tone }}</span>
        </div>
      </article>
    </div>

    <div v-if="showButtons" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">尺寸与阴影类</h3>
        <p class="ds-section__meta">按钮大小和阴影强度都必须能直接走类名 modifier。</p>
      </div>
    </div>

    <div v-if="showButtons" class="ds-grid ds-grid--two">
      <article class="ds-card">
        <span class="ds-card__label">按钮尺寸</span>
        <div class="ds-demo-stack">
          <div v-for="item in buttonSizes" :key="item.label" class="ds-demo-row">
            <button type="button" class="ds-btn ds-btn--primary ds-shadow--sm" :class="item.className">
              <DesignSystemGlyph name="check" />
              <span>{{ item.label }}</span>
            </button>
            <span class="ds-code-tag">{{ item.className }}</span>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">阴影等级</span>
        <div class="ds-demo-stack">
          <div v-for="item in shadowLevels" :key="item.label" class="ds-demo-row">
            <button type="button" class="ds-btn ds-btn--edit ds-btn--md" :class="item.className">
              <DesignSystemGlyph name="edit" />
              <span>{{ item.label }}</span>
            </button>
            <span class="ds-code-tag">{{ item.className }}</span>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showButtons" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">按钮状态</h3>
        <p class="ds-section__meta">默认、加载和禁用至少要先摆成一组正式语法。</p>
      </div>
    </div>

    <div v-if="showButtons" class="ds-grid ds-grid--three">
      <article v-for="item in buttonStates" :key="item.label" class="ds-card">
        <span class="ds-card__label">{{ item.label }}</span>
        <button
          type="button"
          class="ds-btn ds-btn--md"
          :class="[item.tone, { 'is-loading': item.loading }]"
          :disabled="item.disabled"
        >
          <DesignSystemGlyph :name="item.icon" />
          <span>{{ item.title }}</span>
        </button>
      </article>
    </div>

    <div v-if="showControls" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">基础控件预演</h3>
        <p class="ds-section__meta">保留真实交互，后面继续补覆盖时直接沿用这里的组件结构。</p>
      </div>
    </div>

    <div v-if="showControls" class="ds-form-grid">
      <div class="ds-field">
        <label class="ds-field__label" for="ds-search-field">搜索框</label>
        <div class="ds-search">
          <DesignSystemGlyph name="search" />
          <input id="ds-search-field" v-model="searchValue" class="ds-input ds-input--bare" />
        </div>
      </div>

      <div class="ds-field">
        <label class="ds-field__label" for="ds-select-field">选择框</label>
        <div class="ds-select">
          <select id="ds-select-field" v-model="selectedMode">
            <option value="canvas">画板模式</option>
            <option value="pattern">拼豆工作台</option>
            <option value="device">设备参数</option>
          </select>
          <DesignSystemGlyph name="arrow-down" />
        </div>
      </div>

      <div class="ds-field">
        <span class="ds-field__label">开关</span>
        <div class="ds-switch">
          <div class="ds-switch__copy">
            <strong>自动同步</strong>
            <span class="ds-switch__state">{{ syncEnabled ? "已开启" : "已关闭" }}</span>
          </div>
          <button
            type="button"
            class="ds-switch__track"
            :class="{ 'is-active': syncEnabled }"
            @click="syncEnabled = !syncEnabled"
          >
            <span class="ds-switch__thumb"></span>
          </button>
        </div>
      </div>

      <div class="ds-field">
        <span class="ds-field__label">滑块</span>
        <div class="ds-card ds-card--compact">
          <div class="ds-slider__head">
            <span>亮度</span>
            <span>{{ brightness }}%</span>
          </div>
          <div class="ds-slider">
            <input v-model="brightness" type="range" min="0" max="100" />
          </div>
        </div>
      </div>

      <div class="ds-field">
        <span class="ds-field__label">步进器</span>
        <div class="ds-stepper">
          <button type="button" class="ds-stepper__btn" @click="decreaseSpeed">-</button>
          <span class="ds-stepper__value">速度 {{ speed }}</span>
          <button type="button" class="ds-stepper__btn" @click="increaseSpeed">+</button>
        </div>
      </div>

      <div class="ds-field">
        <span class="ds-field__label">单选组</span>
        <div class="ds-choice-row">
          <button
            v-for="item in qualityOptions"
            :key="item.value"
            type="button"
            class="ds-choice-chip"
            :class="{ 'is-active': quality === item.value }"
            @click="quality = item.value"
          >
            {{ item.label }}
          </button>
        </div>
      </div>

      <div class="ds-field">
        <span class="ds-field__label">多选组</span>
        <div class="ds-checkbox-stack">
          <button
            v-for="item in featureOptions"
            :key="item.value"
            type="button"
            class="ds-check-row"
            :class="{ 'is-active': selectedFeatures.includes(item.value) }"
            @click="toggleFeature(item.value)"
          >
            <span class="ds-check-row__box">
              <DesignSystemGlyph v-if="selectedFeatures.includes(item.value)" name="check" />
            </span>
            <span>{{ item.label }}</span>
          </button>
        </div>
      </div>

      <div class="ds-field">
        <span class="ds-field__label">当前预演状态</span>
        <div class="ds-card">
          <span class="ds-card__label">状态汇总</span>
          <span class="ds-card__title">{{ selectedModeLabel }}</span>
          <p class="ds-card__copy">
            搜索词“{{ searchKeywordLabel }}”，同步 {{ syncEnabled ? "开启" : "关闭" }}，亮度 {{ brightness }}%，速度 {{ speed }}，质量 {{ qualityLabel }}，特性 {{ featureSummary }}。
          </p>
        </div>
      </div>

      <div class="ds-field">
        <label class="ds-field__label" for="ds-notes-field">多行输入</label>
        <textarea id="ds-notes-field" v-model="notes" class="ds-textarea"></textarea>
      </div>

      <div class="ds-field">
        <span class="ds-field__label">错误态</span>
        <div class="ds-card ds-card--compact">
          <input v-model="invalidHost" class="ds-input ds-input--danger" />
          <span class="ds-help-text ds-help-text--danger">设备地址不完整，请补齐最后一段。</span>
        </div>
      </div>
    </div>

    <div v-if="showControls" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">调色控件家族</h3>
        <p class="ds-section__meta">直接对照 uniapp 的 ColorPalette 和 ColorPanelPicker，不再让颜色选择器只活在业务页里。</p>
      </div>
    </div>

    <div v-if="showControls" class="ds-grid ds-grid--two">
      <article class="ds-card">
        <span class="ds-card__label">ColorPalette.vue</span>
        <div class="ds-palette-shell">
          <div class="ds-palette-shell__main">
            <div class="ds-palette-shell__head">
              <strong>调色板 ({{ paletteCount }} 色)</strong>
              <span>{{ selectedPaletteCode }}</span>
            </div>

            <div class="ds-palette-groups">
              <div v-for="group in paletteGroups" :key="group.letter" class="ds-palette-group">
                <div class="ds-palette-group__head">
                  <strong>{{ group.letter }}</strong>
                  <span>({{ group.colors.length }})</span>
                </div>
                <div class="ds-palette-grid">
                  <button
                    v-for="color in group.colors"
                    :key="color.code"
                    type="button"
                    class="ds-palette-item"
                    :class="{ 'is-active': selectedPaletteColor === color.hex }"
                    @click="selectPaletteColor(color)"
                  >
                    <span class="ds-palette-item__swatch" :style="{ backgroundColor: color.hex }"></span>
                    <span class="ds-palette-item__code">{{ color.code }}</span>
                  </button>
                </div>
              </div>
            </div>
          </div>

          <div class="ds-palette-index">
            <button
              v-for="group in paletteGroups"
              :key="group.letter + '-index'"
              type="button"
              class="ds-palette-index__item"
              :class="{ 'is-active': activePaletteLetter === group.letter }"
              @click="activePaletteLetter = group.letter"
            >
              {{ group.letter }}
            </button>
          </div>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">ColorPanelPicker.vue</span>
        <div class="ds-picker">
          <div class="ds-picker__head">
            <div class="ds-picker__preview">
              <span class="ds-picker__swatch" :style="{ backgroundColor: currentPickerHex }"></span>
              <div class="ds-picker__meta">
                <strong>当前颜色</strong>
                <span>{{ currentPickerHex.toUpperCase() }}</span>
              </div>
            </div>
            <div class="ds-picker__presets">
              <button
                v-for="preset in pickerPresets"
                :key="preset.hex"
                type="button"
                class="ds-picker__preset"
                :class="{ 'is-active': currentPickerHex === preset.hex }"
                :style="{ backgroundColor: preset.hex }"
                @click="currentPickerHex = preset.hex"
              ></button>
            </div>
          </div>

          <div class="ds-picker__panel" :style="{ backgroundColor: pickerHueColor }">
            <div class="ds-picker__panel-white"></div>
            <div class="ds-picker__panel-black"></div>
            <span class="ds-picker__panel-thumb" :style="pickerThumbStyle"></span>
          </div>

          <div class="ds-picker__channel">
            <span>色相</span>
            <div class="ds-picker__hue-track">
              <span class="ds-picker__hue-thumb" :style="{ left: pickerHuePercent + '%' }"></span>
            </div>
          </div>

          <div class="ds-picker__fields">
            <label class="ds-picker__field ds-picker__field--hex">
              <span>HEX</span>
              <input v-model="pickerHexInput" class="ds-input" @blur="commitPickerHex" />
            </label>
            <label class="ds-picker__field">
              <span>R</span>
              <input :value="pickerRgb.r" class="ds-input" readonly />
            </label>
            <label class="ds-picker__field">
              <span>G</span>
              <input :value="pickerRgb.g" class="ds-input" readonly />
            </label>
            <label class="ds-picker__field">
              <span>B</span>
              <input :value="pickerRgb.b" class="ds-input" readonly />
            </label>
          </div>
        </div>
      </article>
    </div>

    <div v-if="showControls" class="ds-section__head ds-section__head--spaced">
      <div>
        <h3 class="ds-section__title ds-section__title--sub">菜单、上传与数据列表</h3>
        <p class="ds-section__meta">把后面最容易反复返工的高频组件也先收进基线页。</p>
      </div>
    </div>

    <div v-if="showControls" class="ds-grid ds-grid--two">
      <article class="ds-card">
        <span class="ds-card__label">动作菜单</span>
        <div class="ds-action-menu">
          <button
            v-for="item in actionMenuItems"
            :key="item.value"
            type="button"
            class="ds-action-menu__item"
            :class="{ 'is-active': activeAction === item.value }"
            @click="activeAction = item.value"
          >
            <span class="ds-action-menu__main">
              <span class="ds-icon-shell" :class="item.tone">
                <DesignSystemGlyph :name="item.icon" />
              </span>
              <span>{{ item.label }}</span>
            </span>
            <span class="ds-code-tag">{{ item.code }}</span>
          </button>
        </div>
      </article>

      <article class="ds-card">
        <span class="ds-card__label">上传字段</span>
        <div class="ds-upload-panel">
          <div class="ds-upload-panel__head">
            <div>
              <strong class="ds-card__title ds-card__title--compact">资源文件</strong>
              <p class="ds-card__copy">按钮、文件行和删除动作保持同一套硬边语法。</p>
            </div>
            <button type="button" class="ds-btn ds-btn--sm ds-btn--edit ds-shadow--sm" @click="toggleUploadSample">
              <DesignSystemGlyph name="picture" />
              <span>{{ uploadButtonLabel }}</span>
            </button>
          </div>

          <div class="ds-upload-list">
            <div v-for="file in uploadedFiles" :key="file.name" class="ds-upload-item">
              <div class="ds-upload-item__meta">
                <span class="ds-icon-shell ds-icon-shell--blue">
                  <DesignSystemGlyph name="picture" />
                </span>
                <div class="ds-upload-item__copy">
                  <strong>{{ file.name }}</strong>
                  <span>{{ file.meta }}</span>
                </div>
              </div>
              <button type="button" class="ds-upload-item__remove" @click="removeUploadedFile(file.name)">
                移除
              </button>
            </div>
          </div>
        </div>
      </article>
    </div>

    <article v-if="showControls" class="ds-card ds-card--top-spaced">
      <span class="ds-card__label">轻量表格与分页</span>
      <div class="ds-table">
        <div class="ds-table__head">
          <span>名称</span>
          <span>状态</span>
          <span>更新时间</span>
        </div>
        <div v-for="row in pagedRows" :key="row.name" class="ds-table__row">
          <span class="ds-table__name">{{ row.name }}</span>
          <span class="ds-status-pill" :class="'ds-status-pill--' + row.statusTone">{{ row.statusLabel }}</span>
          <span>{{ row.updatedAt }}</span>
        </div>
      </div>

      <div class="ds-pagination">
        <button
          v-for="page in pageNumbers"
          :key="page"
          type="button"
          class="ds-pagination__item"
          :class="{ 'is-active': activePage === page }"
          @click="activePage = page"
        >
          {{ page }}
        </button>
      </div>
    </article>
  </section>
</template>

<script setup>
import { computed, ref, watch } from "vue";
import DesignSystemGlyph from "./DesignSystemGlyph.vue";

const props = defineProps({
  sectionKey: {
    type: String,
    required: true,
  },
});

const showButtons = computed(() => props.sectionKey === "buttons");
const showControls = computed(() => props.sectionKey === "controls");

const buttons = [
  { label: "主操作", title: "立即发送", icon: "check", tone: "ds-btn--primary" },
  { label: "编辑类", title: "编辑资料", icon: "edit", tone: "ds-btn--edit" },
  { label: "Info", title: "同步中", icon: "sync", tone: "ds-btn--info" },
  { label: "Success", title: "保存成功", icon: "check", tone: "ds-btn--success" },
  { label: "Warning", title: "需要确认", icon: "warning", tone: "ds-btn--warning" },
  { label: "Danger", title: "删除作品", icon: "trash", tone: "ds-btn--danger" },
];

const buttonSizes = [
  { label: "超小", className: "ds-btn--xs" },
  { label: "小号", className: "ds-btn--sm" },
  { label: "中号", className: "ds-btn--md" },
  { label: "大号", className: "ds-btn--lg" },
];

const shadowLevels = [
  { label: "无阴影", className: "ds-shadow--none" },
  { label: "轻阴影", className: "ds-shadow--sm" },
  { label: "标准阴影", className: "ds-shadow--md" },
  { label: "强调阴影", className: "ds-shadow--lg" },
];

const buttonStates = [
  { label: "默认", title: "立即发送", icon: "check", tone: "ds-btn--primary" },
  { label: "加载中", title: "发送中", icon: "sync", tone: "ds-btn--edit", loading: true },
  { label: "禁用", title: "暂不可用", icon: "warning", tone: "ds-btn--warning", disabled: true },
];

const searchValue = ref("拼豆图纸");
const selectedMode = ref("canvas");
const syncEnabled = ref(true);
const brightness = ref(68);
const speed = ref(2);
const quality = ref("standard");
const notes = ref("适用于设备参数说明、作品备注和同步提示。");
const invalidHost = ref("192.168.0.");
const selectedFeatures = ref(["grid", "preview"]);
const activeAction = ref("send");
const activePage = ref(1);
const selectedPaletteColor = ref("#ffcb45");
const activePaletteLetter = ref("A");
const currentPickerHex = ref("#ffcb45");
const pickerHexInput = ref("#FFCB45");

const qualityOptions = [
  { value: "compact", label: "紧凑" },
  { value: "standard", label: "标准" },
  { value: "dense", label: "高密度" },
];

const featureOptions = [
  { value: "grid", label: "显示网格" },
  { value: "preview", label: "实时预览" },
  { value: "upload", label: "同步缩略图" },
];

const paletteGroups = [
  {
    letter: "A",
    colors: [
      { code: "A01", hex: "#ffcb45" },
      { code: "A02", hex: "#f97316" },
      { code: "A03", hex: "#ef4444" },
      { code: "A04", hex: "#fbbf24" },
      { code: "A05", hex: "#f59e0b" },
    ],
  },
  {
    letter: "B",
    colors: [
      { code: "B01", hex: "#5ea8ff" },
      { code: "B02", hex: "#3b82f6" },
      { code: "B03", hex: "#0ea5e9" },
      { code: "B04", hex: "#38bdf8" },
      { code: "B05", hex: "#67c23a" },
    ],
  },
  {
    letter: "C",
    colors: [
      { code: "C01", hex: "#f56c6c" },
      { code: "C02", hex: "#909399" },
      { code: "C03", hex: "#111111" },
      { code: "C04", hex: "#ffffff" },
      { code: "C05", hex: "#8b5cf6" },
    ],
  },
];

const pickerPresets = [
  { hex: "#ffcb45" },
  { hex: "#5ea8ff" },
  { hex: "#67c23a" },
  { hex: "#f56c6c" },
  { hex: "#111111" },
];

const actionMenuItems = [
  { value: "send", label: "立即发送", icon: "check", tone: "ds-icon-shell--primary", code: "primary" },
  { value: "duplicate", label: "复制项目", icon: "copy", tone: "ds-icon-shell--info", code: "info" },
  { value: "rename", label: "重命名", icon: "edit", tone: "ds-icon-shell--blue", code: "edit" },
  { value: "remove", label: "删除", icon: "trash", tone: "ds-icon-shell--danger", code: "danger" },
];

const uploadExtraFile = {
  name: "canvas-preview.gif",
  meta: "256KB / 发送预览",
};

const uploadedFiles = ref([
  {
    name: "pattern-plan.glx",
    meta: "12KB / 图纸方案",
  },
  {
    name: "canvas-scene.png",
    meta: "48KB / 预览素材",
  },
]);

const tableRows = [
  { name: "画板模式", statusLabel: "已发布", statusTone: "success", updatedAt: "04-18 21:40" },
  { name: "拼豆工作台", statusLabel: "审核中", statusTone: "warning", updatedAt: "04-18 17:22" },
  { name: "像素挑战海报", statusLabel: "草稿", statusTone: "info", updatedAt: "04-17 09:18" },
  { name: "设备连接引导", statusLabel: "已发布", statusTone: "success", updatedAt: "04-16 13:08" },
  { name: "设备参数模板", statusLabel: "已停用", statusTone: "danger", updatedAt: "04-15 10:45" },
  { name: "社区封面图", statusLabel: "草稿", statusTone: "info", updatedAt: "04-14 08:12" },
];

const pageSize = 3;

const searchKeywordLabel = computed(() => {
  if (searchValue.value) {
    return searchValue.value;
  }
  return "未输入";
});

const selectedModeLabel = computed(() => {
  if (selectedMode.value === "canvas") return "画板模式";
  if (selectedMode.value === "pattern") return "拼豆工作台";
  return "设备参数";
});

const qualityLabel = computed(() => {
  if (quality.value === "compact") return "紧凑";
  if (quality.value === "dense") return "高密度";
  return "标准";
});

const featureSummary = computed(() => {
  if (selectedFeatures.value.length === 0) return "无";
  return featureOptions
    .filter((item) => selectedFeatures.value.includes(item.value))
    .map((item) => item.label)
    .join(" / ");
});

const uploadButtonLabel = computed(() => {
  const exists = uploadedFiles.value.some((item) => item.name === uploadExtraFile.name);
  if (exists) {
    return "移除示例";
  }
  return "添加示例";
});

const paletteCount = computed(() => paletteGroups.reduce((sum, group) => sum + group.colors.length, 0));

const selectedPaletteCode = computed(() => {
  for (const group of paletteGroups) {
    const matched = group.colors.find((item) => item.hex === selectedPaletteColor.value);
    if (matched) {
      return matched.code;
    }
  }
  return "未选择";
});

const pickerRgb = computed(() => hexToRgb(currentPickerHex.value));

const pickerHsv = computed(() => rgbToHsv(pickerRgb.value.r, pickerRgb.value.g, pickerRgb.value.b));

const pickerHuePercent = computed(() => Math.round((pickerHsv.value.h / 360) * 100));

const pickerHueColor = computed(() => {
  const { r, g, b } = hsvToRgb(pickerHsv.value.h, 100, 100);
  return rgbToHex(r, g, b);
});

const pickerThumbStyle = computed(() => ({
  left: pickerHsv.value.s + "%",
  top: 100 - pickerHsv.value.v + "%",
  backgroundColor: currentPickerHex.value,
}));

watch(currentPickerHex, (value) => {
  pickerHexInput.value = value.toUpperCase();
});

const pageNumbers = computed(() => {
  const totalPages = Math.ceil(tableRows.length / pageSize);
  return Array.from({ length: totalPages }, (_, index) => index + 1);
});

const pagedRows = computed(() => {
  const start = (activePage.value - 1) * pageSize;
  const end = start + pageSize;
  return tableRows.slice(start, end);
});

const increaseSpeed = () => {
  if (speed.value < 6) speed.value += 1;
};

const decreaseSpeed = () => {
  if (speed.value > 1) speed.value -= 1;
};

const toggleFeature = (value) => {
  if (selectedFeatures.value.includes(value)) {
    selectedFeatures.value = selectedFeatures.value.filter((item) => item !== value);
    return;
  }
  selectedFeatures.value = [...selectedFeatures.value, value];
};

const toggleUploadSample = () => {
  const exists = uploadedFiles.value.some((item) => item.name === uploadExtraFile.name);
  if (exists) {
    uploadedFiles.value = uploadedFiles.value.filter((item) => item.name !== uploadExtraFile.name);
    return;
  }
  uploadedFiles.value = [...uploadedFiles.value, uploadExtraFile];
};

const removeUploadedFile = (name) => {
  uploadedFiles.value = uploadedFiles.value.filter((item) => item.name !== name);
};

const selectPaletteColor = (color) => {
  selectedPaletteColor.value = color.hex;
  activePaletteLetter.value = color.code.charAt(0).toUpperCase();
  currentPickerHex.value = color.hex;
  pickerHexInput.value = color.hex.toUpperCase();
};

const commitPickerHex = () => {
  const normalized = normalizeHex(pickerHexInput.value);
  if (normalized.length === 0) {
    pickerHexInput.value = currentPickerHex.value.toUpperCase();
    return;
  }
  currentPickerHex.value = normalized;
  pickerHexInput.value = normalized.toUpperCase();
};

function clamp(value, min, max) {
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}

function normalizeHex(value) {
  if (typeof value !== "string") {
    return "";
  }
  const raw = value.trim();
  const body = raw.startsWith("#") ? raw.slice(1) : raw;
  if (!/^[0-9a-fA-F]{6}$/.test(body)) {
    return "";
  }
  return "#" + body.toLowerCase();
}

function rgbToHex(r, g, b) {
  return (
    "#" +
    r.toString(16).padStart(2, "0") +
    g.toString(16).padStart(2, "0") +
    b.toString(16).padStart(2, "0")
  );
}

function hexToRgb(hex) {
  const normalized = normalizeHex(hex);
  return {
    r: parseInt(normalized.slice(1, 3), 16),
    g: parseInt(normalized.slice(3, 5), 16),
    b: parseInt(normalized.slice(5, 7), 16),
  };
}

function rgbToHsv(r, g, b) {
  const red = r / 255;
  const green = g / 255;
  const blue = b / 255;
  const max = Math.max(red, green, blue);
  const min = Math.min(red, green, blue);
  const delta = max - min;
  let hue = 0;

  if (delta !== 0) {
    if (max === red) {
      hue = ((green - blue) / delta) % 6;
    } else if (max === green) {
      hue = (blue - red) / delta + 2;
    } else {
      hue = (red - green) / delta + 4;
    }
  }

  hue = Math.round(hue * 60);
  if (hue < 0) {
    hue = hue + 360;
  }

  const saturation = max === 0 ? 0 : Math.round((delta / max) * 100);
  const brightness = Math.round(max * 100);
  return { h: hue, s: saturation, v: brightness };
}

function hsvToRgb(h, s, v) {
  const hue = ((h % 360) + 360) % 360;
  const saturation = clamp(s, 0, 100) / 100;
  const brightness = clamp(v, 0, 100) / 100;
  const chroma = brightness * saturation;
  const x = chroma * (1 - Math.abs(((hue / 60) % 2) - 1));
  const match = brightness - chroma;
  let red = 0;
  let green = 0;
  let blue = 0;

  if (hue < 60) {
    red = chroma;
    green = x;
  } else if (hue < 120) {
    red = x;
    green = chroma;
  } else if (hue < 180) {
    green = chroma;
    blue = x;
  } else if (hue < 240) {
    green = x;
    blue = chroma;
  } else if (hue < 300) {
    red = x;
    blue = chroma;
  } else {
    red = chroma;
    blue = x;
  }

  return {
    r: Math.round((red + match) * 255),
    g: Math.round((green + match) * 255),
    b: Math.round((blue + match) * 255),
  };
}
</script>
