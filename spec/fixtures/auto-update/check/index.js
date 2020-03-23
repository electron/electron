process.on('uncaughtException', (err) => {
  console.error(err);
  process.exit(1);
});

const { autoUpdater } = require('electron');

autoUpdater.on('error', (err) => {
  console.error(err);
  process.exit(1);
});

const feedUrl = process.argv[1];

autoUpdater.setFeedURL({
  url: feedUrl
});

autoUpdater.checkForUpdates();

autoUpdater.on('update-not-available', () => {
  process.exit(0);
});
