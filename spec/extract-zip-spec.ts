/**
 * Test for extract-zip large file extraction bug on Node.js >= 24.16.0 / 26.1.0
 *
 * Bug: extract-zip (yauzl@2.10.0 + fd-slicer@1.1.0) silently fails to extract
 * files from zip archives when using Node.js versions that enforce proper
 * Readable stream destroy() lifecycle semantics.
 *
 * Root cause: fd-slicer's ReadStream._read() sets `this.destroyed = true` before
 * calling `this.push(null)` at EOF. Node's Readable then calls the stream's
 * destroy() method to complete the lifecycle (emit 'close'). However, fd-slicer's
 * custom destroy() short-circuits when it sees `this.destroyed` is already true,
 * so 'close' is never emitted. This causes `stream.pipeline()` (used by
 * extract-zip) to never resolve, hanging the extraction after the first
 * uncompressed entry that triggers this path.
 *
 * The bug specifically affects STORED (uncompressed) zip entries because
 * compressed entries pass through a zlib inflate transform that has its own
 * proper destroy lifecycle. Since Electron's zip archives use STORED method
 * for the main binary, this causes the electron binary to never be extracted.
 *
 * References:
 * - https://github.com/electron/electron/issues/51619
 * - https://github.com/max-mapper/extract-zip/issues/154
 * - https://github.com/thejoshwolfe/yauzl/issues/169
 * - https://github.com/nodejs/node/issues/63487
 *
 * @see npm/install.js - extractFile() function that calls extract-zip
 */

import { expect } from 'chai';

import * as crypto from 'node:crypto';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

const extract: (zipPath: string, opts: { dir: string }) => Promise<void> = require('../npm/extract');

/**
 * Creates a zip file buffer containing entries stored without compression.
 *
 * This uses the STORED method (compression method 0) which is the same method
 * used for the electron binary in Electron's distribution zip. The bug in
 * fd-slicer only manifests with STORED entries because compressed entries go
 * through a zlib DecompressStream that has proper destroy() semantics.
 */
function createStoredZip(entries: Array<{ name: string; data: Buffer }>): Buffer {
  const localHeaders: Buffer[] = [];
  const centralHeaders: Buffer[] = [];
  let offset = 0;

  for (const entry of entries) {
    const nameBuffer = Buffer.from(entry.name, 'utf8');
    const crc = crc32(entry.data);

    // Local file header (30 bytes + name + data)
    const local = Buffer.alloc(30 + nameBuffer.length + entry.data.length);
    local.writeUInt32LE(0x04034b50, 0); // Local file header signature
    local.writeUInt16LE(20, 4); // Version needed to extract (2.0)
    local.writeUInt16LE(0, 6); // General purpose bit flag
    local.writeUInt16LE(0, 8); // Compression method: STORED
    local.writeUInt16LE(0, 10); // Last mod file time
    local.writeUInt16LE(0, 12); // Last mod file date
    local.writeUInt32LE(crc, 14); // CRC-32
    local.writeUInt32LE(entry.data.length, 18); // Compressed size
    local.writeUInt32LE(entry.data.length, 22); // Uncompressed size
    local.writeUInt16LE(nameBuffer.length, 26); // File name length
    local.writeUInt16LE(0, 28); // Extra field length
    nameBuffer.copy(local, 30);
    entry.data.copy(local, 30 + nameBuffer.length);
    localHeaders.push(local);

    // Central directory header (46 bytes + name)
    const central = Buffer.alloc(46 + nameBuffer.length);
    central.writeUInt32LE(0x02014b50, 0); // Central directory header signature
    central.writeUInt16LE(20, 4); // Version made by
    central.writeUInt16LE(20, 6); // Version needed to extract
    central.writeUInt16LE(0, 8); // General purpose bit flag
    central.writeUInt16LE(0, 10); // Compression method: STORED
    central.writeUInt16LE(0, 12); // Last mod file time
    central.writeUInt16LE(0, 14); // Last mod file date
    central.writeUInt32LE(crc, 16); // CRC-32
    central.writeUInt32LE(entry.data.length, 20); // Compressed size
    central.writeUInt32LE(entry.data.length, 24); // Uncompressed size
    central.writeUInt16LE(nameBuffer.length, 28); // File name length
    central.writeUInt16LE(0, 30); // Extra field length
    central.writeUInt16LE(0, 32); // File comment length
    central.writeUInt16LE(0, 34); // Disk number start
    central.writeUInt16LE(0, 36); // Internal file attributes
    central.writeUInt32LE((0o100755 << 16) >>> 0, 38); // External file attributes (unix mode)
    central.writeUInt32LE(offset, 42); // Relative offset of local header
    nameBuffer.copy(central, 46);
    centralHeaders.push(central);

    offset += local.length;
  }

  const centralDirOffset = offset;
  const centralDirSize = centralHeaders.reduce((sum, h) => sum + h.length, 0);

  // End of central directory record (22 bytes)
  const eocd = Buffer.alloc(22);
  eocd.writeUInt32LE(0x06054b50, 0); // End of central directory signature
  eocd.writeUInt16LE(0, 4); // Number of this disk
  eocd.writeUInt16LE(0, 6); // Disk where central directory starts
  eocd.writeUInt16LE(entries.length, 8); // Number of central directory records on this disk
  eocd.writeUInt16LE(entries.length, 10); // Total number of central directory records
  eocd.writeUInt32LE(centralDirSize, 12); // Size of central directory
  eocd.writeUInt32LE(centralDirOffset, 16); // Offset of start of central directory
  eocd.writeUInt16LE(0, 20); // Comment length

  return Buffer.concat([...localHeaders, ...centralHeaders, eocd]);
}

