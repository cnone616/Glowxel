const PANEL_WIDTH = 64;
const PANEL_HEIGHT = 64;
const GAME_INDEX_PATH = 'material-library/nes-game-assets/index.json';
const LAST_SELECTED_GAME_KEY = 'characterAnimationPageSelectedGameV2';

const GROUP_INTENT_ORDER = {
  base: ['idle', 'pose', 'walk', 'inch-walk', 'run', 'ride', 'roll', 'swim', 'aim-up', 'duck', 'climb', 'block', 'leaf-shield'],
  jump: ['jump', 'spinning-jump', 'jump-knee'],
  attack: ['attack', 'shoot', 'punch', 'kick', 'throw', 'blow'],
  special: ['beat-chest', 'equip-helmet', 'ignition', 'hurt', 'knockdown', 'intro-frame'],
};

const NEUTRAL_COMBO_BASE_IDS = ['idle', 'pose'];

const ACTION_META = {
  idle: { group: 'base', intentId: 'idle', buttonLabel: '待机' },
  pose: { group: 'base', intentId: 'pose', buttonLabel: '姿态' },
  walk: { group: 'base', intentId: 'walk', buttonLabel: '行走' },
  'inch-walk': { group: 'base', intentId: 'inch-walk', buttonLabel: '小步' },
  run: { group: 'base', intentId: 'run', buttonLabel: '奔跑' },
  ride: { group: 'base', intentId: 'ride', buttonLabel: '滑板' },
  roll: { group: 'base', intentId: 'roll', buttonLabel: '滚动' },
  swim: { group: 'base', intentId: 'swim', buttonLabel: '游泳' },
  'aim-up': { group: 'base', intentId: 'aim-up', buttonLabel: '上举' },
  duck: { group: 'base', intentId: 'duck', buttonLabel: '下蹲' },
  climb: { group: 'base', intentId: 'climb', buttonLabel: '攀爬' },
  block: { group: 'base', intentId: 'block', buttonLabel: '防御' },
  'leaf-shield': { group: 'base', intentId: 'leaf-shield', buttonLabel: '叶盾' },
  jump: { group: 'jump', intentId: 'jump', buttonLabel: '跳跃' },
  'spinning-jump': { group: 'jump', intentId: 'spinning-jump', buttonLabel: '旋转跳' },
  'jump-knee': { group: 'jump', intentId: 'jump-knee', buttonLabel: '跳膝' },
  attack: { group: 'attack', intentId: 'attack', buttonLabel: '攻击' },
  'crouch-attack': { group: 'attack', intentId: 'attack', buttonLabel: '攻击', comboBaseId: 'duck' },
  'jump-attack': { group: 'attack', intentId: 'attack', buttonLabel: '攻击', comboBaseId: 'jump' },
  'climb-attack': { group: 'attack', intentId: 'attack', buttonLabel: '攻击', comboBaseId: 'climb' },
  shoot: { group: 'attack', intentId: 'shoot', buttonLabel: '射击' },
  'shooting-idle': { group: 'attack', intentId: 'shoot', buttonLabel: '射击', comboBaseId: 'idle' },
  'shooting-walk': { group: 'attack', intentId: 'shoot', buttonLabel: '射击', comboBaseId: 'walk' },
  'shooting-jump': { group: 'attack', intentId: 'shoot', buttonLabel: '射击', comboBaseId: 'jump' },
  'shooting-aim-up': { group: 'attack', intentId: 'shoot', buttonLabel: '射击', comboBaseId: 'aim-up' },
  'swim-shoot': { group: 'attack', intentId: 'shoot', buttonLabel: '射击', comboBaseId: 'swim' },
  'jump-shoot': { group: 'attack', intentId: 'shoot', buttonLabel: '射击', comboBaseId: 'jump' },
  punch: { group: 'attack', intentId: 'punch', buttonLabel: '直拳' },
  kick: { group: 'attack', intentId: 'kick', buttonLabel: '高踢' },
  throw: { group: 'attack', intentId: 'throw', buttonLabel: '投掷' },
  blow: { group: 'attack', intentId: 'blow', buttonLabel: '吹风' },
  'beat-chest': { group: 'special', intentId: 'beat-chest', buttonLabel: '捶胸' },
  'equip-helmet': { group: 'special', intentId: 'equip-helmet', buttonLabel: '装备头盔' },
  ignition: { group: 'special', intentId: 'ignition', buttonLabel: '点火' },
  hurt: { group: 'special', intentId: 'hurt', buttonLabel: '受伤' },
  knockdown: { group: 'special', intentId: 'knockdown', buttonLabel: '倒地' },
  'intro-frame': { group: 'special', intentId: 'intro-frame', buttonLabel: '登场帧' },
};

const canvas = document.getElementById('themeCanvas');
const ctx = canvas.getContext('2d');
ctx.imageSmoothingEnabled = false;

const previewCanvas = document.createElement('canvas');
previewCanvas.width = PANEL_WIDTH;
previewCanvas.height = PANEL_HEIGHT;
const previewCtx = previewCanvas.getContext('2d');
previewCtx.imageSmoothingEnabled = false;

const collator = new Intl.Collator('zh-CN');

const STATE = {
  gameDefinitions: [],
  selectedGameId: '',
  manifest: null,
  characterAnimations: null,
  selectedRoleId: '',
  selectedFormId: '',
  defaultBaseActionId: '',
  baseActionId: '',
  activeActionId: '',
  previewFrame: 0,
  isPlaying: false,
  playTimerId: null,
  transientPlayback: null,
  lastResolution: null,
  resolvedFrameCache: new Map(),
  renderDiagnostics: {
    clippedPixels: 0,
    overflowX: 0,
    overflowY: 0,
  },
};

function dom(id) {
  return document.getElementById(id);
}

function setPageStatus(message, tone) {
  const element = dom('pageStatus');
  element.textContent = message;
  element.dataset.state = tone;
}

function setSelectionSummary(message) {
  dom('selectionSummary').textContent = message;
}

function setNotice(message, tone) {
  const element = dom('emptyState');
  element.textContent = message;
  element.dataset.state = tone;
}

function createOption(value, label) {
  const option = document.createElement('option');
  option.value = value;
  option.textContent = label;
  return option;
}

function getGameRoot(gameId) {
  return `material-library/nes-game-assets/${gameId}/`;
}

function getManifestPath(gameId) {
  return `${getGameRoot(gameId)}processed/manifest.json`;
}

async function fetchJson(path) {
  const response = await fetch(path, { cache: 'no-store' });
  if (!response.ok) {
    throw new Error(`${path} 加载失败 (${response.status})`);
  }
  return response.json();
}

