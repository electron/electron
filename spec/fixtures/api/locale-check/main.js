const { app } = require('electron');

const locale = process.argv[2].substr(11);
if (locale.length !== 0) {
  app.commandLine.appendSwitch('lang', locale);
}

app.whenReady().then(() => {
  if (process.argv[3] === '--print-env') {
    process.stdout.write(String(process.env.LC_ALL));
  } else if (process.argv[3] === '--print-locale-env') {
    process.stdout.write(
      JSON.stringify({
        locale: app.getLocale(),
        systemLocale: app.getSystemLocale(),
        preferredLanguages: app.getPreferredSystemLanguages(),
        env: {
          LANG: process.env.LANG,
          LANGUAGE: process.env.LANGUAGE,
          LC_ALL: process.env.LC_ALL,
          LC_MESSAGES: process.env.LC_MESSAGES
        }
      })
    );
  } else {
    process.stdout.write(
      `${app.getLocale()}|${app.getSystemLocale()}|${JSON.stringify(app.getPreferredSystemLanguages())}`
    );
  }
  process.stdout.end();

  app.quit();
});
