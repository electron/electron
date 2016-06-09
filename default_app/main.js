const {app, dialog, shell, Menu} = require('electron')

const fs = require('fs')
const Module = require('module')
const path = require('path')
const repl = require('repl')
const url = require('url')

// Parse command line options.
const argv = process.argv.slice(1)
const option = { file: null, help: null, version: null, webdriver: null, modules: [] }
for (let i = 0; i < argv.length; i++) {
  if (argv[i] === '--version' || argv[i] === '-v') {
    option.version = true
    break
  } else if (argv[i].match(/^--app=/)) {
    option.file = argv[i].split('=')[1]
    break
  } else if (argv[i] === '--help' || argv[i] === '-h') {
    option.help = true
    break
  } else if (argv[i] === '--interactive' || argv[i] === '-i') {
    option.interactive = true
  } else if (argv[i] === '--test-type=webdriver') {
    option.webdriver = true
  } else if (argv[i] === '--require' || argv[i] === '-r') {
    option.modules.push(argv[++i])
    continue
  } else if (argv[i][0] === '-') {
    continue
  } else {
    option.file = argv[i]
    break
  }
}

// Quit when all windows are closed and no other one is listening to this.
app.on('window-all-closed', () => {
  if (app.listeners('window-all-closed').length === 1 && !option.interactive) {
    app.quit()
  }
})

// Create default menu.
app.once('ready', () => {
  if (Menu.getApplicationMenu()) return

  const template = [
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
        }
      ]
    },
    {
      label: 'View',
      submenu: [
        {
          label: 'Reload',
          accelerator: 'CmdOrCtrl+R',
          click (item, focusedWindow) {
            if (focusedWindow) focusedWindow.reload()
          }
        },
        {
          label: 'Toggle Full Screen',
          accelerator: (() => {
            return (process.platform === 'darwin') ? 'Ctrl+Command+F' : 'F11'
          })(),
          click (item, focusedWindow) {
            if (focusedWindow) focusedWindow.setFullScreen(!focusedWindow.isFullScreen())
          }
        },
        {
          label: 'Toggle Developer Tools',
          accelerator: (() => {
            return (process.platform === 'darwin') ? 'Alt+Command+I' : 'Ctrl+Shift+I'
          })(),
          click (item, focusedWindow) {
            if (focusedWindow) focusedWindow.toggleDevTools()
          }
        }
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
        }
      ]
    },
    {
      label: 'Help',
      role: 'help',
      submenu: [
        {
          label: 'Learn More',
          click () {
            shell.openExternal('http://electron.atom.io')
          }
        },
        {
          label: 'Documentation',
          click () {
            shell.openExternal(
              `https://github.com/electron/electron/tree/v${process.versions.electron}/docs#readme`
            )
          }
        },
        {
          label: 'Community Discussions',
          click () {
            shell.openExternal('https://discuss.atom.io/c/electron')
          }
        },
        {
          label: 'Search Issues',
          click () {
            shell.openExternal('https://github.com/electron/electron/issues')
          }
        }
      ]
    }
  ]

  if (process.platform === 'darwin') {
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
          accelerator: 'Command+Alt+H',
          role: 'hideothers'
        },
        {
          label: 'Show All',
          role: 'unhide'
        },
        {
          type: 'separator'
        },
        {
          label: 'Quit ' + app.getName(),
          accelerator: 'Command+Q',
          click () { app.quit() }
        }
      ]
    })
    template[3].submenu.push(
      {
        type: 'separator'
      },
      {
        label: 'Bring All to Front',
        role: 'front'
      }
    )
  }

  const menu = Menu.buildFromTemplate(template)
  Menu.setApplicationMenu(menu)
})

if (option.modules.length > 0) {
  Module._preloadModules(option.modules)
}

function loadApplicationPackage (packagePath) {
  // Add a flag indicating app is started from default app.
  process.defaultApp = true

  try {
    // Override app name and version.
    packagePath = path.resolve(packagePath)
    const packageJsonPath = path.join(packagePath, 'package.json')
    if (fs.existsSync(packageJsonPath)) {
      let packageJson
      try {
        packageJson = JSON.parse(fs.readFileSync(packageJsonPath))
      } catch (e) {
        showErrorMessage(`Unable to parse ${packageJsonPath}\n\n${e.message}`)
        return
      }

      if (packageJson.version) {
        app.setVersion(packageJson.version)
      }
      if (packageJson.productName) {
        app.setName(packageJson.productName)
      } else if (packageJson.name) {
        app.setName(packageJson.name)
      }
      app.setPath('userData', path.join(app.getPath('appData'), app.getName()))
      app.setPath('userCache', path.join(app.getPath('cache'), app.getName()))
      app.setAppPath(packagePath)
    }

    try {
      Module._resolveFilename(packagePath, module, true)
    } catch (e) {
      showErrorMessage(`Unable to find Electron app at ${packagePath}\n\n${e.message}`)
      return
    }

    // Run the app.
    Module._load(packagePath, module, true)
  } catch (e) {
    console.error('App threw an error during load')
    console.error(e.stack || e)
    throw e
  }
}

function showErrorMessage (message) {
  app.focus()
  dialog.showErrorBox('Error launching app', message)
  process.exit(1)
}

function loadApplicationByUrl (appUrl) {
  require('./default_app').load(appUrl)
}

function startRepl () {
  repl.start('> ').on('exit', () => {
    process.exit(0)
  })
}

// Start the specified app if there is one specified in command line, otherwise
// start the default app.
if (option.file && !option.webdriver) {
  const file = option.file
  const protocol = url.parse(file).protocol
  const extension = path.extname(file)
  if (protocol === 'http:' || protocol === 'https:' || protocol === 'file:') {
    loadApplicationByUrl(file)
  } else if (extension === '.html' || extension === '.htm') {
    loadApplicationByUrl('file://' + path.resolve(file))
  } else {
    loadApplicationPackage(file)
  }
} else if (option.version) {
  console.log('v' + process.versions.electron)
  process.exit(0)
} else if (option.help) {
  const helpMessage = `Electron ${process.versions.electron} - Build cross platform desktop apps with JavaScript, HTML, and CSS

  Usage: electron [options] [path]

  A path to an Electron app may be specified. The path must be one of the following:

    - index.js file.
    - Folder containing a package.json file.
    - Folder containing an index.js file.
    - .html/.htm file.
    - http://, https://, or file:// URL.

  Options:
    -h, --help            Print this usage message.
    -i, --interactive     Open a REPL to the main process.
    -r, --require         Module to preload (option can be repeated)
    -v, --version         Print the version.`
  console.log(helpMessage)
  process.exit(0)
} else if (option.interactive) {
  startRepl()
} else {
  const indexPath = path.join(__dirname, '/index.html')
  loadApplicationByUrl(`file://${indexPath}`)
}
