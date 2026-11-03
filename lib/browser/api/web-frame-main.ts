import { IpcMainImpl } from '@electron/internal/browser/ipc-main-impl';
import * as deprecate from '@electron/internal/common/deprecate';

import { webContents } from 'electron/main';

const { WebFrameMain, fromId, fromFrameToken } = process._linkedBinding('electron_browser_web_frame_main');

const preconnectDeprecated = deprecate.warnOnce('session preconnect event', 'WebFrameMain preconnect event');

WebFrameMain.prototype._init = function () {
  this.on('preconnect', (event, preconnectUrl, allowCredentials) => {
    if (this.isDestroyed()) return;
    const session = webContents.fromFrame(this)?.session;
    if (session && session.listenerCount('preconnect') !== 0) {
      preconnectDeprecated();
      session.emit('preconnect', event, preconnectUrl, allowCredentials, this);
    }
  });
};

Object.defineProperty(WebFrameMain.prototype, 'ipc', {
  get() {
    const ipc = new IpcMainImpl();
    Object.defineProperty(this, 'ipc', { value: ipc });
    return ipc;
  }
});

export default {
  fromId,
  fromFrameToken
};
