<template>
  <div class="standardize-panel">
    <div class="panel-head">
      <h2>标准化替换</h2>
      <span>{{ groups.length }} 组颜色偏差</span>
    </div>

    <div v-if="groups.length" class="group-list">
      <div v-for="group in groups" :key="group.id" class="group-card">
        <div class="group-top">
          <div class="color-pair">
            <span class="color-badge" :style="{ backgroundColor: group.sourceHex }">{{ group.sourceCode }}</span>
            <span class="arrow">→</span>
            <span class="color-badge" :style="{ backgroundColor: group.targetHex }">{{ group.targetCode }}</span>
          </div>
          <strong>{{ group.count }} 处</strong>
        </div>

        <p class="group-meta">
          实拍校验里这组颜色最常见，优先考虑替换成观察色，或者选一个更标准的相近色。
        </p>

        <div class="group-stats">
          <span>局部可减少 {{ group.estimatedFixCount }} 处偏差</span>
          <span>当前图纸共 {{ group.globalSourceCount }} 处 {{ group.sourceCode }}</span>
        </div>

        <div class="action-row">
          <button
            class="action-btn primary"
            type="button"
            @click="$emit('apply-group-replacement', group, group.targetCode, 'group')"
          >
            仅替换这 {{ group.count }} 处
          </button>
          <button
            class="action-btn"
            type="button"
            @click="$emit('apply-group-replacement', group, group.targetCode, 'global')"
          >
            全图替换 {{ group.sourceCode }}
          </button>
        </div>

        <div v-if="group.similarOptions.length" class="similar-box">
          <span class="similar-label">相近备选</span>
          <div class="similar-grid">
            <button
              v-for="option in group.similarOptions"
              :key="`${group.id}-${option.code}`"
              class="similar-btn"
              type="button"
              @click="$emit('apply-group-replacement', group, option.code, 'group')"
            >
              <i :style="{ backgroundColor: option.hex }"></i>
              <span>{{ option.code }}</span>
            </button>
          </div>
        </div>
      </div>
    </div>

    <div v-else class="empty-state">
      还没有可聚合的颜色偏差。导入实拍图后，这里会把最常见的颜色误差收敛成可替换建议。
    </div>
  </div>
</template>

<script setup>
defineProps({
  groups: {
    type: Array,
    default: () => [],
  },
});

defineEmits(["apply-group-replacement"]);
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

.group-list {
  display: grid;
  gap: 12px;
}

.group-card {
  padding: 14px;
  border-radius: 18px;
  background: #f7fbff;
  border: 1px solid #dbe4f2;
}

.group-top {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: center;
}

.group-top strong {
  color: #18304f;
  font-size: 13px;
}

.color-pair {
  display: flex;
  align-items: center;
  gap: 8px;
}

.color-badge {
  min-width: 54px;
  height: 28px;
  padding: 0 10px;
  border-radius: 999px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 1px solid rgba(9, 19, 33, 0.12);
  color: #18304f;
  font-size: 12px;
  font-weight: 700;
}

.arrow {
  color: #6d7c92;
  font-size: 12px;
  font-weight: 700;
}

.group-meta {
  margin-top: 10px;
  color: #6d7c92;
  font-size: 13px;
  line-height: 1.7;
}

.group-stats {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  margin-top: 10px;
  color: #617089;
  font-size: 12px;
  font-weight: 700;
}

.action-row {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
  margin-top: 12px;
}

.action-btn {
  height: 38px;
  padding: 0 10px;
  border: 1px solid #dbe4f2;
  border-radius: 12px;
  background: #fff;
  color: #27405f;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
}

.action-btn.primary {
  background: linear-gradient(135deg, #2f6dff, #55c4ff);
  border-color: transparent;
  color: var(--nb-ink);
}

.similar-box {
  margin-top: 12px;
}

.similar-label {
  display: block;
  margin-bottom: 8px;
  color: #617089;
  font-size: 12px;
  font-weight: 700;
}

.similar-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 8px;
}

.similar-btn {
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

.similar-btn i {
  width: 14px;
  height: 14px;
  border-radius: 50%;
  border: 1px solid rgba(9, 19, 33, 0.15);
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
</style>
