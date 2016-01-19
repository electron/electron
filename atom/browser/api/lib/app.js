const deprecate = require('electron').deprecate;
const session = require('electron').session;
const Menu = require('electron').Menu;
const EventEmitter = require('events').EventEmitter;

const bindings = process.atomBinding('app');
const downloadItemBindings = process.atomBinding('download_item');
const app = bindings.app;

var slice = [].slice;

app.__proto__ = EventEmitter.prototype;

app.setApplicationMenu = function(menu) {
  return Menu.setApplicationMenu(menu);
};

app.getApplicationMenu = function() {
  return Menu.getApplicationMenu();
};

app.commandLine = {
  appendSwitch: bindings.appendSwitch,
  appendArgument: bindings.appendArgument
};

if (process.platform === 'darwin') {
  app.dock = {
    bounce: function(type) {
      if (type == null) {
        type = 'informational';
      }
      return bindings.dockBounce(type);
    },
    cancelBounce: bindings.dockCancelBounce,
    setBadge: bindings.dockSetBadgeText,
    getBadge: bindings.dockGetBadgeText,
    hide: bindings.dockHide,
    show: bindings.dockShow,
    setMenu: bindings.dockSetMenu
  };
}

var appPath = null;

app.setAppPath = function(path) {
  return appPath = path;
};

app.getAppPath = function() {
  return appPath;
};

// Routes the events to webContents.
var ref1 = ['login', 'certificate-error', 'select-client-certificate'];
var fn = function(name) {
  return app.on(name, function() {
    var args, event, webContents;
    event = arguments[0], webContents = arguments[1], args = 3 <= arguments.length ? slice.call(arguments, 2) : [];
    return webContents.emit.apply(webContents, [name, event].concat(slice.call(args)));
  });
};
var i, len;
for (i = 0, len = ref1.length; i < len; i++) {
  fn(ref1[i]);
}

// Deprecated.

app.getHomeDir = deprecate('app.getHomeDir', 'app.getPath', function() {
  return this.getPath('home');
});

app.getDataPath = deprecate('app.getDataPath', 'app.getPath', function() {
  return this.getPath('userData');
});

app.setDataPath = deprecate('app.setDataPath', 'app.setPath', function(path) {
  return this.setPath('userData', path);
});

app.resolveProxy = deprecate('app.resolveProxy', 'session.defaultSession.resolveProxy', function(url, callback) {
  return session.defaultSession.resolveProxy(url, callback);
});

deprecate.rename(app, 'terminate', 'quit');

deprecate.event(app, 'finish-launching', 'ready', function() {

  // give default app a chance to setup default menu.
  return setImmediate((function(_this) {
    return function() {
      return _this.emit('finish-launching');
    };
  })(this));
});

deprecate.event(app, 'activate-with-no-open-windows', 'activate', function(event, hasVisibleWindows) {
  if (!hasVisibleWindows) {
    return this.emit('activate-with-no-open-windows', event);
  }
});

deprecate.event(app, 'select-certificate', 'select-client-certificate');

// Wrappers for native classes.
var wrapDownloadItem = function(downloadItem) {

  // downloadItem is an EventEmitter.
  downloadItem.__proto__ = EventEmitter.prototype;

  // Deprecated.
  deprecate.property(downloadItem, 'url', 'getURL');
  deprecate.property(downloadItem, 'filename', 'getFilename');
  deprecate.property(downloadItem, 'mimeType', 'getMimeType');
  return deprecate.rename(downloadItem, 'getUrl', 'getURL');
};

downloadItemBindings._setWrapDownloadItem(wrapDownloadItem);

// Only one App object pemitted.
module.exports = app;
