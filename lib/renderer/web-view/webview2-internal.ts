import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import { invokeSync } from '@electron/internal/renderer/ipc-renderer-internal-utils';
const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');
export interface Receipt {
  webContentsId: number;
  attachmentId: string;
}
export function attach(iframe: HTMLIFrameElement, slot: string, id: number): Receipt {
  const frame = mainFrame._findFrameByWindow(iframe.contentWindow!);
  if (!frame) throw new Error('Invalid webview2 iframe');
  return invokeSync(IPC_MESSAGES.WEBVIEW2_ATTACH, slot, id, frame.frameToken);
}
export function release(slot: string): Receipt | null {
  return invokeSync(IPC_MESSAGES.WEBVIEW2_RELEASE, slot);
}
export function subscribe(callback: (slot: string, receipt: Receipt) => void) {
  const listener = (_event: Electron.IpcRendererEvent, slot: string, id: number, attachmentId: string) =>
    callback(slot, { webContentsId: id, attachmentId });
  ipcRendererInternal.on(IPC_MESSAGES.WEBVIEW2_DETACHED, listener);
  return () => ipcRendererInternal.removeListener(IPC_MESSAGES.WEBVIEW2_DETACHED, listener);
}
