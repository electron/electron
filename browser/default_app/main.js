var argv = require('optimist').argv;
var dialog = require('dialog');
var path = require('path');

// Start the specified app if there is one specified in command line, otherwise
// start the default app.
if (argv._.length > 0) {
  try {
    require(path.resolve(argv._[0]));
  } catch(e) {
    if (e.code == 'MODULE_NOT_FOUND') {
      console.error('Specified app is invalid');
      process.exit(1);
    } else {
      throw e;
    }
  }
} else {
  require('./default_app.js');
}
