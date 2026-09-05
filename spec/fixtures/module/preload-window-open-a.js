// Preload fixture for the window.open() override spec. Reports a constant
// marker so the spec can verify which preload ran on the popup's synchronous
// about:blank document. preload-window-open-b.js is a copy with marker 'b'.
const { ipcRenderer } = require('electron');
ipcRenderer.send('preload-ran', 'a', location.href);
