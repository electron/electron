// Opens a sandboxed window, then prints the pids of the browser process and of
// its child processes to stdout, so that the spec can match them against the
// DYLD_PRINT_INITIALIZERS output on stderr.
const { app, BrowserWindow } = require('electron');

const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

// Keep this app's profile away from the default Electron userData directory,
// which the test runner (another Electron instance) may be using.
app.setPath('userData', fs.mkdtempSync(path.join(os.tmpdir(), 'aperitif-fixture-userdata-')));

// dyld writes each log line in several pieces, so output from child processes
// that start at the same moment (the GPU process and the network service do)
// interleaves mid-line. Keep those two in the browser process so that only the
// renderer launches, after the browser has finished initializing.
app.commandLine.appendSwitch('in-process-gpu');
app.commandLine.appendSwitch('enable-features', 'NetworkServiceInProcess2');

app.whenReady().then(async () => {
  const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
  await w.loadURL('about:blank');
  const result = {
    browser: process.pid,
    renderer: w.webContents.getOSProcessId(),
    processes: app.getAppMetrics().map(({ pid, type }) => ({ pid, type }))
  };
  process.stdout.write(`${JSON.stringify(result)}\n`);
  app.quit();
});
