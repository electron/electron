import * as path from 'path';

// Derive a default Linux .desktop filename from an app's human-readable name if one is not set
// by the developer. The name must be compatible with the XDG Desktop Entry specification:
// https://specifications.freedesktop.org/desktop-entry/latest/file-naming.html. Fall back to
// the executable name if it is not possible to generate a suitable name.
//
// The name is lowercased and hyphenated so it is more likely to match the .desktop file created
// when the app is packaged, which is important for desktop integration and matching icons on Wayland.
export function defaultDesktopName(name: string | undefined): string {
  // NFKD decomposes accented chars into base characters + combining marks.
  // \p{M} drops the combining marks.
  const slug =
    name &&
    name
      .normalize('NFKD')
      .replace(/\p{M}/gu, '')
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/^-+|-+$/g, '');
  return slug ? `${slug}.desktop` : `${path.basename(process.execPath)}.desktop`;
}
