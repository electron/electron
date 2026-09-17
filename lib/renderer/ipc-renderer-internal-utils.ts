import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

export function invokeSync<T>(command: string, ...args: any[]): T {
  const [error, result] = ipcRendererInternal.sendSync(command, ...args);

  if (error) {
    throw error;
  } else {
    return result;
  }
}
