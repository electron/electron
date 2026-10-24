// Loads a file out of an asar archive through the file: protocol, swaps the
// archive on disk for a different one while the app is still running, and
// loads the same file again. Prints both results as JSON.
const { app, BrowserWindow } = require('electron');

const fs = require('original-fs');
const os = require('node:os');
const path = require('node:path');
const { pathToFileURL } = require('node:url');

app.setPath('userData', fs.mkdtempSync(path.join(os.tmpdir(), 'asar-replace-userdata-')));

const [initialAsar, replacementAsar, fileInAsar] = process.argv.slice(2);

const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'asar-replace-'));
const liveAsar = path.join(dir, 'app.asar');
fs.copyFileSync(initialAsar, liveAsar);

const target = pathToFileURL(path.join(liveAsar, fileInAsar)).toString();

let window;

async function read() {
  try {
    await window.loadURL(target);
    return { ok: true, body: await window.webContents.executeJavaScript('document.body.innerText') };
  } catch (error) {
    return { ok: false, body: String(error) };
  }
}

app
  .whenReady()
  .then(async () => {
    window = new BrowserWindow({ show: false });
    const before = await read();
    // Swap the archive the way an updater would: write the new one alongside
    // and rename it over the original, so the path now points at a different
    // file than the one the app has open.
    const staged = path.join(dir, 'staged.asar');
    fs.copyFileSync(replacementAsar, staged);
    fs.renameSync(staged, liveAsar);
    const after = await read();
    process.stdout.write(JSON.stringify({ before, after }) + '\n');
  })
  .then(
    () => app.exit(0),
    (error) => {
      process.stderr.write(`${error.stack || error}\n`);
      app.exit(1);
    }
  )
  .finally(() => fs.rmSync(dir, { recursive: true, force: true }));