function clearPlaybackTimer() {
  if (STATE.playTimerId !== null) {
    window.clearTimeout(STATE.playTimerId);
    STATE.playTimerId = null;
  }
}

function stopPlayback(shouldRender) {
  clearPlaybackTimer();
  STATE.isPlaying = false;
  if (shouldRender === true) {
    renderAll();
  }
}

function getRoles() {
  if (STATE.characterAnimations === null) {
    return [];
  }
  if (Array.isArray(STATE.characterAnimations.roles)) {
    return STATE.characterAnimations.roles;
  }
  return [];
}

function getCurrentRole() {
  const roles = getRoles();
  for (let index = 0; index < roles.length; index += 1) {
    if (roles[index].id === STATE.selectedRoleId) {
      return roles[index];
    }
  }
  return null;
}

function getCurrentForm() {
  const role = getCurrentRole();
  if (role === null) {
    return null;
  }
  if (!Array.isArray(role.forms)) {
    return null;
  }
  for (let index = 0; index < role.forms.length; index += 1) {
    if (role.forms[index].id === STATE.selectedFormId) {
      return role.forms[index];
    }
  }
  return null;
}

function getActionById(form, actionId) {
  if (form === null) {
    return null;
  }
  if (!Array.isArray(form.actions)) {
    return null;
  }
  for (let index = 0; index < form.actions.length; index += 1) {
    if (form.actions[index].id === actionId) {
      return form.actions[index];
    }
  }
  return null;
}

function getCurrentAction() {
  const form = getCurrentForm();
  return getActionById(form, STATE.activeActionId);
}

function getBaseAction() {
  const form = getCurrentForm();
  return getActionById(form, STATE.baseActionId);
}

function getDefaultBaseAction(form) {
  if (form === null) {
    return null;
  }
  if (!Array.isArray(form.actions)) {
    return null;
  }

  for (let index = 0; index < form.actions.length; index += 1) {
    if (form.actions[index].loop === true) {
      return form.actions[index];
    }
  }

  if (form.actions.length === 0) {
    return null;
  }
  return form.actions[0];
}

function getCurrentFrameRef() {
  const action = getCurrentAction();
  if (action === null) {
    return null;
  }
  if (!Array.isArray(action.frames)) {
    return null;
  }
  if (STATE.previewFrame < 0 || STATE.previewFrame >= action.frames.length) {
    return null;
  }
  return action.frames[STATE.previewFrame];
}

function getRawFrameById(frameId) {
  if (STATE.characterAnimations === null) {
    return null;
  }
  if (STATE.characterAnimations.frames === undefined || STATE.characterAnimations.frames === null) {
    return null;
  }
  if (!(frameId in STATE.characterAnimations.frames)) {
    return null;
  }
  return STATE.characterAnimations.frames[frameId];
}

function buildAlignedBasePixelMap(baseFrame, targetFrame) {
  const alignedMap = new Map();
  for (let index = 0; index < baseFrame.pixels.length; index += 1) {
    const pixel = baseFrame.pixels[index];
    const worldX = pixel[0] - baseFrame.origin.x;
    const worldY = pixel[1] - baseFrame.origin.y;
    const targetX = worldX + targetFrame.origin.x;
    const targetY = worldY + targetFrame.origin.y;
    if (targetX < 0 || targetY < 0) {
      continue;
    }
    if (targetX >= targetFrame.size.width || targetY >= targetFrame.size.height) {
      continue;
    }
    alignedMap.set(`${targetX}:${targetY}`, [targetX, targetY, pixel[2], pixel[3], pixel[4]]);
  }
  return alignedMap;
}

function buildResolvedPixelsFromPatch(baseFrame, targetFrame, patchBox, patchPixels) {
  const pixelMap = new Map();
  const patchMinX = patchBox.x;
  const patchMinY = patchBox.y;
  const patchMaxX = patchBox.x + patchBox.width;
  const patchMaxY = patchBox.y + patchBox.height;
  const alignedBasePixels = buildAlignedBasePixelMap(baseFrame, targetFrame);

  for (const pixel of alignedBasePixels.values()) {
    const pixelX = pixel[0];
    const pixelY = pixel[1];
    const insidePatchX = pixelX >= patchMinX && pixelX < patchMaxX;
    const insidePatchY = pixelY >= patchMinY && pixelY < patchMaxY;
    if (insidePatchX && insidePatchY) {
      continue;
    }
    pixelMap.set(`${pixelX}:${pixelY}`, pixel);
  }

  for (let index = 0; index < patchPixels.length; index += 1) {
    const pixel = patchPixels[index];
    const pixelX = patchBox.x + pixel[0];
    const pixelY = patchBox.y + pixel[1];
    pixelMap.set(`${pixelX}:${pixelY}`, [pixelX, pixelY, pixel[2], pixel[3], pixel[4]]);
  }

  const pixels = Array.from(pixelMap.values());
  pixels.sort((left, right) => {
    if (left[1] !== right[1]) {
      return left[1] - right[1];
    }
    return left[0] - right[0];
  });
  return pixels;
}

function resolveFrameById(frameId, pendingFrameIds) {
  if (STATE.resolvedFrameCache.has(frameId)) {
    return STATE.resolvedFrameCache.get(frameId);
  }

  if (pendingFrameIds.has(frameId)) {
    return null;
  }
  pendingFrameIds.add(frameId);

  const frame = getRawFrameById(frameId);
  if (frame === null) {
    pendingFrameIds.delete(frameId);
    return null;
  }

  let pixels = null;
  if (Array.isArray(frame.pixels)) {
    pixels = frame.pixels;
  } else if (
    typeof frame.baseRef === 'string' &&
    frame.patchBox !== null &&
    frame.patchBox !== undefined &&
    Array.isArray(frame.patchPixels)
  ) {
    const baseFrame = resolveFrameById(frame.baseRef, pendingFrameIds);
    if (baseFrame !== null && Array.isArray(baseFrame.pixels)) {
      pixels = buildResolvedPixelsFromPatch(baseFrame, frame, frame.patchBox, frame.patchPixels);
    }
  }

  pendingFrameIds.delete(frameId);

  if (pixels === null) {
    return null;
  }

  const resolvedFrame = {
    sourceAsset: frame.sourceAsset,
    sourceBox: frame.sourceBox,
    trimBox: frame.trimBox,
    size: frame.size,
    origin: frame.origin,
    pixelCount: frame.pixelCount,
    pixels,
  };

  STATE.resolvedFrameCache.set(frameId, resolvedFrame);
  return resolvedFrame;
}

