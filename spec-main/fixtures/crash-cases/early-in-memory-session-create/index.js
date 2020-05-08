const { app, session } = require('electron');

app.on('ready', () => {
  session.fromPartition('in-memory');
  process.exit(0);
});
