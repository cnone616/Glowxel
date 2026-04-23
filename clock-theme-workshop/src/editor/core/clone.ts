import type { ThemeDocument } from '../types'

export function cloneDocument(document: ThemeDocument): ThemeDocument {
  return structuredClone(document)
}
