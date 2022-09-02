const { app, BrowserWindow } = require('electron');

const ints = (...args) => args.map(a => parseInt(a, 10));

const [x, y, width, height] = ints(...process.argv.slice(2));

let w;

app.whenReady().then(() => {
  w = new BrowserWindow({
    x,
    y,
    width,
    height
  });
  console.log('__ready__');
});

process.on('SIGTERM', () => {
  process.exit(0);
});
