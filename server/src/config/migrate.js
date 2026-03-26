const { runMigrations } = require('./migrationService');

runMigrations({
  logger: (message) => {
    console.log(message);
  }
})
  .then(({ executedVersions }) => {
    if (!executedVersions.length) {
      console.log('无待执行迁移');
      process.exit(0);
    }

    console.log(`迁移完成: ${executedVersions.join(', ')}`);
    process.exit(0);
  })
  .catch((error) => {
    console.error('迁移失败:', error);
    process.exit(1);
  });
