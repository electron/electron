process.on('uncaughtException', function(err) {
  process.send(err.message);
});

require('time');
process.send('ok');
