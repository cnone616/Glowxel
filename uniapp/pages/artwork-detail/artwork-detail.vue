<template>
  <view class="artwork-detail-page glx-page-shell">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <!-- 顶部导航 -->
    <view class="header glx-topbar glx-page-shell__fixed">
      <view class="header-content">
        <view class="back-btn" @click="handleBack">
          <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
        </view>
        <text class="header-title glx-topbar__title">作品详情</text>
        <view class="header-placeholder"></view>
      </view>
    </view>

    <!-- 主要内容 -->
    <scroll-view scroll-y class="main-content glx-scroll-region glx-page-shell__content">
      <view v-if="isLoading" class="artwork-detail-skeleton">
        <view class="artwork-section">
          <view class="artwork-container artwork-container--skeleton">
            <view class="artwork-skeleton-block artwork-skeleton-block--poster"></view>
          </view>
        </view>

        <view class="info-section">
          <view class="artwork-info glx-panel-card artwork-skeleton-card">
            <view class="artwork-skeleton-bar artwork-skeleton-bar--title"></view>
            <view class="artwork-skeleton-meta">
              <view class="artwork-skeleton-pill"></view>
              <view class="artwork-skeleton-pill"></view>
              <view class="artwork-skeleton-pill"></view>
            </view>
            <view class="artwork-skeleton-tags">
              <view class="artwork-skeleton-pill artwork-skeleton-pill--tag"></view>
              <view class="artwork-skeleton-pill artwork-skeleton-pill--tag artwork-skeleton-pill--tag-short"></view>
            </view>
          </view>

          <view class="author-section glx-panel-card artwork-skeleton-card">
            <view class="author-info">
              <view class="artwork-skeleton-avatar"></view>
              <view class="artwork-skeleton-author">
                <view class="artwork-skeleton-bar artwork-skeleton-bar--name"></view>
                <view class="artwork-skeleton-bar artwork-skeleton-bar--desc"></view>
              </view>
            </view>
            <view class="artwork-skeleton-button"></view>
          </view>
        </view>

        <view class="action-section">
          <view class="action-buttons">
            <view
              v-for="index in 4"
              :key="`action-skeleton-${index}`"
              class="action-btn glx-action-tile action-btn--skeleton"
            >
              <view class="artwork-skeleton-icon"></view>
              <view class="artwork-skeleton-bar artwork-skeleton-bar--action"></view>
            </view>
          </view>
        </view>

        <view class="comment-section artwork-skeleton-card">
          <view class="section-header glx-section-head">
            <view class="artwork-skeleton-bar artwork-skeleton-bar--section"></view>
          </view>
          <view class="artwork-skeleton-comment-list">
            <view
              v-for="index in 2"
              :key="`comment-skeleton-${index}`"
              class="artwork-skeleton-comment"
            >
              <view class="artwork-skeleton-avatar artwork-skeleton-avatar--small"></view>
              <view class="artwork-skeleton-comment-copy">
                <view class="artwork-skeleton-bar artwork-skeleton-bar--comment-name"></view>
                <view class="artwork-skeleton-bar artwork-skeleton-bar--comment-line"></view>
                <view class="artwork-skeleton-bar artwork-skeleton-bar--comment-line artwork-skeleton-bar--comment-line-short"></view>
              </view>
            </view>
          </view>
        </view>

        <view class="related-section artwork-skeleton-card">
          <view class="section-header">
            <view class="artwork-skeleton-bar artwork-skeleton-bar--section"></view>
          </view>
          <view class="related-grid">
            <view
              v-for="index in 2"
              :key="`related-skeleton-${index}`"
              class="artwork-skeleton-related"
            >
              <view class="artwork-skeleton-block artwork-skeleton-block--related"></view>
              <view class="artwork-skeleton-bar artwork-skeleton-bar--related-title"></view>
              <view class="artwork-skeleton-bar artwork-skeleton-bar--related-meta"></view>
            </view>
          </view>
        </view>

        <view class="content-spacer"></view>
      </view>

      <view v-else>
        <!-- 作品展示区域 -->
        <view class="artwork-section">
          <view class="artwork-container">
            <image
              :src="artwork.cover_url"
              class="artwork-image"
              mode="aspectFit"
              @click="previewImage"
            />
            <view class="zoom-hint">
              <Icon name="zoom-in" :size="32" color="currentColor" />
              <text class="zoom-text">点击查看大图</text>
            </view>
          </view>
        </view>

        <!-- 作品信息 -->
        <view class="info-section">
          <view class="artwork-info glx-panel-card">
            <text class="artwork-title">{{ artwork.title }}</text>
            <view class="artwork-meta">
              <view class="meta-item">
                <Icon name="modular" :size="28" color="currentColor" />
                <text class="meta-text"
                  >{{ artwork.width }}×{{ artwork.height }}</text
                >
              </view>
              <view class="meta-item">
                <Icon name="picture" :size="28" color="currentColor" />
                <text class="meta-text">{{ artwork.color_count }}色</text>
              </view>
              <view class="meta-item">
                <Icon name="calendar" :size="28" color="currentColor" />
                <text class="meta-text">{{ formatDate(artwork.created_at) }}</text>
              </view>
            </view>

            <!-- 标签 -->
            <view
              v-if="artwork.tags && artwork.tags.length"
              class="tags-container"
            >
              <view v-for="tag in artwork.tags" :key="tag" class="tag-item">
                <text class="tag-text">#{{ tag }}</text>
              </view>
            </view>
          </view>

          <!-- 作者信息 -->
          <view class="author-section glx-panel-card">
            <view class="author-info" @click="goToUserProfile">
              <Avatar :src="artwork.author_avatar" :size="80" />
              <view class="author-details">
                <text class="author-name">{{ artwork.author_name }}</text>
                <text class="author-stats"
                  >{{ artwork.author_works_count }}个作品</text
                >
              </view>
            </view>
            <view
              class="follow-btn glx-cta-button"
              :class="{ following: artwork.isFollowing }"
              @click="toggleFollow"
            >
              <text class="follow-text">
                {{ artwork.isFollowing ? "已关注" : "关注" }}
              </text>
            </view>
          </view>
        </view>

        <!-- 交互按钮 -->
        <view class="action-section">
          <view class="action-buttons">
            <view
              class="action-btn glx-action-tile"
              :class="{ active: isLiked }"
              @click="toggleLike"
            >
              <Icon
                :name="isLiked ? 'favorite-filling' : 'favorite'"
                :size="40"
                color="var(--nb-ink)"
              />
              <text class="action-text">{{ artwork.likes || 0 }}</text>
            </view>

            <view
              class="action-btn glx-action-tile"
              :class="{ active: isCollected }"
              @click="toggleCollect"
            >
              <Icon
                :name="isCollected ? 'file-common-filling' : 'file-common'"
                :size="40"
                color="var(--nb-ink)"
              />
              <text class="action-text">{{ artwork.collects || 0 }}</text>
            </view>

            <view class="action-btn glx-action-tile" @click="showCommentInput">
              <Icon name="comment" :size="40" color="var(--nb-ink)" />
              <text class="action-text">{{ artwork.comments_count || 0 }}</text>
            </view>

            <view class="action-btn glx-action-tile" @click="shareArtwork">
              <Icon name="share" :size="40" color="var(--nb-ink)" />
              <text class="action-text">分享</text>
            </view>
          </view>
        </view>

        <!-- 评论区域 -->
        <view class="comment-section">
          <view class="section-header glx-section-head">
            <text class="section-title glx-section-title">评论 ({{ comments.length }})</text>
          </view>

          <!-- 评论列表 -->
          <view v-if="comments.length > 0" class="comment-list">
            <Comment
              v-for="comment in comments"
              :key="comment.id"
              :comment="comment"
              @reply="handleReply"
              @like="handleCommentLike"
              @delete="handleCommentDelete"
            />
          </view>

          <!-- 空状态 -->
          <view v-else class="empty-comments">
            <Icon name="comment" :size="80" color="var(--text-tertiary)" />
            <text class="empty-text">还没有评论，快来抢沙发吧！</text>
          </view>
        </view>

        <!-- 相关推荐 -->
        <view class="related-section">
          <view class="section-header">
            <text class="section-title">相关推荐</text>
          </view>
          <view class="related-grid">
            <ArtworkCard
              v-for="item in relatedArtworks"
              :key="item.id"
              :artwork="item"
              @click="goToArtwork"
            />
          </view>
        </view>

        <view class="content-spacer"></view>
      </view>
    </scroll-view>

    <!-- 底部评论输入 -->
    <view v-if="!isLoading && showCommentBar" class="comment-input-bar">
      <view class="comment-input-container">
        <Input
          v-model="commentText"
          type="text"
          placeholder="写下你的评论..."
          :focus="commentInputFocus"
          clearable
          @blur="handleCommentBlur"
        />
        <view
          class="send-comment-btn glx-primary-action"
          :class="{ active: commentText.trim(), disabled: !commentText.trim() }"
          @click="sendComment"
        >
          <text class="send-comment-text">发送</text>
        </view>
      </view>
    </view>

    <view v-else-if="!isLoading" class="bottom-action-bar glx-page-shell__fixed">
      <view class="bottom-action-shell">
        <view class="bottom-action-main">
          <text class="bottom-action-title">更多操作</text>
          <text class="bottom-action-desc">下载、模板、举报</text>
        </view>
        <view class="bottom-action-btn" @click="showMoreActions">
          <Icon name="more-horizontal" :size="32" color="var(--nb-ink)" />
          <text class="bottom-action-text">打开</text>
        </view>
      </view>
    </view>

    <!-- 更多操作弹窗 -->
    <Modal v-model:visible="showMoreModal" title="更多操作">
      <view class="more-actions">
        <view class="action-item primary" @click="downloadArtwork">
          <Icon name="download" :size="40" color="currentColor" />
          <text class="action-label">下载作品</text>
        </view>
        <view class="action-item primary" @click="useAsTemplate">
          <Icon name="copy" :size="40" color="currentColor" />
          <text class="action-label">用作模板</text>
        </view>
        <view class="action-item danger" @click="reportArtwork">
          <Icon name="warning" :size="40" color="currentColor" />
          <text class="action-label">举报作品</text>
        </view>
      </view>
    </Modal>

    <LoadingOverlay />
    <Toast />
  </view>
