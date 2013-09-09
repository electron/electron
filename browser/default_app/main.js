var app = require('app');
var argv = require('optimist').argv;
var dialog = require('dialog');
var path = require('path');

// Quit when all windows are closed and no other one is listening to this.
app.on('window-all-closed', function() {
  if (app.listeners('window-all-closed').length == 1)
    app.quit();
});

// Start the specified app if there is one specified in command line, otherwise
// start the default app.
if (argv._.length > 0) {
  try {
    require(path.resolve(argv._[0]));
  } catch(e) {
    if (e.code == 'MODULE_NOT_FOUND') {
      console.error(e.stack);
      console.error('Specified app is invalid');
      process.exit(1);
    } else {
      throw e;
    }
  }
} else {
  require('./default_app.js');
}
