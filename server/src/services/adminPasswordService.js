const bcrypt = require('bcryptjs');
const crypto = require('crypto');

const BCRYPT_ROUNDS = 10;
const LEGACY_ADMIN_PASSWORD_SALT = 'glowxel';

const isBcryptHash = (hash) => /^\$2[aby]\$\d{2}\$/.test(hash);

const hashLegacyAdminPassword = (password) => {
  return crypto
    .createHash('sha256')
    .update(password + LEGACY_ADMIN_PASSWORD_SALT)
    .digest('hex');
};

const hashAdminPassword = async (password) => {
  return bcrypt.hash(password, BCRYPT_ROUNDS);
};

const verifyAdminPassword = async (password, storedHash) => {
  if (!storedHash) {
    return { matched: false, needsUpgrade: false };
  }

  if (isBcryptHash(storedHash)) {
    const matched = await bcrypt.compare(password, storedHash);
    return { matched, needsUpgrade: false };
  }

  const matched = hashLegacyAdminPassword(password) === storedHash;
  return { matched, needsUpgrade: matched };
};

module.exports = {
  hashAdminPassword,
  hashLegacyAdminPassword,
  verifyAdminPassword
};