</template>

<script>
import {
  artworkAPI,
  commentAPI,
  likeAPI,
  collectionAPI,
  followAPI,
} from "../../api/index.js";
import { ARTKAL_COLORS_FULL } from "../../data/artkal-colors-full.js";
import { useToast } from "../../composables/useToast.js";
import { useProjectStore } from "../../store/project.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Avatar from "../../components/Avatar.vue";
import Comment from "../../components/Comment.vue";
import ArtworkCard from "../../components/ArtworkCard.vue";
import LoadingOverlay from "../../components/LoadingOverlay.vue";
import Modal from "../../components/Modal.vue";
import Input from "../../components/Input.vue";
import Toast from "../../components/Toast.vue";

import { PERLER_BOARD_SIZE } from "../../constants/perler.js";

const toast = useToast();

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Avatar,
    Comment,
    ArtworkCard,
    LoadingOverlay,
    Modal,
    Input,
    Toast,
  },

  data() {
    return {
      artworkId: "",
      artwork: {},
      comments: [],
      relatedArtworks: [],

      // 交互状态
      isLiked: false,
      isCollected: false,

      // 评论相关
      showCommentBar: false,
      commentText: "",
      commentInputFocus: false,

      // 弹窗状态
      showMoreModal: false,

      // 加载状态
      isLoading: true,
    };
  },

  onLoad(options) {
    this.artworkId = options.id;
    if (this.artworkId) {
      this.loadArtworkDetail();
    }
  },

  methods: {
    async saveArtworkImage(imageUrl, fileName) {
      if (!imageUrl) {
        throw new Error("image url missing");
      }

      // #ifdef H5
      const link = document.createElement("a");
      link.href = imageUrl;
      link.download = `${fileName}-${Date.now()}.png`;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      return;
      // #endif

      // #ifndef H5
      const downloadRes = await new Promise((resolve, reject) => {
        uni.downloadFile({
          url: imageUrl,
          success: (res) => {
            if (res.statusCode === 200) {
              resolve(res);
              return;
            }
            reject(new Error("download failed"));
          },
          fail: reject,
        });
      });

      await new Promise((resolve, reject) => {
        uni.saveImageToPhotosAlbum({
          filePath: downloadRes.tempFilePath,
          success: resolve,
          fail: reject,
        });
      });
      // #endif
    },

    getPaddedSize(value) {
      return Math.ceil(value / PERLER_BOARD_SIZE) * PERLER_BOARD_SIZE;
    },

    buildArtworkPixelsMap(pixelList) {
      if (!Array.isArray(pixelList)) {
        throw new Error("invalid artwork pixels");
      }

      return new Map(pixelList);
    },

    buildArtworkPaletteCodes(pixels) {
      const hexColors = [...new Set(Array.from(pixels.values()))];
      return hexColors.map((hex) => {
        const matchedColor = ARTKAL_COLORS_FULL.find(
          (color) => color.hex.toUpperCase() === hex.toUpperCase(),
        );
        if (!matchedColor) {
          throw new Error(`unknown artkal color: ${hex}`);
        }
        return matchedColor.code;
      });
    },

    async loadArtworkDetail() {
      try {
        this.isLoading = true;

        const artRes = await artworkAPI.getArtworkById(this.artworkId);
        if (artRes.success && artRes.data && artRes.data.artwork) {
          this.artwork = artRes.data.artwork;
        }

        const cmtRes = await commentAPI.getComments(this.artworkId);
        if (cmtRes.success && cmtRes.data) {
          this.comments = cmtRes.data.list;
        }

        const relRes = await artworkAPI.getRelatedArtworks(this.artworkId);
        if (relRes.success && relRes.data) {
          this.relatedArtworks = relRes.data.list;
        }

        this.isLiked = await likeAPI.isLiked(this.artworkId);
        this.isCollected = await collectionAPI.isCollected(this.artworkId);
      } catch (error) {
        console.error("加载作品详情失败:", error);
        toast.showError("加载失败");
      } finally {
        this.isLoading = false;
      }
    },

    handleBack() {
      uni.navigateBack();
    },

    previewImage() {
      uni.previewImage({
        urls: [this.artwork.cover_url],
        current: this.artwork.cover_url,
      });
    },

    goToUserProfile() {
      uni.navigateTo({
        url: `/pages/user-detail/user-detail?id=${this.artwork.author_id}`,
      });
    },

    async toggleFollow() {
      try {
        const res = await followAPI.toggleFollow(this.artwork.author_id);
        if (!(res.success && res.data)) {
          throw new Error("follow failed");
        }
        this.artwork.isFollowing = res.data.followed;

        toast.showSuccess(res.data.followed ? "关注成功" : "取消关注");
      } catch (error) {
        console.error("关注操作失败:", error);
        toast.showError("操作失败");
      }
    },

    async toggleLike() {
      try {
        const liked = !this.isLiked;

        if (liked) {
          await likeAPI.likeArtwork(this.artworkId);
          this.artwork.likes += 1;
        } else {
          await likeAPI.unlikeArtwork(this.artworkId);
          this.artwork.likes -= 1;
        }

        this.isLiked = liked;
      } catch (error) {
        console.error("点赞操作失败:", error);
        toast.showError("操作失败");
      }
    },

    async toggleCollect() {
      try {
        const collected = !this.isCollected;

        if (collected) {
          await collectionAPI.collectArtwork(this.artworkId);
          this.artwork.collects += 1;

          this._addCollectedProject();
        } else {
          await collectionAPI.uncollectArtwork(this.artworkId);
          this.artwork.collects -= 1;

          this._removeCollectedProject();
        }

        this.isCollected = collected;

        toast.showSuccess(collected ? "已收藏并添加到我的项目" : "取消收藏");
      } catch (error) {
        console.error("收藏操作失败:", error);
        toast.showError("操作失败");
      }
    },

    // 收藏时添加只读项目到本地
    _addCollectedProject() {
      if (!this.artwork) return;
      const projectStore = useProjectStore();
      projectStore.addCollectedProject({
        name: this.artwork.title,
        width: this.artwork.width,
        height: this.artwork.height,
        palette: [],
        thumbnail: this.artwork.cover_url,
        artworkId: this.artwork.id,
        originalAuthor: {
          id: this.artwork.author_id,
          name: this.artwork.author_name,
          avatar: this.artwork.author_avatar,
        },
      });
    },

    // 取消收藏时删除对应的本地项目
    _removeCollectedProject() {
      if (!this.artwork) return;
      const projectStore = useProjectStore();
      const project = projectStore.projects.find(
        (p) => p.source === "collected" && p.artworkId === this.artwork.id,
      );
      if (project) {
        projectStore.deleteProject(project.id);
      }
    },

    showCommentInput() {
      this.showCommentBar = true;
      this.commentInputFocus = true;
    },

    handleCommentBlur() {
      if (!this.commentText.trim()) {
        this.showCommentBar = false;
      }
    },

    async sendComment() {
      if (!this.commentText.trim()) return;

      try {
        const cmtRes = await commentAPI.addComment(
          this.artworkId,
          this.commentText.trim(),
        );
        const newComment =
          cmtRes.success && cmtRes.data ? cmtRes.data.comment : null;
        if (!newComment) throw new Error("评论失败");

        this.comments.unshift(newComment);
        this.artwork.comments_count += 1;
        this.commentText = "";
        this.showCommentBar = false;

        toast.showSuccess("评论成功");
      } catch (error) {
        console.error("发表评论失败:", error);
        toast.showError("评论失败");
      }
    },

    handleReply(comment) {
      this.commentText = `@${comment.user_name} `;
      this.showCommentInput();
    },

    async handleCommentLike(comment) {
      try {
        if (!comment.isLiked) {
          await commentAPI.likeComment(comment.id);
          comment.likes += 1;
          comment.isLiked = true;
        }
      } catch (error) {
        console.error("评论点赞失败:", error);
      }
    },

    async handleCommentDelete(comment) {
      try {
        await commentAPI.deleteComment(comment.id);

        const index = this.comments.findIndex((c) => c.id === comment.id);
        if (index > -1) {
          this.comments.splice(index, 1);
          this.artwork.comments_count -= 1;
        }

        toast.showSuccess("删除成功");
      } catch (error) {
        console.error("删除评论失败:", error);
        toast.showError("删除失败");
      }
    },

    shareArtwork() {
      // 微信小程序分享
      uni.showShareMenu({
        withShareTicket: true,
      });
    },

    showMoreActions() {
      this.showMoreModal = true;
    },

    async downloadArtwork() {
      this.showMoreModal = false;
      try {
        await this.saveArtworkImage(this.artwork.cover_url, this.artwork.title);
        toast.showSuccess("下载成功");
      } catch (error) {
        console.error("下载作品失败:", error);
        toast.showError("下载失败");
      }
    },

    async useAsTemplate() {
      this.showMoreModal = false;
      toast.showLoading("载入作品...");

      try {
        const res = await artworkAPI.getArtworkPixels(this.artworkId);
        if (!(res.success && res.data && Array.isArray(res.data.pixels))) {
          throw new Error("load artwork pixels failed");
        }

        const pixels = this.buildArtworkPixelsMap(res.data.pixels);
        const palette = this.buildArtworkPaletteCodes(pixels);
        const paddedWidth = this.getPaddedSize(res.data.width);
        const paddedHeight = this.getPaddedSize(res.data.height);
        const projectStore = useProjectStore();
        const projectId = projectStore.addProject(
          this.artwork.title,
          res.data.width,
          res.data.height,
          palette.length,
          palette,
          paddedWidth,
          paddedHeight,
          this.artwork.cover_url,
        );

        if (!projectId) {
          throw new Error("project create failed");
        }

        projectStore.saveProjectPixels(projectId, pixels);

        toast.hideLoading();
        toast.showSuccess("已创建草稿");
        uni.navigateTo({
          url: `/pages/overview/overview?id=${projectId}`,
        });
      } catch (error) {
        toast.hideLoading();
        console.error("载入作品失败:", error);
        toast.showError("载入失败");
      }
    },

    reportArtwork() {
      this.showMoreModal = false;
      uni.setClipboardData({
        data: `举报作品：${this.artwork.title}（ID: ${this.artwork.id}）`,
        success: () => {
          toast.showSuccess("举报信息已复制");
        },
        fail: () => {
          toast.showError("复制失败");
        },
      });
    },

    goToArtwork(artwork) {
      uni.navigateTo({
        url: `/pages/artwork-detail/artwork-detail?id=${artwork.id}`,
      });
    },

    formatDate(dateString) {
      const date = new Date(dateString);
      const now = new Date();
      const diff = now - date;

      const minute = 60 * 1000;
      const hour = 60 * minute;
      const day = 24 * hour;

      if (diff < hour) {
        return Math.floor(diff / minute) + "分钟前";
      } else if (diff < day) {
        return Math.floor(diff / hour) + "小时前";
      } else if (diff < 7 * day) {
        return Math.floor(diff / day) + "天前";
      } else {
        return date.toLocaleDateString();
      }
    },
  },
};
</script>

