<template>
  <div class="issues-panel">
    <div class="panel-head">
      <h2>问题列表</h2>
      <span>{{ issues.length }} 项显示中</span>
    </div>

    <div class="filter-row">
      <button
        v-for="item in filterOptions"
        :key="item.value"
        class="filter-chip"
        :class="{ active: filterType === item.value }"
        type="button"
        @click="$emit('change-filter', item.value)"
      >
        {{ item.label }}
      </button>
    </div>

    <div class="issue-list">
      <button
        v-for="issue in issues"
        :key="issue.id"
        class="issue-card"
        :class="{ active: selectedIssueId === issue.id }"
        type="button"
        @click="$emit('select', issue.id)"
      >
        <span class="issue-type">{{ issue.type }}</span>
        <strong>{{ issue.title }}</strong>
        <span class="issue-meta">{{ issue.meta }}</span>
      </button>
    </div>
    <div v-if="!issues.length" class="empty-state">
      当前筛选条件下没有问题项，换个筛选或重新导入参考图就能继续比对。
    </div>
  </div>
</template>

<script setup>
defineProps({
  issues: {
    type: Array,
    default: () => [],
  },
  selectedIssueId: {
    type: String,
    default: "",
  },
  filterType: {
    type: String,
    default: "all",
  },
  filterOptions: {
    type: Array,
    default: () => [],
  },
});

defineEmits(["select", "change-filter"]);
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

.filter-row {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 14px;
}

.filter-chip {
  border: 0;
  height: 32px;
  padding: 0 12px;
  border-radius: 999px;
  background: #eef4fb;
  color: #375374;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
}

.filter-chip.active {
  background: #1d68ff;
  color: var(--nb-ink);
}

.issue-list {
  display: grid;
  gap: 10px;
}

.issue-card {
  display: flex;
  flex-direction: column;
  gap: 6px;
  align-items: flex-start;
  width: 100%;
  padding: 14px;
  border-radius: 18px;
  background: #f6f9fd;
  border: 1px solid #dbe4f2;
  text-align: left;
  cursor: pointer;
}

.issue-card.active {
  border-color: #79b7ff;
  background: linear-gradient(180deg, #f7fbff, #edf5ff);
}

.issue-type {
  padding: 4px 8px;
  border-radius: 999px;
  background: #dceeff;
  color: #1d68ff;
  font-size: 11px;
  font-weight: 700;
}

.issue-card strong {
  color: #18304f;
  line-height: 1.5;
}

.issue-meta {
  color: #6d7c92;
  font-size: 13px;
  line-height: 1.6;
}

.empty-state {
  margin-top: 12px;
  padding: 14px;
  border-radius: 18px;
  background: #f8fafc;
  border: 1px dashed #cfdceb;
  color: #6d7c92;
  font-size: 13px;
  line-height: 1.7;
}
</style>
