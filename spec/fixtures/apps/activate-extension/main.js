const { app, session } = require('electron');

const path = require('node:path');
const { setTimeout } = require('node:timers/promises');

app.whenReady().then(async () => {
  const ses = session.defaultSession;
  const fixtures = path.join(__dirname, '..', '..');
  const extPath = path.join(fixtures, 'extensions', 'simple-background-sw');

  const activatedPromise = new Promise((resolve) => {
    ses.serviceWorkers.on('running-status-changed', ({ versionId, runningStatus }) => {
      console.log(`service worker ${versionId} now ${runningStatus}`);
      if (runningStatus === 'running') {
        resolve();
      }
    });
  });

  await ses.loadExtension(extPath);
  await activatedPromise;

  // Need to wait for data to be flushed?
  await setTimeout(1000);

  app.exit(0);
});
