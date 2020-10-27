const { app } = require('electron');

process.on('message', () => {
  console.log('Notified to quit');
  app.quit();
});
