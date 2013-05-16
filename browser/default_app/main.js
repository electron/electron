var app = require('app');
var delegate = require('atom_delegate');
var ipc = require('ipc');
var Menu = require('menu');
var MenuItem = require('menu_item');
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

  var appleMenu = new Menu;
  appleMenu.append(new MenuItem({
    label: 'About Atom Shell',
    click: function() {
      Menu.sendActionToFirstResponder('orderFrontStandardAboutPanel:');
    }
  }));
  appleMenu.append(new MenuItem({ type: 'separator' }));
  appleMenu.append(new MenuItem({
    label: 'Hide Atom Shell',
    accelerator: 'Command+H',
    click: function() {
      Menu.sendActionToFirstResponder('hide:');
    }
  }));
  appleMenu.append(new MenuItem({
    label: 'Hide Others',
    accelerator: 'Command+Shift+H',
    click: function() {
      Menu.sendActionToFirstResponder('hideOtherApplications:');
    }
  }));
  appleMenu.append(new MenuItem({ type: 'separator' }));
  appleMenu.append(new MenuItem({
    label: 'Quit',
    accelerator: 'Command+Q',
    click: function() {
      app.quit();
    }
  }));

  menu.append(new MenuItem({ type: 'submenu', submenu: appleMenu }));

  Menu.setApplicationMenu(menu);

  ipc.on('message', function(processId, routingId, type) {
    if (type == 'menu')
      menu.popup(mainWindow);
  });
}
