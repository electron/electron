const { app, BrowserWindow } = require('electron');

app.whenReady().then(() => {
  const w = new BrowserWindow({
    x: 21,
    y: 13,
    width: 401,
    height: 301,
    frame: !process.argv.includes('--frameless'),
    show: false
  });
  const before = w.getBounds();
  for (let i = 0; i < 5; i++) {
    w.setBounds(w.getBounds());
  }
  console.log(JSON.stringify({ before, after: w.getBounds() }));
  app.quit();
});
