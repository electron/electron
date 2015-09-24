var app = require('app');
var ipc = require('ipc');
var dialog = require('dialog');
var path = require('path');
var BrowserWindow = require('browser-window');

var window = null;
process.port = 0;  // will be used by crash-reporter spec.

app.commandLine.appendSwitch('js-flags', '--expose_gc');
app.commandLine.appendSwitch('ignore-certificate-errors');

// Accessing stdout in the main process will result in the process.stdout
// throwing UnknownSystemError in renderer process sometimes. This line makes
// sure we can reproduce it in renderer process.
process.stdout;

ipc.on('message', function(event, arg) {
  event.sender.send('message', arg);
});

ipc.on('console.log', function(event, args) {
  console.error.apply(console, args);
});

ipc.on('console.error', function(event, args) {
  console.error.apply(console, args);
});

ipc.on('process.exit', function(event, code) {
  process.exit(code);
});

ipc.on('eval', function(event, script) {
  event.returnValue = eval(script);
});

ipc.on('echo', function(event, msg) {
  event.returnValue = msg;
});

if (process.argv[2] == '--ci') {
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
  require('protocol').registerStringProtocol('test-if-crashes', function() {});

  window = new BrowserWindow({
    title: 'Electron Tests',
    show: false,
    width: 800,
    height: 600,
    'web-preferences': {
      javascript: true  // Test whether web-preferences crashes.
    },
  });
  window.loadUrl('file://' + __dirname + '/index.html');
  window.on('unresponsive', function() {
    var chosen = dialog.showMessageBox(window, {
      type: 'warning',
      buttons: ['Close', 'Keep Waiting'],
      message: 'Window is not responsing',
      detail: 'The window is not responding. Would you like to force close it or just keep waiting?'
    });
    if (chosen == 0) window.destroy();
  });

  // For session's download test, listen 'will-download' event in browser, and
  // reply the result to renderer for verifying
  var downloadFilePath = path.join(__dirname, '..', 'fixtures', 'mock.pdf');
  require('ipc').on('set-download-option', function(event, need_cancel) {
    window.webContents.session.once('will-download',
        function(e, item, webContents) {
          item.setSavePath(downloadFilePath);
          item.on('done', function(e, state) {
            window.webContents.send('download-done',
                                    state,
                                    item.getUrl(),
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
