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
  ev.result = eval(script);
});

process.on('uncaughtException', function() {
  window.openDevTools();
});

app.on('window-all-closed', function() {
  app.terminate();
});

app.on('will-finish-launching', function() {
  // Reigster some protocols, used by the protocol spec.
  // FIXME(zcbenz): move this to somewhere else.
  var protocol = require('protocol');
  protocol.registerProtocol('atom-string', function(url) {
    return url;
  });
});

app.on('finish-launching', function() {
  window = new BrowserWindow({
    title: 'atom-shell tests',
    show: false,
    width: 800,
    height: 600
  });
  window.loadUrl('file://' + __dirname + '/index.html');
});