/**
 * CRC-32 implementation for zip file creation.
 */
function crc32(buf: Buffer): number {
  const table = new Uint32Array(256);
  for (let i = 0; i < 256; i++) {
    let c = i;
    for (let j = 0; j < 8; j++) {
      c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    }
    table[i] = c;
  }

  let crc = 0xffffffff;
  for (let i = 0; i < buf.length; i++) {
    crc = table[(crc ^ buf[i]) & 0xff] ^ (crc >>> 8);
  }
  return (crc ^ 0xffffffff) >>> 0;
}

describe('extract-zip', () => {
  let tmpDir: string;

  beforeEach(() => {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-extract-zip-test-'));
  });

  afterEach(() => {
    fs.rmSync(tmpDir, { recursive: true, force: true });
  });

  it('extracts a zip containing only small text files', async () => {
    const entries = [
      { name: 'version', data: Buffer.from('42.3.2\n') },
      { name: 'LICENSE', data: Buffer.from('MIT License\n'.repeat(50)) }
    ];

    const zipBuffer = createStoredZip(entries);
    const zipPath = path.join(tmpDir, 'small-files.zip');
    const extractDir = path.join(tmpDir, 'out');
    fs.writeFileSync(zipPath, zipBuffer);

    await extract(zipPath, { dir: extractDir });

    for (const entry of entries) {
      const extractedPath = path.join(extractDir, entry.name);
      expect(fs.existsSync(extractedPath)).to.equal(true, `Expected file "${entry.name}" to be extracted`);
      const content = fs.readFileSync(extractedPath);
      expect(content.length).to.equal(entry.data.length);
      expect(content.equals(entry.data)).to.equal(true);
    }
  });

  it('extracts a zip containing a large binary file (simulating electron binary)', async () => {
    // This test simulates the Electron distribution zip structure:
    // - Small text files (version, LICENSE) that extract fine
    // - A large binary file (simulating the electron binary) that fails on
    //   Node.js >= 24.16.0 due to the fd-slicer destroy() lifecycle bug
    //
    // The bug causes stream.pipeline() to never resolve for STORED entries
    // when fd-slicer's ReadStream reaches EOF, because:
    // 1. ReadStream._read() sets this.destroyed = true
    // 2. ReadStream._read() calls this.push(null) to signal EOF
    // 3. Node's Readable calls stream.destroy() to complete lifecycle
    // 4. fd-slicer's destroy() sees this.destroyed === true, returns early
    // 5. 'close' event is never emitted
    // 6. stream.pipeline() never resolves
    //
    // We use 20MB as it's large enough to trigger the multi-read path in
    // fd-slicer (highWaterMark is 16KB by default), which is necessary to
    // reproduce the issue. The actual electron binary is 130-250MB.
    const LARGE_FILE_SIZE = 20 * 1024 * 1024; // 20 MB

    const entries = [
      { name: 'version', data: Buffer.from('42.3.2\n') },
      { name: 'LICENSE', data: Buffer.from('MIT License\n') },
      { name: 'electron', data: crypto.randomBytes(LARGE_FILE_SIZE) }
    ];

    const zipBuffer = createStoredZip(entries);
    const zipPath = path.join(tmpDir, 'electron-dist.zip');
    const extractDir = path.join(tmpDir, 'dist');
    fs.writeFileSync(zipPath, zipBuffer);

    // Apply a timeout to detect the hang behavior.
    // On affected Node versions, extract() will never resolve because
    // stream.pipeline() hangs waiting for the 'close' event that fd-slicer
    // never emits.
    const TIMEOUT_MS = 30_000;
    let timer: ReturnType<typeof setTimeout>;
    const result = await Promise.race([
      extract(zipPath, { dir: extractDir }).then(() => 'completed'),
      new Promise<string>((resolve) => {
        timer = setTimeout(() => resolve('timeout'), TIMEOUT_MS);
      })
    ]);
    clearTimeout(timer!);

    expect(result).to.equal(
      'completed',
      `extract-zip hung for ${TIMEOUT_MS}ms without completing. ` +
        'This is the known fd-slicer destroy() lifecycle bug that affects ' +
        'Node.js >= 24.16.0. See: https://github.com/electron/electron/issues/51619'
    );

    // Verify all files were extracted with correct content
    for (const entry of entries) {
      const extractedPath = path.join(extractDir, entry.name);
      expect(fs.existsSync(extractedPath)).to.equal(
        true,
        `Expected file "${entry.name}" to be extracted, but it is missing`
      );
      const stat = fs.statSync(extractedPath);
      expect(stat.size).to.equal(entry.data.length);
    }

    // Verify the large binary file content integrity
    const extractedBinary = fs.readFileSync(path.join(extractDir, 'electron'));
    const expectedHash = crypto.createHash('sha256').update(entries[2].data).digest('hex');
    const actualHash = crypto.createHash('sha256').update(extractedBinary).digest('hex');
    expect(actualHash).to.equal(expectedHash, 'Extracted binary content does not match original');
  });

  it('extracts multiple large stored entries sequentially', async () => {
    // This test verifies that extraction doesn't hang or fail after the first
    // large entry. The bug can cause the zipfile's entry iteration to stall
    // after a single large stored entry because readEntry() is never called
    // again (pipeline never resolves, so the 'entry' callback never proceeds).
    const FILE_SIZE = 5 * 1024 * 1024; // 5 MB each

    const entries = [
      { name: 'version', data: Buffer.from('42.3.2\n') },
      { name: 'chromedriver', data: crypto.randomBytes(FILE_SIZE) },
      { name: 'mksnapshot', data: crypto.randomBytes(FILE_SIZE) },
      { name: 'electron', data: crypto.randomBytes(FILE_SIZE) },
      { name: 'LICENSES.chromium.html', data: Buffer.from('<html></html>\n'.repeat(1000)) }
    ];

    const zipBuffer = createStoredZip(entries);
    const zipPath = path.join(tmpDir, 'multi-large.zip');
    const extractDir = path.join(tmpDir, 'dist');
    fs.writeFileSync(zipPath, zipBuffer);

    const TIMEOUT_MS = 30_000;
    let timer: ReturnType<typeof setTimeout>;
    const result = await Promise.race([
      extract(zipPath, { dir: extractDir }).then(() => 'completed'),
      new Promise<string>((resolve) => {
        timer = setTimeout(() => resolve('timeout'), TIMEOUT_MS);
      })
    ]);
    clearTimeout(timer!);

    expect(result).to.equal(
      'completed',
      'extract-zip hung while extracting multiple large stored entries. ' +
        'See: https://github.com/electron/electron/issues/51619'
    );

    // Verify all files were extracted
    for (const entry of entries) {
      const extractedPath = path.join(extractDir, entry.name);
      expect(fs.existsSync(extractedPath)).to.equal(true, `Expected file "${entry.name}" to be extracted`);
      const stat = fs.statSync(extractedPath);
      expect(stat.size).to.equal(entry.data.length);
    }
  });
});
