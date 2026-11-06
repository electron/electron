import { fetchWithSession } from '@electron/internal/browser/api/net-fetch';
import * as deprecate from '@electron/internal/common/deprecate';

import { net, type UtilityProcess } from 'electron/main';

const { fromPartition, fromPath, Session } = process._linkedBinding('electron_browser_session');

Session.prototype._init = function () {
  if (this.extensions) {
    const rerouteExtensionEvent = (eventName: string) => {
      const warn = deprecate.warnOnce(`${eventName} event`, `session.extensions ${eventName} event`);
      this.extensions.on(eventName as any, (...args: any[]) => {
        if (this.listenerCount(eventName) !== 0) {
          warn();
          this.emit(eventName, ...args);
        }
      });
    };
    rerouteExtensionEvent('extension-loaded');
    rerouteExtensionEvent('extension-unloaded');
    rerouteExtensionEvent('extension-ready');
  }
};

Session.prototype.fetch = function (input: RequestInfo, init?: RequestInit) {
  return fetchWithSession(input, init, this, net.request);
};

const getPreloadsDeprecated = deprecate.warnOnce('session.getPreloads', 'session.getPreloadScripts');
Session.prototype.getPreloads = function () {
  getPreloadsDeprecated();
  return this.getPreloadScripts()
    .filter((script) => script.type === 'frame')
    .map((script) => script.filePath);
};

const setPreloadsDeprecated = deprecate.warnOnce('session.setPreloads', 'session.registerPreloadScript');
Session.prototype.setPreloads = function (preloads) {
  setPreloadsDeprecated();
  this.getPreloadScripts()
    .filter((script) => script.type === 'frame')
    .forEach((script) => {
      this.unregisterPreloadScript(script.id);
    });
  preloads
    .map(
      (filePath) =>
        ({
          type: 'frame',
          filePath,
          _deprecated: true
        }) as Electron.PreloadScriptRegistration
    )
    .forEach((script) => {
      this.registerPreloadScript(script);
    });
};

Session.prototype.getAllExtensions = deprecate.moveAPI(
  function (this: Electron.Session) {
    return this.extensions.getAllExtensions();
  },
  'session.getAllExtensions',
  'session.extensions.getAllExtensions'
);
Session.prototype.getExtension = deprecate.moveAPI(
  function (this: Electron.Session, extensionId) {
    return this.extensions.getExtension(extensionId);
  },
  'session.getExtension',
  'session.extensions.getExtension'
);
Session.prototype.loadExtension = deprecate.moveAPI(
  function (this: Electron.Session, path, options) {
    return this.extensions.loadExtension(path, options);
  },
  'session.loadExtension',
  'session.extensions.loadExtension'
);
Session.prototype.removeExtension = deprecate.moveAPI(
  function (this: Electron.Session, extensionId) {
    return this.extensions.removeExtension(extensionId);
  },
  'session.removeExtension',
  'session.extensions.removeExtension'
);

Session.prototype.registerLocalAIHandler = function (handler: UtilityProcess | null) {
  // We need to unwrap the userland `ForkUtilityProcess` object and get the underlying
  // `ElectronInternal.UtilityProcessWrapper` before we call the C++ function
  return this._registerLocalAIHandler(handler !== null ? (handler as any)._unwrapHandle() : null);
};

export default {
  fromPartition,
  fromPath,
  get defaultSession() {
    return fromPartition('');
  }
};
