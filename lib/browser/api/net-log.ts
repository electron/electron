// TODO(deepak1556): Deprecate and remove standalone netLog module,
// it is now a property of session module.
import * as deprecate from '@electron/internal/common/deprecate';
import { app, session } from 'electron/main';

const startLogging: typeof session.defaultSession.netLog.startLogging = async (path, options) => {
  deprecate.warn('netLog.startLogging', 'session.defaultSession.netLog.startLogging');
  if (!app.isReady()) return;
  return session.defaultSession.netLog.startLogging(path, options);
};

const stopLogging: typeof session.defaultSession.netLog.stopLogging = async () => {
  deprecate.warn('netLog.stopLogging', 'session.defaultSession.netLog.stopLogging');
  if (!app.isReady()) return;
  return session.defaultSession.netLog.stopLogging();
};

export default {
  startLogging,
  stopLogging,
  get currentlyLogging(): boolean {
    deprecate.warn('netLog.currentlyLogging', 'session.defaultSession.netLog.currentlyLogging');
    if (!app.isReady()) return false;
    return session.defaultSession.netLog.currentlyLogging;
  }
};
