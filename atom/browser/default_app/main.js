var app = require('app');
var dialog = require('dialog');
var fs = require('fs');
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
    // Override app name and version.
    var packagePath = path.resolve(argv._[0]);
    var packageJsonPath = path.join(packagePath, 'package.json');
    if (fs.existsSync(packageJsonPath)) {
      var packageJson = JSON.parse(fs.readFileSync(packageJsonPath));
      if (packageJson.version)
        app.setVersion(packageJson.version);
      if (packageJson.productName)
        app.setName(packageJson.productName);
      else if (packageJson.name)
        app.setName(packageJson.name);
    }

    // Run the app.
    require('module')._load(packagePath, module, true);
  } catch(e) {
    if (e.code == 'MODULE_NOT_FOUND') {
      app.focus();
      console.error(e.stack);
      dialog.showMessageBox({
        type: 'warning',
        buttons: ['OK'],
        title: 'Error opening app',
        message: 'The app provided is not a valid atom-shell app, please read the docs on how to write one:',
        detail: 'https://github.com/atom/atom-shell/tree/master/docs'
      });
      process.exit(1);
    } else {
      console.error('App throwed an error when running', e);
      throw e;
    }
  }
} else if (argv.version) {
  console.log('v' + process.versions['atom-shell']);
  process.exit(0);
} else {
  require('./default_app.js');
}
