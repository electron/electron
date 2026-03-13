const { app, autoUpdater } = require('electron');

const fs = require('node:fs');
const path = require('node:path');

process.on('uncaughtException', (err) => {
  console.error(err);
  process.exit(1);
});

let installInvoked = false;

autoUpdater.on('error', (err) => {
  // Once quitAndInstall() has been invoked the second in-flight check may
  // surface a cancellation/network error as the process tears down; ignore
  // errors after that point so we test the actual install race, not teardown.
  if (installInvoked) {
    console.log('Ignoring post-install error:', err && err.message);
    return;
  }
  console.error(err);
  process.exit(1);
});

const urlPath = path.resolve(__dirname, '../../../../url.txt');
let feedUrl = process.argv[1];

if (feedUrl === 'remain-open') {
  // Hold the event loop
  setInterval(() => {});
} else {
  if (!feedUrl || !feedUrl.startsWith('http')) {
    feedUrl = `${fs.readFileSync(urlPath, 'utf8')}/${app.getVersion()}`;
  } else {
    fs.writeFileSync(urlPath, `${feedUrl}/updated`);
  }

  autoUpdater.setFeedURL({
    url: feedUrl
  });

  autoUpdater.checkForUpdates();

  autoUpdater.on('update-available', () => {
    console.log('Update Available');
  });

  let downloadedOnce = false;

  autoUpdater.on('update-downloaded', () => {
    console.log('Update Downloaded');
    if (!downloadedOnce) {
      downloadedOnce = true;
      // Simulate a periodic update check firing after an update was already
      // staged. The test server is expected to stall this second download so
      // that it remains in flight while we call quitAndInstall().
      // The short delay lets checkForUpdatesCommand's RACCommand executing
      // state settle; calling immediately would hit the command's "disabled"
      // guard since RACCommand disallows concurrent execution.
      setTimeout(() => {
        autoUpdater.checkForUpdates();
        // Give Squirrel enough time to enter the second check (creating a new
        // temporary directory, which with the regression prunes the directory
        // that the staged update lives in) before invoking the install.
        setTimeout(() => {
          console.log('Calling quitAndInstall mid-download');
          installInvoked = true;
          autoUpdater.quitAndInstall();
        }, 3000);
      }, 1000);
    } else {
      // Should not reach here — the second download is stalled on purpose.
      console.log('Unexpected second download completion');
      autoUpdater.quitAndInstall();
    }
  });

  autoUpdater.on('update-not-available', () => {
    console.error('No update available');
    process.exit(1);
  });
}
