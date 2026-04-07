<template>
  <div class="source-panel">
    <div class="panel-head">
      <h2>开始转换</h2>
      <span>先选类型，再走对应流程</span>
    </div>

    <div class="source-actions primary-actions">
      <button class="chip action primary" type="button" @click="$emit('trigger-import', 'current-image')">
        图片生成
      </button>
      <button class="chip action primary" type="button" @click="$emit('trigger-import', 'numbered-sheet-image')">
        上传拼豆图纸
      </button>
      <button class="chip action" type="button" @click="$emit('trigger-import', 'capture-review-image')">
        导入实拍图校验
      </button>
    </div>

    <div class="source-summary">
      <div class="summary-row">
        <span>当前图纸</span>
        <strong>{{ document.name }}</strong>
      </div>
      <div class="summary-row">
        <span>当前尺寸</span>
        <strong>{{ document.width }} × {{ document.height }}</strong>
      </div>
      <div class="summary-row">
        <span>实拍校验</span>
        <strong>{{ captureReviewName }}</strong>
      </div>
      <div v-if="importReport" class="summary-row">
        <span>识别区域</span>
        <strong>{{ importReport.cropWidth }} × {{ importReport.cropHeight }}</strong>
      </div>
      <div v-if="importReport" class="summary-row">
        <span>去背景</span>
        <strong>{{ importReport.backgroundRemoved ? '已开启' : '未开启' }}</strong>
      </div>
    </div>

    <div class="source-actions compact-actions">
      <button
        class="chip action"
        type="button"
        :disabled="!captureReviewDocument"
        @click="$emit('clear-capture-review')"
      >
        清空实拍校验
      </button>
      <button class="chip action subtle" type="button" @click="$emit('export-json')">
        导出结果
      </button>
    </div>
  </div>
</template>

<script setup>
import { computed } from "vue";

const props = defineProps({
  document: {
    type: Object,
    required: true,
  },
  referenceDocument: {
    type: Object,
    default: null,
  },
  captureReviewDocument: {
    type: Object,
    default: null,
  },
  importReport: {
    type: Object,
    default: null,
  },
});

defineEmits(["trigger-import", "clear-reference", "clear-capture-review", "export-json"]);

const captureReviewName = computed(() => {
  if (!props.captureReviewDocument) {
    return "未导入";
  }
  return props.captureReviewDocument.name;
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

.source-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.primary-actions {
  display: grid;
  grid-template-columns: 1fr;
}

.chip {
  border: 0;
  cursor: pointer;
  padding: 9px 12px;
  border-radius: 999px;
  background: #eef4fb;
  color: #375374;
  font-size: 13px;
}

.chip.action {
  border: 1px solid #d7e3f4;
}

.chip.action.primary {
  background: linear-gradient(135deg, #2f6dff, #58c0ff);
  border-color: transparent;
  color: #ffffff;
}

.chip:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.chip.action.subtle {
  background: #f7fbff;
}

.source-summary {
  margin-top: 14px;
  padding: 14px;
  border-radius: 16px;
  background: #f7fbff;
  border: 1px solid #dbe4f2;
}

.summary-row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  font-size: 13px;
  color: #617089;
}

.summary-row + .summary-row {
  margin-top: 8px;
}

.summary-row strong {
  color: #18304f;
  text-align: right;
}

.compact-actions {
  margin-top: 14px;
}
</style>
