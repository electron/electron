process.on('uncaughtException', function (err) {
  process.send(err.message);
});

const echo = require('@electron-ci/echo');
process.send(echo('ok'));
