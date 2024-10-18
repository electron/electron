import { fetchWithSession } from '@electron/internal/browser/api/net-fetch';

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

export default {
  fromPartition,
  fromPath,
  get defaultSession () {
    return fromPartition('');
  }
};
