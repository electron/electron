var app = require('app');
var dialog = require('dialog');
var delegate = require('atom-delegate');
var ipc = require('ipc');
var Menu = require('menu');
var MenuItem = require('menu-item');
var BrowserWindow = require('browser-window');

var mainWindow = null;
var menu = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  app.terminate();
});

app.on('open-url', function(event, url) {
  dialog.showMessageBox({message: url, buttons: ['OK']});
});

app.on('finish-launching', function() {
  app.commandLine.appendSwitch('js-flags', '--harmony_collections');

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

  mainWindow.on('unresponsive', function() {
    console.log('unresponsive');
  });

  var template = [
    {
      label: 'Atom Shell',
      submenu: [
        {
          label: 'About Atom Shell',
          selector: 'orderFrontStandardAboutPanel:'
        },
        {
          type: 'separator'
        },
        {
          label: 'Hide Atom Shell',
          accelerator: 'Command+H',
          selector: 'hide:'
        },
        {
          label: 'Hide Others',
          accelerator: 'Command+Shift+H',
          selector: 'hideOtherApplications:'
        },
        {
          label: 'Show All',
          selector: 'unhideAllApplications:'
        },
        {
          type: 'separator'
        },
        {
          label: 'Quit',
          accelerator: 'Command+Q',
          click: function() { app.quit(); }
        },
      ]
    },
    {
      label: 'Edit',
      submenu: [
        {
          label: 'Undo',
          accelerator: 'Command+Z',
          selector: 'undo:'
        },
        {
          label: 'Redo',
          accelerator: 'Shift+Command+Z',
          selector: 'redo:'
        },
        {
          type: 'separator'
        },
        {
          label: 'Cut',
          accelerator: 'Command+X',
          selector: 'cut:'
        },
        {
          label: 'Copy',
          accelerator: 'Command+C',
          selector: 'copy:'
        },
        {
          label: 'Paste',
          accelerator: 'Command+V',
          selector: 'paste:'
        },
        {
          label: 'Select All',
          accelerator: 'Command+A',
          selector: 'selectAll:'
        },
      ]
    },
    {
      label: 'View',
      submenu: [
        {
          label: 'Reload',
          accelerator: 'Command+R',
          click: function() { BrowserWindow.getFocusedWindow().restart(); }
        },
        {
          label: 'Enter Fullscreen',
          click: function() { BrowserWindow.getFocusedWindow().setFullscreen(true); }
        },
        {
          label: 'Toggle DevTools',
          accelerator: 'Alt+Command+I',
          click: function() { BrowserWindow.getFocusedWindow().toggleDevTools(); }
        },
      ]
    },
    {
      label: 'Window',
      submenu: [
        {
          label: 'Minimize',
          accelerator: 'Command+M',
          selector: 'performMiniaturize:'
        },
        {
          label: 'Close',
          accelerator: 'Command+W',
          selector: 'performClose:'
        },
        {
          type: 'separator'
        },
        {
          label: 'Bring All to Front',
          selector: 'arrangeInFront:'
        },
      ]
    },
  ];

  menu = Menu.buildFromTemplate(template);

  if (process.platform == 'darwin')
    Menu.setApplicationMenu(menu);
  else
    mainWindow.setMenu(menu);

  ipc.on('message', function(processId, routingId, type) {
    console.log(type);
    if (type == 'menu')
      menu.popup(mainWindow);
  });
});
