// main.js
const { app, Notification } = require('electron');

function die(err) {
  console.error(err && err.stack ? err.stack : String(err));
  process.exit(1);
}

process.on('uncaughtException', die);
process.on('unhandledRejection', die);

app
  .whenReady()
  .then(() => {
    if (process.platform !== 'darwin') {
      console.log('skip: only repro on darwin');
      app.exit(0);
      return;
    }

    if (!Notification.isSupported()) {
      console.log('skip: Notification not supported');
      app.exit(0);
      return;
    }

    const n = new Notification({
      title: 'repro',
      body: 'reentrant close'
    });

    // Key point: Close again within the close event to create reentrancy.
    n.once('close', () => {
      console.log('[js] close event fired, closing again...');
      n.close();
    });

    app.on('before-quit', () => {
      console.log('[js] before-quit, closing notification...');
      n.close();
    });

    n.show();

    setImmediate(() => app.quit());
  })
  .catch(die);
