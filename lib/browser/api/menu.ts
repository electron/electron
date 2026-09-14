import { setApplicationMenuWasSet } from '@electron/internal/browser/default-menu';

import { BaseWindow, Menu as MenuType } from 'electron/main';

const bindings = process._linkedBinding('electron_browser_menu');
const { Menu } = bindings as { Menu: typeof MenuType };

let applicationMenu: MenuType | null = null;

Menu.prototype.popup = function (options = {}) {
  if (options == null || typeof options !== 'object') {
    throw new TypeError('Options must be an object');
  }
  let { window, x, y, positioningItem, sourceType, callback } = options;

  // no callback passed
  if (!callback || typeof callback !== 'function') callback = () => {};

  // set defaults
  if (typeof x !== 'number') x = -1;
  if (typeof y !== 'number') y = -1;
  if (typeof positioningItem !== 'number') positioningItem = -1;
  if (typeof sourceType !== 'string' || !sourceType) sourceType = 'mouse';

  // find which window to use
  const wins = BaseWindow.getAllWindows();
  if (!wins || !wins.includes(window as any)) {
    window = BaseWindow.getFocusedWindow() as any;
    if (!window && wins && wins.length > 0) {
      window = wins[0] as any;
    }
    if (!window) {
      throw new Error('Cannot open Menu without a BaseWindow present');
    }
  }

  this.popupAt(window as unknown as BaseWindow, options.frame, x, y, positioningItem, sourceType, callback);
  return { browserWindow: window, x, y, position: positioningItem };
};

Menu.prototype.closePopup = function (window) {
  if (window instanceof BaseWindow) {
    this.closePopupAt(window.id);
  } else {
    // Passing -1 (invalid) would make closePopupAt close the all menu runners
    // belong to this menu.
    this.closePopupAt(-1);
  }
};

Menu.prototype.getMenuItemById = function (id) {
  const items = this.items;

  let found = items.find((item) => item.id === id) || null;
  for (let i = 0; !found && i < items.length; i++) {
    const { submenu } = items[i];
    if (submenu) {
      found = submenu.getMenuItemById(id);
    }
  }
  return found;
};

Menu.getApplicationMenu = () => applicationMenu;

// set application menu with a preexisting menu
Menu.setApplicationMenu = function (menu: MenuType) {
  if (menu && menu.constructor !== Menu) {
    throw new TypeError('Invalid menu');
  }

  applicationMenu = menu;
  setApplicationMenuWasSet();

  if (process.platform === 'darwin') {
    if (!menu) return;
    bindings.setApplicationMenu(menu);
  } else {
    const windows = BaseWindow.getAllWindows();
    windows.map((w) => w.setMenu(menu));
  }
};

module.exports = Menu;
