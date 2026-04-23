const { app } = require('electron');

app.whenReady().then(async () => {
  const url = process.argv[2];

  if (process.env.ELECTRON_PROTOCOL_LOOKUP_MODE === 'info') {
    try {
      const info = await app.getApplicationInfoForProtocol(url);
      process.stdout.write(
        JSON.stringify({
          name: info.name,
          path: info.path,
          hasIcon: !info.icon.isEmpty()
        })
      );
    } catch (error) {
      process.stderr.write(`${error.message}\n`);
      process.exitCode = 1;
    }
    app.quit();
    return;
  }

  const name = app.getApplicationNameForProtocol(url);
  process.stdout.write(JSON.stringify({ name }));
  app.quit();
});
