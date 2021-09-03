import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import deprecate from '@electron/internal/common/api/deprecate';
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

let warned = process.noDeprecation;
export async function getSources (options: Electron.SourcesOptions) {
  if (!warned) {
    deprecate.log('The use of \'desktopCapturer.getSources\' in the renderer process is deprecated and will be removed. See https://www.electronjs.org/docs/breaking-changes#removed-desktopcapturergetsources-in-the-renderer for more details.');
    warned = true;
  }
  return await ipcRendererInternal.invoke(IPC_MESSAGES.DESKTOP_CAPTURER_GET_SOURCES, options, getCurrentStack());
}
