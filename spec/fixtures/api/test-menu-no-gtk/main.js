const { app, BrowserWindow, Menu, nativeTheme } = require('electron');

app
  .whenReady()
  .then(async () => {
    const win = new BrowserWindow({ show: false });
    await win.loadURL('data:text/html,<h1>Menu without GTK</h1>');

    for (const themeSource of ['dark', 'light']) {
      nativeTheme.themeSource = themeSource;
      await new Promise((resolve) => setImmediate(resolve));
      win.setMenu(
        Menu.buildFromTemplate([
          {
            label: 'Test',
            submenu: [{ label: 'Enabled' }, { label: 'Disabled', enabled: false }]
          }
        ])
      );
      if (!win.isMenuBarVisible()) throw new Error('Expected a visible menu bar');
    }

    console.log('Menu created and refreshed without GTK');
    app.quit();
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });
