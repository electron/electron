const { app } = require('electron');

const locale = process.argv[2].substr(11);
if (locale.length !== 0) {
  app.commandLine.appendSwitch('lang', locale);
}

app.whenReady().then(() => {
  if (process.argv[3] === '--print-env') {
    process.stdout.write(String(process.env.LC_ALL));
  } else {
    process.stdout.write(`${app.getLocale()}|${app.getSystemLocale()}|${JSON.stringify(app.getPreferredSystemLanguages())}`);
  }
  process.stdout.end();

  app.quit();
});
