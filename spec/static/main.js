// Disable use of deprecated functions.
process.throwDeprecation = true;

const electron      = require('electron');
const app           = electron.app;
const ipcMain       = electron.ipcMain;
const dialog        = electron.dialog;
const BrowserWindow = electron.BrowserWindow;

const path = require('path');
const url  = require('url');

var argv = require('yargs')
  .boolean('ci')
  .string('g').alias('g', 'grep')
  .boolean('i').alias('i', 'invert')
  .argv;

var window = null;
process.port = 0;  // will be used by crash-reporter spec.

app.commandLine.appendSwitch('js-flags', '--expose_gc');
app.commandLine.appendSwitch('ignore-certificate-errors');
app.commandLine.appendSwitch('disable-renderer-backgrounding');

// Accessing stdout in the main process will result in the process.stdout
// throwing UnknownSystemError in renderer process sometimes. This line makes
// sure we can reproduce it in renderer process.
process.stdout;

ipcMain.on('message', function(event, arg) {
  event.sender.send('message', arg);
});

ipcMain.on('console.log', function(event, args) {
  console.error.apply(console, args);
});

ipcMain.on('console.error', function(event, args) {
  console.error.apply(console, args);
});

ipcMain.on('process.exit', function(event, code) {
  process.exit(code);
});

ipcMain.on('eval', function(event, script) {
  event.returnValue = eval(script);
});

ipcMain.on('echo', function(event, msg) {
  event.returnValue = msg;
});

global.isCi = !!argv.ci;
if (global.isCi) {
  process.removeAllListeners('uncaughtException');
  process.on('uncaughtException', function(error) {
    console.error(error, error.stack);
    process.exit(1);
  });
}

app.on('window-all-closed', function() {
  app.quit();
});

app.on('ready', function() {
  // Test if using protocol module would crash.
  electron.protocol.registerStringProtocol('test-if-crashes', function() {});

  window = new BrowserWindow({
    title: 'Electron Tests',
    show: false,
    width: 800,
    height: 600,
    'web-preferences': {
      javascript: true  // Test whether web-preferences crashes.
    },
  });
  window.loadURL(url.format({
    pathname: __dirname + '/index.html',
    protocol: 'file',
    query: {
      grep: argv.grep,
      invert: argv.invert ? 'true': ''
    }
  }));
  window.on('unresponsive', function() {
    var chosen = dialog.showMessageBox(window, {
      type: 'warning',
      buttons: ['Close', 'Keep Waiting'],
      message: 'Window is not responsing',
      detail: 'The window is not responding. Would you like to force close it or just keep waiting?'
    });
    if (chosen === 0) window.destroy();
  });

  // For session's download test, listen 'will-download' event in browser, and
  // reply the result to renderer for verifying
  var downloadFilePath = path.join(__dirname, '..', 'fixtures', 'mock.pdf');
  ipcMain.on('set-download-option', function(event, need_cancel) {
    window.webContents.session.once('will-download',
        function(e, item, webContents) {
          item.setSavePath(downloadFilePath);
          item.on('done', function(e, state) {
            window.webContents.send('download-done',
                                    state,
                                    item.getURL(),
                                    item.getMimeType(),
                                    item.getReceivedBytes(),
                                    item.getTotalBytes(),
                                    item.getContentDisposition(),
                                    item.getFilename());
          });
          if (need_cancel)
            item.cancel();
        });
    event.returnValue = "done";
  });
});
