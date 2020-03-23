const { app } = require('electron');

app.whenReady().then(() => {
  process.stdout.write(app.getLocale());
  process.stdout.end();

  app.quit();
});
