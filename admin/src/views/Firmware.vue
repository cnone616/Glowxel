<template>
  <div>
    <a-card title="固件版本管理" :bordered="false">
      <template #extra>
        <a-button type="primary" @click="showUpload = true"
          >上传新版本</a-button
        >
      </template>

      <a-table
        :dataSource="list"
        :columns="columns"
        :loading="loading"
        rowKey="version"
        :pagination="false"
      >
        <template #bodyCell="{ column, record }">
          <template v-if="column.key === 'is_force'">
            <a-tag :color="record.is_force ? 'red' : 'default'">{{
              record.is_force ? "强制" : "可选"
            }}</a-tag>
          </template>
          <template v-if="column.key === 'file_size'">
            {{ (record.file_size / 1024).toFixed(1) }} KB
          </template>
          <template v-if="column.key === 'created_at'">
            {{ new Date(record.created_at).toLocaleString() }}
          </template>
        </template>
      </a-table>
    </a-card>

    <!-- 上传弹窗 -->
    <a-modal
      v-model:open="showUpload"
      title="上传固件"
      @ok="handleUpload"
      :confirmLoading="uploading"
      okText="上传"
      cancelText="取消"
    >
      <a-form :model="form" layout="vertical">
        <a-form-item label="版本号" required>
          <a-input v-model:value="form.version" placeholder="如 1.2.3" />
        </a-form-item>
        <a-form-item label="设备型号">
          <a-select v-model:value="form.device_type">
            <a-select-option value="renlight-64">renlight-64</a-select-option>
          </a-select>
        </a-form-item>
        <a-form-item label="更新说明">
          <a-textarea v-model:value="form.changelog" :rows="3" />
        </a-form-item>
        <a-form-item label="强制更新">
          <a-switch v-model:checked="form.is_force" />
        </a-form-item>
        <a-form-item label="固件文件 (.bin)" required>
          <input type="file" accept=".bin" @change="onFileChange" />
        </a-form-item>
      </a-form>
    </a-modal>
  </div>
</template>

<script setup>
import { ref, onMounted } from "vue";
import { message } from "ant-design-vue";
import http from "../api/http.js";

const list = ref([]);
const loading = ref(false);
const showUpload = ref(false);
const uploading = ref(false);
const form = ref({
  version: "",
  device_type: "renlight-64",
  changelog: "",
  is_force: false,
});
const file = ref(null);

const columns = [
  { title: "版本号", dataIndex: "version", key: "version" },
  { title: "设备型号", dataIndex: "device_type", key: "device_type" },
  { title: "文件大小", dataIndex: "file_size", key: "file_size" },
  { title: "更新类型", dataIndex: "is_force", key: "is_force" },
  {
    title: "更新说明",
    dataIndex: "changelog",
    key: "changelog",
    ellipsis: true,
  },
  { title: "上传时间", dataIndex: "created_at", key: "created_at" },
];

async function loadList() {
  loading.value = true;
  try {
    const res = await http.get("/firmware/list");
    if (res.code === 0) list.value = res.data.versions || [];
  } finally {
    loading.value = false;
  }
}

function onFileChange(e) {
  file.value = e.target.files[0] || null;
}

async function handleUpload() {
  if (!form.value.version) {
    message.error("请填写版本号");
    return;
  }
  if (!file.value) {
    message.error("请选择固件文件");
    return;
  }
  uploading.value = true;
  try {
    const fd = new FormData();
    fd.append("firmware", file.value);
    fd.append("version", form.value.version);
    fd.append("device_type", form.value.device_type);
    fd.append("changelog", form.value.changelog);
    fd.append("is_force", form.value.is_force ? "1" : "0");
    const res = await http.post("/firmware/upload", fd, {
      headers: { "Content-Type": "multipart/form-data" },
    });
    if (res.code === 0) {
      message.success("上传成功");
      showUpload.value = false;
      form.value = {
        version: "",
        device_type: "renlight-64",
        changelog: "",
        is_force: false,
      };
      file.value = null;
      loadList();
    } else {
      message.error(res.message || "上传失败");
    }
  } catch (e) {
    message.error("上传失败");
  } finally {
    uploading.value = false;
  }
}

onMounted(loadList);
</script>
