var app = require('app');
var delegate = require('atom_delegate');
var ipc = require('ipc');
var Window = require('window');

var mainWindow = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  app.terminate();
});

// Echo every message back.
ipc.on('message', function(process_id, routing_id) {
  ipc.send.apply(ipc, arguments);
});

ipc.on('sync-message', function(event, process_id, routing_id) {
  event.result = arguments;
});

delegate.browserMainParts.preMainMessageLoopRun = function() {
  mainWindow = new Window({ width: 800, height: 600 });
  mainWindow.loadUrl('file://' + __dirname + '/index.html');

  mainWindow.on('page-title-updated', function(event, title) {
    event.preventDefault();

    this.setTitle('Atom Shell - ' + title);
  });

  mainWindow.on('closed', function() {
    console.log('closed');
    mainWindow = null;
  });
}
