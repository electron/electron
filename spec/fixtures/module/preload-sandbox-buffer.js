const { ipcRenderer } = require('electron');

const before = typeof Buffer;
const roundTrip = Buffer.from('héllo', 'utf8').toString('base64');
// Preloads may replace the global; later reads must see the replacement.
// Without context isolation globalThis is the page's window, so clean up.
const original = Buffer;
globalThis.Buffer = 'replaced';
const replaced = globalThis.Buffer;
if (process.contextIsolated) globalThis.Buffer = original;
else delete globalThis.Buffer;

ipcRenderer.send('answer', {
  before,
  roundTrip,
  replaced,
  restored: typeof Buffer.alloc,
  isBuffer: Buffer.isBuffer(Buffer.alloc(1)),
  contextIsolated: process.contextIsolated
});
