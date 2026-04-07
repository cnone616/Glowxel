<template>
  <div class="selection-panel">
    <div class="panel-head">
      <h2>区域处理</h2>
      <span>{{ selectionCount }} 点</span>
    </div>

    <div v-if="selectionCount" class="selection-detail">
      <div class="detail-row">
        <span>范围</span>
        <strong>{{ boundsText }}</strong>
      </div>
      <div class="detail-row">
        <span>宽高</span>
        <strong>{{ sizeText }}</strong>
      </div>
    </div>

    <div v-else class="empty-state">
      在中间画布按住鼠标拖拽，就能框选一片区域后再批量处理。
    </div>

    <div class="action-grid">
      <button
        class="action-btn primary"
        type="button"
        :disabled="!selectionCount || !hasReference"
        @click="$emit('region-action', 'reference')"
      >
        区域按参考
      </button>
      <button
        class="action-btn"
        type="button"
        :disabled="!selectionCount"
        @click="$emit('region-action', 'remove')"
      >
        区域擦除
      </button>
      <button
        class="action-btn"
        type="button"
        :disabled="!selectionCount"
        @click="$emit('region-action', 'neighbor')"
      >
        区域邻域主色
      </button>
      <button
        class="action-btn"
        type="button"
        :disabled="!selectionCount"
        @click="$emit('clear-selection')"
      >
        清空区域
      </button>
    </div>
  </div>
</template>

<script setup>
import { computed } from "vue";

const props = defineProps({
  selectionCount: {
    type: Number,
    default: 0,
  },
  selectionBounds: {
    type: Object,
    default: null,
  },
  hasReference: {
    type: Boolean,
    default: false,
  },
});

defineEmits(["region-action", "clear-selection"]);

const boundsText = computed(() => {
  if (!props.selectionBounds) {
    return "--";
  }
  const { minX, minY, maxX, maxY } = props.selectionBounds;
  return `(${minX}, ${minY}) -> (${maxX}, ${maxY})`;
});

const sizeText = computed(() => {
  if (!props.selectionBounds) {
    return "--";
  }
  const width = props.selectionBounds.maxX - props.selectionBounds.minX + 1;
  const height = props.selectionBounds.maxY - props.selectionBounds.minY + 1;
  return `${width} × ${height}`;
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

.selection-detail {
  padding: 14px;
  border-radius: 18px;
  background: #f7fbff;
  border: 1px solid #dbe4f2;
}

.detail-row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  font-size: 13px;
  color: #617089;
}

.detail-row + .detail-row {
  margin-top: 8px;
}

.detail-row strong {
  color: #18304f;
}

.empty-state {
  padding: 14px;
  border-radius: 18px;
  background: #f8fafc;
  border: 1px dashed #cfdceb;
  color: #6d7c92;
  font-size: 13px;
  line-height: 1.7;
}

.action-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
  margin-top: 14px;
}

.action-btn {
  height: 40px;
  padding: 0 14px;
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
  color: #fff;
  border-color: transparent;
}

.action-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}
</style>
