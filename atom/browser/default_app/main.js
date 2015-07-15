var app = require('app');
var dialog = require('dialog');
var fs = require('fs');
var path = require('path');
var Menu = require('menu');
var BrowserWindow = require('browser-window');

// Quit when all windows are closed and no other one is listening to this.
app.on('window-all-closed', function() {
  if (app.listeners('window-all-closed').length == 1)
    app.quit();
});

// Parse command line options.
var argv = process.argv.slice(1);
var option = { file: null, help: null, version: null, webdriver: null };
for (var i in argv) {
  if (argv[i] == '--version' || argv[i] == '-v') {
    option.version = true;
    break;
  } else if (argv[i] == '--help' || argv[i] == '-h') {
    option.help = true;
    break;
  } else if (argv[i] == '--test-type=webdriver') {
    option.webdriver = true;
  } else if (argv[i][0] == '-') {
    continue;
  } else {
    option.file = argv[i];
    break;
  }
}

// Create default menu.
app.once('ready', function() {
  if (Menu.getApplicationMenu())
    return;

  var template;
  if (process.platform == 'darwin') {
    template = [
      {
        label: 'Electron',
        submenu: [
          {
            label: 'About Electron',
            selector: 'orderFrontStandardAboutPanel:'
          },
          {
            type: 'separator'
          },
          {
            label: 'Services',
            submenu: []
          },
          {
            type: 'separator'
          },
          {
            label: 'Hide Electron',
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
            click: function() {
              var focusedWindow = BrowserWindow.getFocusedWindow();
              if (focusedWindow)
                focusedWindow.reload();
            }
          },
          {
            label: 'Toggle Full Screen',
            accelerator: 'Ctrl+Command+F',
            click: function() {
              var focusedWindow = BrowserWindow.getFocusedWindow();
              if (focusedWindow)
                focusedWindow.setFullScreen(!focusedWindow.isFullScreen());
            }
          },
          {
            label: 'Toggle Developer Tools',
            accelerator: 'Alt+Command+I',
            click: function() {
              var focusedWindow = BrowserWindow.getFocusedWindow();
              if (focusedWindow)
                focusedWindow.toggleDevTools();
            }
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
      {
        label: 'Help',
        submenu: [
          {
            label: 'Learn More',
            click: function() { require('shell').openExternal('http://electron.atom.io') }
          },
          {
            label: 'Documentation',
            click: function() { require('shell').openExternal('https://github.com/atom/electron/tree/master/docs#readme') }
          },
          {
            label: 'Community Discussions',
            click: function() { require('shell').openExternal('https://discuss.atom.io/c/electron') }
          },
          {
            label: 'Search Issues',
            click: function() { require('shell').openExternal('https://github.com/atom/electron/issues') }
          }
        ]
      }
    ];
  } else {
    template = [
      {
        label: '&File',
        submenu: [
          {
            label: '&Open',
            accelerator: 'Ctrl+O',
          },
          {
            label: '&Close',
            accelerator: 'Ctrl+W',
            click: function() {
              var focusedWindow = BrowserWindow.getFocusedWindow();
              if (focusedWindow)
                focusedWindow.close();
            }
          },
        ]
      },
      {
        label: '&View',
        submenu: [
          {
            label: '&Reload',
            accelerator: 'Ctrl+R',
            click: function() {
              var focusedWindow = BrowserWindow.getFocusedWindow();
              if (focusedWindow)
                focusedWindow.reload();
            }
          },
          {
            label: 'Toggle &Full Screen',
            accelerator: 'F11',
            click: function() {
              var focusedWindow = BrowserWindow.getFocusedWindow();
              if (focusedWindow)
                focusedWindow.setFullScreen(!focusedWindow.isFullScreen());
            }
          },
          {
            label: 'Toggle &Developer Tools',
            accelerator: 'Shift+Ctrl+I',
            click: function() {
              var focusedWindow = BrowserWindow.getFocusedWindow();
              if (focusedWindow)
                focusedWindow.toggleDevTools();
            }
          },
        ]
      },
      {
        label: 'Help',
        submenu: [
          {
            label: 'Learn More',
            click: function() { require('shell').openExternal('http://electron.atom.io') }
          },
          {
            label: 'Documentation',
            click: function() { require('shell').openExternal('https://github.com/atom/electron/tree/master/docs#readme') }
          },
          {
            label: 'Community Discussions',
            click: function() { require('shell').openExternal('https://discuss.atom.io/c/electron') }
          },
          {
            label: 'Search Issues',
            click: function() { require('shell').openExternal('https://github.com/atom/electron/issues') }
          }
        ]
      }
    ];
  }

  var menu = Menu.buildFromTemplate(template);
  Menu.setApplicationMenu(menu);
});

// Start the specified app if there is one specified in command line, otherwise
// start the default app.
if (option.file && !option.webdriver) {
  try {
    // Override app name and version.
    var packagePath = path.resolve(option.file);
    var packageJsonPath = path.join(packagePath, 'package.json');
    if (fs.existsSync(packageJsonPath)) {
      var packageJson = JSON.parse(fs.readFileSync(packageJsonPath));
      if (packageJson.version)
        app.setVersion(packageJson.version);
      if (packageJson.productName)
        app.setName(packageJson.productName);
      else if (packageJson.name)
        app.setName(packageJson.name);
      app.setPath('userData', path.join(app.getPath('appData'), app.getName()));
      app.setPath('userCache', path.join(app.getPath('cache'), app.getName()));
      app.setAppPath(packagePath);
    }

    // Run the app.
    require('module')._load(packagePath, module, true);
  } catch(e) {
    if (e.code == 'MODULE_NOT_FOUND') {
      app.focus();
      dialog.showErrorBox('Error opening app', 'The app provided is not a valid electron app, please read the docs on how to write one:\nhttps://github.com/atom/electron/tree/master/docs\n\n' + e.toString());
      process.exit(1);
    } else {
      console.error('App threw an error when running', e);
      throw e;
    }
  }
} else if (option.version) {
  console.log('v' + process.versions.electron);
  process.exit(0);
} else if (option.help) {
  var helpMessage = "Electron v" + process.versions.electron + " - Cross Platform Desktop Application Shell\n\n";
  helpMessage    += "Usage: electron [options] [path]\n\n";
  helpMessage    += "A path to an Electron application may be specified. The path must be to \n";
  helpMessage    += "an index.js file or to a folder containing a package.json or index.js file.\n\n";
  helpMessage    += "Options:\n";
  helpMessage    += "  -h, --help            Print this usage message.\n";
  helpMessage    += "  -v, --version         Print the version.";
  console.log(helpMessage);
  process.exit(0);
} else {
  require('./default_app.js');
}