function getCurrentResolvedFrame() {
  const frameRef = getCurrentFrameRef();
  if (frameRef === null) {
    return null;
  }
  return resolveFrameById(frameRef.ref, new Set());
}

function getCurrentRawFrame() {
  const frameRef = getCurrentFrameRef();
  if (frameRef === null) {
    return null;
  }
  return getRawFrameById(frameRef.ref);
}

function formatBox(box) {
  if (box === null || box === undefined) {
    return '-';
  }
  return `${box.x}, ${box.y}, ${box.width}x${box.height}`;
}

function formatSize(size) {
  if (size === null || size === undefined) {
    return '-';
  }
  return `${size.width} x ${size.height}`;
}

function formatOrigin(origin) {
  if (origin === null || origin === undefined) {
    return '-';
  }
  return `${origin.x}, ${origin.y}`;
}

function fillSelect(selectId, items, selectedId, placeholder) {
  const select = dom(selectId);
  select.innerHTML = '';

  if (items.length === 0) {
    const option = createOption('', placeholder);
    option.disabled = true;
    option.selected = true;
    select.append(option);
    select.disabled = true;
    return;
  }

  for (let index = 0; index < items.length; index += 1) {
    select.append(createOption(items[index].id, items[index].label));
  }

  select.disabled = false;
  select.value = selectedId;
}

function getIntentOrder(group, intentId) {
  const groupOrder = GROUP_INTENT_ORDER[group];
  if (!Array.isArray(groupOrder)) {
    return 999;
  }
  for (let index = 0; index < groupOrder.length; index += 1) {
    if (groupOrder[index] === intentId) {
      return index;
    }
  }
  return 999;
}

function getActionMeta(action) {
  if (action.id in ACTION_META) {
    const meta = ACTION_META[action.id];
    return {
      group: meta.group,
      intentId: meta.intentId,
      buttonLabel: meta.buttonLabel,
      comboBaseId: meta.comboBaseId === undefined ? null : meta.comboBaseId,
    };
  }

  return {
    group: 'special',
    intentId: action.id,
    buttonLabel: action.label,
    comboBaseId: null,
  };
}

function collectSemanticGroups(form) {
  const groupedIntents = {
    base: new Map(),
    jump: new Map(),
    attack: new Map(),
    special: new Map(),
  };

  if (form === null || !Array.isArray(form.actions)) {
    return {
      base: [],
      jump: [],
      attack: [],
      special: [],
    };
  }

  for (let index = 0; index < form.actions.length; index += 1) {
    const action = form.actions[index];
    const meta = getActionMeta(action);
    const groupMap = groupedIntents[meta.group];
    if (!groupMap.has(meta.intentId)) {
      groupMap.set(meta.intentId, {
        group: meta.group,
        intentId: meta.intentId,
        label: meta.buttonLabel,
        order: getIntentOrder(meta.group, meta.intentId),
        actions: [],
      });
    }
    groupMap.get(meta.intentId).actions.push({ action, meta });
  }

  const result = {
    base: [],
    jump: [],
    attack: [],
    special: [],
  };

  const groups = ['base', 'jump', 'attack', 'special'];
  for (let groupIndex = 0; groupIndex < groups.length; groupIndex += 1) {
    const group = groups[groupIndex];
    const intents = Array.from(groupedIntents[group].values());
    intents.sort((left, right) => {
      if (left.order !== right.order) {
        return left.order - right.order;
      }
      return collator.compare(left.label, right.label);
    });
    result[group] = intents;
  }

  return result;
}

function getCurrentSemanticGroups() {
  const form = getCurrentForm();
  return collectSemanticGroups(form);
}

function getBaseIntentButtonAction(intent) {
  if (intent.actions.length === 0) {
    return null;
  }

  let selectedVariant = intent.actions[0];
  for (let index = 0; index < intent.actions.length; index += 1) {
    const variant = intent.actions[index];
    if (variant.action.loop === true) {
      selectedVariant = variant;
      break;
    }
  }
  return selectedVariant.action;
}

function isNeutralComboBaseId(comboBaseId) {
  for (let index = 0; index < NEUTRAL_COMBO_BASE_IDS.length; index += 1) {
    if (NEUTRAL_COMBO_BASE_IDS[index] === comboBaseId) {
      return true;
    }
  }
  return false;
}

function rankVariantForExact(variant) {
  let score = 0;
  if (variant.action.loop === false) {
    score += 20;
  }
  return score;
}

function rankVariantForFallback(variant) {
  let score = 0;
  if (variant.action.loop === false) {
    score += 400;
  }
  if (variant.meta.comboBaseId === null) {
    score += 200;
  } else if (isNeutralComboBaseId(variant.meta.comboBaseId)) {
    score += 120;
  }
  score += 1000 - getIntentOrder(variant.meta.group, variant.meta.intentId);
  return score;
}

function pickBestVariant(variants, ranker) {
  if (variants.length === 0) {
    return null;
  }

  let bestVariant = variants[0];
  let bestScore = ranker(bestVariant);
  for (let index = 1; index < variants.length; index += 1) {
    const candidate = variants[index];
    const score = ranker(candidate);
    if (score > bestScore) {
      bestVariant = candidate;
      bestScore = score;
    }
  }
  return bestVariant;
}

function resolveIntent(intent) {
  const baseAction = getBaseAction();
  const baseActionId = baseAction === null ? '' : baseAction.id;
  const exactMatches = [];
  const fallbackMatches = [];

  for (let index = 0; index < intent.actions.length; index += 1) {
    const variant = intent.actions[index];
    if (variant.meta.comboBaseId === baseActionId) {
      exactMatches.push(variant);
      continue;
    }
    fallbackMatches.push(variant);
  }

  const exactVariant = pickBestVariant(exactMatches, rankVariantForExact);
  if (exactVariant !== null) {
    return {
      action: exactVariant.action,
      policy: '当前基础态精确组合动作',
    };
  }

  const fallbackVariant = pickBestVariant(fallbackMatches, rankVariantForFallback);
  if (fallbackVariant !== null) {
    if (fallbackVariant.action.loop === false) {
      return {
        action: fallbackVariant.action,
        policy: '通用一次性动作',
      };
    }
    return {
      action: fallbackVariant.action,
      policy: '通用语义动作',
    };
  }

  if (baseAction !== null) {
    return {
      action: baseAction,
      policy: '当前基础态本身',
    };
  }

  return null;
}

function resetSelectionState() {
  STATE.selectedRoleId = '';
  STATE.selectedFormId = '';
  STATE.defaultBaseActionId = '';
  STATE.baseActionId = '';
  STATE.activeActionId = '';
  STATE.previewFrame = 0;
  STATE.transientPlayback = null;
  STATE.lastResolution = null;
}

