var ipc = require('ipc');
window.onload = function() {
  ipc.send('answer', typeof window.process);
}
