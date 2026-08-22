import * as roles from '@electron/internal/browser/api/menu-item-roles';

import { Menu, BaseWindow, WebContents, KeyboardEvent } from 'electron/main';

let nextCommandId = 0;

const badgeTypes = ['alerts', 'updates', 'new-items', 'none'];

const validateBadge = (badge: any) => {
  if (badge == null) return;
  if (typeof badge !== 'object') {
    throw new TypeError('badge must be a MenuItemBadge object');
  }
  const type = badge.type ?? 'none';
  if (!badgeTypes.includes(type)) {
    throw new TypeError(`Invalid badge type '${type}': must be one of ${badgeTypes.join(', ')}`);
  }
  if (type === 'none') {
    if (typeof badge.content !== 'string') {
      throw new TypeError("badge.content must be a string when badge.type is 'none'");
    }
    if (badge.count != null) {
      throw new TypeError("badge.count cannot be used when badge.type is 'none'");
    }
  } else {
    if (!Number.isInteger(badge.count) || badge.count < 0) {
      throw new TypeError(`badge.count must be a non-negative integer when badge.type is '${type}'`);
    }
    if (badge.content != null) {
      throw new TypeError("badge.content can only be used when badge.type is 'none'");
    }
  }
};

const MenuItem = function (this: any, options: any) {
  // Preserve extra fields specified by user
  for (const key in options) {
    if (!(key in this)) this[key] = options[key];
  }
  if (typeof this.role === 'string' || this.role instanceof String) {
    this.role = this.role.toLowerCase();
  }
  this.submenu = this.submenu || roles.getDefaultSubmenu(this.role);
  if (this.submenu != null && this.submenu.constructor !== Menu) {
    this.submenu = Menu.buildFromTemplate(this.submenu);
  }
  if (this.type == null && this.submenu != null) {
    this.type = 'submenu';
  }
  if (this.type === 'submenu' && (this.submenu == null || this.submenu.constructor !== Menu)) {
    throw new Error('Invalid submenu');
  }

  this.overrideReadOnlyProperty('type', roles.getDefaultType(this.role));
  this.overrideReadOnlyProperty('role');
  this.overrideReadOnlyProperty('accelerator', roles.getDefaultAccelerator(this.role));
  this.overrideReadOnlyProperty('submenu');

  this.overrideProperty('icon');
  this.overrideProperty('label', roles.getDefaultLabel(this.role));
  this.overrideProperty('accessibilityLabel', '');
  this.overrideProperty('sublabel', '');
  this.overrideProperty('toolTip', '');
  this.overrideProperty('enabled', true);
  this.overrideProperty('visible', true);
  this.overrideProperty('checked', false);
  this.overrideProperty('acceleratorWorksWhenHidden', true);
  this.overrideProperty('registerAccelerator', roles.shouldRegisterAccelerator(this.role));

  if (process.platform === 'darwin') {
    validateBadge(options.badge);
    let badgeValue = options.badge ?? undefined;
    Object.defineProperty(this, 'badge', {
      get: () => badgeValue,
      set: (newValue) => {
        validateBadge(newValue);
        badgeValue = newValue ?? undefined;
        // Push the change to the native item if this item is already in a menu.
        if (this.menu) {
          const index = this.menu.getIndexOfCommandId(this.commandId);
          if (index !== -1) this.menu.setBadge(index, badgeValue ?? null);
        }
      },
      enumerable: true
    });
  }

  if (!MenuItem.types.includes(this.type)) {
    throw new Error(`Unknown menu item type: ${this.type}`);
  }

  this.overrideReadOnlyProperty('commandId', ++nextCommandId);

  Object.defineProperty(this, 'userAccelerator', {
    get: () => {
      if (process.platform !== 'darwin') return null;
      if (!this.menu) return null;
      return this.menu._getUserAcceleratorAt(this.commandId);
    },
    enumerable: true
  });

  const click = options.click;
  this.click = (event: KeyboardEvent, focusedWindow: BaseWindow, focusedWebContents: WebContents) => {
    // Manually flip the checked flags when clicked.
    if (!roles.shouldOverrideCheckStatus(this.role) && (this.type === 'checkbox' || this.type === 'radio')) {
      this.checked = !this.checked;
    }

    if (!roles.execute(this.role, focusedWindow, focusedWebContents)) {
      if (typeof click === 'function') {
        click(this, focusedWindow, event);
      } else if (typeof this.selector === 'string' && process.platform === 'darwin') {
        Menu.sendActionToFirstResponder(this.selector);
      }
    }
  };
};

MenuItem.types = ['normal', 'separator', 'submenu', 'checkbox', 'radio', 'header', 'palette'];

MenuItem.prototype.getDefaultRoleAccelerator = function () {
  return roles.getDefaultAccelerator(this.role);
};

MenuItem.prototype.getCheckStatus = function () {
  if (!roles.shouldOverrideCheckStatus(this.role)) return this.checked;
  return roles.getCheckStatus(this.role);
};

MenuItem.prototype.overrideProperty = function (name: string, defaultValue: any = null) {
  if (this[name] == null) {
    this[name] = defaultValue;
  }
};

MenuItem.prototype.overrideReadOnlyProperty = function (name: string, defaultValue: any) {
  this.overrideProperty(name, defaultValue);
  Object.defineProperty(this, name, {
    enumerable: true,
    writable: false,
    value: this[name]
  });
};

module.exports = MenuItem;
