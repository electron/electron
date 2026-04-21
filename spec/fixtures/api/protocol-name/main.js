const { app } = require('electron');

app.whenReady().then(() => {
  const url = process.argv[2];
  const name = app.getApplicationNameForProtocol(url);
  process.stdout.write(JSON.stringify({ name }));
  app.quit();
});
