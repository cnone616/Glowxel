<template>
  <div class="inspector-panel">
    <div class="panel-head">
      <h2>当前问题</h2>
      <span>{{ issueLabel }}</span>
    </div>

    <div v-if="selectedIssue" class="issue-detail">
      <strong>{{ selectedIssue.title }}</strong>
      <p>{{ selectedIssue.meta }}</p>
      <div class="detail-grid">
        <div class="detail-card">
          <span>坐标</span>
          <strong>{{ positionText }}</strong>
        </div>
        <div class="detail-card">
          <span>优先级</span>
          <strong>{{ selectedIssue.severity }}</strong>
        </div>
      </div>

      <div v-if="pixelContext" class="pixel-context">
        <div class="context-row">
          <span>当前颜色</span>
          <strong>{{ pixelContext.currentCode }}</strong>
        </div>
        <div class="context-row">
          <span>参考颜色</span>
          <strong>{{ pixelContext.referenceCode }}</strong>
        </div>
      </div>

      <div class="suggestion-box">
        {{ suggestionText }}
      </div>
    </div>

    <div v-else class="empty-state">
      选中右侧问题项后，这里会显示问题详情、坐标和修正建议。
    </div>

    <div class="panel-head actions-head">
      <h2>修正动作</h2>
      <span>{{ actionStateText }}</span>
    </div>

    <div class="action-grid">
      <button
        class="action-btn primary"
        type="button"
        :disabled="!selectedIssue"
        @click="$emit('issue-action', 'suggestion')"
      >
        应用建议
      </button>
      <button
        class="action-btn"
        type="button"
        :disabled="!selectedIssue"
        @click="$emit('issue-action', 'reference')"
      >
        改成参考
      </button>
      <button
        class="action-btn"
        type="button"
        :disabled="!selectedIssue"
        @click="$emit('issue-action', 'neighbor')"
      >
        合并邻域色
      </button>
      <button
        class="action-btn"
        type="button"
        :disabled="!selectedIssue"
        @click="$emit('issue-action', 'remove')"
      >
        删除当前点
      </button>
      <button
        class="action-btn"
        type="button"
        :disabled="!selectedIssue"
        @click="$emit('issue-action', 'ignore')"
      >
        忽略此项
      </button>
    </div>

    <div class="panel-head actions-head">
      <h2>批量处理</h2>
      <span>{{ filteredIssueCount }} 项</span>
    </div>

    <div class="action-grid">
      <button
        class="action-btn primary"
        type="button"
        :disabled="!filteredIssueCount"
        @click="$emit('batch-action', 'suggestion')"
      >
        当前筛选应用建议
      </button>
      <button
        class="action-btn"
        type="button"
        :disabled="!filteredIssueCount"
        @click="$emit('batch-action', 'reference')"
      >
        当前筛选按参考
      </button>
      <button
        class="action-btn"
        type="button"
        :disabled="!filteredIssueCount"
        @click="$emit('batch-action', 'ignore')"
      >
        当前筛选忽略
      </button>
      <button
        class="action-btn"
        type="button"
        :disabled="!ignoredIssueCount"
        @click="$emit('clear-ignored')"
      >
        清空忽略
      </button>
    </div>
  </div>
</template>

<script setup>
import { computed } from "vue";

const props = defineProps({
  selectedIssue: {
    type: Object,
    default: null,
  },
  pixelContext: {
    type: Object,
    default: null,
  },
  filteredIssueCount: {
    type: Number,
    default: 0,
  },
  ignoredIssueCount: {
    type: Number,
    default: 0,
  },
});

defineEmits(["issue-action", "batch-action", "clear-ignored"]);

const issueLabel = computed(() => {
  if (!props.selectedIssue) {
    return "未选中";
  }
  return props.selectedIssue.type;
});

const positionText = computed(() => {
  if (!props.selectedIssue) {
    return "--";
  }
  if (!props.selectedIssue.position) {
    return "--";
  }
  return `(${props.selectedIssue.position.x}, ${props.selectedIssue.position.y})`;
});

const suggestionText = computed(() => {
  if (!props.selectedIssue) {
    return "";
  }
  if (props.selectedIssue.type === "孤立点") {
    return "建议先检查周围 8 邻域颜色，优先合并到邻域主色，减少碎点。";
  }
  if (props.selectedIssue.type === "缺失点") {
    return "当前图纸缺少参考像素，后续可以直接补点或批量按参考图修正。";
  }
  if (props.selectedIssue.type === "多余点") {
    return "当前位置在当前图纸存在额外像素，建议结合整体轮廓判断是否删除。";
  }
  if (props.selectedIssue.type === "颜色偏差") {
    return "当前位置颜色与参考图不一致，优先检查是否需要替换为参考色号。";
  }
  return "这个问题已经定位出来了，下一步可以接批量修正和局部编辑动作。";
});

const actionStateText = computed(() => {
  if (!props.selectedIssue) {
    return "请选择问题";
  }
  return "已接入真实修改";
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

.actions-head {
  margin-top: 18px;
}

.issue-detail {
  padding: 16px;
  border-radius: 18px;
  background: #f7fbff;
  border: 1px solid #dbe4f2;
}

.issue-detail strong {
  display: block;
  color: #18304f;
  line-height: 1.6;
}

.issue-detail p,
.empty-state {
  margin-top: 8px;
  color: #6d7c92;
  line-height: 1.7;
  font-size: 14px;
}

.empty-state {
  padding: 16px;
  border-radius: 18px;
  background: #f8fafc;
  border: 1px dashed #cfdceb;
}

.detail-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
  margin-top: 14px;
}

.detail-card {
  padding: 12px;
  border-radius: 14px;
  background: #eef5ff;
  border: 1px solid #dbe4f2;
}

.detail-card span {
  display: block;
  font-size: 12px;
  color: #6d7c92;
}

.detail-card strong {
  display: block;
  margin-top: 6px;
  color: #18304f;
  font-size: 14px;
}

.pixel-context {
  margin-top: 14px;
  padding: 12px 14px;
  border-radius: 14px;
  background: #fff;
  border: 1px solid #dbe4f2;
}

.context-row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  color: #617089;
  font-size: 13px;
}

.context-row + .context-row {
  margin-top: 8px;
}

.context-row strong {
  color: #18304f;
}

.suggestion-box {
  margin-top: 14px;
  padding: 12px 14px;
  border-radius: 14px;
  background: #f0f7ff;
  border: 1px solid #dbe4f2;
  color: #375374;
  font-size: 13px;
  line-height: 1.7;
}

.action-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
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
  color: var(--nb-ink);
  border-color: transparent;
}

.action-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}
</style>
