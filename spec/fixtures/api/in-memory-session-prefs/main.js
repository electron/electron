const { app, session } = require('electron');

app.setPath('userData', process.argv[2]);

app.whenReady().then(() => {
  const ses = session.fromPartition('in-memory');
  process.stdout.write(JSON.stringify({ spellCheckerEnabled: ses.isSpellCheckerEnabled() }));
  app.quit();
});
