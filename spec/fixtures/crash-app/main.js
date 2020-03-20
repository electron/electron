const { app } = require('electron');

app.on('ready', () => {
  process.crash();
});
