// Reads a file out of default_app.asar through the fd / stream / copy based
// fs APIs (which serve bytes directly from the archive) rather than
// fs.readFile, so that the block-based integrity validation gets exercised.
// The mode is chosen with the ASAR_FD_READ_MODE environment variable.
const { app } = require('electron');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

// Keep this app's profile away from the default Electron userData directory,
// which the test runner (another Electron instance) may be using.
app.setPath('userData', fs.mkdtempSync(path.join(os.tmpdir(), 'asar-fixture-userdata-')));

const target = path.join(process.resourcesPath, 'default_app.asar', 'index.html');
const mode = process.env.ASAR_FD_READ_MODE || 'fd';

async function readViaFd() {
  const fd = fs.openSync(target, 'r');
  const { size } = fs.fstatSync(fd);
  const buffer = Buffer.alloc(size);
  let read = 0;
  while (read < size) {
    const n = fs.readSync(fd, buffer, read, Math.min(1024, size - read), null);
    if (n === 0) break;
    read += n;
  }
  fs.closeSync(fd);
  return buffer.subarray(0, read);
}

function readViaStream() {
  return new Promise((resolve, reject) => {
    const chunks = [];
    fs.createReadStream(target, { highWaterMark: 1024 })
      .on('data', (chunk) => chunks.push(chunk))
      .on('error', reject)
      .on('end', () => resolve(Buffer.concat(chunks)));
  });
}

async function readViaFileHandle() {
  const handle = await fs.promises.open(target, 'r');
  try {
    return await handle.readFile();
  } finally {
    await handle.close();
  }
}

async function readViaCopy() {
  const dest = path.join(fs.mkdtempSync(path.join(os.tmpdir(), 'asar-fd-reads-')), 'index.html');
  await fs.promises.copyFile(target, dest);
  return fs.readFileSync(dest);
}

const readers = { fd: readViaFd, stream: readViaStream, handle: readViaFileHandle, copy: readViaCopy };

// app.exit() (which process.exit() maps to in the main process) is only
// reliable once the app is ready; exiting during startup can be lost.
const exitWhenReady = (code) => app.whenReady().then(() => process.exit(code));

(async () => {
  const content = await readers[mode]();
  // Only after the fd-based read succeeded do we compare against fs.readFile,
  // which validates the whole-file hash independently.
  const expected = fs.readFileSync(target);
  if (!content.equals(expected)) {
    console.log(`mismatch: ${mode} read ${content.length} bytes, expected ${expected.length}`);
    return exitWhenReady(3);
  }
  console.log(`${mode}-read-ok`);
  return exitWhenReady(0);
})().catch((error) => {
  console.log(`${mode}-read-failed: ${error && error.stack}`);
  return exitWhenReady(4);
});
