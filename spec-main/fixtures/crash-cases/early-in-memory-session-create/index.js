const { app, session } = require('electron');

app.on('ready', () => {
  session.fromPartition('in-memory');
  setImmediate(() => {
    process.exit(0);
  });
});
