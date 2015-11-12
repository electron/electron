var ipc = require('ipc-renderer');
ipc.on('ping', function(event, message) {
  ipc.sendToHost('pong', message);
});
