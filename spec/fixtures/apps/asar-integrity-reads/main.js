// Exercises the block-validated, fd/stream based read paths of the asar fs
// wrapper against a multi-block archive (integrity-reads.asar) that the spec
// generates next to default_app.asar and registers in the app's integrity
// table.  ASAR_INTEGRITY_READS_MODE selects the scenario; progress markers
// are printed so the spec can tell how far a run got before an integrity
// violation terminated it.
const { app } = require('electron');
const fs = require('node:fs');
const originalFs = require('node:original-fs');
const os = require('node:os');
const path = require('node:path');

// Keep this app's profile away from the default Electron userData directory,
// which the test runner (another Electron instance) may be using.
app.setPath('userData', fs.mkdtempSync(path.join(os.tmpdir(), 'asar-fixture-userdata-')));

const mode = process.env.ASAR_INTEGRITY_READS_MODE || 'sweep';
const archive = path.join(process.resourcesPath, 'integrity-reads.asar');
const multi = path.join(archive, 'multi.bin');
const exact = path.join(archive, 'exact.bin');
const small = path.join(archive, 'small.bin');

const BLOCK = 4 * 1024 * 1024;
// Must match the generator in asar-integrity-spec.ts.
const MULTI_SIZE = 2 * BLOCK + 1024 * 1024 + 5;
const EXACT_SIZE = BLOCK;
const SMALL_SIZE = 1000;
const patternByte = (i) => (i * 7 + (i >> 8)) & 0xff;
const MARKER = (n) => `ASARBLOCK${n}MAGIC!`;

function expectedRange(start, end) {
  const buf = Buffer.alloc(end - start);
  for (let i = start; i < end; i++) buf[i - start] = patternByte(i);
  for (let n = 0; n * BLOCK < MULTI_SIZE; n++) {
    const marker = Buffer.from(MARKER(n));
    for (let j = 0; j < marker.length; j++) {
      const pos = n * BLOCK + j;
      if (pos >= start && pos < end) buf[pos - start] = marker[j];
    }
  }
  return buf;
}
function expectedFile(size) {
  const buf = Buffer.alloc(size);
  for (let i = 0; i < size; i++) buf[i] = patternByte(i);
  return buf;
}

const assert = (cond, msg) => {
  if (!cond) {
    throw new Error(`assertion failed: ${msg}`);
  }
};
const streamAll = (p, opts) =>
  new Promise((resolve, reject) => {
    const chunks = [];
    fs.createReadStream(p, opts)
      .on('data', (c) => chunks.push(c))
      .on('error', reject)
      .on('end', () => resolve(Buffer.concat(chunks)));
  });

