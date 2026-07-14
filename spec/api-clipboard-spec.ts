import { nativeImage } from 'electron/common';
import { ClipboardItem, clipboard } from 'electron/main';

import { expect } from 'chai';

import { Buffer } from 'node:buffer';
import * as path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

import { ifdescribe, ifit } from './lib/spec-helpers';

const BOOKMARK_MIME = 'electron application/bookmark';
const FIND_TEXT_MIME = 'electron application/findtext';
const URI_LIST_MIME = 'text/uri-list';

// Helper that mirrors the old `clipboard.read(format)`/`clipboard.readBuffer`
// API on top of the new `clipboard.read()` shape so the tests stay focused
// on the behavior of the new MIME-typed API rather than on plumbing.
async function readType(mime: string): Promise<Buffer | undefined> {
  const items = await clipboard.read();
  for (const item of items) {
    if (item.types.includes(mime)) {
      // `readType` is only used for Blob-valued MIME types; the bookmark
      // MIME type (which resolves to an object) has its own helper.
      // `getType` resolves to a `Blob`; the tests reason about raw bytes,
      // so read them back into a Buffer here.
      const blob = (await item.getType(mime)) as Blob;
      return Buffer.from(await blob.arrayBuffer());
    }
  }
  return undefined;
}

// Wrap raw bytes in a `Blob` — the payload type the W3C `ClipboardItem`
// constructor accepts for non-text MIME types.
function toBlob(data: Buffer | string): Blob {
  return new Blob([data]);
}

// `getType('electron application/bookmark')` resolves to a `{ title, url }`
// object rather than a Buffer, so it needs its own typed read helper.
async function readBookmark(): Promise<Electron.ClipboardBookmark | undefined> {
  const items = await clipboard.read();
  for (const item of items) {
    if (item.types.includes(BOOKMARK_MIME)) {
      return (await item.getType(BOOKMARK_MIME)) as Electron.ClipboardBookmark;
    }
  }
  return undefined;
}

// `text/uri-list` maps to the OS "copied files" format and reads back as an
// RFC 2483 `file://` URI list. Decode it into absolute paths so tests can
// compare against the paths that were written (independent of any URI
// normalization the platform clipboard applies).
async function readUriListPaths(): Promise<string[] | undefined> {
  const buffer = await readType(URI_LIST_MIME);
  if (!buffer) return undefined;
  return buffer
    .toString('utf8')
    .split(/\r?\n/)
    .filter(Boolean)
    .map((uri) => fileURLToPath(uri));
}

