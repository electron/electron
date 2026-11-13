const { app, BrowserWindow } = require('electron');

const { once } = require('node:events');

app
  .whenReady()
  .then(async () => {
    const window = new BrowserWindow({ show: false });
    const contents = window.webContents;
    const preconnectPage = 'data:text/html,<link rel="preconnect" href="http://127.0.0.1:12345">';
    if (process.argv.includes('--destroy-frame-on-preconnect')) {
      const frame = contents.mainFrame;
      const session = contents.session;
      let sessionPreconnects = 0;
      let quitting = false;
      session.on('preconnect', () => sessionPreconnects++);
      process.once('exit', () =>
        console.log(`session preconnects: ${sessionPreconnects}, listeners: ${session.listenerCount('preconnect')}`)
      );
      // WebContents destruction is synchronous during shutdown.
      app.once('quit', () => contents.destroy());
      frame.once('preconnect', () => {
        quitting = true;
        app.exit();
        console.log(`frame destroyed inside listener: ${frame.isDestroyed()}`);
      });
      contents.loadURL(preconnectPage).catch((error) => {
        if (!quitting || !/ERR_(ABORTED|FAILED)/.test(error.message)) {
          console.error(error);
          app.exit(1);
        }
      });
      return;
    }

    const target = process.argv.includes('--session-preconnect') ? contents.session : contents.mainFrame;
    const preconnect = once(target, 'preconnect');
    await contents.loadURL(preconnectPage);
    await preconnect;
    console.log('preconnect observed');
    app.quit();
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });
