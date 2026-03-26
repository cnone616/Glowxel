module.exports.up = async ({ connection }) => {
  const alterStatements = [
    "ALTER TABLE users ADD COLUMN role ENUM('user','admin') DEFAULT 'user' AFTER status",
    "ALTER TABLE users ADD COLUMN admin_username VARCHAR(50) NULL AFTER role",
    "ALTER TABLE users ADD COLUMN admin_password VARCHAR(128) NULL AFTER admin_username"
  ];

  for (const statement of alterStatements) {
    try {
      await connection.query(statement);
    } catch (error) {
      if (error.code !== 'ER_DUP_FIELDNAME') {
        throw error;
      }
    }
  }
};