describe('clipboard module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');

  describe('reading images via clipboard.read()', () => {
    it('round-trips a NativeImage through the image/* MIME type', async () => {
      const p = path.join(fixtures, 'assets', 'logo.png');
      const i = nativeImage.createFromPath(p);
      await clipboard.write([new ClipboardItem({ 'image/png': toBlob(i.toPNG()) })]);

      const buffer = await readType('image/png');
      expect(buffer).to.be.an.instanceOf(Buffer);
      const readImage = nativeImage.createFromBuffer(buffer!);
      expect(readImage.toDataURL()).to.equal(i.toDataURL());
    });

    it('exposes no image type when only text is on the clipboard', async () => {
      clipboard.clear();
      await clipboard.writeText('Not an Image');
      const buffer = await readType('image/png');
      expect(buffer).to.be.undefined();
    });
  });

  describe('clipboard.readText()', () => {
    it('returns unicode string correctly', async () => {
      const text = '千江有水千江月，万里无云万里天';
      await clipboard.writeText(text);
      expect(await clipboard.readText()).to.equal(text);
    });

    it('returns a Promise', () => {
      expect(clipboard.readText()).to.be.an.instanceOf(Promise);
    });

    it('writeText returns a Promise', () => {
      expect(clipboard.writeText('test')).to.be.an.instanceOf(Promise);
    });
  });

  describe('clipboard.has()', () => {
    it('resolves with true when the clipboard contains the format', async () => {
      await clipboard.writeText('has-format');
      expect(await clipboard.has('text/plain')).to.be.true();
    });

    it('resolves with false when the clipboard does not contain the format', async () => {
      clipboard.clear();
      expect(await clipboard.has('text/html')).to.be.false();
    });

    it('resolves with true for a user-defined custom MIME type', async () => {
      const mime = 'web text/plain+electron-test';
      await clipboard.write([new ClipboardItem({ [mime]: toBlob(Buffer.from('x', 'utf8')) })]);
      expect(await clipboard.has(mime)).to.be.true();
    });

    it('resolves true with electron application/osclipboard;format="X" for a raw format X on write and read', async () => {
      const rawFormat = 'public/utf8-plain-text';
      const mime = `electron application/osclipboard;format="${rawFormat}"`;
      await clipboard.write([new ClipboardItem({ [mime]: toBlob(Buffer.from('x', 'utf8')) })]);
      expect(await clipboard.has(mime)).to.be.true();
    });

    it('returns a Promise (Chromium clipboard format enumeration is async)', () => {
      expect(clipboard.has('text/plain')).to.be.an.instanceOf(Promise);
    });
  });

  describe('reading HTML via clipboard.read()', () => {
    it('returns markup correctly via the text/html MIME type', async () => {
      let text = '<string>Hi</string>';
      // CL: https://chromium-review.googlesource.com/c/chromium/src/+/5187335
      if (process.platform === 'darwin') {
        text = "<meta charset='utf-8'><string>Hi</string>";
      }
      await clipboard.write([new ClipboardItem({ 'text/html': '<string>Hi</string>' })]);
      const buffer = await readType('text/html');
      expect(buffer).to.be.an.instanceOf(Buffer);
      expect(buffer!.toString('utf8')).to.equal(text);
    });
  });

  describe('reading RTF via clipboard.read()', () => {
    it('returns rtf text correctly via the text/rtf MIME type', async () => {
      const rtf = '{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\nThis is some {\\b bold} text.\\par\n}';
      await clipboard.write([new ClipboardItem({ 'text/rtf': rtf })]);
      const buffer = await readType('text/rtf');
      expect(buffer).to.be.an.instanceOf(Buffer);
      expect(buffer!.toString('utf8')).to.equal(rtf);
    });
  });

  ifdescribe(process.platform !== 'linux')('reading bookmarks via clipboard.read()', () => {
    it('returns title and url via the electron application/bookmark MIME type', async () => {
      await clipboard.write([
        new ClipboardItem({
          [BOOKMARK_MIME]: {
            title: 'a title',
            url: 'https://electronjs.org/'
          }
        })
      ]);

      const bookmark = await readBookmark();
      expect(bookmark).to.be.an('object');
      if (process.platform !== 'win32') {
        expect(bookmark!.title).to.equal('a title');
      }
      expect(bookmark!.url).to.equal('https://electronjs.org/');

      await clipboard.writeText('no bookmark');
      const empty = await readBookmark();
      if (empty) {
        expect(empty).to.deep.equal({ title: '', url: '' });
      }
    });
  });

  describe('clipboard.read()', () => {
    ifit(process.platform !== 'linux')('does not crash when reading various custom clipboard types', async () => {
      const rawFormat = process.platform === 'darwin' ? 'NSFilenamesPboardType' : 'FileNameW';
      // Raw platform formats with no standard MIME mapping are surfaced
      // under the `electron application/osclipboard;format="..."` MIME.
      const type = `electron application/osclipboard;format="${rawFormat}"`;

      const items = await clipboard.read();
      expect(items).to.be.an('array');
      for (const item of items) {
        if (item.types.includes(type)) {
          await item.getType(type);
        }
      }
    });

    it('can read data written with the electron application/osclipboard MIME type', async () => {
      const testText = 'Testing read';
      const buffer = Buffer.from(testText, 'utf8');
      // A raw OS clipboard format round-trips through the same
      // `electron application/osclipboard;format="X"` MIME on write and read.
      const mime = 'electron application/osclipboard;format="public/utf8-plain-text"';
      await clipboard.write([new ClipboardItem({ [mime]: toBlob(buffer) })]);
      const read = await readType(mime);
      expect(read).to.be.an.instanceOf(Buffer);
      expect(read!.toString('utf8')).to.equal(testText);
    });
  });

  describe('clipboard.write()', () => {
    it('writes mixed data via a ClipboardItem with multiple MIME types', async () => {
      const text = 'https://electronjs.org/';
      const rtf = '{\\rtf1\\utf8 text}';
      const p = path.join(fixtures, 'assets', 'logo.png');
      const i = nativeImage.createFromPath(p);
      let markup = '<b>Hi</b>';
      // CL: https://chromium-review.googlesource.com/c/chromium/src/+/5187335
      if (process.platform === 'darwin') {
        markup = "<meta charset='utf-8'><b>Hi</b>";
      }
      const bookmark = { title: 'a title', url: text };
      await clipboard.write([
        new ClipboardItem({
          'text/plain': text,
          'text/html': '<b>Hi</b>',
          'text/rtf': rtf,
          'image/png': toBlob(i.toPNG()),
          [BOOKMARK_MIME]: bookmark
        })
      ]);

      expect(await clipboard.readText()).to.equal(text);
      const htmlBuffer = await readType('text/html');
      expect(htmlBuffer!.toString('utf8')).to.equal(markup);
      const rtfBuffer = await readType('text/rtf');
      expect(rtfBuffer!.toString('utf8')).to.equal(rtf);
      const imageBuffer = await readType('image/png');
      const readImage = nativeImage.createFromBuffer(imageBuffer!);
      expect(readImage.toDataURL()).to.equal(i.toDataURL());

      if (process.platform !== 'linux') {
        const readBookmarkValue = await readBookmark();
        expect(readBookmarkValue).to.be.an('object');
        if (process.platform !== 'win32') {
          expect(readBookmarkValue).to.deep.equal(bookmark);
        } else {
          expect(readBookmarkValue!.url).to.equal(bookmark.url);
        }
      }
    });

    it('returns a Promise', () => {
      expect(clipboard.write([new ClipboardItem({ 'text/plain': 'x' })])).to.be.an.instanceOf(Promise);
    });
  });

  describe('Blob payloads (W3C symmetry)', () => {
    it('getType() resolves to a Blob tagged with the MIME type', async () => {
      const payload = Buffer.from('blob symmetry', 'utf8');
      const mime = 'web application/electron-blob-test';
      await clipboard.write([new ClipboardItem({ [mime]: new Blob([payload], { type: mime }) })]);

      const items = await clipboard.read();
      const item = items.find((i) => i.types.includes(mime));
      expect(item, 'expected the custom MIME on the clipboard').to.exist();

      const blob = await item!.getType(mime);
      expect(blob).to.be.an.instanceOf(Blob);
      expect((blob as Blob).type).to.equal(mime);
      expect(Buffer.from(await (blob as Blob).arrayBuffer()).equals(payload)).to.equal(true);
    });

    it('round-trips a Blob read back into clipboard.write()', async () => {
      const original = Buffer.from([0x10, 0x20, 0x30, 0x40]);
      const mime = 'web application/electron-blob-roundtrip';
      await clipboard.write([new ClipboardItem({ [mime]: new Blob([original]) })]);

      const [first] = await clipboard.read();
      const blob = (await first.getType(mime)) as Blob;

      // A Blob produced by `getType()` can be fed straight back into a new
      // `ClipboardItem` — no manual Buffer conversion required.
      await clipboard.write([new ClipboardItem({ [mime]: blob })]);
      const read = await readType(mime);
      expect(read).to.be.an.instanceOf(Buffer);
      expect(read!.equals(original)).to.equal(true);
    });

    it('rejects an invalid payload at write() time', async () => {
      // Per-payload type validation is deferred to native, so a bad payload
      // (here a number) is not caught by the constructor but rejects the
      // `clipboard.write()` promise with a TypeError.
      const item = new ClipboardItem({ 'application/x-electron-invalid': 42 as any });
      let error: Error | undefined;
      try {
        await clipboard.write([item]);
      } catch (err) {
        error = err as Error;
      }
      expect(error).to.be.an.instanceOf(TypeError);
    });

    it('accepts a Blob payload for a text MIME type', async () => {
      // Text MIME types accept a string or a Blob; a text Blob is decoded
      // as UTF-8 before being committed.
      const text = 'blob-backed text';
      await clipboard.write([new ClipboardItem({ 'text/plain': new Blob([Buffer.from(text, 'utf8')]) })]);
      expect(await clipboard.readText()).to.equal(text);
    });

    it('accepts a string payload for a non-text MIME type (UTF-8 encoded)', async () => {
      // Per the W3C spec a string is a valid payload for any MIME type, not
      // just the text ones — it is UTF-8 encoded into the payload bytes.
      const mime = 'web application/electron-string-binary';
      await clipboard.write([new ClipboardItem({ [mime]: 'plain string bytes' })]);
      const read = await readType(mime);
      expect(read).to.be.an.instanceOf(Buffer);
      expect(read!.toString('utf8')).to.equal('plain string bytes');
    });

    it('accepts a Promise that resolves to a Blob', async () => {
      const payload = Buffer.from('promised bytes', 'utf8');
      const mime = 'web application/electron-promise-blob';
      await clipboard.write([new ClipboardItem({ [mime]: Promise.resolve(new Blob([payload])) })]);
      const read = await readType(mime);
      expect(read).to.be.an.instanceOf(Buffer);
      expect(read!.equals(payload)).to.equal(true);
    });

    it('accepts a Promise that resolves to a string for a text MIME type', async () => {
      const text = 'promised text';
      await clipboard.write([new ClipboardItem({ 'text/plain': Promise.resolve(text) })]);
      expect(await clipboard.readText()).to.equal(text);
    });

    it('rejects a non-string, non-Blob payload at write() time', async () => {
      // Payload must be a string or a Blob (or a Buffer once a
      // Blob has been resolved by the facade). When a payload that is neither —
      // here a number — `clipboard.write()` rejects with a TypeError.
      const item = new ClipboardItem({ 'text/plain': 42 as any });
      let error: Error | undefined;
      try {
        await clipboard.write([item]);
      } catch (err) {
        error = err as Error;
      }
      expect(error).to.be.an.instanceOf(TypeError);
    });
  });

  ifdescribe(process.platform === 'darwin')('reading/writing the find pasteboard via clipboard.read()', () => {
    it('reads and writes text via the electron application/findtext MIME type', async () => {
      await clipboard.write([new ClipboardItem({ [FIND_TEXT_MIME]: 'find this' })]);
      const buffer = await readType(FIND_TEXT_MIME);
      expect(buffer).to.be.an.instanceOf(Buffer);
      expect(buffer!.toString('utf8')).to.equal('find this');
    });
  });

  describe('arbitrary and "web " custom MIME types', () => {
    it('round-trips an arbitrary custom MIME type', async () => {
      const rawFormat = 'application/x-electron-test';
      const payload = Buffer.from([0x01, 0x02, 0x03, 0xff, 0x00, 0x42]);
      await clipboard.write([new ClipboardItem({ [rawFormat]: toBlob(payload) })]);
      // An arbitrary MIME with no standard mapping is written under its
      // bare platform name and surfaced on read under the
      // `electron application/osclipboard;format="..."` MIME.
      const read = await readType(`electron application/osclipboard;format="${rawFormat}"`);
      expect(read).to.be.an.instanceOf(Buffer);
      expect(payload.equals(read!)).to.equal(true);
    });

    it('preserves bytes for a W3C "web " custom format', async () => {
      const mime = 'web text/plain+electron-test';
      const payload = Buffer.from('arbitrary custom payload', 'utf8');
      await clipboard.write([new ClipboardItem({ [mime]: toBlob(payload) })]);
      const read = await readType(mime);
      expect(read).to.be.an.instanceOf(Buffer);
      expect(payload.equals(read!)).to.equal(true);
    });

    it('does not funnel a "web " custom format through the image helpers', async () => {
      // `web image/png+foo` is a *custom* user-defined format that just
      // happens to mention image/png in its name. Its bytes must round-trip
      // verbatim rather than being decoded as an image.
      const mime = 'web image/png+electron-test';
      const payload = Buffer.from([0xde, 0xad, 0xbe, 0xef, 0x00, 0x01, 0x02]);
      await clipboard.write([new ClipboardItem({ [mime]: toBlob(payload) })]);
      const read = await readType(mime);
      expect(read).to.be.an.instanceOf(Buffer);
      expect(payload.equals(read!)).to.equal(true);
    });

    it('exposes custom MIME types in the ClipboardItem types array', async () => {
      const mime = 'web application/electron-types-test';
      await clipboard.write([new ClipboardItem({ [mime]: toBlob(Buffer.from('hi', 'utf8')) })]);
      const items = await clipboard.read();
      const types = items.flatMap((item) => Array.from(item.types));
      expect(types).to.include(mime);
    });

    it('surfaces a "web " custom format under its web MIME name', async () => {
      const mime = 'web application/electron-plumbing-test';
      await clipboard.write([new ClipboardItem({ [mime]: toBlob(Buffer.from('payload', 'utf8')) })]);
      const items = await clipboard.read();
      const types = items.flatMap((item) => Array.from(item.types));
      expect(types).to.include(mime);
    });
  });

  describe('reading/writing raw OS clipboard formats', () => {
    it('round-trips a Blob for the specified format', async () => {
      const payload = Buffer.from('writeBuffer', 'utf8');
      const rawFormat = 'public/utf8-plain-text';
      const mime = `electron application/osclipboard;format="${rawFormat}"`;
      await clipboard.write([new ClipboardItem({ [mime]: toBlob(payload) })]);
      // The raw OS format round-trips through the same osclipboard MIME.
      const read = await readType(mime);
      expect(read).to.be.an.instanceOf(Buffer);
      expect(payload.equals(read!)).to.equal(true);
    });

    it('rejects clipboard.write() with a TypeError when a non-Blob is specified as the payload', async () => {
      // Per-payload type validation is deferred to native, so an invalid
      // payload (here a number for a binary MIME type) is not caught by the
      // constructor; `clipboard.write()` rejects with a TypeError instead.
      const item = new ClipboardItem({
        'electron application/osclipboard;format="public/utf8-plain-text"': 42 as any
      });
      let error: Error | undefined;
      try {
        await clipboard.write([item]);
      } catch (err) {
        error = err as Error;
      }
      expect(error).to.be.an.instanceOf(TypeError);
    });
  });

  // `text/uri-list` maps to the operating system's native "copied files"
  // format (CF_HDROP on Windows, NSFilenamesPboardType on macOS,
  // text/uri-list on Linux) rather than a generic text payload, so files can
  // be pasted into and read from native applications. The payload is an
  // RFC 2483 `file://` URI list.
  describe('reading/writing files via the text/uri-list MIME type', () => {
    const fileA = path.join(fixtures, 'assets', 'logo.png');
    const fileB = __filename;

    it('round-trips a single file path', async () => {
      await clipboard.write([new ClipboardItem({ [URI_LIST_MIME]: pathToFileURL(fileA).href })]);
      expect(await readUriListPaths()).to.deep.equal([fileA]);
    });

    it('round-trips multiple file paths', async () => {
      const uriList = [pathToFileURL(fileA).href, pathToFileURL(fileB).href].join('\r\n');
      await clipboard.write([new ClipboardItem({ [URI_LIST_MIME]: uriList })]);
      expect(await readUriListPaths()).to.deep.equal([fileA, fileB]);
    });

    it('getType() resolves to a Blob of file:// URIs', async () => {
      await clipboard.write([new ClipboardItem({ [URI_LIST_MIME]: pathToFileURL(fileA).href })]);
      const items = await clipboard.read();
      const item = items.find((i) => i.types.includes(URI_LIST_MIME));
      expect(item, 'expected text/uri-list on the clipboard').to.exist();
      const blob = (await item!.getType(URI_LIST_MIME)) as Blob;
      expect(blob).to.be.an.instanceOf(Blob);
      const paths = (await blob.text())
        .split(/\r?\n/)
        .filter(Boolean)
        .map((uri) => fileURLToPath(uri));
      expect(paths).to.deep.equal([fileA]);
    });

    it('exposes text/uri-list in the ClipboardItem types array', async () => {
      await clipboard.write([new ClipboardItem({ [URI_LIST_MIME]: pathToFileURL(fileA).href })]);
      const items = await clipboard.read();
      const types = items.flatMap((item) => Array.from(item.types));
      expect(types).to.include(URI_LIST_MIME);
    });

    it('reports availability via clipboard.has()', async () => {
      await clipboard.write([new ClipboardItem({ [URI_LIST_MIME]: pathToFileURL(fileA).href })]);
      expect(await clipboard.has(URI_LIST_MIME)).to.be.true();
    });
  });

  // The primary selection clipboard is a Linux-only concept exposed under
  // `clipboard.selection`. It mirrors the top-level `clipboard` interface
  // but operates against a *separate* system buffer, so writes to the
  // selection clipboard must not affect the regular system clipboard and
  // vice-versa.
  ifdescribe(process.platform === 'linux')('clipboard.selection (Linux)', () => {
    it('exposes a selection namespace mirroring the top-level surface', () => {
      expect(clipboard.selection).to.be.an('object');
      expect(clipboard.selection!.read).to.be.a('function');
      expect(clipboard.selection!.write).to.be.a('function');
      expect(clipboard.selection!.readText).to.be.a('function');
      expect(clipboard.selection!.writeText).to.be.a('function');
      expect(clipboard.selection!.has).to.be.a('function');
      expect(clipboard.selection!.clear).to.be.a('function');
    });

    it('round-trips text via selection.writeText/selection.readText', async () => {
      const text = 'selection clipboard text 千江';
      await clipboard.selection!.writeText(text);
      expect(await clipboard.selection!.readText()).to.equal(text);
    });

    it('round-trips arbitrary MIME types via selection.write/selection.read', async () => {
      const mime = 'text/html';
      const html = '<b>selection</b>';
      await clipboard.selection!.write([new ClipboardItem({ [mime]: html })]);
      const items = await clipboard.selection!.read();
      const item = items.find((i) => i.types.includes(mime));
      expect(item, 'expected text/html on selection clipboard').to.exist();
      const blob = (await item!.getType(mime)) as Blob;
      expect(Buffer.from(await blob.arrayBuffer()).toString('utf8')).to.equal(html);
    });

    it('is independent of the system clipboard', async () => {
      await clipboard.writeText('system clipboard payload');
      await clipboard.selection!.writeText('selection clipboard payload');
      expect(await clipboard.readText()).to.equal('system clipboard payload');
      expect(await clipboard.selection!.readText()).to.equal('selection clipboard payload');
    });

    it('selection.has reports format availability', async () => {
      await clipboard.selection!.writeText('has-format');
      expect(await clipboard.selection!.has('text/plain')).to.be.true();
    });

    it('selection.clear empties the selection clipboard without touching the system clipboard', async () => {
      await clipboard.writeText('keep-me');
      await clipboard.selection!.writeText('drop-me');
      clipboard.selection!.clear();
      expect(await clipboard.selection!.readText()).to.equal('');
      expect(await clipboard.readText()).to.equal('keep-me');
    });

    it('all selection read/write methods return Promises', () => {
      expect(clipboard.selection!.readText()).to.be.an.instanceOf(Promise);
      expect(clipboard.selection!.writeText('p')).to.be.an.instanceOf(Promise);
      expect(clipboard.selection!.read()).to.be.an.instanceOf(Promise);
      expect(clipboard.selection!.write([new ClipboardItem({ 'text/plain': 'p' })])).to.be.an.instanceOf(Promise);
      expect(clipboard.selection!.has('text/plain')).to.be.an.instanceOf(Promise);
    });
  });

  ifit(process.platform !== 'linux')('does not expose clipboard.selection off Linux', () => {
    expect(clipboard.selection).to.be.undefined();
  });
});
