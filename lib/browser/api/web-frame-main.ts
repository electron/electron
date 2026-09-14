import { IpcMainImpl } from '@electron/internal/browser/ipc-main-impl';
import { MessagePortMain } from '@electron/internal/browser/message-port-main';
import { printToPDF } from '@electron/internal/browser/print-to-pdf';

const { WebFrameMain, fromId, fromFrameToken } = process._linkedBinding('electron_browser_web_frame_main');
const apiBridgeBinding = process._linkedBinding('electron_browser_api_bridge');

Object.defineProperty(WebFrameMain.prototype, 'ipc', {
  get() {
    const ipc = new IpcMainImpl();
    Object.defineProperty(this, 'ipc', { value: ipc });
    return ipc;
  }
});

Object.defineProperty(WebFrameMain.prototype, 'apiBridge', {
  get() {
    const frame = this as Electron.WebFrameMain;
    const apiBridge: Electron.ApiBridgeFrameMain = {
      pass: (name, api, options) => apiBridgeBinding.pass(frame, name, api, options, false),
      passToIsolatedWorld: (name, api, options) => apiBridgeBinding.pass(frame, name, api, options, true),
      revoke: (name) => apiBridgeBinding.revoke(frame, name, false),
      revokeFromIsolatedWorld: (name) => apiBridgeBinding.revoke(frame, name, true)
    };
    Object.defineProperty(this, 'apiBridge', { value: apiBridge });
    return apiBridge;
  }
});

WebFrameMain.prototype.send = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new TypeError('Missing required channel argument');
  }

  try {
    return this._send(false /* internal */, channel, args);
  } catch (e) {
    console.error('Error sending from webFrameMain: ', e);
  }
};

WebFrameMain.prototype._sendInternal = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new TypeError('Missing required channel argument');
  }

  try {
    return this._send(true /* internal */, channel, args);
  } catch (e) {
    console.error('Error sending from webFrameMain: ', e);
  }
};

WebFrameMain.prototype.printToPDF = async function (options) {
  return printToPDF(this, options);
};

WebFrameMain.prototype.postMessage = function (...args) {
  if (Array.isArray(args[2])) {
    args[2] = args[2].map((o) => (o instanceof MessagePortMain ? o._internalPort : o));
  }
  this._postMessage(...args);
};

export default {
  fromId,
  fromFrameToken
};
