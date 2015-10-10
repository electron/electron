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
var option = { file: null, help: null, version: null, webdriver: null, modules: [] };
for (var i = 0; i < argv.length; i++) {
  if (argv[i] == '--version' || argv[i] == '-v') {
    option.version = true;
    break;
  } else if (argv[i].match(/^--app=/)) {
    option.file = argv[i].split('=')[1];
    break;
  } else if (argv[i] == '--help' || argv[i] == '-h') {
    option.help = true;
    break;
  } else if (argv[i] == '--test-type=webdriver') {
    option.webdriver = true;
  } else if (argv[i] == '--require' || argv[i] == '-r') {
    option.modules.push(argv[++i]);
    continue;
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

  var template = [
    {
      label: 'Edit',
      submenu: [
        {
          label: 'Undo',
          accelerator: 'CmdOrCtrl+Z',
          role: 'undo'
        },
        {
          label: 'Redo',
          accelerator: 'Shift+CmdOrCtrl+Z',
          role: 'redo'
        },
        {
          type: 'separator'
        },
        {
          label: 'Cut',
          accelerator: 'CmdOrCtrl+X',
          role: 'cut'
        },
        {
          label: 'Copy',
          accelerator: 'CmdOrCtrl+C',
          role: 'copy'
        },
        {
          label: 'Paste',
          accelerator: 'CmdOrCtrl+V',
          role: 'paste'
        },
        {
          label: 'Select All',
          accelerator: 'CmdOrCtrl+A',
          role: 'selectall'
        },
      ]
    },
    {
      label: 'View',
      submenu: [
        {
          label: 'Reload',
          accelerator: 'CmdOrCtrl+R',
          click: function(item, focusedWindow) {
            if (focusedWindow)
              focusedWindow.reload();
          }
        },
        {
          label: 'Toggle Full Screen',
          accelerator: (function() {
            if (process.platform == 'darwin')
              return 'Ctrl+Command+F';
            else
              return 'F11';
          })(),
          click: function(item, focusedWindow) {
            if (focusedWindow)
              focusedWindow.setFullScreen(!focusedWindow.isFullScreen());
          }
        },
        {
          label: 'Toggle Developer Tools',
          accelerator: (function() {
            if (process.platform == 'darwin')
              return 'Alt+Command+I';
            else
              return 'Ctrl+Shift+I';
          })(),
          click: function(item, focusedWindow) {
            if (focusedWindow)
              focusedWindow.toggleDevTools();
          }
        },
      ]
    },
    {
      label: 'Window',
      role: 'window',
      submenu: [
        {
          label: 'Minimize',
          accelerator: 'CmdOrCtrl+M',
          role: 'minimize'
        },
        {
          label: 'Close',
          accelerator: 'CmdOrCtrl+W',
          role: 'close'
        },
      ]
    },
    {
      label: 'Help',
      role: 'help',
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
    },
  ];

  if (process.platform == 'darwin') {
    template.unshift({
      label: 'Electron',
      submenu: [
        {
          label: 'About Electron',
          role: 'about'
        },
        {
          type: 'separator'
        },
        {
          label: 'Services',
          role: 'services',
          submenu: []
        },
        {
          type: 'separator'
        },
        {
          label: 'Hide Electron',
          accelerator: 'Command+H',
          role: 'hide'
        },
        {
          label: 'Hide Others',
          accelerator: 'Command+Shift+H',
          role: 'hideothers:'
        },
        {
          label: 'Show All',
          role: 'unhide:'
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
    });
    template[3].submenu.push(
      {
        type: 'separator'
      },
      {
        label: 'Bring All to Front',
        role: 'front'
      }
    );
  }

  var menu = Menu.buildFromTemplate(template);
  Menu.setApplicationMenu(menu);
});

if (option.modules.length > 0) {
  require('module')._preloadModules(option.modules);
}

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
  helpMessage    += "  -r, --require         Module to preload (option can be repeated)\n";
  helpMessage    += "  -h, --help            Print this usage message.\n";
  helpMessage    += "  -v, --version         Print the version.";
  console.log(helpMessage);
  process.exit(0);
} else {
  require('./default_app.js');
}
