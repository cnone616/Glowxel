<script setup>
import { computed } from 'vue'

const props = defineProps({
  project: {
    type: Object,
    required: true,
  },
})

defineEmits(['open', 'delete'])

const updatedLabel = computed(() => {
  return new Date(props.project.updatedAt).toLocaleString('zh-CN', {
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
  })
})
</script>

<template>
  <article class="project-card panel">
    <div class="project-top">
      <div>
        <p class="tiny">最近编辑</p>
        <h3>{{ project.name }}</h3>
      </div>
      <span class="chip">{{ project.materialCount }} 素材</span>
    </div>

    <div class="project-metrics">
      <div>
        <span class="tiny">更新时间</span>
        <strong>{{ updatedLabel }}</strong>
      </div>
      <div>
        <span class="tiny">场景对象</span>
        <strong>{{ project.objectCount }}</strong>
      </div>
      <div>
        <span class="tiny">当前导出帧</span>
        <strong>{{ project.frameCount }}</strong>
      </div>
    </div>

    <div class="project-actions">
      <button @click="$emit('open')">进入工作室</button>
      <button class="secondary" @click="$emit('delete')">删除</button>
    </div>
  </article>
</template>

<style scoped>
.project-card {
  padding: 20px;
  display: grid;
  gap: 18px;
}

.project-top,
.project-actions,
.project-metrics {
  display: flex;
  justify-content: space-between;
  gap: 12px;
}

.project-top {
  align-items: start;
}

.project-top h3 {
  margin: 4px 0 0;
  font-size: 22px;
}

.project-metrics strong {
  display: block;
  margin-top: 6px;
}

.project-actions {
  flex-wrap: wrap;
}
</style>
