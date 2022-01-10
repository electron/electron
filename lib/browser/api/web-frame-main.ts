import { MessagePortMain } from '@electron/internal/browser/message-port-main';

const { WebFrameMain, fromId } = process._linkedBinding('electron_browser_web_frame_main');

WebFrameMain.prototype.send = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument');
  }

  try {
    return this._send(false /* internal */, channel, args);
  } catch (e) {
    console.error('Error sending from webFrameMain: ', e);
  }
};

WebFrameMain.prototype._sendInternal = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument');
  }

  try {
    return this._send(true /* internal */, channel, args);
  } catch (e) {
    console.error('Error sending from webFrameMain: ', e);
  }
};

WebFrameMain.prototype.postMessage = function (...args) {
  if (Array.isArray(args[2])) {
    args[2] = args[2].map(o => o instanceof MessagePortMain ? o._internalPort : o);
  }
  this._postMessage(...args);
};

export default {
  fromId
};
