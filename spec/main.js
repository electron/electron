var app = require('app');
var BrowserWindow = require('browser-window');

var window = null;

app.on('finish-launching', function() {
  window = new BrowserWindow({
    title: 'atom-shell tests',
    width: 800,
    height: 600
  });
  window.loadUrl('file://' + __dirname + '/index.html');
  window.focus();
});
