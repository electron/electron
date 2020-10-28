const { app } = require('electron');

const locale = process.argv[2].substr(11);
app.commandLine.appendSwitch('lang', locale);

app.whenReady().then(() => {
  process.stdout.write(app.getLocale());
  process.stdout.end();

  app.quit();
});
