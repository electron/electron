// TODO(deepak1556): Deprecate and remove standalone netLog module,
// it is now a property of session module.
import { app, session } from 'electron';

export const startLogging: typeof session.defaultSession.netLog.startLogging = async (path, options) => {
  if (!app.isReady()) return;
  return session.defaultSession.netLog.startLogging(path, options);
};

export const stopLogging: typeof session.defaultSession.netLog.stopLogging = async () => {
  if (!app.isReady()) return;
  return session.defaultSession.netLog.stopLogging();
};

Object.defineProperty(module.exports, 'currentlyLogging', {
  get: () => {
    if (!app.isReady()) return false;
    return session.defaultSession.netLog.currentlyLogging;
  },
  enumerable: true,
  configurable: true
});
