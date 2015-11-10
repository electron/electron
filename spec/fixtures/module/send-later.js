var ipc = require('ipc-renderer');
window.onload = function() {
  ipc.send('answer', typeof window.process);
}
