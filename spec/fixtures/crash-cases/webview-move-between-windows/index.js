const { app, BrowserWindow, WebContentsView } = require('electron');

function createWindow () {
  // Create the browser window.
  const mainWindow = new BrowserWindow();
  const secondaryWindow = new BrowserWindow();

  const contentsView = new WebContentsView();
  mainWindow.contentView.addChildView(contentsView);
  mainWindow.webContents.setDevToolsWebContents(contentsView.webContents);
  mainWindow.openDevTools();

  contentsView.setBounds({
    x: 400,
    y: 0,
    width: 400,
    height: 600
  });

  setTimeout(() => {
    secondaryWindow.contentView.addChildView(contentsView);
    setTimeout(() => {
      mainWindow.contentView.addChildView(contentsView);
      app.quit();
    }, 1000);
  }, 1000);
}

app.whenReady().then(() => {
  createWindow();
});