async function sweep() {
  const multiExpected = expectedRange(0, MULTI_SIZE);
  const exactExpected = expectedFile(EXACT_SIZE);
  const smallExpected = expectedFile(SMALL_SIZE);

  // Whole-file streams with chunk sizes that do and don't divide the block size.
  for (const hwm of [64 * 1024, 1000, 1 << 20]) {
    assert((await streamAll(multi, { highWaterMark: hwm })).equals(multiExpected), `stream hwm=${hwm}`);
  }
  assert((await streamAll(exact, { highWaterMark: 12345 })).equals(exactExpected), 'stream exact');
  assert((await streamAll(small)).equals(smallExpected), 'stream small');
  console.log('sweep-streams-ok');

  // Ranged streams around block boundaries and the tail.
  const ranges = [
    [0, 0],
    [BLOCK - 3, BLOCK + 2],
    [BLOCK, BLOCK],
    [2 * BLOCK - 1, 2 * BLOCK],
    [BLOCK - 1, MULTI_SIZE - 1],
    [MULTI_SIZE - 5, MULTI_SIZE - 1],
    [MULTI_SIZE - 1, MULTI_SIZE - 1],
    [1, 2 * BLOCK + 7]
  ];
  for (const [start, end] of ranges) {
    const got = await streamAll(multi, { start, end, highWaterMark: 777 });
    assert(got.equals(expectedRange(start, end + 1)), `ranged stream ${start}-${end}`);
  }
  console.log('sweep-ranges-ok');

  // Sync fd reads: odd chunk sizes, cursor semantics, positional jumps across blocks, EOF.
  {
    const fd = fs.openSync(multi, 'r');
    const st = fs.fstatSync(fd);
    assert(st.size === MULTI_SIZE && st.isFile(), 'fstat');
    const out = Buffer.alloc(MULTI_SIZE);
    let read = 0;
    while (read < MULTI_SIZE) {
      const n = fs.readSync(fd, out, read, Math.min(4097, MULTI_SIZE - read), null);
      if (n === 0) break;
      read += n;
    }
    assert(read === MULTI_SIZE && out.equals(multiExpected), 'readSync loop');
    assert(fs.readSync(fd, Buffer.alloc(10), 0, 10, null) === 0, 'readSync at EOF');
    for (const pos of [2 * BLOCK + 10, 0, BLOCK - 1, MULTI_SIZE - 3, BLOCK * 2 - 5, 12345]) {
      const buf = Buffer.alloc(9);
      const n = fs.readSync(fd, buf, 0, 9, pos);
      const exp = expectedRange(pos, Math.min(pos + 9, MULTI_SIZE));
      assert(n === exp.length && buf.subarray(0, n).equals(exp), `positional read @${pos}`);
    }
    assert(fs.readSync(fd, Buffer.alloc(1), 0, 1, MULTI_SIZE + 100) === 0, 'read past EOF');
    // readv crossing a block boundary
    const bufs = [Buffer.alloc(100), Buffer.alloc(200), Buffer.alloc(BLOCK)];
    const n = fs.readvSync(fd, bufs, BLOCK - 150);
    assert(n === 300 + BLOCK, `readv bytes ${n}`);
    assert(Buffer.concat(bufs).equals(expectedRange(BLOCK - 150, BLOCK - 150 + n)), 'readv content');
    fs.closeSync(fd);
  }
  console.log('sweep-fd-ok');

  // Async fd reads and readFile(fd) from a moved cursor.
  {
    const fd = await fs.promises.open(multi, 'r');
    const { bytesRead, buffer } = await fd.read(Buffer.alloc(BLOCK + 3), 0, BLOCK + 3, BLOCK - 1);
    assert(bytesRead === BLOCK + 3 && buffer.equals(expectedRange(BLOCK - 1, 2 * BLOCK + 2)), 'handle.read');
    const stat = await fd.stat();
    assert(stat.size === MULTI_SIZE, 'handle.stat');
    // Move the cursor with a null-position read, then readFile from there.
    await fd.read(Buffer.alloc(2 * BLOCK + 5), 0, 2 * BLOCK + 5, null);
    const rest = await fd.readFile();
    assert(rest.equals(expectedRange(2 * BLOCK + 5, MULTI_SIZE)), 'handle.readFile from cursor');
    const web = fd.readableWebStream();
    let webTotal = 0;
    for await (const chunk of web) webTotal += chunk.byteLength;
    assert(webTotal === 0, 'web stream at EOF');
    await fd.close();

    const fd2 = fs.openSync(multi, 'r');
    fs.readSync(fd2, Buffer.alloc(BLOCK + 1), 0, BLOCK + 1, null);
    const rest2 = fs.readFileSync(fd2);
    assert(rest2.equals(expectedRange(BLOCK + 1, MULTI_SIZE)), 'readFileSync(fd) from cursor');
    fs.closeSync(fd2);

    const fd3 = fs.openSync(small, 'r');
    fs.readSync(fd3, Buffer.alloc(10), 0, 10, null);
    assert(fs.readFileSync(fd3, 'latin1') === smallExpected.subarray(10).toString('latin1'), 'readFileSync(fd, enc)');
    fs.closeSync(fd3);
  }
  console.log('sweep-handle-ok');

  // Concurrent consumers of the same entry, more than the retained-block cap.
  {
    const streams = [];
    for (let i = 0; i < 40; i++) streams.push(streamAll(multi, { highWaterMark: 65536 + i }));
    const results = await Promise.all(streams);
    for (const r of results) assert(r.equals(multiExpected), 'concurrent stream');
  }
  console.log('sweep-concurrent-ok');

  // Copies.
  {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'asar-integrity-reads-'));
    fs.copyFileSync(multi, path.join(dir, 'a'));
    await fs.promises.copyFile(multi, path.join(dir, 'b'));
    fs.cpSync(archive, path.join(dir, 'c'), { recursive: true });
    assert(fs.readFileSync(path.join(dir, 'a')).equals(multiExpected), 'copyFileSync');
    assert(fs.readFileSync(path.join(dir, 'b')).equals(multiExpected), 'promises.copyFile');
    assert(fs.readFileSync(path.join(dir, 'c', 'multi.bin')).equals(multiExpected), 'cpSync');
    assert(fs.readFileSync(path.join(dir, 'c', 'small.bin')).equals(smallExpected), 'cpSync small');
    fs.rmSync(dir, { recursive: true, force: true });
  }
  console.log('sweep-copy-ok');
  console.log('sweep-ok');
}

