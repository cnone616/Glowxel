import { defineStore } from "pinia";
import {
  clonePatternDocument,
  createPatternDocument,
  createPatternSnapshot,
  deserializePatternDocument,
  serializePatternDocument,
} from "@/modules/pattern-workbench/core/model/patternDocument.js";
import { importPatternFromCsv } from "@/modules/pattern-workbench/core/importers/patternImportCsv.js";
import {
  importPatternFromCaptureImage,
  importPatternFromImage,
} from "@/modules/pattern-workbench/core/importers/patternImportImage.js";
import { importPatternFromNumberedSheet } from "@/modules/pattern-workbench/core/importers/patternImportNumberedSheet.js";
import { diffPatternDocuments } from "@/modules/pattern-workbench/core/diff/patternDiff.js";
import { detectPatternIssues } from "@/modules/pattern-workbench/core/analysis/patternIssueDetector.js";

export const usePatternWorkbenchStore = defineStore("patternWorkbench", {
  state: () => {
    const initialDocument = createPatternDocument();

    return {
      activeMode: "generate",
      activeView: "current",
      document: initialDocument,
      referenceDocument: null,
      captureReviewDocument: null,
      captureReviewDiffResult: {
        missing: [],
        extra: [],
        changed: [],
        uncertain: [],
      },
      diffResult: {
        missing: [],
        extra: [],
        changed: [],
        uncertain: [],
      },
      issues: [],
      ignoredIssueIds: [],
      selectedIssueId: "",
      snapshots: [
        createPatternSnapshot({
          id: "snapshot-initial",
          label: "初始空白版本",
          stage: "draft",
          document: serializePatternDocument(initialDocument),
        }),
      ],
      lastImportReport: null,
      loading: false,
      error: "",
    };
  },

  getters: {
    selectedIssue(state) {
      return (
        state.issues.find((item) => item.id === state.selectedIssueId) || null
      );
    },
    issueCount(state) {
      return state.issues.length;
    },
    currentDocumentExport(state) {
      return serializePatternDocument(state.document);
    },
    captureReviewSummary(state) {
      if (!state.captureReviewDocument) {
        return null;
      }

      const width = Math.max(state.document.width, state.captureReviewDocument.width);
      const height = Math.max(state.document.height, state.captureReviewDocument.height);
      const totalCells = width * height;
      const mismatchCount =
        state.captureReviewDiffResult.missing.length +
        state.captureReviewDiffResult.extra.length +
        state.captureReviewDiffResult.changed.length;
      const matchedCount = Math.max(totalCells - mismatchCount, 0);

      return {
        totalCells,
        mismatchCount,
        matchedCount,
        matchRatio: totalCells ? matchedCount / totalCells : 0,
      };
    },
  },

  actions: {
    rebuildCaptureReview() {
      if (!this.captureReviewDocument) {
        this.captureReviewDiffResult = {
          missing: [],
          extra: [],
          changed: [],
          uncertain: [],
        };
        return;
      }

      this.captureReviewDiffResult = diffPatternDocuments(
        this.document,
        this.captureReviewDocument,
      );
    },

    rebuildAnalysis() {
      this.diffResult = diffPatternDocuments(this.document, this.referenceDocument);
      const nextIssues = detectPatternIssues(this.document, this.diffResult);
      this.issues = nextIssues.filter(
        (item) => !this.ignoredIssueIds.includes(item.id),
      );
      this.selectedIssueId = this.issues[0]?.id || "";
      this.rebuildCaptureReview();
    },

    pushSnapshot(label, stage, document) {
      this.snapshots.unshift(
        createPatternSnapshot({
          id: `${stage}-${Date.now()}`,
          label,
          stage,
          document: serializePatternDocument(document),
        }),
      );
    },

    applyCurrentDocument(document, snapshotLabel, stage) {
      this.document = deserializePatternDocument(document);
      this.ignoredIssueIds = [];
      this.pushSnapshot(snapshotLabel, stage, this.document);
      this.rebuildAnalysis();
    },

    applyGeneratedResult(result, snapshotLabel = "", stage = "generated") {
      if (!result || !result.document) {
        return false;
      }

      this.lastImportReport = result.report || null;
      this.applyCurrentDocument(
        result.document,
        snapshotLabel || `生成图纸 ${result.document.name}`,
        stage,
      );
      return true;
    },

    applyReferenceDocument(document) {
      this.referenceDocument = deserializePatternDocument(document);
      this.rebuildAnalysis();
    },

    applyCaptureReviewDocument(document) {
      this.captureReviewDocument = deserializePatternDocument(document);
      this.rebuildCaptureReview();
    },

    setActiveMode(value) {
      this.activeMode = value;
    },

    setActiveView(value) {
      this.activeView = value;
    },

    selectIssue(issueId) {
      this.selectedIssueId = issueId;
    },

    getDocumentCode(x, y) {
      return this.document.pixels.get(`${x},${y}`) || "";
    },

    getReferenceCode(x, y) {
      if (!this.referenceDocument) {
        return "";
      }
      return this.referenceDocument.pixels.get(`${x},${y}`) || "";
    },

    setDocumentPixel(nextDocument, x, y, code) {
      const key = `${x},${y}`;
      if (code) {
        nextDocument.pixels.set(key, code);
        return;
      }
      nextDocument.pixels.delete(key);
    },

    getDominantNeighborCode(x, y) {
      const counts = new Map();

      for (let offsetY = -1; offsetY <= 1; offsetY += 1) {
        for (let offsetX = -1; offsetX <= 1; offsetX += 1) {
          if (offsetX === 0 && offsetY === 0) {
            continue;
          }

          const neighborCode = this.getDocumentCode(x + offsetX, y + offsetY);
          if (!neighborCode) {
            continue;
          }
          counts.set(neighborCode, (counts.get(neighborCode) || 0) + 1);
        }
      }

      const sortedEntries = Array.from(counts.entries()).sort(
        (left, right) => right[1] - left[1],
      );

      if (!sortedEntries.length) {
        return "";
      }

      return sortedEntries[0][0];
    },

    commitPatchedDocument(nextDocument, snapshotLabel, stage, nextSelectedIssueId = "") {
      nextDocument.updatedAt = Date.now();
      this.document = nextDocument;
      this.pushSnapshot(snapshotLabel, stage, nextDocument);
      this.rebuildAnalysis();
      if (nextSelectedIssueId) {
        const matchedIssue = this.issues.find((item) => item.id === nextSelectedIssueId);
        if (matchedIssue) {
          this.selectedIssueId = matchedIssue.id;
        }
      }
    },

    applyIssueAction(issueId, actionType) {
      const issue = this.issues.find((item) => item.id === issueId);
      if (!issue || !issue.position) {
        return false;
      }

      const { x, y } = issue.position;
      const nextDocument = clonePatternDocument(this.document);
      const referenceCode = this.getReferenceCode(x, y);
      const dominantNeighborCode = this.getDominantNeighborCode(x, y);

      if (actionType === "ignore") {
        if (!this.ignoredIssueIds.includes(issue.id)) {
          this.ignoredIssueIds = [...this.ignoredIssueIds, issue.id];
        }
        this.rebuildAnalysis();
        return true;
      }

      if (actionType === "reference") {
        this.setDocumentPixel(nextDocument, x, y, referenceCode);
        this.commitPatchedDocument(nextDocument, `按参考修正 ${issue.title}`, "patched");
        return true;
      }

      if (actionType === "remove") {
        this.setDocumentPixel(nextDocument, x, y, "");
        this.commitPatchedDocument(nextDocument, `删除像素 ${issue.title}`, "patched");
        return true;
      }

      if (actionType === "neighbor") {
        this.setDocumentPixel(nextDocument, x, y, dominantNeighborCode);
        this.commitPatchedDocument(nextDocument, `合并邻域主色 ${issue.title}`, "patched");
        return true;
      }

      if (actionType === "suggestion") {
        if (issue.type === "缺失点" || issue.type === "颜色偏差") {
          this.setDocumentPixel(nextDocument, x, y, referenceCode);
          this.commitPatchedDocument(nextDocument, `应用建议 ${issue.title}`, "patched");
          return true;
        }

        if (issue.type === "多余点") {
          this.setDocumentPixel(nextDocument, x, y, "");
          this.commitPatchedDocument(nextDocument, `应用建议 ${issue.title}`, "patched");
          return true;
        }

        if (issue.type === "孤立点") {
          const nextCode = referenceCode || dominantNeighborCode;
          this.setDocumentPixel(nextDocument, x, y, nextCode);
          this.commitPatchedDocument(nextDocument, `应用建议 ${issue.title}`, "patched");
          return true;
        }
      }

      return false;
    },

    applyIssueActionBatch(issueIds, actionType) {
      if (!Array.isArray(issueIds) || !issueIds.length) {
        return false;
      }

      const nextDocument = clonePatternDocument(this.document);
      const matchedIssues = this.issues.filter((item) => issueIds.includes(item.id));

      if (!matchedIssues.length) {
        return false;
      }

      if (actionType === "ignore") {
        const mergedIgnoredIssueIds = new Set(this.ignoredIssueIds);
        matchedIssues.forEach((item) => {
          mergedIgnoredIssueIds.add(item.id);
        });
        this.ignoredIssueIds = Array.from(mergedIgnoredIssueIds);
        this.rebuildAnalysis();
        return true;
      }

      matchedIssues.forEach((issue) => {
        if (!issue.position) {
          return;
        }

        const { x, y } = issue.position;
        const referenceCode = this.getReferenceCode(x, y);
        const dominantNeighborCode = this.getDominantNeighborCode(x, y);

        if (actionType === "reference") {
          this.setDocumentPixel(nextDocument, x, y, referenceCode);
          return;
        }

        if (actionType === "remove") {
          this.setDocumentPixel(nextDocument, x, y, "");
          return;
        }

        if (actionType === "neighbor") {
          this.setDocumentPixel(nextDocument, x, y, dominantNeighborCode);
          return;
        }

        if (actionType === "suggestion") {
          if (issue.type === "缺失点" || issue.type === "颜色偏差") {
            this.setDocumentPixel(nextDocument, x, y, referenceCode);
            return;
          }
          if (issue.type === "多余点") {
            this.setDocumentPixel(nextDocument, x, y, "");
            return;
          }
          if (issue.type === "孤立点") {
            this.setDocumentPixel(nextDocument, x, y, referenceCode || dominantNeighborCode);
          }
        }
      });

      this.commitPatchedDocument(
        nextDocument,
        `批量处理 ${matchedIssues.length} 个问题`,
        "patched",
      );
      return true;
    },

    applyRegionAction(positions, actionType) {
      if (!Array.isArray(positions) || !positions.length) {
        return false;
      }

      const nextDocument = clonePatternDocument(this.document);

      positions.forEach((position) => {
        const x = position.x;
        const y = position.y;
        const referenceCode = this.getReferenceCode(x, y);
        const dominantNeighborCode = this.getDominantNeighborCode(x, y);

        if (actionType === "reference") {
          this.setDocumentPixel(nextDocument, x, y, referenceCode);
          return;
        }

        if (actionType === "remove") {
          this.setDocumentPixel(nextDocument, x, y, "");
          return;
        }

        if (actionType === "neighbor") {
          this.setDocumentPixel(nextDocument, x, y, dominantNeighborCode);
        }
      });

      this.commitPatchedDocument(
        nextDocument,
        `区域处理 ${positions.length} 个点`,
        "patched",
      );
      return true;
    },

    applyRegionFill(positions, colorCode) {
      if (!Array.isArray(positions) || !positions.length || !colorCode) {
        return false;
      }

      const nextDocument = clonePatternDocument(this.document);
      positions.forEach((position) => {
        this.setDocumentPixel(nextDocument, position.x, position.y, colorCode);
      });

      this.commitPatchedDocument(
        nextDocument,
        `区域填充 ${positions.length} 个点`,
        "patched",
      );
      return true;
    },

    applyBrushStroke(positions, actionType, colorCode) {
      if (!Array.isArray(positions) || !positions.length) {
        return false;
      }

      const nextDocument = clonePatternDocument(this.document);
      positions.forEach((position) => {
        if (actionType === "erase") {
          this.setDocumentPixel(nextDocument, position.x, position.y, "");
          return;
        }
        if (actionType === "paint" && colorCode) {
          this.setDocumentPixel(nextDocument, position.x, position.y, colorCode);
        }
      });

      this.commitPatchedDocument(
        nextDocument,
        `${actionType === "erase" ? "橡皮擦除" : "画笔绘制"} ${positions.length} 个点`,
        "patched",
      );
      return true;
    },

    replaceColor(sourceCode, targetCode, positions = []) {
      if (!sourceCode || !targetCode || sourceCode === targetCode) {
        return false;
      }

      const nextDocument = clonePatternDocument(this.document);
      const hasRegion = Array.isArray(positions) && positions.length > 0;

      if (hasRegion) {
        positions.forEach((position) => {
          const currentCode = nextDocument.pixels.get(`${position.x},${position.y}`) || "";
          if (currentCode === sourceCode) {
            this.setDocumentPixel(nextDocument, position.x, position.y, targetCode);
          }
        });
        this.commitPatchedDocument(
          nextDocument,
          `区域替换颜色 ${sourceCode} -> ${targetCode}`,
          "patched",
        );
        return true;
      }

      nextDocument.pixels.forEach((code, key) => {
        if (code === sourceCode) {
          nextDocument.pixels.set(key, targetCode);
        }
      });

      this.commitPatchedDocument(
        nextDocument,
        `全图替换颜色 ${sourceCode} -> ${targetCode}`,
        "patched",
      );
      return true;
    },

    clearIgnoredIssues() {
      this.ignoredIssueIds = [];
      this.rebuildAnalysis();
    },

    restoreSnapshot(snapshotId) {
      const snapshot = this.snapshots.find((item) => item.id === snapshotId);
      if (!snapshot || !snapshot.document) {
        return false;
      }
      this.document = deserializePatternDocument(snapshot.document);
      this.pushSnapshot(`恢复版本 ${snapshot.label}`, "restored", this.document);
      this.rebuildAnalysis();
      return true;
    },

    async importCurrentImage(file, options = {}) {
      this.loading = true;
      this.error = "";
      try {
        const result = await importPatternFromImage(file, options);
        this.lastImportReport = result.report || null;
        this.applyCurrentDocument(result.document, `导入图片 ${result.document.name}`, "generated");
      } catch (error) {
        this.error = error.message;
        throw error;
      } finally {
        this.loading = false;
      }
    },

    async importReferenceImage(file, options = {}) {
      this.loading = true;
      this.error = "";
      try {
        const result = await importPatternFromImage(file, options);
        this.applyReferenceDocument(result.document);
      } catch (error) {
        this.error = error.message;
        throw error;
      } finally {
        this.loading = false;
      }
    },

    async importCurrentNumberedSheet(file, options = {}) {
      this.loading = true;
      this.error = "";
      try {
        const result = await importPatternFromNumberedSheet(file, options);
        this.lastImportReport = result.report;
        this.applyCurrentDocument(
          result.document,
          `识别编号图纸 ${result.document.name}`,
          "generated",
        );
      } catch (error) {
        this.error = error.message;
        throw error;
      } finally {
        this.loading = false;
      }
    },

    importCurrentCsv(text, options = {}) {
      this.error = "";
      this.lastImportReport = null;
      const document = importPatternFromCsv(text, options);
      this.applyCurrentDocument(document, `导入 CSV ${document.name}`, "imported");
    },

    importCurrentJson(text) {
      this.error = "";
      this.lastImportReport = null;
      const parsed = JSON.parse(text);
      const document = deserializePatternDocument(parsed);
      this.applyCurrentDocument(document, `导入 JSON ${document.name}`, "imported");
    },

    importReferenceCsv(text, options = {}) {
      this.error = "";
      const document = importPatternFromCsv(text, options);
      this.applyReferenceDocument(document);
    },

    importReferenceJson(text) {
      this.error = "";
      const parsed = JSON.parse(text);
      const document = deserializePatternDocument(parsed);
      this.applyReferenceDocument(document);
    },

    clearReferenceDocument() {
      this.referenceDocument = null;
      this.rebuildAnalysis();
    },

    clearCaptureReviewDocument() {
      this.captureReviewDocument = null;
      this.captureReviewDiffResult = {
        missing: [],
        extra: [],
        changed: [],
        uncertain: [],
      };
    },

    async importCaptureReviewImage(file, options = {}) {
      this.loading = true;
      this.error = "";
      try {
        const document = await importPatternFromCaptureImage(file, options);
        this.applyCaptureReviewDocument(document);
      } catch (error) {
        this.error = error.message;
        throw error;
      } finally {
        this.loading = false;
      }
    },
  },
});
