const NOTIFICATION_REMINDER_STORAGE_KEY = "notification_reminder_list";

function cloneReminder(reminder) {
  return JSON.parse(JSON.stringify(reminder));
}

function sortReminderList(reminders) {
  return reminders.slice().sort((left, right) => {
    const leftTime = left.hour * 60 + left.minute;
    const rightTime = right.hour * 60 + right.minute;

    if (leftTime !== rightTime) {
      return leftTime - rightTime;
    }

    return left.reminderId.localeCompare(right.reminderId);
  });
}

export function createReminderId() {
  const timestamp = Date.now().toString(36);
  const randomValue = Math.floor(Math.random() * 1679616)
    .toString(36)
    .padStart(4, "0");
  return `reminder_${timestamp}_${randomValue}`;
}

export function createNotificationReminder(contentType) {
  if (
    contentType !== "text" &&
    contentType !== "static" &&
    contentType !== "animation"
  ) {
    throw new Error("不支持的提示类型");
  }

  const reminderId = createReminderId();

  if (contentType === "text") {
    return {
      reminderId,
      reminderName: "文本提醒",
      contentType: "text",
      hour: 9,
      minute: 0,
      repeatMode: "once",
      text: "记得喝水",
      icon: "drink",
      accentColor: "#64C8FF",
      textTemplate: "headline",
      staticTemplate: "badge",
      animationTemplate: "pulse",
    };
  }

  if (contentType === "static") {
    return {
      reminderId,
      reminderName: "静态提醒",
      contentType: "static",
      hour: 18,
      minute: 0,
      repeatMode: "daily",
      text: "下班休息",
      icon: "moon",
      accentColor: "#7DDF8A",
      textTemplate: "headline",
      staticTemplate: "poster",
      animationTemplate: "pulse",
    };
  }

  return {
    reminderId,
    reminderName: "动图提醒",
    contentType: "animation",
    hour: 20,
    minute: 0,
    repeatMode: "yearly",
    text: "生日快乐",
    icon: "fireworks",
    accentColor: "#FFB020",
    textTemplate: "headline",
    staticTemplate: "badge",
    animationTemplate: "fireworks",
  };
}

export function getNotificationReminderList() {
  const storedValue = uni.getStorageSync(NOTIFICATION_REMINDER_STORAGE_KEY);
  if (!Array.isArray(storedValue)) {
    return [];
  }

  return sortReminderList(storedValue.map((item) => cloneReminder(item)));
}

export function getNotificationReminder(reminderId) {
  const reminders = getNotificationReminderList();
  const target = reminders.find((item) => item.reminderId === reminderId);
  if (!target) {
    return null;
  }

  return cloneReminder(target);
}

export function saveNotificationReminderList(reminders) {
  const safeList = sortReminderList(reminders.map((item) => cloneReminder(item)));
  uni.setStorageSync(NOTIFICATION_REMINDER_STORAGE_KEY, safeList);
}

export function upsertNotificationReminder(reminder) {
  const reminders = getNotificationReminderList();
  const nextList = [];
  let updated = false;

  reminders.forEach((item) => {
    if (item.reminderId === reminder.reminderId) {
      nextList.push(cloneReminder(reminder));
      updated = true;
      return;
    }

    nextList.push(cloneReminder(item));
  });

  if (!updated) {
    nextList.push(cloneReminder(reminder));
  }

  saveNotificationReminderList(nextList);
}

export function removeNotificationReminder(reminderId) {
  const reminders = getNotificationReminderList();
  const nextList = reminders.filter((item) => item.reminderId !== reminderId);
  saveNotificationReminderList(nextList);
}

export function duplicateNotificationReminder(reminderId) {
  const reminder = getNotificationReminder(reminderId);
  if (!reminder) {
    return null;
  }

  const duplicatedReminder = cloneReminder(reminder);
  duplicatedReminder.reminderId = createReminderId();
  duplicatedReminder.reminderName = `${reminder.reminderName}副本`;
  upsertNotificationReminder(duplicatedReminder);
  return duplicatedReminder;
}
