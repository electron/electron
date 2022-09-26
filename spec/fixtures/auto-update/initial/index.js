process.on('uncaughtException', (err) => {
  console.error(err);
  process.exit(1);
});

const { autoUpdater } = require('electron');

const feedUrl = process.argv[1];

console.log('Setting Feed URL');

autoUpdater.setFeedURL({
  url: feedUrl
});

console.log('Feed URL Set:', feedUrl);

process.exit(0);
