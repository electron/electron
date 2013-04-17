var atom = require('atom');
var Window = require('window');

var mainWindow = null;

atom.browserMainParts.preMainMessageLoopRun = function() {
  mainWindow = new Window({ width: 800, height: 600 });

  setTimeout(function() {
    mainWindow.destroy();
  }, 10000);
}