<style scoped>
.artwork-detail-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--nb-paper);
}

/* 状态栏占位 */
.status-bar {
  background: var(--nb-paper);
  flex-shrink: 0;
}

/* 顶部导航 */
.header {
  background: var(--nb-paper);
  flex-shrink: 0;
}

.header-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 32rpx;
}

.back-btn,
.header-placeholder {
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 0;
  transition: all 0.2s ease;
}

.header-title {
  flex: 1;
  text-align: center;
}

/* 主要内容 */
.main-content {
  flex: 1;
  overflow-y: auto;
}

.artwork-detail-skeleton {
  padding-bottom: 0;
}

/* 作品展示区域 */
.artwork-section {
  padding: 24rpx 32rpx 0;
}

.artwork-container {
  position: relative;
  background-color: var(--nb-surface);
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
  border-radius: 0;
  overflow: hidden;
}

.artwork-image {
  width: 100%;
  height: 560rpx;
  display: block;
  background: #e9edf6;
}

.artwork-container--skeleton {
  padding: 24rpx;
  background: #ffffff;
}

.artwork-skeleton-card {
  background: #ffffff;
}

.artwork-skeleton-block,
.artwork-skeleton-bar,
.artwork-skeleton-pill,
.artwork-skeleton-avatar,
.artwork-skeleton-button,
.artwork-skeleton-icon {
  background: #d7d7d7;
  box-sizing: border-box;
}

