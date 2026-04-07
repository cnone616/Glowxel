function getPixelValue(document, x, y) {
  if (!document) {
    return "";
  }
  return document.pixels.get(`${x},${y}`) || "";
}

export function diffPatternDocuments(currentDocument, referenceDocument) {
  if (!currentDocument || !referenceDocument) {
    return {
      missing: [],
      extra: [],
      changed: [],
      uncertain: [],
    };
  }

  const width = Math.max(currentDocument.width, referenceDocument.width);
  const height = Math.max(currentDocument.height, referenceDocument.height);
  const missing = [];
  const extra = [];
  const changed = [];

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const current = getPixelValue(currentDocument, x, y);
      const reference = getPixelValue(referenceDocument, x, y);

      if (!current && reference) {
        missing.push({ x, y, current, reference });
        continue;
      }

      if (current && !reference) {
        extra.push({ x, y, current, reference });
        continue;
      }

      if (current && reference && current !== reference) {
        changed.push({ x, y, current, reference });
      }
    }
  }

  return {
    missing,
    extra,
    changed,
    uncertain: [],
  };
}
