<template>
  <div class="confirm-panel">
    <div class="panel-head">
      <h2>颜色确认</h2>
      <span>{{ groups.length }} 组颜色</span>
    </div>

    <div v-if="report" class="report-card">
      <div class="report-row">
        <span>网格识别</span>
        <strong>{{ report.width }} × {{ report.height }}</strong>
      </div>
      <div class="report-row">
        <span>裁剪区域</span>
        <strong>{{ report.cropWidth }} × {{ report.cropHeight }}</strong>
      </div>
      <div class="report-row">
        <span>编号辅助</span>
        <strong>{{ report.recognizedCellCount }}/{{ report.totalCells }}</strong>
      </div>
    </div>

    <div v-if="groups.length" class="group-list">
      <div
        v-for="group in groups"
        :key="group.code"
        class="group-card"
        :class="{ active: group.code === focusColorCode }"
      >
        <div class="group-top">
          <div class="group-main">
            <span class="color-dot" :style="{ backgroundColor: group.hex }"></span>
            <div>
              <strong>{{ group.code }}</strong>
              <p>{{ group.name }}</p>
            </div>
          </div>
          <em>{{ group.count }} 格</em>
        </div>

        <div class="similar-box">
          <span class="similar-label">感觉不对时，直接换成相近色</span>
          <div class="similar-grid">
            <button
              v-for="option in group.similarOptions"
              :key="`${group.code}-${option.code}`"
              class="similar-btn"
              type="button"
              @click="$emit('replace-group-color', group.code, option.code)"
            >
              <i :style="{ backgroundColor: option.hex }"></i>
              <span>{{ option.code }}</span>
            </button>
          </div>
        </div>
      </div>
    </div>

    <div v-else class="empty-state">
      先导入图纸后，这里会按颜色分组，用户可以直接确认或替换。
    </div>
  </div>
</template>

<script setup>
defineProps({
  groups: {
    type: Array,
    default: () => [],
  },
  report: {
    type: Object,
    default: null,
  },
  focusColorCode: {
    type: String,
    default: "",
  },
})

defineEmits(["replace-group-color"])
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

.report-card,
.group-card,
.empty-state {
  border-radius: 18px;
  border: 1px solid #dbe4f2;
}

.report-card,
.group-card {
  background: #f7fbff;
}

.report-card {
  padding: 14px;
  margin-bottom: 14px;
}

.report-row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  color: #617089;
  font-size: 13px;
}

.report-row + .report-row {
  margin-top: 8px;
}

.report-row strong {
  color: #18304f;
}

.group-list {
  display: grid;
  gap: 12px;
}

.group-card {
  padding: 14px;
}

.group-card.active {
  border-color: #79b7ff;
  box-shadow: 0 10px 24px rgba(49, 109, 255, 0.1);
}

.group-top {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: center;
}

.group-main {
  display: flex;
  gap: 10px;
  align-items: center;
}

.group-main strong {
  color: #18304f;
  font-size: 15px;
}

.group-main p {
  margin-top: 4px;
  color: #6d7c92;
  font-size: 12px;
}

.group-top em {
  color: #375374;
  font-style: normal;
  font-size: 12px;
  font-weight: 700;
}

.color-dot {
  width: 22px;
  height: 22px;
  border-radius: 50%;
  border: 1px solid rgba(9, 19, 33, 0.15);
  flex-shrink: 0;
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
  background: #f8fafc;
  border-style: dashed;
  color: #6d7c92;
  font-size: 13px;
  line-height: 1.7;
}
</style>