.artwork-skeleton-block--poster {
  width: 100%;
  height: 512rpx;
  background: #111111;
}

.artwork-skeleton-meta,
.artwork-skeleton-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
}

.artwork-skeleton-bar {
  height: 24rpx;
}

.artwork-skeleton-bar--title {
  width: 72%;
  height: 40rpx;
  margin-bottom: 24rpx;
}

.artwork-skeleton-bar--name {
  width: 220rpx;
  height: 32rpx;
  margin-bottom: 14rpx;
}

.artwork-skeleton-bar--desc {
  width: 168rpx;
}

.artwork-skeleton-bar--action {
  width: 56rpx;
  height: 20rpx;
}

.artwork-skeleton-bar--section {
  width: 200rpx;
  height: 32rpx;
}

.artwork-skeleton-bar--comment-name {
  width: 132rpx;
  margin-bottom: 16rpx;
}

.artwork-skeleton-bar--comment-line {
  width: 100%;
  margin-bottom: 12rpx;
}

.artwork-skeleton-bar--comment-line-short {
  width: 76%;
  margin-bottom: 0;
}

.artwork-skeleton-bar--related-title {
  width: 86%;
  margin-top: 16rpx;
  margin-bottom: 12rpx;
}

.artwork-skeleton-bar--related-meta {
  width: 54%;
}

