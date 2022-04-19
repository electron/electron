process.on('uncaughtException', function (err) {
  process.send(err.message);
});

const echo = require('@electron/echo');
process.send(echo('ok'));
