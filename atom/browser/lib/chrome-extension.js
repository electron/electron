var app, electron, extensionInfoMap, fs, getExtensionInfoFromPath, getHostForPath, getPathForHost, hostPathMap, hostPathMapNextKey, loadedExtensions, loadedExtensionsPath, path, url;

electron = require('electron');

fs = require('fs');

path = require('path');

url = require('url');


/* Mapping between hostname and file path. */

hostPathMap = {};

hostPathMapNextKey = 0;

getHostForPath = function(path) {
  var key;
  key = "extension-" + (++hostPathMapNextKey);
  hostPathMap[key] = path;
  return key;
};

getPathForHost = function(host) {
  return hostPathMap[host];
};


/* Cache extensionInfo. */

extensionInfoMap = {};

getExtensionInfoFromPath = function(srcDirectory) {
  var manifest, page;
  manifest = JSON.parse(fs.readFileSync(path.join(srcDirectory, 'manifest.json')));
  if (extensionInfoMap[manifest.name] == null) {

    /*
      We can not use 'file://' directly because all resources in the extension
      will be treated as relative to the root in Chrome.
     */
    page = url.format({
      protocol: 'chrome-extension',
      slashes: true,
      hostname: getHostForPath(srcDirectory),
      pathname: manifest.devtools_page
    });
    extensionInfoMap[manifest.name] = {
      startPage: page,
      name: manifest.name,
      srcDirectory: srcDirectory,
      exposeExperimentalAPIs: true
    };
    return extensionInfoMap[manifest.name];
  }
};


/* The loaded extensions cache and its persistent path. */

loadedExtensions = null;

loadedExtensionsPath = null;


/* Persistent loaded extensions. */

app = electron.app;

app.on('will-quit', function() {
  var e, error1, error2;
  try {
    loadedExtensions = Object.keys(extensionInfoMap).map(function(key) {
      return extensionInfoMap[key].srcDirectory;
    });
    try {
      fs.mkdirSync(path.dirname(loadedExtensionsPath));
    } catch (error1) {
      e = error1;
    }
    return fs.writeFileSync(loadedExtensionsPath, JSON.stringify(loadedExtensions));
  } catch (error2) {
    e = error2;
  }
});


/* We can not use protocol or BrowserWindow until app is ready. */

app.once('ready', function() {
  var BrowserWindow, chromeExtensionHandler, e, error1, i, init, len, protocol, srcDirectory;
  protocol = electron.protocol, BrowserWindow = electron.BrowserWindow;

  /* Load persistented extensions. */
  loadedExtensionsPath = path.join(app.getPath('userData'), 'DevTools Extensions');
  try {
    loadedExtensions = JSON.parse(fs.readFileSync(loadedExtensionsPath));
    if (!Array.isArray(loadedExtensions)) {
      loadedExtensions = [];
    }

    /* Preheat the extensionInfo cache. */
    for (i = 0, len = loadedExtensions.length; i < len; i++) {
      srcDirectory = loadedExtensions[i];
      getExtensionInfoFromPath(srcDirectory);
    }
  } catch (error1) {
    e = error1;
  }

  /* The chrome-extension: can map a extension URL request to real file path. */
  chromeExtensionHandler = function(request, callback) {
    var directory, parsed;
    parsed = url.parse(request.url);
    if (!(parsed.hostname && (parsed.path != null))) {
      return callback();
    }
    if (!/extension-\d+/.test(parsed.hostname)) {
      return callback();
    }
    directory = getPathForHost(parsed.hostname);
    if (directory == null) {
      return callback();
    }
    return callback(path.join(directory, parsed.path));
  };
  protocol.registerFileProtocol('chrome-extension', chromeExtensionHandler, function(error) {
    if (error) {
      return console.error('Unable to register chrome-extension protocol');
    }
  });
  BrowserWindow.prototype._loadDevToolsExtensions = function(extensionInfoArray) {
    var ref;
    return (ref = this.devToolsWebContents) != null ? ref.executeJavaScript("DevToolsAPI.addExtensions(" + (JSON.stringify(extensionInfoArray)) + ");") : void 0;
  };
  BrowserWindow.addDevToolsExtension = function(srcDirectory) {
    var extensionInfo, j, len1, ref, window;
    extensionInfo = getExtensionInfoFromPath(srcDirectory);
    if (extensionInfo) {
      ref = BrowserWindow.getAllWindows();
      for (j = 0, len1 = ref.length; j < len1; j++) {
        window = ref[j];
        window._loadDevToolsExtensions([extensionInfo]);
      }
      return extensionInfo.name;
    }
  };
  BrowserWindow.removeDevToolsExtension = function(name) {
    return delete extensionInfoMap[name];
  };

  /* Load persistented extensions when devtools is opened. */
  init = BrowserWindow.prototype._init;
  return BrowserWindow.prototype._init = function() {
    init.call(this);
    return this.on('devtools-opened', function() {
      return this._loadDevToolsExtensions(Object.keys(extensionInfoMap).map(function(key) {
        return extensionInfoMap[key];
      }));
    });
  };
});