.artwork-skeleton-pill {
  width: 160rpx;
  height: 48rpx;
  border: 2rpx solid rgba(0, 0, 0, 0.08);
}

.artwork-skeleton-pill--tag {
  width: 116rpx;
  height: 40rpx;
}

.artwork-skeleton-pill--tag-short {
  width: 84rpx;
}

.artwork-skeleton-avatar {
  width: 80rpx;
  height: 80rpx;
  border: 2rpx solid rgba(0, 0, 0, 0.08);
}

.artwork-skeleton-avatar--small {
  width: 64rpx;
  height: 64rpx;
}

.artwork-skeleton-author {
  flex: 1;
}

.artwork-skeleton-button {
  width: 136rpx;
  height: 64rpx;
  border: 2rpx solid rgba(0, 0, 0, 0.08);
}

.action-btn--skeleton {
  pointer-events: none;
}

.artwork-skeleton-icon {
  width: 40rpx;
  height: 40rpx;
}

.artwork-skeleton-comment-list {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.artwork-skeleton-comment {
  display: flex;
  align-items: flex-start;
  gap: 18rpx;
}

.artwork-skeleton-comment-copy {
  flex: 1;
}

.artwork-skeleton-related {
  min-width: 0;
}

.artwork-skeleton-block--related {
  width: 100%;
  height: 220rpx;
  background: #111111;
}

.zoom-hint {
  position: absolute;
  bottom: 24rpx;
  right: 24rpx;
  display: flex;
  align-items: center;
  gap: 8rpx;
  background-color: rgba(0, 0, 0, 0.74);
  padding: 12rpx 16rpx;
  border-radius: 0;
  border: 2rpx solid #000000;
  color: #ffffff;
}

.zoom-text {
  font-size: 24rpx;
  color: #ffffff;
}

/* 信息区域 */
.info-section {
  margin: 24rpx 32rpx 24rpx;
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.artwork-info {
  padding: 28rpx;
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
}

.artwork-title {
  font-size: 40rpx;
  font-weight: 700;
  color: var(--nb-ink);
  margin-bottom: 16rpx;
  display: block;
}

.artwork-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 24rpx;
  margin-bottom: 24rpx;
}

.meta-item {
  display: flex;
  align-items: center;
  gap: 8rpx;
  color: var(--text-secondary);
}

.meta-text {
  font-size: 26rpx;
  color: currentColor;
}

.tags-container {
  display: flex;
  flex-wrap: wrap;
  gap: 12rpx;
}

.tag-item {
  background-color: var(--nb-paper);
  padding: 8rpx 16rpx;
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
}

.tag-text {
  font-size: 24rpx;
  color: var(--nb-ink);
  font-weight: 700;
}

/* 作者信息 */
.author-section {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 28rpx;
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
}

.author-info {
  display: flex;
  align-items: center;
  gap: 24rpx;
  flex: 1;
}

.author-details {
  flex: 1;
}

.author-name {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--nb-ink);
  display: block;
  margin-bottom: 4rpx;
}

