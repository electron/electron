import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

const { hasSwitch } = process._linkedBinding('electron_common_command_line');

const enableStacks = hasSwitch('enable-api-filtering-logging');

function getCurrentStack () {
  const target = {};
  if (enableStacks) {
    Error.captureStackTrace(target, getCurrentStack);
  }
  return (target as any).stack;
}

export async function getSources (options: Electron.SourcesOptions) {
  return await ipcRendererInternal.invoke(IPC_MESSAGES.DESKTOP_CAPTURER_GET_SOURCES, options, getCurrentStack());
}
