const fs = require('fs');
const path = require('path');

process.on('uncaughtException', (err) => {
  console.error(err);
  process.exit(1);
});

const { app, autoUpdater } = require('electron');

autoUpdater.on('error', (err) => {
  console.error(err);
  process.exit(1);
});

const urlPath = path.resolve(__dirname, '../../../../url.txt');
let feedUrl = process.argv[1];
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

autoUpdater.on('update-downloaded', () => {
  console.log('Update Downloaded');
  autoUpdater.quitAndInstall();
});

autoUpdater.on('update-not-available', () => {
  console.error('No update available');
  process.exit(1);
});
