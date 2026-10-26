const { app } = require('electron');

app.whenReady().then(() => {
  process.stdout.write('ready\n');
});
