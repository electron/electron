var getNextId, ipcRenderer, isValid, nativeImage, nextId, ref,
  indexOf = [].indexOf || function(item) { for (var i = 0, l = this.length; i < l; i++) { if (i in this && this[i] === item) return i; } return -1; };

ref = require('electron'), ipcRenderer = ref.ipcRenderer, nativeImage = ref.nativeImage;

nextId = 0;

getNextId = function() {
  return ++nextId;
};


/* |options.type| can not be empty and has to include 'window' or 'screen'. */

isValid = function(options) {
  return ((options != null ? options.types : void 0) != null) && Array.isArray(options.types);
};

exports.getSources = function(options, callback) {
  var captureScreen, captureWindow, id;
  if (!isValid(options)) {
    return callback(new Error('Invalid options'));
  }
  captureWindow = indexOf.call(options.types, 'window') >= 0;
  captureScreen = indexOf.call(options.types, 'screen') >= 0;
  if (options.thumbnailSize == null) {
    options.thumbnailSize = {
      width: 150,
      height: 150
    };
  }
  id = getNextId();
  ipcRenderer.send('ATOM_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', captureWindow, captureScreen, options.thumbnailSize, id);
  return ipcRenderer.once("ATOM_RENDERER_DESKTOP_CAPTURER_RESULT_" + id, function(event, sources) {
    var source;
    return callback(null, (function() {
      var i, len, results;
      results = [];
      for (i = 0, len = sources.length; i < len; i++) {
        source = sources[i];
        results.push({
          id: source.id,
          name: source.name,
          thumbnail: nativeImage.createFromDataURL(source.thumbnail)
        });
      }
      return results;
    })());
  });
};
