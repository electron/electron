import { getSourcesImpl, setSkipCursorImpl } from '@electron/internal/browser/desktop-capturer';

export async function getSources (options: Electron.SourcesOptions) {
  return getSourcesImpl(null, options);
}

export async function setSkipCursor (sourceId: string, skipCursor: boolean) {
  return setSkipCursorImpl(null, sourceId, skipCursor);
}
