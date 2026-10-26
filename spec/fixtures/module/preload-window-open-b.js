// Preload fixture for the window.open() override spec. Reports a constant
// marker so the spec can verify which preload ran on the popup's synchronous
// about:blank document. preload-window-open-a.js is a copy with marker 'a'.
const { ipcRenderer } = require('electron');
ipcRenderer.send('preload-ran', 'b', location.href);