function initializeCurrentSelection() {
  const roles = getRoles();
  if (roles.length === 0) {
    resetSelectionState();
    return;
  }

  const role = roles[0];
  STATE.selectedRoleId = role.id;

  if (!Array.isArray(role.forms) || role.forms.length === 0) {
    STATE.selectedFormId = '';
    STATE.defaultBaseActionId = '';
    STATE.baseActionId = '';
    STATE.activeActionId = '';
    STATE.previewFrame = 0;
    STATE.transientPlayback = null;
    STATE.lastResolution = null;
    return;
  }

  const form = role.forms[0];
  STATE.selectedFormId = form.id;

  const defaultBaseAction = getDefaultBaseAction(form);
  if (defaultBaseAction === null) {
    STATE.defaultBaseActionId = '';
    STATE.baseActionId = '';
    STATE.activeActionId = '';
  } else {
    STATE.defaultBaseActionId = defaultBaseAction.id;
    STATE.baseActionId = defaultBaseAction.id;
    STATE.activeActionId = defaultBaseAction.id;
  }

  STATE.previewFrame = 0;
  STATE.transientPlayback = null;
  STATE.lastResolution = null;
}

function buildResolutionRecord(group, intentId, intentLabel, matchedAction, policy, returnLabel) {
  return {
    group,
    intentId,
    intentLabel,
    matchedActionId: matchedAction === null ? '' : matchedAction.id,
    matchedActionLabel: matchedAction === null ? '-' : matchedAction.label,
    policy,
    returnLabel,
  };
}

function applyFormSelection(formId, continuePlayback, resolutionText) {
  stopPlayback(false);
  STATE.selectedFormId = formId;
  const form = getCurrentForm();
  const defaultBaseAction = getDefaultBaseAction(form);

  if (defaultBaseAction === null) {
    STATE.defaultBaseActionId = '';
    STATE.baseActionId = '';
    STATE.activeActionId = '';
    STATE.previewFrame = 0;
    STATE.transientPlayback = null;
    STATE.lastResolution = buildResolutionRecord('form', formId, '切换形态', null, resolutionText, '-');
    renderAll();
    return;
  }

  STATE.defaultBaseActionId = defaultBaseAction.id;
  STATE.baseActionId = defaultBaseAction.id;
  STATE.activeActionId = defaultBaseAction.id;
  STATE.previewFrame = 0;
  STATE.transientPlayback = null;
  STATE.lastResolution = buildResolutionRecord('form', formId, '切换形态', defaultBaseAction, resolutionText, defaultBaseAction.label);

  renderAll();
  if (continuePlayback === true) {
    startPlayback();
  }
}

function applyRoleSelection(roleId, continuePlayback) {
  stopPlayback(false);
  STATE.selectedRoleId = roleId;
  const role = getCurrentRole();
  if (role === null || !Array.isArray(role.forms) || role.forms.length === 0) {
    STATE.selectedFormId = '';
    STATE.defaultBaseActionId = '';
    STATE.baseActionId = '';
    STATE.activeActionId = '';
    STATE.previewFrame = 0;
    STATE.transientPlayback = null;
    STATE.lastResolution = buildResolutionRecord('role', roleId, '切换角色', null, '角色下没有可用形态', '-');
    renderAll();
    return;
  }

  applyFormSelection(role.forms[0].id, continuePlayback, '切换角色后落到默认基础态');
}

function activateBaseAction(actionId, continuePlayback) {
  const form = getCurrentForm();
  const action = getActionById(form, actionId);
  if (action === null) {
    return;
  }

  stopPlayback(false);
  STATE.baseActionId = action.id;
  STATE.activeActionId = action.id;
  STATE.previewFrame = 0;
  STATE.transientPlayback = null;
  STATE.lastResolution = buildResolutionRecord('base', action.id, action.label, action, '基础态切换', action.label);

  renderAll();
  if (continuePlayback === true) {
    startPlayback();
  }
}

function triggerIntent(group, intentId) {
  const semanticGroups = getCurrentSemanticGroups();
  const groupIntents = semanticGroups[group];
  if (!Array.isArray(groupIntents)) {
    return;
  }

  let intent = null;
  for (let index = 0; index < groupIntents.length; index += 1) {
    if (groupIntents[index].intentId === intentId) {
      intent = groupIntents[index];
      break;
    }
  }
  if (intent === null) {
    return;
  }

  const resolution = resolveIntent(intent);
  if (resolution === null) {
    return;
  }

  const baseAction = getBaseAction();
  const baseLabel = baseAction === null ? '-' : baseAction.label;

  if (baseAction !== null && resolution.action.id === baseAction.id && resolution.policy === '当前基础态本身') {
    stopPlayback(false);
    STATE.activeActionId = baseAction.id;
    STATE.previewFrame = 0;
    STATE.transientPlayback = null;
    STATE.lastResolution = buildResolutionRecord(group, intent.intentId, intent.label, baseAction, resolution.policy, baseLabel);
    renderAll();
    return;
  }

  const wasPlaying = STATE.isPlaying;
  stopPlayback(false);
  STATE.activeActionId = resolution.action.id;
  STATE.previewFrame = 0;
  STATE.transientPlayback = {
    group,
    intentId: intent.intentId,
    intentLabel: intent.label,
    resumeBasePlayback: wasPlaying,
  };
  STATE.lastResolution = buildResolutionRecord(group, intent.intentId, intent.label, resolution.action, resolution.policy, baseLabel);

  renderAll();
  startPlayback();
}

function returnToBaseState(continuePlayback) {
  stopPlayback(false);
  const baseAction = getBaseAction();
  if (baseAction === null) {
    STATE.activeActionId = '';
    STATE.previewFrame = 0;
    STATE.transientPlayback = null;
    renderAll();
    return;
  }

  STATE.activeActionId = baseAction.id;
  STATE.previewFrame = 0;
  STATE.transientPlayback = null;
  renderAll();

  if (continuePlayback === true) {
    startPlayback();
  }
}

function startPlayback() {
  const action = getCurrentAction();
  if (action === null || !Array.isArray(action.frames) || action.frames.length === 0) {
    return;
  }

  const lastIndex = action.frames.length - 1;
  if (STATE.previewFrame >= lastIndex) {
    STATE.previewFrame = 0;
  }

  STATE.isPlaying = true;
  renderTransport();
  renderCanvas();
  scheduleNextFrame();
}

function togglePlayback() {
  if (STATE.isPlaying === true) {
    stopPlayback(true);
    return;
  }
  startPlayback();
}

