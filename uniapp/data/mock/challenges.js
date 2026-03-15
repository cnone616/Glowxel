// 挑战数据Mock - 社区挑战活动
export const mockChallenges = [
  {
    id: 1,
    title: "春日花园挑战",
    banner: "/static/images/challenge1.png",
    icon: "task",
    description: "用像素艺术创作春天花园主题作品，展现春日的生机与美好",
    startDate: "2024-03-01",
    endDate: "2024-03-31",
    status: "active", // active, upcoming, ended
    participants: 234,
    submissions: 89,
    prize: "限定徽章 + 精美周边",
    difficulty: "简单",
    tags: ["春天", "花园", "自然"],
    rules: [
      "作品尺寸不限，建议32x32以上",
      "必须包含花朵或植物元素",
      "颜色数量不超过16种",
      "原创作品，不得抄袭"
    ],
    judgesCriteria: [
      "创意性 (30%)",
      "技术水平 (25%)",
      "主题契合度 (25%)",
      "视觉效果 (20%)"
    ]
  },
  {
    id: 2,
    title: "复古游戏角色大赛",
    banner: "/static/images/challenge2.png",
    icon: "task",
    description: "重现经典游戏角色，或创作原创的复古风格游戏角色",
    startDate: "2024-02-15",
    endDate: "2024-03-15",
    status: "active",
    participants: 456,
    submissions: 178,
    prize: "现金奖励 + 作品展示机会",
    difficulty: "中等",
    tags: ["游戏", "复古", "角色"],
    rules: [
      "角色尺寸建议16x16或32x32",
      "必须符合8位或16位游戏风格",
      "可以是经典角色重制或原创角色",
      "需要提供角色背景故事"
    ],
    judgesCriteria: [
      "复古风格还原度 (35%)",
      "角色设计创意 (30%)",
      "像素技法 (20%)",
      "故事背景 (15%)"
    ]
  },
  {
    id: 3,
    title: "美食像素艺术节",
    banner: "/static/images/challenge3.png",
    icon: "task",
    description: "创作让人垂涎欲滴的美食像素艺术作品",
    startDate: "2024-03-15",
    endDate: "2024-04-15",
    status: "upcoming",
    participants: 0,
    submissions: 0,
    prize: "美食体验券 + 专属称号",
    difficulty: "简单",
    tags: ["美食", "生活", "创意"],
    rules: [
      "必须以美食为主题",
      "作品要有食欲感",
      "可以是单品或组合",
      "鼓励添加环境元素"
    ],
    judgesCriteria: [
      "视觉吸引力 (40%)",
      "细节表现 (30%)",
      "创意性 (20%)",
      "技术水平 (10%)"
    ]
  },
  {
    id: 4,
    title: "像素动物园",
    banner: "/static/images/challenge4.png",
    icon: "task",
    description: "创作各种可爱动物的像素艺术，建造属于你的像素动物园",
    startDate: "2024-01-01",
    endDate: "2024-02-01",
    status: "ended",
    participants: 678,
    submissions: 234,
    prize: "动物主题徽章套装",
    difficulty: "简单",
    tags: ["动物", "可爱", "自然"],
    rules: [
      "必须以动物为主角",
      "可以是现实或虚构动物",
      "鼓励表现动物特征",
      "可以添加简单背景"
    ],
    judgesCriteria: [
      "动物特征表现 (35%)",
      "可爱程度 (30%)",
      "像素技法 (25%)",
      "整体效果 (10%)"
    ]
  }
]

// 挑战状态统计
export const challengeStats = {
  active: mockChallenges.filter(c => c.status === "active").length,
  upcoming: mockChallenges.filter(c => c.status === "upcoming").length,
  ended: mockChallenges.filter(c => c.status === "ended").length,
  totalParticipants: mockChallenges.reduce((sum, c) => sum + c.participants, 0),
  totalSubmissions: mockChallenges.reduce((sum, c) => sum + c.submissions, 0)
}

// 获取所有挑战
export function getAllChallenges() {
  return mockChallenges
}

// 根据状态获取挑战
export function getChallengesByStatus(status) {
  return mockChallenges.filter(challenge => challenge.status === status)
}

// 获取活跃挑战
export function getActiveChallenges() {
  return mockChallenges.filter(challenge => challenge.status === "active")
}

// 获取即将开始的挑战
export function getUpcomingChallenges() {
  return mockChallenges.filter(challenge => challenge.status === "upcoming")
}

// 获取热门挑战（按参与人数排序）
export function getPopularChallenges(limit = 3) {
  return mockChallenges
    .sort((a, b) => b.participants - a.participants)
    .slice(0, limit)
}

// 根据ID获取挑战
export function getChallengeById(id) {
  return mockChallenges.find(challenge => challenge.id === id)
}

// 参加挑战
export function joinChallenge(challengeId) {
  const challenge = getChallengeById(challengeId)
  if (challenge && challenge.status === "active") {
    challenge.participants += 1
    return { success: true, message: "成功参加挑战！" }
  }
  return { success: false, message: "无法参加此挑战" }
}

// 提交作品到挑战
export function submitToChallenge(challengeId, artwork) {
  const challenge = getChallengeById(challengeId)
  if (challenge && challenge.status === "active") {
    challenge.submissions += 1
    return { success: true, message: "作品提交成功！" }
  }
  return { success: false, message: "提交失败" }
}