.author-stats {
  font-size: 24rpx;
  color: #4a4a4a;
}

.follow-btn {
  padding: 16rpx 32rpx;
  background-color: var(--nb-yellow);
  border: 2rpx solid var(--nb-ink);
  box-shadow: none;
  transition: all 0.2s ease;
}

.follow-btn.following {
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
}


.follow-text {
  font-size: 28rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.follow-btn.following .follow-text {
  color: var(--nb-ink);
}

/* 交互按钮 */
.action-section {
  margin: 0 32rpx 32rpx;
}

.action-buttons {
  display: flex;
  background-color: var(--nb-surface);
  border-radius: 0;
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
  overflow: hidden;
}

.action-btn {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 20rpx 12rpx;
  border-radius: 0;
  transition: all 0.2s ease;
  border-right: 2rpx solid var(--nb-ink);
  background: var(--nb-surface);
}

.action-btn:last-child {
  border-right: 0;
}

.action-btn:active {
  background-color: var(--nb-paper);
}

.action-btn.active {
  background: var(--nb-yellow);
}

.action-text {
  font-size: 24rpx;
  color: var(--nb-ink);
  font-weight: 700;
}

.action-btn.active .action-text {
  color: var(--nb-ink);
}

/* 评论区域 */
.comment-section {
  margin: 0 32rpx 32rpx;
  background-color: var(--nb-surface);
  border-radius: 0;
  padding: 32rpx;
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
}

.section-header {
  margin-bottom: 24rpx;
}

.section-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--nb-ink);
}

