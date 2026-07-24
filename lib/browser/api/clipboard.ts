import { ClipboardItem, kFromNative, kToNative } from '@electron/internal/browser/api/clipboard-item';

// The native binding is `Buffer`-based and synchronous, but it's typed as the
// public `Electron.Clipboard`. This module wraps its `read`/`write` so the
// public surface speaks the W3C `ClipboardItem` (Blob) shape: `write()`
// resolves each item's `Blob`s to `Buffer`s before the single native commit,
// and `read()` wraps native readers so `getType()` yields a `Blob`.
// Everything else passes straight through.
const binding = process._linkedBinding('electron_browser_clipboard');

function wrapClipboard(native: Electron.Clipboard) {
  const api: any = {
    clear: () => native.clear(),
    has: (format: string) => native.has(format),
    readText: () => native.readText(),
    writeText: (text: string) => native.writeText(text),
    read: async (): Promise<ClipboardItem[]> => {
      const items = await native.read();
      return items.map((item) => (ClipboardItem as any)[kFromNative](item));
    },
    write: async (items: ClipboardItem[]): Promise<void> => {
      if (!Array.isArray(items)) {
        throw new TypeError('clipboard.write expects an array of ClipboardItem');
      }
      // Resolve every item's Blob payloads to Buffers first, then commit
      // them all through a single synchronous native write so the entries
      // still land on the system clipboard atomically.
      const nativeItems = [];
      for (const item of items) {
        if (!(item instanceof ClipboardItem)) {
          throw new TypeError('clipboard.write expects an array of ClipboardItem');
        }
        nativeItems.push(await (item as any)[kToNative]());
      }
      return native.write(nativeItems);
    }
  };
  return api;
}

const clipboard: any = wrapClipboard(binding);
if (binding.selection) {
  clipboard.selection = wrapClipboard(binding.selection);
}

export default clipboard as Electron.Clipboard;
