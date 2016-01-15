const fs = require('fs');
const path = require('path');
const util = require('util');
const Module = require('module');

var slice = [].slice;

// We modified the original process.argv to let node.js load the atom.js,
// we need to restore it here.
process.argv.splice(1, 1);

// Clear search paths.
require(path.resolve(__dirname, '..', '..', 'common', 'lib', 'reset-search-paths'));

// Import common settings.
require(path.resolve(__dirname, '..', '..', 'common', 'lib', 'init'));

var globalPaths = Module.globalPaths;

if (!process.env.ELECTRON_HIDE_INTERNAL_MODULES) {
  globalPaths.push(path.resolve(__dirname, '..', 'api', 'lib'));
}

// Expose public APIs.
globalPaths.push(path.resolve(__dirname, '..', 'api', 'lib', 'exports'));

if (process.platform === 'win32') {
  // Redirect node's console to use our own implementations, since node can not
  // handle console output when running as GUI program.
  var consoleLog = function() {
    var args;
    args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
    return process.log(util.format.apply(util, args) + "\n");
  };
  var streamWrite = function(chunk, encoding, callback) {
    if (Buffer.isBuffer(chunk)) {
      chunk = chunk.toString(encoding);
    }
    process.log(chunk);
    if (callback) {
      callback();
    }
    return true;
  };
  console.log = console.error = console.warn = consoleLog;
  process.stdout.write = process.stderr.write = streamWrite;

  // Always returns EOF for stdin stream.
  var Readable = require('stream').Readable;
  var stdin = new Readable;
  stdin.push(null);
  process.__defineGetter__('stdin', function() {
    return stdin;
  });
}

// Don't quit on fatal error.
process.on('uncaughtException', function(error) {

  // Do nothing if the user has a custom uncaught exception handler.
  var dialog, message, ref, stack;
  if (process.listeners('uncaughtException').length > 1) {
    return;
  }

  // Show error in GUI.
  dialog = require('electron').dialog;
  stack = (ref = error.stack) != null ? ref : error.name + ": " + error.message;
  message = "Uncaught Exception:\n" + stack;
  return dialog.showErrorBox('A JavaScript error occurred in the main process', message);
});

// Emit 'exit' event on quit.
var app = require('electron').app;

app.on('quit', function(event, exitCode) {
  return process.emit('exit', exitCode);
});

// Map process.exit to app.exit, which quits gracefully.
process.exit = app.exit;

// Load the RPC server.
require('./rpc-server');

// Load the guest view manager.
require('./guest-view-manager');

require('./guest-window-manager');

// Now we try to load app's package.json.
var packageJson = null;
var searchPaths = ['app', 'app.asar', 'default_app'];
var i, len, packagePath;
for (i = 0, len = searchPaths.length; i < len; i++) {
  packagePath = searchPaths[i];
  try {
    packagePath = path.join(process.resourcesPath, packagePath);
    packageJson = JSON.parse(fs.readFileSync(path.join(packagePath, 'package.json')));
    break;
  } catch (error) {
    continue;
  }
}

if (packageJson == null) {
  process.nextTick(function() {
    return process.exit(1);
  });
  throw new Error("Unable to find a valid app");
}

// Set application's version.
if (packageJson.version != null) {
  app.setVersion(packageJson.version);
}

// Set application's name.
if (packageJson.productName != null) {
  app.setName(packageJson.productName);
} else if (packageJson.name != null) {
  app.setName(packageJson.name);
}

// Set application's desktop name.
if (packageJson.desktopName != null) {
  app.setDesktopName(packageJson.desktopName);
} else {
  app.setDesktopName((app.getName()) + ".desktop");
}

// Chrome 42 disables NPAPI plugins by default, reenable them here
app.commandLine.appendSwitch('enable-npapi');

// Set the user path according to application's name.
app.setPath('userData', path.join(app.getPath('appData'), app.getName()));

app.setPath('userCache', path.join(app.getPath('cache'), app.getName()));

app.setAppPath(packagePath);

// Load the chrome extension support.
require('./chrome-extension');

// Load internal desktop-capturer module.
require('./desktop-capturer');

// Set main startup script of the app.
var mainStartupScript = packageJson.main || 'index.js';

// Finally load app's main.js and transfer control to C++.
Module._load(path.join(packagePath, mainStartupScript), Module, true);
