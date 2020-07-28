// TODO(deepak1556): Deprecate and remove standalone netLog module,
// it is now a property of session module.
import { app, session } from 'electron/main';

const startLogging: typeof session.defaultSession.netLog.startLogging = async (path, options) => {
  if (!app.isReady()) return;
  return session.defaultSession.netLog.startLogging(path, options);
};

const stopLogging: typeof session.defaultSession.netLog.stopLogging = async () => {
  if (!app.isReady()) return;
  return session.defaultSession.netLog.stopLogging();
};

export default {
  startLogging,
  stopLogging,
  get currentlyLogging (): boolean {
    if (!app.isReady()) return false;
    return session.defaultSession.netLog.currentlyLogging;
  }
};
