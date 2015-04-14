var app = require('app');
var dialog = require('dialog');
var fs = require('fs');
var path = require('path');

// Quit when all windows are closed and no other one is listening to this.
app.on('window-all-closed', function() {
  if (app.listeners('window-all-closed').length == 1)
    app.quit();
});

// Parse command line options.
var argv = process.argv.slice(1);
var option = { file: null, version: null, webdriver: null };
for (var i in argv) {
  if (argv[i] == '--version' || argv[i] == '-v') {
    option.version = true;
    break;
  } else if (argv[i] == '--test-type=webdriver') {
    option.webdriver = true;
  } else if (argv[i][0] == '-') {
    continue;
  } else {
    option.file = argv[i];
    break;
  }
}

// Start the specified app if there is one specified in command line, otherwise
// start the default app.
if (option.file && !option.webdriver) {
  try {
    // Override app name and version.
    var packagePath = path.resolve(option.file);
    var packageJsonPath = path.join(packagePath, 'package.json');
    if (fs.existsSync(packageJsonPath)) {
      var packageJson = JSON.parse(fs.readFileSync(packageJsonPath));
      if (packageJson.version)
        app.setVersion(packageJson.version);
      if (packageJson.productName)
        app.setName(packageJson.productName);
      else if (packageJson.name)
        app.setName(packageJson.name);
      app.setPath('userData', path.join(app.getPath('appData'), app.getName()));
      app.setPath('userCache', path.join(app.getPath('cache'), app.getName()));
    }

    // Run the app.
    require('module')._load(packagePath, module, true);
  } catch(e) {
    if (e.code == 'MODULE_NOT_FOUND') {
      app.focus();
      dialog.showErrorBox('Error opening app', 'The app provided is not a valid electron app, please read the docs on how to write one:\nhttps://github.com/atom/electron/tree/master/docs');
      process.exit(1);
    } else {
      console.error('App threw an error when running', e);
      throw e;
    }
  }
} else if (option.version) {
  console.log('v' + process.versions['electron']);
  process.exit(0);
} else {
  require('./default_app.js');
}
