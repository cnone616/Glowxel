import { defineStore } from 'pinia'

export const useEditorStore = defineStore('editorStore', {
  state: () => ({
    selectedFrameIndex: 0,
    previewFrameIndex: 0,
    selectedLayerKey: 'background',
    selectedTool: 'brush',
    selectedColor: { r: 0, g: 245, b: 212 },
    onionSkin: true,
    previewPlaying: false,
  }),
  actions: {
    setFrameIndex(frameIndex) {
      this.selectedFrameIndex = frameIndex
    },
    setPreviewFrameIndex(frameIndex) {
      this.previewFrameIndex = frameIndex
    },
    setLayerKey(layerKey) {
      this.selectedLayerKey = layerKey
    },
    setTool(toolKey) {
      this.selectedTool = toolKey
    },
    setColor(color) {
      this.selectedColor = color
    },
    toggleOnionSkin() {
      this.onionSkin = !this.onionSkin
    },
    togglePreviewPlaying() {
      this.previewPlaying = !this.previewPlaying
    },
    pausePreview() {
      this.previewPlaying = false
    },
    playPreview() {
      this.previewPlaying = true
    },
    resetForProject() {
      this.selectedFrameIndex = 0
      this.previewFrameIndex = 0
      this.selectedLayerKey = 'background'
      this.selectedTool = 'brush'
      this.previewPlaying = false
    },
  },
})
