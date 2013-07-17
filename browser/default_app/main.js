var argv = require('optimist').argv;
var dialog = require('dialog');
var path = require('path');

// Start the specified app if there is one specified in command line, otherwise
// start the default app.
if (argv._.length > 0) {
  process.on('uncaughtException', function() {
    process.exit(1);
  });

  require(path.resolve(argv._[0]));
} else {
  require('./default_app.js');
}
