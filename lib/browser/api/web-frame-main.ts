import { MessagePortMain } from '@electron/internal/browser/message-port-main';
import { IpcMainImpl } from '@electron/internal/browser/ipc-main-impl';

const { WebFrameMain, fromId } = process._linkedBinding('electron_browser_web_frame_main');
const DEFAULT_TIMEOUT = 30000;

Object.defineProperty(WebFrameMain.prototype, 'ipc', {
  get () {
    const ipc = new IpcMainImpl();
    Object.defineProperty(this, 'ipc', { value: ipc });
    return ipc;
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

WebFrameMain.prototype.postMessage = function (...args) {
  if (Array.isArray(args[2])) {
    args[2] = args[2].map(o => o instanceof MessagePortMain ? o._internalPort : o);
  }
  this._postMessage(...args);
};

WebFrameMain.prototype.invoke = async function (...args) {
  // Check if the first argument is an options object
  const isOptionsProvided = typeof args[0] === 'object';
  const { maxTimeoutMs = DEFAULT_TIMEOUT } = isOptionsProvided ? args.shift() : {};
  const channel = args.shift();

  if (typeof channel !== 'string') {
    throw new TypeError('Missing required channel argument');
  }

  if (typeof maxTimeoutMs !== 'number') {
    throw new TypeError('Invalid timeout argument');
  }

  return new Promise((resolve, reject) => {
    const timeoutId = setTimeout(() => {
      reject(new Error(`Timeout after ${maxTimeoutMs}ms waiting for IPC response on channel '${channel}'`));
    }, maxTimeoutMs);

    this._invoke(channel, args).then(({ error, result }) => {
      clearTimeout(timeoutId);
      if (error) {
        reject(new Error(`Error invoking remote method '${channel}': ${error}`));
      } else {
        resolve(result);
      }
    });
  });
};

export default {
  fromId
};
