var app = require('app');
var ipc = require('ipc');
var BrowserWindow = require('browser-window');

var window = null;

app.commandLine.appendSwitch('js-flags', '--expose_gc');

ipc.on('message', function() {
  ipc.send.apply(this, arguments);
});

ipc.on('console.log', function(pid, rid, args) {
  console.log.apply(console, args);
});

ipc.on('console.error', function(pid, rid, args) {
  console.log.apply(console, args);
});

ipc.on('process.exit', function(pid, rid, code) {
  process.exit(code);
});

process.on('uncaughtException', function() {
  window.openDevTools();
});

app.on('window-all-closed', function() {
  app.terminate();
});

app.on('finish-launching', function() {
  window = new BrowserWindow({
    title: 'atom-shell tests',
    width: 800,
    height: 600
  });
  window.loadUrl('file://' + __dirname + '/index.html');
  window.focus();
});
