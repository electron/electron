var atom = require('atom');
var ipc = require('ipc');
var Window = require('window');

var mainWindow = null;

// Echo every message back.
ipc.on('message', function(process_id, routing_id) {
  ipc.send.apply(ipc, arguments);
});

atom.browserMainParts.preMainMessageLoopRun = function() {
  mainWindow = new Window({ width: 800, height: 600 });
  mainWindow.url = 'file://' + __dirname + '/index.html';

  mainWindow.on('page-title-updated', function(event, title) {
    event.preventDefault();

    this.title = 'Atom Shell - ' + title;
  });
}
