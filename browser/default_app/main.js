var atom = require('atom');
var Window = require('window');

var mainWindow = null;

atom.browserMainParts.preMainMessageLoopRun = function() {
  mainWindow = new Window({ width: 800, height: 600 });
  mainWindow.url = 'file://' + __dirname + '/index.html';

  console.log(mainWindow.id);

  mainWindow.on('page-title-updated', function(event, title) {
    event.preventDefault();

    this.title = 'Atom Shell - ' + title;
  });
}
