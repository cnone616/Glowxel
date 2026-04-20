function createContext(canvas) {
  return {
    canvas,
    fillStyle: '#000000',
    font: '7px monospace',
    textBaseline: 'top',
    globalAlpha: 1,
    imageSmoothingEnabled: false,
    clearRect() {},
    fillRect() {},
    fillText() {},
    drawImage() {},
    save() {},
    restore() {},
    beginPath() {},
    moveTo() {},
    lineTo() {},
    stroke() {},
    getImageData() {
      return {
        data: new Uint8ClampedArray(canvas.width * canvas.height * 4),
      }
    },
  }
}

Object.defineProperty(HTMLCanvasElement.prototype, 'getContext', {
  value() {
    return createContext(this)
  },
})

Object.defineProperty(HTMLCanvasElement.prototype, 'toBlob', {
  value(callback, type) {
    callback(new Blob([type || 'application/octet-stream'], { type: type || 'application/octet-stream' }))
  },
})