function scheduleNextFrame() {
  clearPlaybackTimer();

  if (STATE.isPlaying !== true) {
    return;
  }

  const frameRef = getCurrentFrameRef();
  if (frameRef === null) {
    stopPlayback(true);
    return;
  }

  STATE.playTimerId = window.setTimeout(() => {
    advancePlayback();
  }, frameRef.durationMs);
}

function advancePlayback() {
  const action = getCurrentAction();
  if (action === null || !Array.isArray(action.frames)) {
    stopPlayback(true);
    return;
  }

  const lastIndex = action.frames.length - 1;
  if (STATE.previewFrame >= lastIndex) {
    if (STATE.transientPlayback !== null) {
      returnToBaseState(STATE.transientPlayback.resumeBasePlayback);
      return;
    }

    if (action.loop === true) {
      STATE.previewFrame = 0;
      renderAll();
      scheduleNextFrame();
      return;
    }

    STATE.previewFrame = lastIndex;
    stopPlayback(false);
    renderAll();
    return;
  }

  STATE.previewFrame += 1;
  renderAll();
  scheduleNextFrame();
}

function stepPreviewFrame(delta) {
  const action = getCurrentAction();
  if (action === null || !Array.isArray(action.frames)) {
    return;
  }

  stopPlayback(false);

  let nextIndex = STATE.previewFrame + delta;
  if (nextIndex < 0) {
    nextIndex = 0;
  }

  const maxIndex = action.frames.length - 1;
  if (nextIndex > maxIndex) {
    nextIndex = maxIndex;
  }

  STATE.previewFrame = nextIndex;
  renderAll();
}

function cycleForm(direction) {
  const role = getCurrentRole();
  if (role === null || !Array.isArray(role.forms) || role.forms.length === 0) {
    return;
  }

  let currentIndex = 0;
  for (let index = 0; index < role.forms.length; index += 1) {
    if (role.forms[index].id === STATE.selectedFormId) {
      currentIndex = index;
      break;
    }
  }

  let nextIndex = currentIndex + direction;
  if (nextIndex < 0) {
    nextIndex = role.forms.length - 1;
  }
  if (nextIndex >= role.forms.length) {
    nextIndex = 0;
  }

  const wasPlaying = STATE.isPlaying;
  applyFormSelection(role.forms[nextIndex].id, wasPlaying, '切 form 后落到默认基础态');
}

function getPlaybackModeLabel() {
  const action = getCurrentAction();
  if (action === null) {
    return '-';
  }

  if (STATE.transientPlayback !== null) {
    if (STATE.isPlaying === true) {
      return '触发动作单轮';
    }
    return '触发动作停帧';
  }

  if (STATE.isPlaying === true && action.loop === true) {
    return '基础态循环';
  }
  if (STATE.isPlaying === true) {
    return '动作播放中';
  }
  return '停帧预览';
}

function getFrameRenderType(frame) {
  if (frame === null) {
    return '-';
  }
  if (Array.isArray(frame.pixels)) {
    return '全量像素';
  }
  if (typeof frame.baseRef === 'string' && Array.isArray(frame.patchPixels)) {
    return '差异帧';
  }
  return '未知';
}

function renderSelectionControls() {
  fillSelect('gameSelect', STATE.gameDefinitions, STATE.selectedGameId, '暂无游戏');

  const roles = getRoles();
  const roleItems = [];
  for (let index = 0; index < roles.length; index += 1) {
    roleItems.push({ id: roles[index].id, label: roles[index].label });
  }
  fillSelect('roleSelect', roleItems, STATE.selectedRoleId, '暂无角色');

  const role = getCurrentRole();
  const formItems = [];
  if (role !== null && Array.isArray(role.forms)) {
    for (let index = 0; index < role.forms.length; index += 1) {
      formItems.push({ id: role.forms[index].id, label: role.forms[index].label });
    }
  }
  fillSelect('formSelect', formItems, STATE.selectedFormId, '暂无形态');
}

function renderTransport() {
  const action = getCurrentAction();
  const frameRange = dom('previewFrameRange');
  const frameLabel = dom('previewFrameLabel');
  const prevButton = dom('previewPrevBtn');
  const playButton = dom('previewPlayBtn');
  const nextButton = dom('previewNextBtn');

  if (action === null || !Array.isArray(action.frames) || action.frames.length === 0) {
    frameRange.min = '0';
    frameRange.max = '0';
    frameRange.value = '0';
    frameRange.disabled = true;
    frameLabel.textContent = '等待动作帧';
    prevButton.disabled = true;
    playButton.disabled = true;
    nextButton.disabled = true;
    playButton.textContent = '播放';
    return;
  }

  const maxIndex = action.frames.length - 1;
  frameRange.min = '0';
  frameRange.max = String(maxIndex);
  frameRange.value = String(STATE.previewFrame);
  frameRange.disabled = false;
  frameLabel.textContent = `第 ${STATE.previewFrame + 1} / ${action.frames.length} 帧 · ${action.label}`;
  prevButton.disabled = STATE.previewFrame === 0;
  nextButton.disabled = STATE.previewFrame === maxIndex;
  playButton.disabled = false;
  playButton.textContent = STATE.isPlaying === true ? '暂停' : '播放';
}

function renderCanvas() {
  previewCtx.clearRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
  previewCtx.fillStyle = '#05090c';
  previewCtx.fillRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
  previewCtx.fillStyle = 'rgba(66, 211, 183, 0.12)';
  previewCtx.fillRect(0, 56, PANEL_WIDTH, 1);
  previewCtx.fillStyle = 'rgba(255, 255, 255, 0.03)';
  previewCtx.fillRect(0, 57, PANEL_WIDTH, PANEL_HEIGHT - 57);

  const frame = getCurrentResolvedFrame();
  let clippedPixels = 0;
  let overflowX = 0;
  let overflowY = 0;
  if (frame !== null && frame.origin !== null && frame.origin !== undefined && Array.isArray(frame.pixels)) {
    const anchorX = Math.floor(PANEL_WIDTH / 2);
    const anchorY = 56;

    for (let index = 0; index < frame.pixels.length; index += 1) {
      const pixel = frame.pixels[index];
      const drawX = anchorX - frame.origin.x + pixel[0];
      const drawY = anchorY - frame.origin.y + pixel[1];

      if (drawX < 0 || drawX >= PANEL_WIDTH || drawY < 0 || drawY >= PANEL_HEIGHT) {
        clippedPixels += 1;
        if (drawX < 0 || drawX >= PANEL_WIDTH) {
          overflowX += 1;
        }
        if (drawY < 0 || drawY >= PANEL_HEIGHT) {
          overflowY += 1;
        }
        continue;
      }

      previewCtx.fillStyle = `rgb(${pixel[2]}, ${pixel[3]}, ${pixel[4]})`;
      previewCtx.fillRect(drawX, drawY, 1, 1);
    }
  }

  STATE.renderDiagnostics = {
    clippedPixels,
    overflowX,
    overflowY,
  };

  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.fillStyle = '#04080a';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.drawImage(previewCanvas, 0, 0, canvas.width, canvas.height);

  ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
  ctx.lineWidth = 1;
  for (let step = 0; step <= canvas.width; step += 10) {
    ctx.beginPath();
    ctx.moveTo(step, 0);
    ctx.lineTo(step, canvas.height);
    ctx.stroke();
  }

  for (let step = 0; step <= canvas.height; step += 10) {
    ctx.beginPath();
    ctx.moveTo(0, step);
    ctx.lineTo(canvas.width, step);
    ctx.stroke();
  }

  ctx.strokeStyle = 'rgba(125, 245, 220, 0.24)';
  ctx.beginPath();
  ctx.moveTo(0, 560);
  ctx.lineTo(canvas.width, 560);
  ctx.stroke();

  dom('stageShell').classList.toggle('is-playing', STATE.isPlaying);
}

