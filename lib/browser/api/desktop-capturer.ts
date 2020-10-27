import { getSourcesImpl } from '@electron/internal/browser/desktop-capturer';

export async function getSources (options: Electron.SourcesOptions) {
  return getSourcesImpl(null, options);
}
