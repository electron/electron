'use strict';

const ipcMain = require('electron').ipcMain;
const BrowserWindow = require('electron').BrowserWindow;

var hasProp = {}.hasOwnProperty;
var slice = [].slice;
var frameToGuest = {};

// Copy attribute of |parent| to |child| if it is not defined in |child|.
var mergeOptions = function(child, parent) {
  var key, value;
  for (key in parent) {
    if (!hasProp.call(parent, key)) continue;
    value = parent[key];
    if (!(key in child)) {
      if (typeof value === 'object') {
        child[key] = mergeOptions({}, value);
      } else {
        child[key] = value;
      }
    }
  }
  return child;
};

// Merge |options| with the |embedder|'s window's options.
var mergeBrowserWindowOptions = function(embedder, options) {
  if (embedder.browserWindowOptions != null) {

    // Inherit the original options if it is a BrowserWindow.
    mergeOptions(options, embedder.browserWindowOptions);
  } else {

    // Or only inherit web-preferences if it is a webview.
    if (options.webPreferences == null) {
      options.webPreferences = {};
    }
    mergeOptions(options.webPreferences, embedder.getWebPreferences());
  }
  return options;
};

// Create a new guest created by |embedder| with |options|.
var createGuest = function(embedder, url, frameName, options) {
  var closedByEmbedder, closedByUser, guest, guestId;
  guest = frameToGuest[frameName];
  if (frameName && (guest != null)) {
    guest.loadURL(url);
    return guest.id;
  }

  // Remember the embedder window's id.
  if (options.webPreferences == null) {
    options.webPreferences = {};
  }
  const embedderWindow = BrowserWindow.fromWebContents(embedder)
  options.webPreferences.openerId = embedderWindow != null ? embedderWindow.id : undefined;
  guest = new BrowserWindow(options);
  guest.loadURL(url);

  // When |embedder| is destroyed we should also destroy attached guest, and if
  // guest is closed by user then we should prevent |embedder| from double
  // closing guest.
  guestId = guest.id;
  closedByEmbedder = function() {
    guest.removeListener('closed', closedByUser);
    return guest.destroy();
  };
  closedByUser = function() {
    embedder.send("ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_" + guestId);
    return embedder.removeListener('render-view-deleted', closedByEmbedder);
  };
  embedder.once('render-view-deleted', closedByEmbedder);
  guest.once('closed', closedByUser);
  if (frameName) {
    frameToGuest[frameName] = guest;
    guest.frameName = frameName;
    guest.once('closed', function() {
      return delete frameToGuest[frameName];
    });
  }
  return guest.id;
};

// Routed window.open messages.
ipcMain.on('ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPEN', function() {
  var args, event, frameName, options, url;
  event = arguments[0], args = 2 <= arguments.length ? slice.call(arguments, 1) : [];
  url = args[0], frameName = args[1], options = args[2];
  options = mergeBrowserWindowOptions(event.sender, options);
  event.sender.emit('new-window', event, url, frameName, 'new-window', options);
  if ((event.sender.isGuest() && !event.sender.allowPopups) || event.defaultPrevented) {
    return event.returnValue = null;
  } else {
    return event.returnValue = createGuest(event.sender, url, frameName, options);
  }
});

ipcMain.on('ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', function(event, guestId) {
  const guestWindow = BrowserWindow.fromId(guestId);
  return guestWindow != null ? guestWindow.destroy() : undefined;
});

ipcMain.on('ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', function() {
  const event = arguments[0];
  const guestId = arguments[1];
  const method = arguments[2];
  const args = 4 <= arguments.length ? slice.call(arguments, 3) : [];
  const guestWindow = BrowserWindow.fromId(guestId);
  return event.returnValue = guestWindow != null ? guestWindow[method].apply(guestWindow, args) : undefined;
});

ipcMain.on('ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', function(event, guestId, message, targetOrigin, sourceOrigin) {
  const senderWindow = BrowserWindow.fromWebContents(event.sender);
  const sourceId = senderWindow != null ? senderWindow.id : undefined;
  if (sourceId == null) {
    return;
  }

  const guestWindow = BrowserWindow.fromId(guestId);
  const guestContents = guestWindow != null ? guestWindow.webContents : undefined;
  if ((guestContents != null ? guestContents.getURL().indexOf(targetOrigin) : void 0) === 0 || targetOrigin === '*') {
    return guestContents != null ? guestContents.send('ATOM_SHELL_GUEST_WINDOW_POSTMESSAGE', sourceId, message, sourceOrigin) : void 0;
  }
});

ipcMain.on('ATOM_SHELL_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', function() {
  const guestId = arguments[1];
  const method = arguments[2];
  const args = 4 <= arguments.length ? slice.call(arguments, 3) : [];
  const guestWindow = BrowserWindow.fromId(guestId);
  const guestContents = guestWindow != null ? guestContents.webContents : null
  return guestContents != null ? guestContents[method].apply(guestContents, args) : undefined;
});
