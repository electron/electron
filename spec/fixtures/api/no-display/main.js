const { app, nativeTheme } = require('electron');

// Register a shutdown observer before the platform fails to initialise.
nativeTheme.themeSource = 'system';

app.whenReady().then(() => {
  // Only reached if a display was available after all.
  app.exit(2);
});