function renderMeta() {
  const manifestLabelElement = dom('currentGameLabel');
  const roleLabelElement = dom('currentRoleLabel');
  const frameMetaElement = dom('frameMeta');
  const panelTitleElement = dom('panelTitle');
  const libraryMetaElement = dom('libraryMeta');
  const activeActionLabelElement = dom('activeActionLabel');
  const baseActionLabelElement = dom('baseActionLabel');
  const playbackModeLabelElement = dom('playbackModeLabel');
  const resolutionMetaElement = dom('resolutionMeta');

  if (STATE.manifest === null) {
    manifestLabelElement.textContent = '等待游戏';
  } else {
    manifestLabelElement.textContent = STATE.manifest.label;
  }

  const role = getCurrentRole();
  const form = getCurrentForm();
  const action = getCurrentAction();
  const baseAction = getBaseAction();
  const frameRef = getCurrentFrameRef();
  const rawFrame = getCurrentRawFrame();
  const diagnostics = STATE.renderDiagnostics;

  if (role === null || form === null || action === null || frameRef === null) {
    roleLabelElement.textContent = '等待角色';
    frameMetaElement.textContent = '尚未载入动作';
    panelTitleElement.textContent = '角色帧动画预览';
    libraryMetaElement.textContent = '游戏 → 角色 → 形态';
    activeActionLabelElement.textContent = '-';
    baseActionLabelElement.textContent = '-';
    playbackModeLabelElement.textContent = '-';
    resolutionMetaElement.textContent = '尚未建立动作关系';
    setSelectionSummary('等待人物动作数据');
    return;
  }

  roleLabelElement.textContent = `${role.label} · ${form.label}`;
  frameMetaElement.textContent = `第 ${STATE.previewFrame + 1} / ${action.frames.length} 帧 · ${frameRef.durationMs} ms · ${getFrameRenderType(
    rawFrame
  )} · 64x64${diagnostics.clippedPixels > 0 ? ` 裁切 ${diagnostics.clippedPixels}` : ' 完整可见'}`;
  panelTitleElement.textContent = `${role.label} · ${action.label}`;
  libraryMetaElement.textContent = `${STATE.manifest.label} / ${role.label} / ${form.label}`;
  activeActionLabelElement.textContent = action.label;
  baseActionLabelElement.textContent = baseAction === null ? '-' : baseAction.label;
  playbackModeLabelElement.textContent = getPlaybackModeLabel();

  if (STATE.lastResolution === null) {
    resolutionMetaElement.textContent = '基础态待命';
  } else {
    resolutionMetaElement.textContent = `${STATE.lastResolution.intentLabel} · ${STATE.lastResolution.policy}`;
  }

  setSelectionSummary(`${STATE.manifest.label} / ${role.label} / ${form.label} / ${action.label}`);
}

function renderSummary() {
  const form = getCurrentForm();
  const defaultBaseAction = getActionById(form, STATE.defaultBaseActionId);
  const baseAction = getBaseAction();
  const currentAction = getCurrentAction();

  dom('defaultBaseActionValue').textContent = defaultBaseAction === null ? '-' : defaultBaseAction.label;
  dom('currentBaseActionValue').textContent = baseAction === null ? '-' : baseAction.label;
  dom('currentResolvedActionValue').textContent = currentAction === null ? '-' : currentAction.label;

  if (STATE.lastResolution === null) {
    dom('lastTriggerValue').textContent = '未触发';
    dom('resolutionPolicyValue').textContent = '基础态待命';
    dom('returnTargetValue').textContent = baseAction === null ? '-' : baseAction.label;
    return;
  }

  dom('lastTriggerValue').textContent = STATE.lastResolution.intentLabel;
  dom('resolutionPolicyValue').textContent = STATE.lastResolution.policy;
  dom('returnTargetValue').textContent = STATE.lastResolution.returnLabel;
}

function renderDetails() {
  const action = getCurrentAction();
  const frameRef = getCurrentFrameRef();
  const rawFrame = getCurrentRawFrame();
  const frame = getCurrentResolvedFrame();

  if (action === null || frameRef === null || rawFrame === null || frame === null) {
    dom('actionLoopValue').textContent = '-';
    dom('actionFrameCountValue').textContent = '-';
    dom('frameRefValue').textContent = '-';
    dom('frameRenderTypeValue').textContent = '-';
    dom('frameDurationValue').textContent = '-';
    dom('sourceAssetValue').textContent = '-';
    dom('sourceBoxValue').textContent = '-';
    dom('trimBoxValue').textContent = '-';
    dom('frameSizeValue').textContent = '-';
    dom('originValue').textContent = '-';
    dom('pixelCountValue').textContent = '-';
    dom('clipCountValue').textContent = '-';
    return;
  }

  dom('actionLoopValue').textContent = action.loop === true ? '是' : '否';
  dom('actionFrameCountValue').textContent = String(action.frames.length);
  dom('frameRefValue').textContent = frameRef.ref;
  dom('frameRenderTypeValue').textContent = getFrameRenderType(rawFrame);
  dom('frameDurationValue').textContent = `${frameRef.durationMs} ms`;
  dom('sourceAssetValue').textContent =
    typeof frame.sourceAsset === 'string' && frame.sourceAsset.length > 0 ? frame.sourceAsset : '-';
  dom('sourceBoxValue').textContent = formatBox(frame.sourceBox);
  dom('trimBoxValue').textContent = formatBox(frame.trimBox);
  dom('frameSizeValue').textContent = formatSize(frame.size);
  dom('originValue').textContent = formatOrigin(frame.origin);
  dom('pixelCountValue').textContent = typeof frame.pixelCount === 'number' ? String(frame.pixelCount) : '-';
  if (STATE.renderDiagnostics.clippedPixels > 0) {
    dom('clipCountValue').textContent = `${STATE.renderDiagnostics.clippedPixels} px · X ${STATE.renderDiagnostics.overflowX} / Y ${STATE.renderDiagnostics.overflowY}`;
  } else {
    dom('clipCountValue').textContent = '0 px';
  }
}

