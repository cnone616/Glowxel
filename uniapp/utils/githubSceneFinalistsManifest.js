const githubSceneFinalistsManifest = {
  generatedAt: "2026-04-15",
  sourceSummary: {
    primaryRepos: [
      "hZambrzycki/aseprite-pixel-assets",
      "xliii/cityscape",
    ],
    secondaryRepos: [
      "shndnth/Pixl",
    ],
  },
  priority: [
    {
      id: "sunset_ridge_c",
      title: "Sunset Ridge C",
      theme: "sunset_landscape",
      sourceRepo: "hZambrzycki/aseprite-pixel-assets",
      sourceAsset: "08-LandScape.aseprite",
      boardFile: "static/github-scene-finalists/priority/sunset_ridge_c.png",
      note: "当前最稳的晚霞山脊版本，适合作为首个静态风景入库候选。",
    },
    {
      id: "scifi_lake_a",
      title: "SciFi Lake A",
      theme: "scifi_lake",
      sourceRepo: "hZambrzycki/aseprite-pixel-assets",
      sourceAsset: "07-SciFi.aseprite",
      boardFile: "static/github-scene-finalists/priority/scifi_lake_a.png",
      note: "双月+行星主体清楚，适合做高辨识度科幻风景。",
    },
    {
      id: "city_evening_b",
      title: "City Evening B",
      theme: "city_evening",
      sourceRepo: "xliii/cityscape",
      sourceAsset: "index.html?type=evening",
      boardFile: "static/github-scene-finalists/priority/city_evening_b.png",
      note: "楼体结构稳定，额外补了霓虹点缀，适合先走赛博城市方向。",
    },
  ],
  backup: [
    {
      id: "sunset_ridge_a",
      title: "Sunset Ridge A",
      theme: "sunset_landscape",
      sourceRepo: "hZambrzycki/aseprite-pixel-assets",
      sourceAsset: "08-LandScape.aseprite",
      boardFile: "static/github-scene-finalists/backup/sunset_ridge_a.png",
      note: "偏保守的暖色地平线版本。",
    },
    {
      id: "scifi_lake_b",
      title: "SciFi Lake B",
      theme: "scifi_lake",
      sourceRepo: "hZambrzycki/aseprite-pixel-assets",
      sourceAsset: "07-SciFi.aseprite",
      boardFile: "static/github-scene-finalists/backup/scifi_lake_b.png",
      note: "保留更多天空和月亮元素。",
    },
    {
      id: "city_night_d",
      title: "City Night D",
      theme: "city_night",
      sourceRepo: "xliii/cityscape",
      sourceAsset: "index.html?type=night",
      boardFile: "static/github-scene-finalists/backup/city_night_d.png",
      note: "更黑、更密的夜景版本，适合后续做夜间霓虹分支。",
    },
    {
      id: "city_evening_f",
      title: "City Evening F",
      theme: "city_evening",
      sourceRepo: "xliii/cityscape",
      sourceAsset: "index.html?type=evening",
      boardFile: "static/github-scene-finalists/backup/city_evening_f.png",
      note: "保留更多窗灯变化，适合继续调成动态城市候选。",
    },
  ],
  sheet: {
    file: "static/github-scene-finalists/sheets/github_finalists_sheet.png",
    note: "用于人工筛选的大总览图。",
  },
};

export default githubSceneFinalistsManifest;
