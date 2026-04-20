<template>
  <div class="edit-panel">
    <div class="panel-head">
      <h2>编辑工具</h2>
      <span>{{ toolLabel }}</span>
    </div>

    <div class="tool-row">
      <button
        v-for="item in toolOptions"
        :key="item.value"
        class="tool-btn"
        :class="{ active: toolMode === item.value }"
        type="button"
        @click="$emit('change-tool', item.value)"
      >
        {{ item.label }}
      </button>
    </div>

    <div class="selected-color">
      <span>当前颜色</span>
      <strong>{{ selectedColorCode || '未选择' }}</strong>
      <i class="selected-color-dot" :style="{ backgroundColor: selectedColorHex }"></i>
    </div>

    <input
      v-model="searchQuery"
      class="search-input"
      type="text"
      placeholder="搜索色号或名称"
    >

    <div class="quick-section">
      <span class="section-label">常用颜色</span>
      <div class="color-grid">
        <button
          v-for="item in usedColors.slice(0, 18)"
          :key="item.code"
          class="color-chip"
          :class="{ active: item.code === selectedColorCode }"
          type="button"
          :title="item.code"
          @click="$emit('select-color', item.code)"
        >
          <i :style="{ backgroundColor: item.hex }"></i>
          <span>{{ item.code }}</span>
        </button>
      </div>
    </div>

    <div class="quick-section">
      <span class="section-label">颜色库</span>
      <div class="color-grid all">
        <button
          v-for="item in filteredColors.slice(0, 36)"
          :key="item.code"
          class="color-chip"
          :class="{ active: item.code === selectedColorCode }"
          type="button"
          :title="`${item.code} ${item.name}`"
          @click="$emit('select-color', item.code)"
        >
          <i :style="{ backgroundColor: item.hex }"></i>
          <span>{{ item.code }}</span>
        </button>
      </div>
    </div>

    <div class="quick-section">
      <span class="section-label">快捷处理</span>
      <div class="action-grid">
        <button
          class="action-btn primary"
          type="button"
          :disabled="!selectionCount || !selectedColorCode"
          @click="$emit('selection-action', 'fill')"
        >
          区域填充所选色
        </button>
        <button
          class="action-btn"
          type="button"
          :disabled="!focusColorCode || !selectedColorCode"
          @click="$emit('replace-focus-color')"
        >
          全图替换焦点色
        </button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, ref } from "vue";

const props = defineProps({
  toolMode: {
    type: String,
    default: "inspect",
  },
  selectedColorCode: {
    type: String,
    default: "",
  },
  selectedColorHex: {
    type: String,
    default: "#152131",
  },
  usedColors: {
    type: Array,
    default: () => [],
  },
  allColors: {
    type: Array,
    default: () => [],
  },
  selectionCount: {
    type: Number,
    default: 0,
  },
  focusColorCode: {
    type: String,
    default: "",
  },
});

defineEmits([
  "change-tool",
  "select-color",
  "selection-action",
  "replace-focus-color",
]);

const searchQuery = ref("");

const toolOptions = [
  { value: "inspect", label: "查看" },
  { value: "select", label: "框选" },
  { value: "paint", label: "画笔" },
  { value: "erase", label: "橡皮" },
];

const filteredColors = computed(() => {
  if (!searchQuery.value) {
    return props.allColors;
  }
  const keyword = searchQuery.value.toLowerCase();
  return props.allColors.filter((item) => {
    return item.code.toLowerCase().includes(keyword) || item.name.toLowerCase().includes(keyword);
  });
});

const toolLabel = computed(() => {
  const matchedTool = toolOptions.find((item) => item.value === props.toolMode);
  if (!matchedTool) {
    return "查看";
  }
  return matchedTool.label;
});
</script>

<style scoped>
.panel-head {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  gap: 12px;
  margin-bottom: 14px;
}

.panel-head h2 {
  font-size: 18px;
  color: #18304f;
}

.panel-head span {
  font-size: 12px;
  color: #7b889f;
}

.tool-row {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 8px;
}

.tool-btn {
  height: 36px;
  border: 1px solid #dbe4f2;
  border-radius: 12px;
  background: #eff4fb;
  color: #27405f;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
}

.tool-btn.active {
  background: linear-gradient(135deg, #2f6dff, #55c4ff);
  border-color: transparent;
  color: var(--nb-ink);
}

.selected-color {
  display: grid;
  grid-template-columns: auto 1fr 16px;
  gap: 10px;
  align-items: center;
  margin-top: 14px;
  padding: 12px 14px;
  border-radius: 16px;
  background: #f7fbff;
  border: 1px solid #dbe4f2;
  color: #617089;
  font-size: 13px;
}

.selected-color strong {
  color: #18304f;
  text-align: right;
}

.selected-color-dot {
  width: 16px;
  height: 16px;
  border-radius: 50%;
  border: 1px solid rgba(9, 19, 33, 0.15);
}

.search-input {
  width: 100%;
  height: 38px;
  margin-top: 14px;
  padding: 0 12px;
  border: 1px solid #dbe4f2;
  border-radius: 12px;
  background: #fff;
  color: #18304f;
  font-size: 13px;
  outline: none;
}

.quick-section {
  margin-top: 14px;
}

.section-label {
  display: block;
  margin-bottom: 10px;
  color: #617089;
  font-size: 12px;
  font-weight: 700;
}

.color-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 8px;
}

.color-grid.all {
  max-height: 232px;
  overflow: auto;
  padding-right: 2px;
}

.color-chip {
  display: grid;
  grid-template-columns: 14px 1fr;
  gap: 8px;
  align-items: center;
  height: 34px;
  padding: 0 10px;
  border: 1px solid #dbe4f2;
  border-radius: 10px;
  background: #fff;
  color: #27405f;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
  text-align: left;
}

.color-chip i {
  width: 14px;
  height: 14px;
  border-radius: 50%;
  border: 1px solid rgba(9, 19, 33, 0.15);
}

.color-chip.active {
  background: #eef6ff;
  border-color: #79b7ff;
}

.action-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.action-btn {
  height: 40px;
  padding: 0 12px;
  border: 1px solid #dbe4f2;
  border-radius: 12px;
  background: #eff4fb;
  color: #27405f;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

.action-btn.primary {
  background: linear-gradient(135deg, #2f6dff, #55c4ff);
  border-color: transparent;
  color: var(--nb-ink);
}

.action-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}
</style>
