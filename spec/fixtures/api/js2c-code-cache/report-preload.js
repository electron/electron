// Reports this renderer's js2c code-cache status to the main process on the
// channel passed via --js2c-cc-channel.
const { ipcRenderer } = require('electron');

const arg = process.argv.find((a) => a.startsWith('--js2c-cc-channel='));
const channel = arg ? arg.slice('--js2c-cc-channel='.length) : 'js2c-cc-status';

function status() {
  return process._linkedBinding('electron_common_v8_util').getJs2cCodeCacheStatus();
}

try {
  ipcRenderer.send(channel, status());
} catch (err) {
  ipcRenderer.send(channel, { __error: String((err && err.stack) || err) });
}