async function blockStream() {
  let received = 0;
  let reported = -1;
  await new Promise((resolve, reject) => {
    fs.createReadStream(multi, { highWaterMark: 65536 })
      .on('data', (c) => {
        received += c.length;
        const block = Math.floor((received - 1) / BLOCK);
        while (reported < block) {
          reported++;
          console.log(`reached-block-${reported}`);
        }
      })
      .on('error', reject)
      .on('end', resolve);
  });
  assert(received === MULTI_SIZE, 'stream length');
  console.log('stream-ok');
}

async function rangeBlock0() {
  const got = await streamAll(multi, { start: 0, end: 1024 * 1024 - 1 });
  assert(got.equals(expectedRange(0, 1024 * 1024)), 'range content');
  const fd = fs.openSync(multi, 'r');
  const buf = Buffer.alloc(100);
  assert(fs.readSync(fd, buf, 0, 100, 500) === 100 && buf.equals(expectedRange(500, 600)), 'positional block 0');
  fs.closeSync(fd);
  console.log('range-block0-ok');
}

async function tamperAfterRead() {
  // Reader A validates block 0 and keeps it.
  const a = fs.openSync(multi, 'r');
  const first = Buffer.alloc(100);
  assert(fs.readSync(a, first, 0, 100, 0) === 100 && first.equals(expectedRange(0, 100)), 'A first read');

  // Now corrupt block 0 of the entry in place on disk (same inode).
  const raw = originalFs.readFileSync(archive);
  const at = raw.indexOf(Buffer.from(MARKER(0)));
  assert(at !== -1, 'marker present');
  const rawFd = originalFs.openSync(archive, 'r+');
  originalFs.writeSync(rawFd, Buffer.from('TAMPERED0MAGIC!!'), 0, 16, at);
  originalFs.closeSync(rawFd);

  // A serves from its already-validated block; nothing has been re-read.
  assert(fs.readSync(a, first, 0, 100, 0) === 100 && first.equals(expectedRange(0, 100)), 'A cached read');
  console.log('tamper-after-read-A-ok');

  // A fresh reader must re-validate block 0 and die.
  const b = fs.openSync(multi, 'r');
  fs.readSync(b, Buffer.alloc(100), 0, 100, 0);
  console.log('tamper-after-read-B-unexpectedly-ok');
}

async function readFileWhole() {
  const got = fs.readFileSync(multi);
  assert(got.equals(expectedRange(0, MULTI_SIZE)), 'readFileSync content');
  console.log('readfile-ok');
}

const modes = {
  sweep,
  'block-stream': blockStream,
  'range-block0': rangeBlock0,
  'tamper-after-read': tamperAfterRead,
  readfile: readFileWhole
};

// app.exit() (which process.exit() maps to in the main process) is only
// reliable once the app is ready; exiting during startup can be lost.
const exitWhenReady = (code) => app.whenReady().then(() => process.exit(code));

modes[mode]().then(
  () => exitWhenReady(0),
  (error) => {
    console.log(`${mode}-failed: ${error && error.stack}`);
    return exitWhenReady(4);
  }
);