function appendEmptyState(container, message, className) {
  const block = document.createElement('div');
  block.className = className;
  block.textContent = message;
  container.append(block);
}

function renderSemanticButtons() {
  const semanticGroups = getCurrentSemanticGroups();
  renderBaseButtons(semanticGroups.base);
  renderTriggerButtons('jumpActionButtons', semanticGroups.jump, 'jump');
  renderTriggerButtons('attackActionButtons', semanticGroups.attack, 'attack');
  renderTriggerButtons('specialActionButtons', semanticGroups.special, 'special');
  renderFormButtons();
}

function renderBaseButtons(intents) {
  const container = dom('baseActionButtons');
  container.innerHTML = '';

  if (intents.length === 0) {
    appendEmptyState(container, '当前形态没有可用基础态。', 'semantic-empty');
    return;
  }

  for (let index = 0; index < intents.length; index += 1) {
    const intent = intents[index];
    const action = getBaseIntentButtonAction(intent);
    if (action === null) {
      continue;
    }

    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'semantic-btn';
    button.dataset.tone = 'base';
    button.dataset.active = action.id === STATE.baseActionId ? 'true' : 'false';
    button.dataset.current = action.id === STATE.activeActionId && STATE.transientPlayback === null ? 'true' : 'false';
    button.textContent = intent.label;
    button.addEventListener('click', () => {
      const wasPlaying = STATE.isPlaying;
      activateBaseAction(action.id, wasPlaying);
    });
    container.append(button);
  }
}

function renderTriggerButtons(containerId, intents, group) {
  const container = dom(containerId);
  container.innerHTML = '';

  if (intents.length === 0) {
    appendEmptyState(container, '当前形态没有这一类动作。', 'semantic-empty');
    return;
  }

  for (let index = 0; index < intents.length; index += 1) {
    const intent = intents[index];
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'semantic-btn';
    button.dataset.tone = 'trigger';
    button.dataset.active =
      STATE.lastResolution !== null &&
      STATE.lastResolution.group === group &&
      STATE.lastResolution.intentId === intent.intentId
        ? 'true'
        : 'false';
    button.dataset.current =
      STATE.transientPlayback !== null &&
      STATE.transientPlayback.group === group &&
      STATE.transientPlayback.intentId === intent.intentId
        ? 'true'
        : 'false';
    button.textContent = intent.label;
    button.addEventListener('click', () => {
      triggerIntent(group, intent.intentId);
    });
    container.append(button);
  }
}

function renderFormButtons() {
  const container = dom('formActionButtons');
  container.innerHTML = '';

  const role = getCurrentRole();
  if (role === null || !Array.isArray(role.forms) || role.forms.length === 0) {
    appendEmptyState(container, '当前角色没有可用形态。', 'semantic-empty');
    return;
  }

  for (let index = 0; index < role.forms.length; index += 1) {
    const form = role.forms[index];
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'semantic-btn';
    button.dataset.tone = 'form';
    button.dataset.active = form.id === STATE.selectedFormId ? 'true' : 'false';
    button.dataset.current = 'false';
    button.textContent = form.label;
    button.addEventListener('click', () => {
      if (form.id === STATE.selectedFormId) {
        return;
      }
      const wasPlaying = STATE.isPlaying;
      applyFormSelection(form.id, wasPlaying, '切 form 后落到默认基础态');
    });
    container.append(button);
  }
}

function renderFrameStrip() {
  const container = dom('frameStrip');
  const sequenceMeta = dom('sequenceMeta');
  container.innerHTML = '';

  const action = getCurrentAction();
  if (action === null || !Array.isArray(action.frames) || action.frames.length === 0) {
    sequenceMeta.textContent = '等待动作帧';
    appendEmptyState(container, '载入动作后，这里会显示帧序列条。', 'strip-empty');
    return;
  }

  sequenceMeta.textContent = `共 ${action.frames.length} 帧 · 点击定位`;

  for (let index = 0; index < action.frames.length; index += 1) {
    const frameRef = action.frames[index];
    const rawFrame = getRawFrameById(frameRef.ref);

    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'frame-chip';
    button.dataset.active = index === STATE.previewFrame ? 'true' : 'false';
    button.title = frameRef.ref;

    const title = document.createElement('strong');
    title.textContent = `帧 ${index + 1}`;

    const duration = document.createElement('span');
    duration.textContent = `${frameRef.durationMs} ms`;

    const renderType = document.createElement('em');
    renderType.textContent = getFrameRenderType(rawFrame);

    button.append(title, duration, renderType);
    button.addEventListener('click', () => {
      stopPlayback(false);
      STATE.previewFrame = index;
      renderAll();
    });
    container.append(button);
  }
}

function renderAll() {
  renderSelectionControls();
  renderSemanticButtons();
  renderTransport();
  renderCanvas();
  renderMeta();
  renderSummary();
  renderDetails();
  renderFrameStrip();
}

