'use strict';

const electron = require('electron');
const { EventEmitter } = require('events');
const { TopLevelWindow } = process.electronBinding('top_level_window');

Object.setPrototypeOf(TopLevelWindow.prototype, EventEmitter.prototype);

TopLevelWindow.prototype._init = function () {
  // Avoid recursive require.
  const { app } = electron;

  // Simulate the application menu on platforms other than macOS.
  if (process.platform !== 'darwin') {
    const menu = app.applicationMenu;
    if (menu) this.setMenu(menu);
  }
};

// Properties

Object.defineProperty(TopLevelWindow.prototype, 'autoHideMenuBar', {
  get: function () { return this.isMenuBarAutoHide(); },
  set: function (autoHide) { this.setAutoHideMenuBar(autoHide); }
});

Object.defineProperty(TopLevelWindow.prototype, 'visibleOnAllWorkspaces', {
  get: function () { return this.isVisibleOnAllWorkspaces(); },
  set: function (visible) { this.setVisibleOnAllWorkspaces(visible); }
});

Object.defineProperty(TopLevelWindow.prototype, 'fullScreen', {
  get: function () { return this.isFullScreen(); },
  set: function (full) { this.setFullScreen(full); }
});

Object.defineProperty(TopLevelWindow.prototype, 'simpleFullScreen', {
  get: function () { return this.isSimpleFullScreen(); },
  set: function (simple) { this.setSimpleFullScreen(simple); }
});

Object.defineProperty(TopLevelWindow.prototype, 'kiosk', {
  get: function () { return this.isKiosk(); },
  set: function (kiosk) { this.setKiosk(kiosk); }
});

Object.defineProperty(TopLevelWindow.prototype, 'documentEdited', {
  get: function () { return this.isDocumentEdited(); },
  set: function (edited) { this.setDocumentEdited(edited); }
});

Object.defineProperty(TopLevelWindow.prototype, 'shadow', {
  get: function () { return this.hasShadow(); },
  set: function (shadow) { this.setHasShadow(shadow); }
});

Object.defineProperty(TopLevelWindow.prototype, 'representedFilename', {
  get: function () { return this.getRepresentedFilename(); },
  set: function (filename) { this.setRepresentedFilename(filename); }
});

Object.defineProperty(TopLevelWindow.prototype, 'minimizable', {
  get: function () { return this.isMinimizable(); },
  set: function (min) { this.setMinimizable(min); }
});

Object.defineProperty(TopLevelWindow.prototype, 'title', {
  get: function () { return this.getTitle(); },
  set: function (title) { this.setTitle(title); }
});

Object.defineProperty(TopLevelWindow.prototype, 'maximizable', {
  get: function () { return this.isMaximizable(); },
  set: function (max) { this.setMaximizable(max); }
});

Object.defineProperty(TopLevelWindow.prototype, 'resizable', {
  get: function () { return this.isResizable(); },
  set: function (res) { this.setResizable(res); }
});

Object.defineProperty(TopLevelWindow.prototype, 'menuBarVisible', {
  get: function () { return this.isMenuBarVisible(); },
  set: function (visible) { this.setMenuBarVisibility(visible); }
});

Object.defineProperty(TopLevelWindow.prototype, 'fullScreenable', {
  get: function () { return this.isFullScreenable(); },
  set: function (full) { this.setFullScreenable(full); }
});

Object.defineProperty(TopLevelWindow.prototype, 'closable', {
  get: function () { return this.isClosable(); },
  set: function (close) { this.setClosable(close); }
});

Object.defineProperty(TopLevelWindow.prototype, 'movable', {
  get: function () { return this.isMovable(); },
  set: function (move) { this.setMovable(move); }
});

TopLevelWindow.getFocusedWindow = () => {
  return TopLevelWindow.getAllWindows().find((win) => win.isFocused());
};

module.exports = TopLevelWindow;
