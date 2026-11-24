// Runs a WebAssembly function that reads past the end of its memory, either
// in the main process or in a renderer, and prints how the trap was reported.
// Usage: electron main.js <main|sandboxed|unsandboxed>
const { app, BrowserWindow } = require('electron');

// (module (memory 1) (func (export "f") (param i32) (result i32)
//   local.get 0 i32.load))
const wasmBytes = [
  0, 97, 115, 109, 1, 0, 0, 0, 1, 6, 1, 96, 1, 127, 1, 127, 3, 2, 1, 0, 5, 3, 1, 0, 1, 7, 5, 1, 1, 102, 0, 0, 10, 9, 1,
  7, 0, 32, 0, 40, 2, 0, 11
];

function runOutOfBoundsLoad(bytes) {
  const instance = new WebAssembly.Instance(new WebAssembly.Module(new Uint8Array(bytes)));
  try {
    instance.exports.f(0x10000);
    return 'no-trap';
  } catch (e) {
    return `caught ${e.name}: ${e.message}`;
  }
}

const mode = process.argv[2];

app.whenReady().then(async () => {
  if (mode === 'main') {
    console.log(`RESULT ${runOutOfBoundsLoad(wasmBytes)}`);
    app.exit(0);
    return;
  }

  const w = new BrowserWindow({
    show: false,
    webPreferences: { sandbox: mode === 'sandboxed' }
  });
  w.webContents.on('render-process-gone', (_event, details) => {
    console.log(`RESULT renderer-gone ${details.reason}`);
    app.exit(0);
  });
  await w.loadURL('about:blank');
  const result = await w.webContents.executeJavaScript(
    `(${runOutOfBoundsLoad.toString()})(${JSON.stringify(wasmBytes)})`
  );
  console.log(`RESULT ${result}`);
  app.exit(0);
});
