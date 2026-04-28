<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">QA</span>
      <h1 class="glx-page-shell__title">Design Compare</h1>
      <p class="glx-page-shell__desc">
        这是隐藏的比对页，用来记录网站正式页面和 `uniapp` 对照关系，不进入主导航。
      </p>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-stack">
        <article v-for="item in compareRows" :key="item.webRoute" class="glx-list-card compare-row">
          <div class="glx-list-card__copy">
            <strong class="glx-list-card__title">{{ item.title }}</strong>
            <span class="glx-list-card__desc">网站：{{ item.webRoute }} ｜ UniApp：{{ item.uniappPage }}</span>
          </div>
          <span class="glx-chip" :class="statusClass(item.status)">{{ item.status }}</span>
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
const compareRows = [
  { title: "工作台", webRoute: "/workspace", uniappPage: "pages/workspace/workspace", status: "active" },
  { title: "项目总览", webRoute: "/overview/:id", uniappPage: "pages/overview/overview", status: "active" },
  { title: "辅助处理", webRoute: "/assist/:id", uniappPage: "pages/assist/assist", status: "active" },
  { title: "发布作品", webRoute: "/publish-project/:id", uniappPage: "pages/publish-project/publish-project", status: "active" },
  { title: "个人中心", webRoute: "/profile", uniappPage: "pages/profile/profile", status: "active" },
  { title: "云端同步", webRoute: "/cloud-sync", uniappPage: "pages/cloud-sync/cloud-sync", status: "active" },
  { title: "设备控制", webRoute: "/device-control", uniappPage: "pages/control/control", status: "active" },
];

function statusClass(status) {
  if (status === "active") {
    return "glx-chip--green";
  }
  return "glx-chip--yellow";
}
</script>

<style scoped>
.compare-row {
  justify-content: space-between;
}
</style>