.comment-list {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.empty-comments {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 48rpx;
  text-align: center;
}

.empty-text {
  font-size: 28rpx;
  color: #777777;
  margin-top: 16rpx;
}

/* 相关推荐 */
.related-section {
  margin: 0 32rpx 32rpx;
  background-color: var(--nb-surface);
  border-radius: 0;
  padding: 32rpx;
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
}

.related-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

.content-spacer {
  height: 132rpx;
}

/* 评论输入栏 */
.comment-input-bar {
  background-color: var(--nb-surface);
  border-top: 2rpx solid var(--nb-ink);
  padding: 24rpx 32rpx;
  padding-bottom: calc(24rpx + var(--layout-bottom-offset));
}

.comment-input-container {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.send-comment-btn {
  min-height: 72rpx;
  padding: 0 26rpx;
  flex-shrink: 0;
  transition: background-color 0.2s ease;
}

.send-comment-btn.disabled {
  background-color: var(--nb-surface);
}

.send-comment-text {
  font-size: 26rpx;
  color: var(--nb-ink);
  font-weight: 700;
}

.bottom-action-bar {
  padding: 16rpx 32rpx;
  padding-bottom: calc(16rpx + var(--layout-bottom-offset));
  background: var(--nb-paper);
  border-top: 2rpx solid var(--nb-ink);
}

.bottom-action-shell {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 24rpx;
  padding: 18rpx 20rpx;
  background: var(--nb-surface);
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.bottom-action-main {
  display: flex;
  flex-direction: column;
  gap: 6rpx;
  min-width: 0;
}

.bottom-action-title {
  font-size: 28rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.bottom-action-desc {
  font-size: 22rpx;
  color: #4a4a4a;
}

.bottom-action-btn {
  flex-shrink: 0;
  min-width: 148rpx;
  height: 72rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 10rpx;
  background: var(--nb-yellow);
  border: 2rpx solid var(--nb-ink);
  box-sizing: border-box;
}

.bottom-action-text {
  font-size: 26rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

/* 更多操作弹窗 */
.more-actions {
  padding: 24rpx 0;
}

.action-item {
  display: flex;
  align-items: center;
  gap: 24rpx;
  padding: 24rpx 32rpx;
  transition: all 0.2s ease;
  color: var(--nb-ink);
}

.action-item:active {
  background-color: var(--nb-paper);
}

.action-item.primary {
  color: var(--nb-blue);
}

.action-item.danger {
  color: var(--nb-coral);
}

.action-label {
  font-size: 32rpx;
  color: currentColor;
}
</style>
