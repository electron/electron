var app = require('app');
var path = require('path');
var optimist = require('optimist');

// Quit when all windows are closed and no other one is listening to this.
app.on('window-all-closed', function() {
  if (app.listeners('window-all-closed').length == 1)
    app.quit();
});

var argv = optimist(process.argv.slice(1)).boolean('ci').argv;

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
} else if (argv.version) {
  console.log('v' + process.versions['atom-shell']);
  process.exit(0);
} else {
  require('./default_app.js');
}
