var app = require('app');
var atom = require('atom');
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

atom.browserMainParts.preMainMessageLoopRun = function() {
  mainWindow = new Window({ width: 800, height: 600 });
  mainWindow.loadURL('file://' + __dirname + '/index.html');

  mainWindow.on('page-title-updated', function(event, title) {
    event.preventDefault();

    this.setTitle('Atom Shell - ' + title);
  });
}
