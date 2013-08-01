process.on('uncaughtException', function(err) {
  process.send(err.message);
});

require('int64-native');
process.send('ok');
