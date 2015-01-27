var ipc = require('ipc');
ipc.on('ping', function(message) {
  ipc.sendToHost('pong', message);
});
