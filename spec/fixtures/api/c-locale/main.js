// Launched by chromium-spec.ts with LC_ALL/LANG pointing at the "C" locale (or
// at a locale that is not installed). Prints the filename Electron ends up with
// for a non-ASCII name in two places that go through the C library's multibyte
// conversion on Linux: a download's Content-Disposition filename, and a file
// pasted into a page from the OS clipboard. The second one CHECK-crashed the
// browser process before https://github.com/electron/electron/issues/54153.
const { app, BrowserWindow, ClipboardItem, clipboard, ipcMain, session } = require('electron');

const { once } = require('node:events');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');
const { pathToFileURL } = require('node:url');

const NAME = 'naïve — file.txt';

async function downloadFilename() {
  const server = http.createServer((req, res) => {
    res.writeHead(200, {
      'Content-Type': 'application/octet-stream',
      'Content-Disposition': `attachment; filename*=UTF-8''${encodeURIComponent(NAME)}`
    });
    res.end('hello');
  });
  server.listen(0, '127.0.0.1');
  await once(server, 'listening');
  const willDownload = once(session.defaultSession, 'will-download');
  session.defaultSession.downloadURL(`http://127.0.0.1:${server.address().port}/`);
  const [event, item] = await willDownload;
  event.preventDefault();
  server.close();
  return item.getFilename();
}

async function pastedFilename() {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-c-locale-'));
  const file = path.join(dir, NAME);
  fs.writeFileSync(file, 'hello');
  const w = new BrowserWindow({ show: true, webPreferences: { nodeIntegration: true, contextIsolation: false } });
  try {
    await w.loadFile(path.join(__dirname, 'index.html'));
    await clipboard.write([new ClipboardItem({ 'text/uri-list': pathToFileURL(file).href })]);
    const pasted = once(ipcMain, 'pasted');
    w.webContents.focus();
    w.webContents.paste();
    const [, names] = await pasted;
    return names.join('|');
  } finally {
    w.destroy();
    fs.rmSync(dir, { recursive: true, force: true });
  }
}

app.whenReady().then(async () => {
  try {
    const result = { download: await downloadFilename(), paste: await pastedFilename() };
    process.stdout.write(JSON.stringify(result) + '\n', () => app.exit(0));
  } catch (error) {
    console.error(error);
    app.exit(1);
  }
});
