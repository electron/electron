// The native binding is typed as the public `Electron.ClipboardItem`, but its
// constructor is the real validator: per the W3C spec it accepts a `Buffer`
// or a string for any MIME type (a string is UTF-8 encoded into the payload
// bytes), and a `{ title, url }` object for the bookmark format, throwing
// `TypeError` for anything else.
// This JS class is a thin, W3C-aligned async facade over it — it accepts
// `Blob`s and `Promise`s (whose values can only be read asynchronously),
// resolves each payload to the `Buffer`/string/object native expects just
// before the write, and lets native own type validation. That mirrors the way
// `net.WebSocket` resolves a `Blob` before calling `WebSocketWrapper::Send`.
// Read-side items from `clipboard.read()` are wrapped so `getType()` hands
// back a `Blob`.
const NativeClipboardItem = process._linkedBinding('electron_browser_clipboard_item') as unknown as new (
  items: Record<string, Buffer | string | Electron.ClipboardBookmark>
) => Electron.ClipboardItem;

const BOOKMARK_MIME = 'electron application/bookmark';

// Sentinel passed to the constructor to build a read-side item wrapping a
// native reader, without going through the public validation path.
const kReadSide = Symbol('readSide');

// Internal hooks used by `clipboard.ts` to bridge to the native binding.
export const kToNative = Symbol('toNative');
export const kFromNative = Symbol('fromNative');

// A payload may be provided directly, or as a `Promise` that resolves to a
// `Blob` or string; the bookmark format takes a `ClipboardBookmark` object.
type ResolvedPayload = string | Blob | Electron.ClipboardBookmark;
type Payload = ResolvedPayload | Promise<string | Blob>;

export class ClipboardItem {
  #data: Record<string, Payload> | null = null;
  #native: Electron.ClipboardItem | null = null;

  constructor(items: Record<string, Payload>, native?: Electron.ClipboardItem) {
    // Read-side construction (internal): wrap a native reader.
    if ((items as unknown) === kReadSide) {
      this.#native = native!;
      return;
    }

    if (typeof items !== 'object' || items === null || Array.isArray(items)) {
      throw new TypeError(
        "Failed to construct 'ClipboardItem': 1 argument required — an object mapping MIME types to payloads."
      );
    }
    if (Object.keys(items).length === 0) {
      throw new TypeError("Failed to construct 'ClipboardItem': at least one MIME type is required.");
    }

    // Per-payload type validation is deferred to the native constructor (see
    // `[kToNative]`), which is also where a `Promise` payload's resolved value
    // is checked — the synchronous constructor can't know what a `Promise`
    // will resolve to.
    this.#data = { ...items };
  }

  // Wrap a native read-side item so its payloads surface as `Blob`s.
  static [kFromNative](native: Electron.ClipboardItem): ClipboardItem {
    return new ClipboardItem(kReadSide as unknown as Record<string, Payload>, native);
  }

  get types(): string[] {
    if (this.#native) return [...this.#native.types];
    return Object.keys(this.#data!);
  }

  async getType(type: string): Promise<Blob | Electron.ClipboardBookmark> {
    // Read-side: the native reader returns a `Buffer` (or a bookmark
    // object); wrap the bytes in a `Blob` tagged with the MIME type.
    if (this.#native) {
      const payload = await this.#native.getType(type);
      if (type === BOOKMARK_MIME) return payload as Electron.ClipboardBookmark;
      return new Blob([payload as Blob], { type });
    }

    // Constructed: resolve from the stored payload map, awaiting a `Promise`
    // payload if present.
    if (!Object.prototype.hasOwnProperty.call(this.#data!, type)) {
      throw new Error(`The type '${type}' was not found in the ClipboardItem`);
    }
    const value = await this.#data![type];
    if (type === BOOKMARK_MIME) return value as Electron.ClipboardBookmark;
    if (value instanceof Blob) return value;
    return new Blob([Buffer.from(value as string, 'utf8')], { type });
  }

  // Resolve every `Blob` payload to a `Buffer` and build the native,
  // synchronous `ClipboardItem` that `clipboard.write()` commits.
  async [kToNative](): Promise<Electron.ClipboardItem> {
    if (this.#native) {
      throw new TypeError(
        'clipboard.write cannot accept a ClipboardItem returned from ' +
          'clipboard.read() — construct a new ClipboardItem to write.'
      );
    }

    // Resolve each payload — awaiting a `Promise`, then reading a `Blob`'s
    // bytes — to the `Buffer`/string/object the native constructor accepts,
    // and let native validate the type against the MIME. Native decodes a
    // `Buffer` as UTF-8 for text MIME types, so no MIME-specific handling is
    // needed here.
    const resolved: Record<string, Buffer | string | Electron.ClipboardBookmark> = {};
    for (const mime of Object.keys(this.#data!)) {
      const value = await this.#data![mime];
      resolved[mime] = value instanceof Blob ? Buffer.from(await value.arrayBuffer()) : value;
    }
    return new NativeClipboardItem(resolved);
  }
}

export default ClipboardItem;
