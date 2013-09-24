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

ipc.on('eval', function(ev, pid, rid, script) {
  ev.returnValue = eval(script);
});

ipc.on('echo', function(ev, pid, rid, msg) {
  ev.returnValue = msg;
});

process.on('uncaughtException', function() {
  window.openDevTools();
});

app.on('window-all-closed', function() {
  app.terminate();
});

app.on('finish-launching', function() {
  // Test if using protocol module would crash.
  require('protocol').registerProtocol('test-if-crashes', function() {});

  window = new BrowserWindow({
    title: 'atom-shell tests',
    show: false,
    width: 800,
    height: 600
  });
  window.loadUrl('file://' + __dirname + '/index.html');
});
