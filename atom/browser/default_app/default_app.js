var app = require('app');
var BrowserWindow = require('browser-window');

var mainWindow = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  app.quit();
});

app.on('ready', function() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    'auto-hide-menu-bar': true,
    'use-content-size': true,
  });
  mainWindow.loadUrl('file://' + __dirname + '/index.html');
  mainWindow.focus();
});
