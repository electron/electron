const { app, BrowserWindow } = require('electron');

let win;
app.whenReady().then(function () {
  win = new BrowserWindow({});
  win.setMenu(null);

  setTimeout(() => {
    if (win.isMenuBarVisible()) {
      console.log('Window has a menu');
    } else {
      console.log('Window has no menu');
    }
    app.quit();
  });
});
