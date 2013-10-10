var app = require('app');
var ipc = require('ipc');
var BrowserWindow = require('browser-window');
var Menu = require('menu');

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
  var template = [
    {
      label: 'Atom',
      submenu: [
        {
          label: 'Quit',
          accelerator: 'Command+Q',
          click: function(item, window) { app.quit(); }
        },
      ],
    },
    {
      label: 'Window',
      submenu: [
        {
          label: 'Open',
          accelerator: 'Command+O',
        },
        {
          label: 'Close',
          accelerator: 'Command+W',
          click: function(item, window) { window.close(); }
        },
      ]
    },
    {
      label: 'View',
      submenu: [
        {
          label: 'Reload',
          accelerator: 'Command+R',
          click: function(item, window) { window.restart(); }
        },
        {
          label: 'Enter Fullscreen',
          click: function(item, window) { window.setFullScreen(true); }
        },
        {
          label: 'Toggle DevTools',
          accelerator: 'Alt+Command+I',
          click: function(item, window) { window.toggleDevTools(); }
        },
      ]
    },
  ];

  var menu = Menu.buildFromTemplate(template);
  app.setApplicationMenu(menu);

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
