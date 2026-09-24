import { IpcMainImpl } from '@electron/internal/browser/ipc-main-impl';

const { WebFrameMain, fromId, fromFrameToken } = process._linkedBinding('electron_browser_web_frame_main');

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
