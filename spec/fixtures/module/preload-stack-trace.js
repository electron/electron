// Preload fixture for the preload stack trace spec.
//
// IMPORTANT: the line number of the `throw` below is asserted by the spec.
// If you edit this file, update the expected line number in
// api-browser-window-spec.ts ("preload script stack traces").
const { ipcRenderer } = require('electron');
try {
  throw new Error('preload-stack-trace-marker'); // <-- line 8
} catch (error) {
  ipcRenderer.send('preload-stack-trace', { message: error.message, stack: error.stack });
}
