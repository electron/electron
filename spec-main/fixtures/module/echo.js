process.on('uncaughtException', function (err) {
  process.send(err.message);
});

const echo = require('echo');
process.send(echo('ok'));
