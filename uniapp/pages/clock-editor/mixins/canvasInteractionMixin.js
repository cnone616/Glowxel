export default {
  methods: {
    handlePixelClick(x, y) {
      if (this.currentTool === "move") return;
      const key = `${x},${y}`;
      const newPixels = new Map(this.canvasPixels);
      if (this.currentTool === "eraser") {
        if (!newPixels.has(key)) return;
        newPixels.delete(key);
      } else {
        if (newPixels.get(key) === this.selectedColor) return;
        newPixels.set(key, this.selectedColor);
      }
      this.canvasPixels = newPixels;
      const newHistory = this.history.slice(0, this.historyIndex + 1);
      newHistory.push(new Map(newPixels));
      if (newHistory.length > 50) newHistory.shift();
      this.history = newHistory;
      this.historyIndex = newHistory.length - 1;
    },

    handleUndo() {
      if (this.historyIndex > 0) {
        this.historyIndex--;
        this.canvasPixels = new Map(this.history[this.historyIndex]);
      }
    },

    handleRedo() {
      if (this.historyIndex < this.history.length - 1) {
        this.historyIndex++;
        this.canvasPixels = new Map(this.history[this.historyIndex]);
      }
    },

    handleZoom(delta) {
      const centerX = this.containerSize.width / 2;
      const centerY = this.containerSize.height / 2;
      const newZoom = Math.max(1, Math.min(20, this.zoom + delta));
      const scale = newZoom / this.zoom;
      this.pan = {
        x: centerX - (centerX - this.pan.x) * scale,
        y: centerY - (centerY - this.pan.y) * scale,
      };
      this.zoom = newZoom;
    },

    handlePan(dx, dy) {
      this.pan = { x: this.pan.x + dx, y: this.pan.y + dy };
    },

    handlePinchZoom(delta, centerX, centerY) {
      const newZoom = Math.max(1, Math.min(20, this.zoom + delta));
      const scale = newZoom / this.zoom;
      this.pan = {
        x: centerX - (centerX - this.pan.x) * scale,
        y: centerY - (centerY - this.pan.y) * scale,
      };
      this.zoom = newZoom;
    },

    handleFit() {
      const size = this.containerSize.width;
      const fitZoom = Math.floor((size * 0.9) / 64);
      this.zoom = Math.max(2, fitZoom);
      this.pan = {
        x: (size - 64 * this.zoom) / 2,
        y: (size - 64 * this.zoom) / 2,
      };
    },
  },
};
