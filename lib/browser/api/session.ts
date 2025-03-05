import { fetchWithSession } from '@electron/internal/browser/api/net-fetch';
import { addIpcDispatchListeners } from '@electron/internal/browser/ipc-dispatch';
import * as deprecate from '@electron/internal/common/deprecate';

import { net } from 'electron/main';

const { fromPartition, fromPath, Session } = process._linkedBinding('electron_browser_session');
const { isDisplayMediaSystemPickerAvailable } = process._linkedBinding('electron_browser_desktop_capturer');

// Fake video window that activates the native system picker
// This is used to get around the need for a screen/window
// id in Chrome's desktopCapturer.
let fakeVideoWindowId = -1;
// See content/public/browser/desktop_media_id.h
const kMacOsNativePickerId = -4;
const systemPickerVideoSource = Object.create(null);
Object.defineProperty(systemPickerVideoSource, 'id', {
  get () {
    return `window:${kMacOsNativePickerId}:${fakeVideoWindowId--}`;
  }
});
systemPickerVideoSource.name = '';
Object.freeze(systemPickerVideoSource);

Session.prototype._init = function () {
  addIpcDispatchListeners(this);

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

Session.prototype.setDisplayMediaRequestHandler = function (handler, opts) {
  if (!handler) return this._setDisplayMediaRequestHandler(handler, opts);

  this._setDisplayMediaRequestHandler(async (req, callback) => {
    if (opts && opts.useSystemPicker && isDisplayMediaSystemPickerAvailable()) {
      return callback({ video: systemPickerVideoSource });
    }

    return handler(req, callback);
  }, opts);
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
  preloads.map(filePath => ({
    type: 'frame',
    filePath,
    _deprecated: true
  }) as Electron.PreloadScriptRegistration).forEach(script => {
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

export default {
  fromPartition,
  fromPath,
  get defaultSession () {
    return fromPartition('');
  }
};
