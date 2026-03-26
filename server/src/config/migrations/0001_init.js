module.exports.up = async ({ executeSqlFile, connection, initSqlPath }) => {
  await executeSqlFile(connection, initSqlPath);
};
