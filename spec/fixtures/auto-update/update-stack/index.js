const { app, autoUpdater } = require('electron');

const fs = require('node:fs');
const path = require('node:path');

process.on('uncaughtException', (err) => {
  console.error(err);
  process.exit(1);
});

autoUpdater.on('error', (err) => {
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

  let updateStackCount = 0;

  autoUpdater.on('update-downloaded', () => {
    updateStackCount++;
    console.log('Update Downloaded');
    if (updateStackCount > 1) {
      autoUpdater.quitAndInstall();
    } else {
      setTimeout(() => {
        autoUpdater.checkForUpdates();
      }, 1000);
    }
  });

  autoUpdater.on('update-not-available', () => {
    console.error('No update available');
    process.exit(1);
  });
}
