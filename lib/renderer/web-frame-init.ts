import { webFrame, WebFrame } from 'electron/renderer';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

// All keys of WebFrame that extend Function
type WebFrameMethod = {
  [K in keyof WebFrame]:
    WebFrame[K] extends Function ? K : never
}

export const webFrameInit = () => {
  // Call webFrame method
  ipcRendererUtils.handle(IPC_MESSAGES.RENDERER_WEB_FRAME_METHOD, (
    event, method: keyof WebFrameMethod, ...args: any[]
  ) => {
    // The TypeScript compiler cannot handle the sheer number of
    // call signatures here and simply gives up. Incorrect invocations
    // will be caught by "keyof WebFrameMethod" though.
    return (webFrame[method] as any)(...args);
  });
};
