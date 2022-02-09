const { app } = require('electron');

function tearDown (err) {
  if (err) {
    process.exit(1);
  }

  // this should not get executed
  process.exit();
}

process.on('uncaughtException', tearDown);
process.on('exit', tearDown);

app.on('ready', () => {
  // eslint-disable-next-line no-constant-condition
  if (true) {
    throw new Error('An uncaught exception!');
  }

  app.quit();
});
