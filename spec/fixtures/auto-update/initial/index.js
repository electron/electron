process.on('uncaughtException', (err) => {
  console.error(err);
  process.exit(1);
});

const { autoUpdater } = require('electron');

const feedUrl = process.argv[1];
const serverType = process.argv[2];

console.log('Setting Feed URL');

const opts = { url: feedUrl };
if (serverType) {
  opts.serverType = serverType;
}

autoUpdater.setFeedURL(opts);

console.log('Feed URL Set:', feedUrl);
console.log('Server Type Set:', serverType);

process.exit(0);
