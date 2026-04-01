import * as deprecate from '@electron/internal/common/deprecate';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';

import { EventEmitter } from 'events';

const { mainFrame, WebFrame } = process._linkedBinding('electron_renderer_web_frame');

// @ts-expect-error - WebFrame types are cursed. It's an instanced class, but
// the docs define it as a static module.
// TODO(smaddock): Fix web-frame.md to define it as an instance class.
const WebFramePrototype: Electron.WebFrame = WebFrame.prototype;
const framesWithWorldCallback = new WeakSet<Electron.WebFrame>();

Object.setPrototypeOf(WebFramePrototype, EventEmitter.prototype);

// Register the native callback only when a wrapper actually listens for
// isolated world creation. This avoids wiring every frame-returning accessor.
const ensureWorldCallback = (frame: Electron.WebFrame) => {
  if (framesWithWorldCallback.has(frame)) return;

  frame._setIsolatedWorldCreationCallback((worldId: number) => {
    frame.emit('isolated-world-created', worldId);
  });
  framesWithWorldCallback.add(frame);
};

for (const method of ['on', 'addListener', 'once', 'prependListener', 'prependOnceListener'] as const) {
  const original = WebFramePrototype[method] as Function;
  if (!original) continue;
  (WebFramePrototype as any)[method] = function (this: Electron.WebFrame, eventName: string | symbol, ...args: any[]) {
    if (eventName === 'isolated-world-created') {
      ensureWorldCallback(this);
    }
    return original.call(this, eventName, ...args);
  };
}

const routingIdDeprecated = deprecate.warnOnce('webFrame.routingId', 'webFrame.frameToken');
Object.defineProperty(WebFramePrototype, 'routingId', {
  configurable: true,
  get: function (this: Electron.WebFrame) {
    routingIdDeprecated();
    return ipcRendererUtils.invokeSync<number>(
      IPC_MESSAGES.BROWSER_GET_FRAME_ROUTING_ID_SYNC,
      this.frameToken
    );
  }
});

const findFrameByRoutingIdDeprecated = deprecate.warnOnce('webFrame.findFrameByRoutingId', 'webFrame.findFrameByToken');
WebFramePrototype.findFrameByRoutingId = function (
  routingId: number
): Electron.WebFrame | null {
  findFrameByRoutingIdDeprecated();
  const frameToken = ipcRendererUtils.invokeSync<string | undefined>(
    IPC_MESSAGES.BROWSER_GET_FRAME_TOKEN_SYNC,
    routingId
  );
  return frameToken ? this.findFrameByToken(frameToken) : null;
};

export default mainFrame;
