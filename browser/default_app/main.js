var app = require('app');
var delegate = require('atom_delegate');
var ipc = require('ipc');
var Menu = require('menu');
var BrowserWindow = require('browser_window');

var mainWindow = null;
var menu = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  app.terminate();
});

delegate.browserMainParts.preMainMessageLoopRun = function() {
  mainWindow = new BrowserWindow({ width: 800, height: 600 });
  mainWindow.loadUrl('file://' + __dirname + '/index.html');

  mainWindow.on('page-title-updated', function(event, title) {
    event.preventDefault();

    this.setTitle('Atom Shell - ' + title);
  });

  mainWindow.on('closed', function() {
    console.log('closed');
    mainWindow = null;
  });

  menu = new Menu;
  menu.appendItem(0, 'Open GitHub');

  menu.delegate = {
    getAcceleratorForCommandId: function(commandId) {
      if (commandId == 0)
        return 'Ctrl+g';
    }
  }

  menu.on('execute', function(commandId) {
    if (commandId == 0)
      mainWindow.loadUrl('https://github.com');
  });

  ipc.on('message', function(processId, routingId, type) {
    if (type == 'menu')
      menu.popup(mainWindow);
  });
}
