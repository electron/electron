const { app, session } = require('electron');

app.setPath('userData', process.argv[2]);

app.whenReady().then(() => {
  // Create the in-memory session before anything touches the default session.
  const inMemory = session.fromPartition('in-memory');
  const defaultSession = session.defaultSession;

  defaultSession.setSpellCheckerEnabled(false);
  const inMemoryAfterDefaultChange = inMemory.isSpellCheckerEnabled();

  inMemory.setSpellCheckerEnabled(true);
  const defaultAfterInMemoryChange = defaultSession.isSpellCheckerEnabled();
  const inMemoryAfterInMemoryChange = inMemory.isSpellCheckerEnabled();

  process.stdout.write(
    JSON.stringify({ inMemoryAfterDefaultChange, defaultAfterInMemoryChange, inMemoryAfterInMemoryChange })
  );
  app.quit();
});
