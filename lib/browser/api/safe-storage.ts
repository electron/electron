import * as deprecate from '@electron/internal/common/deprecate';

const { safeStorage } = process._linkedBinding('electron_browser_safe_storage');

const isEncryptionAvailableDeprecated = deprecate.warnOnce(
  'safeStorage.isEncryptionAvailable',
  'safeStorage.isAsyncEncryptionAvailable'
);
const isEncryptionAvailable = safeStorage.isEncryptionAvailable.bind(safeStorage);
safeStorage.isEncryptionAvailable = () => {
  isEncryptionAvailableDeprecated();
  return isEncryptionAvailable();
};

const encryptStringDeprecated = deprecate.warnOnce('safeStorage.encryptString', 'safeStorage.encryptStringAsync');
const encryptString = safeStorage.encryptString.bind(safeStorage);
safeStorage.encryptString = (plainText: string) => {
  encryptStringDeprecated();
  return encryptString(plainText);
};

const decryptStringDeprecated = deprecate.warnOnce('safeStorage.decryptString', 'safeStorage.decryptStringAsync');
const decryptString = safeStorage.decryptString.bind(safeStorage);
safeStorage.decryptString = (encrypted: Buffer) => {
  decryptStringDeprecated();
  return decryptString(encrypted);
};

export default safeStorage;
