import { fetchWithSession } from '@electron/internal/browser/api/net-fetch';
import { net } from 'electron/main';
const { fromPartition, fromPath, Session } = process._linkedBinding('electron_browser_session');

Session.prototype.fetch = function (input: RequestInfo, init?: RequestInit) {
  return fetchWithSession(input, init, this, net.request);
};

Session.prototype.setPermissionCheckHandler = function (handler) {
  if (!handler) return this._setPermissionCheckHandler(handler);

  return this._setPermissionCheckHandler((...args) => {
    if (handler(...args)) return 'granted';
    return 'denied';
  });
};

Session.prototype.setPermissionRequestHandler = function (handler) {
  if (!handler) return this._setPermissionRequestHandler(handler);

  return this._setPermissionRequestHandler((wc, perm, cb: any, d, eo) => {
    return handler(wc, perm, (granted) => cb(granted ? 'granted' : 'denied'), d, eo);
  });
};

Session.prototype.setPermissionHandlers = function (handlers) {
  if (!handlers) {
    this._setPermissionCheckHandler(null);
    this._setPermissionRequestHandler(null);
    return;
  }

  this._setPermissionCheckHandler((_, permission, effectiveOrigin, details) => {
    return handlers.isGranted(permission, effectiveOrigin, details).status;
  });

  this._setPermissionRequestHandler((_, permission, callback, details, effectiveOrigin) => {
    Promise.resolve(handlers.onRequest(permission, effectiveOrigin, details))
      .then((result) => callback(result.status === 'granted'))
      .catch((err) => {
        this.emit('error', err);
        callback(false);
      });
  });
};

export default {
  fromPartition,
  fromPath,
  get defaultSession () {
    return fromPartition('');
  }
};
