import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import { deserialize } from '@electron/internal/common/type-utils';

const { hasSwitch } = process.electronBinding('command_line');

const enableStacks = hasSwitch('enable-api-filtering-logging');

function getCurrentStack () {
  const target = {};
  if (enableStacks) {
    Error.captureStackTrace(target, getCurrentStack);
  }
  return (target as any).stack;
}

export async function getSources (options: Electron.SourcesOptions) {
  return deserialize(await ipcRendererInternal.invoke('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', options, getCurrentStack()));
}

export function getMediaSourceIdForWebContents (webContentsId: number) {
  return ipcRendererInternal.invoke<string>('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_MEDIA_SOURCE_ID_FOR_WEB_CONTENTS', webContentsId, getCurrentStack());
}