function bindEvents() {
  dom('gameSelect').addEventListener('change', async event => {
    await loadGame(event.target.value);
  });

  dom('roleSelect').addEventListener('change', event => {
    const wasPlaying = STATE.isPlaying;
    applyRoleSelection(event.target.value, wasPlaying);
  });

  dom('formSelect').addEventListener('change', event => {
    const wasPlaying = STATE.isPlaying;
    applyFormSelection(event.target.value, wasPlaying, '切 form 后落到默认基础态');
  });

  dom('previewPrevBtn').addEventListener('click', () => {
    stepPreviewFrame(-1);
  });

  dom('previewPlayBtn').addEventListener('click', () => {
    togglePlayback();
  });

  dom('previewNextBtn').addEventListener('click', () => {
    stepPreviewFrame(1);
  });

  dom('previewFrameRange').addEventListener('input', event => {
    const action = getCurrentAction();
    if (action === null || !Array.isArray(action.frames)) {
      return;
    }

    stopPlayback(false);
    STATE.previewFrame = parseInt(event.target.value, 10);
    renderAll();
  });

  window.addEventListener('keydown', event => {
    if (event.metaKey || event.ctrlKey || event.altKey) {
      return;
    }

    const target = event.target;
    if (target instanceof HTMLInputElement || target instanceof HTMLSelectElement || target instanceof HTMLTextAreaElement) {
      return;
    }

    if (event.code === 'ArrowLeft') {
      event.preventDefault();
      stepPreviewFrame(-1);
      return;
    }

    if (event.code === 'ArrowRight') {
      event.preventDefault();
      stepPreviewFrame(1);
      return;
    }

    if (event.code === 'Space') {
      event.preventDefault();
      togglePlayback();
      return;
    }

    if (event.code === 'KeyQ') {
      event.preventDefault();
      cycleForm(-1);
      return;
    }

    if (event.code === 'KeyE') {
      event.preventDefault();
      cycleForm(1);
      return;
    }

    if (event.code === 'Digit1') {
      event.preventDefault();
      triggerShortcutBase(['idle', 'pose', 'walk', 'roll', 'ride', 'swim']);
      return;
    }

    if (event.code === 'Digit2') {
      event.preventDefault();
      triggerShortcutBase(['run', 'walk', 'ride', 'swim', 'inch-walk', 'climb']);
      return;
    }

    if (event.code === 'Digit3') {
      event.preventDefault();
      triggerShortcutIntent('jump', ['jump', 'spinning-jump', 'jump-knee']);
      return;
    }

    if (event.code === 'Digit4') {
      event.preventDefault();
      triggerShortcutIntent('attack', ['attack', 'shoot', 'punch', 'kick', 'throw', 'blow']);
    }
  });
}

function triggerShortcutBase(preferredIntentIds) {
  const semanticGroups = getCurrentSemanticGroups();
  const intents = semanticGroups.base;
  if (!Array.isArray(intents) || intents.length === 0) {
    return;
  }

  for (let preferredIndex = 0; preferredIndex < preferredIntentIds.length; preferredIndex += 1) {
    for (let intentIndex = 0; intentIndex < intents.length; intentIndex += 1) {
      const intent = intents[intentIndex];
      if (intent.intentId !== preferredIntentIds[preferredIndex]) {
        continue;
      }
      const action = getBaseIntentButtonAction(intent);
      if (action === null) {
        return;
      }
      const wasPlaying = STATE.isPlaying;
      activateBaseAction(action.id, wasPlaying);
      return;
    }
  }

  const fallbackAction = getBaseIntentButtonAction(intents[0]);
  if (fallbackAction === null) {
    return;
  }
  const wasPlaying = STATE.isPlaying;
  activateBaseAction(fallbackAction.id, wasPlaying);
}

function triggerShortcutIntent(group, preferredIntentIds) {
  const semanticGroups = getCurrentSemanticGroups();
  const intents = semanticGroups[group];
  if (!Array.isArray(intents) || intents.length === 0) {
    return;
  }

  for (let preferredIndex = 0; preferredIndex < preferredIntentIds.length; preferredIndex += 1) {
    for (let intentIndex = 0; intentIndex < intents.length; intentIndex += 1) {
      if (intents[intentIndex].intentId !== preferredIntentIds[preferredIndex]) {
        continue;
      }
      triggerIntent(group, intents[intentIndex].intentId);
      return;
    }
  }

  triggerIntent(group, intents[0].intentId);
}

async function loadGameDefinitions() {
  setPageStatus('正在加载游戏目录…', 'loading');
  const indexData = await fetchJson(GAME_INDEX_PATH);

  if (!Array.isArray(indexData.games) || indexData.games.length === 0) {
    throw new Error('游戏目录为空，无法初始化页面');
  }

  STATE.gameDefinitions = [];
  for (let index = 0; index < indexData.games.length; index += 1) {
    const game = indexData.games[index];
    STATE.gameDefinitions.push({
      id: game.gameId,
      label: game.label,
    });
  }

  let nextGameId = STATE.gameDefinitions[0].id;
  const rememberedGameId = window.localStorage.getItem(LAST_SELECTED_GAME_KEY);
  if (typeof rememberedGameId === 'string' && rememberedGameId.length > 0) {
    for (let index = 0; index < STATE.gameDefinitions.length; index += 1) {
      if (STATE.gameDefinitions[index].id === rememberedGameId) {
        nextGameId = rememberedGameId;
        break;
      }
    }
  }

  STATE.selectedGameId = nextGameId;
  renderSelectionControls();
  await loadGame(nextGameId);
}

async function loadGame(gameId) {
  stopPlayback(false);
  STATE.selectedGameId = gameId;
  STATE.manifest = null;
  STATE.characterAnimations = null;
  STATE.resolvedFrameCache = new Map();
  STATE.renderDiagnostics = {
    clippedPixels: 0,
    overflowX: 0,
    overflowY: 0,
  };
  resetSelectionState();
  window.localStorage.setItem(LAST_SELECTED_GAME_KEY, gameId);

  setPageStatus('正在读取游戏 manifest…', 'loading');
  setNotice('正在读取该游戏的人物动作入口…', 'info');
  renderAll();

  try {
    const manifest = await fetchJson(getManifestPath(gameId));
    STATE.manifest = manifest;

    if (typeof manifest.characterAnimationsPath !== 'string' || manifest.characterAnimationsPath.length === 0) {
      setPageStatus('该游戏尚未整理人物动作数据', 'warning');
      setNotice('该游戏尚未整理人物动作数据，且不会回退旧流程。', 'warning');
      renderAll();
      return;
    }

    setPageStatus('正在加载人物动作数据…', 'loading');
    const characterAnimations = await fetchJson(`${getGameRoot(gameId)}${manifest.characterAnimationsPath}`);
    STATE.characterAnimations = characterAnimations;
    STATE.resolvedFrameCache = new Map();
    initializeCurrentSelection();
    setPageStatus('人物动作数据已加载', 'ready');

    const roleCount = Array.isArray(characterAnimations.roles) ? characterAnimations.roles.length : 0;
    setNotice(`已加载 ${roleCount} 个角色入口，语义按钮已切到状态机模式。`, 'ready');
    renderAll();
  } catch (error) {
    STATE.manifest = null;
    STATE.characterAnimations = null;
    STATE.resolvedFrameCache = new Map();
    resetSelectionState();
    setPageStatus('人物动作数据加载失败', 'error');
    setNotice(error.message, 'error');
    renderAll();
  }
}

async function init() {
  bindEvents();
  renderAll();

  try {
    await loadGameDefinitions();
  } catch (error) {
    setPageStatus('初始化失败', 'error');
    setNotice(error.message, 'error');
  }
}

init();
