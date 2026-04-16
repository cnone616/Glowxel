<template>
  <ConfirmModal
    :visible="visible"
    :title="title"
    :content="content"
    :confirm-text="confirmText"
    :cancel-text="cancelText"
    :show-cancel="showCancel"
    :mask-closable="maskClosable"
    :danger="danger"
    @confirm="handleConfirm"
    @cancel="handleCancel"
    @update:visible="handleVisibleChange"
  />
</template>

<script>
import ConfirmModal from './ConfirmModal.vue'
import { bindDialogInstance, unbindDialogInstance } from '../composables/useDialog.js'

export default {
  components: {
    ConfirmModal
  },

  data() {
    return {
      visible: false,
      title: '提示',
      content: '',
      confirmText: '确定',
      cancelText: '取消',
      showCancel: true,
      maskClosable: true,
      danger: false,
      resolver: null
    }
  },

  mounted() {
    bindDialogInstance(this)
  },

  beforeDestroy() {
    if (this.resolver) {
      this.resolver(false)
      this.resolver = null
    }
    unbindDialogInstance(this)
  },

  methods: {
    open(options = {}) {
      if (this.resolver) {
        this.resolver(false)
        this.resolver = null
      }

      this.title = options.title || '提示'
      this.content = options.content || ''
      this.confirmText = options.confirmText || '确定'
      this.cancelText = options.cancelText || '取消'
      this.showCancel = options.showCancel !== false
      this.maskClosable = options.maskClosable !== false
      this.danger = options.danger === true
      this.visible = true

      return new Promise((resolve) => {
        this.resolver = resolve
      })
    },

    close(result) {
      this.visible = false
      if (this.resolver) {
        this.resolver(result)
        this.resolver = null
      }
    },

    handleConfirm() {
      this.close(true)
    },

    handleCancel() {
      this.close(false)
    },

    handleVisibleChange(nextVisible) {
      if (nextVisible) {
        this.visible = true
        return
      }

      this.handleCancel()
    }
  }
}
</script>
