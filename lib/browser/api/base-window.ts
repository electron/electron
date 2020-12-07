import { EventEmitter } from 'events';
import type { BaseWindow as TLWT } from 'electron/main';
const { BaseWindow } = process._linkedBinding('electron_browser_base_window') as { BaseWindow: typeof TLWT };

Object.setPrototypeOf(BaseWindow.prototype, EventEmitter.prototype);

BaseWindow.prototype._init = function () {
  // Avoid recursive require.
  const { app } = require('electron');

  // Simulate the application menu on platforms other than macOS.
  if (process.platform !== 'darwin') {
    const menu = app.applicationMenu;
    if (menu) this.setMenu(menu);
  }
};

// Properties

Object.defineProperty(BaseWindow.prototype, 'autoHideMenuBar', {
  get() { return this.isMenuBarAutoHide(); },
  set(autoHide) { this.setAutoHideMenuBar(autoHide); }
});

Object.defineProperty(BaseWindow.prototype, 'visibleOnAllWorkspaces', {
  get() { return this.isVisibleOnAllWorkspaces(); },
  set(visible) { this.setVisibleOnAllWorkspaces(visible); }
});

Object.defineProperty(BaseWindow.prototype, 'fullScreen', {
  get() { return this.isFullScreen(); },
  set(full) { this.setFullScreen(full); }
});

Object.defineProperty(BaseWindow.prototype, 'simpleFullScreen', {
  get() { return this.isSimpleFullScreen(); },
  set(simple) { this.setSimpleFullScreen(simple); }
});

Object.defineProperty(BaseWindow.prototype, 'kiosk', {
  get() { return this.isKiosk(); },
  set(kiosk) { this.setKiosk(kiosk); }
});

Object.defineProperty(BaseWindow.prototype, 'documentEdited', {
  get() { return this.isFullscreen(); },
  set(edited) { this.setDocumentEdited(edited); }
});

Object.defineProperty(BaseWindow.prototype, 'shadow', {
  get() { return this.hasShadow(); },
  set(shadow) { this.setHasShadow(shadow); }
});

Object.defineProperty(BaseWindow.prototype, 'representedFilename', {
  get() { return this.getRepresentedFilename(); },
  set(filename) { this.setRepresentedFilename(filename); }
});

Object.defineProperty(BaseWindow.prototype, 'minimizable', {
  get() { return this.isMinimizable(); },
  set(min) { this.setMinimizable(min); }
});

Object.defineProperty(BaseWindow.prototype, 'title', {
  get() { return this.getTitle(); },
  set(title) { this.setTitle(title); }
});

Object.defineProperty(BaseWindow.prototype, 'maximizable', {
  get() { return this.isMaximizable(); },
  set(max) { this.setMaximizable(max); }
});

Object.defineProperty(BaseWindow.prototype, 'resizable', {
  get() { return this.isResizable(); },
  set(res) { this.setResizable(res); }
});

Object.defineProperty(BaseWindow.prototype, 'menuBarVisible', {
  get() { return this.isMenuBarVisible(); },
  set(visible) { this.setMenuBarVisibility(visible); }
});

Object.defineProperty(BaseWindow.prototype, 'fullScreenable', {
  get() { return this.isFullScreenable(); },
  set(full) { this.setFullScreenable(full); }
});

Object.defineProperty(BaseWindow.prototype, 'closable', {
  get() { return this.isClosable(); },
  set(close) { this.setClosable(close); }
});

Object.defineProperty(BaseWindow.prototype, 'movable', {
  get() { return this.isMovable(); },
  set(move) { this.setMovable(move); }
});

BaseWindow.getFocusedWindow = () => {
  return BaseWindow.getAllWindows().find((win) => win.isFocused());
};

module.exports = BaseWindow;
