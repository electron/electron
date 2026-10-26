const { app, nativeTheme } = require('electron');

app.whenReady().then(() => {
  const read = () => [nativeTheme.themeSource, nativeTheme.shouldUseDarkColors];
  const states = [read()];
  for (const source of ['light', 'system']) {
    nativeTheme.themeSource = source;
    states.push(read());
  }
  process.stdout.write(JSON.stringify(states));
  app.quit();
});
