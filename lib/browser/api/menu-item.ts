import * as roles from '@electron/internal/browser/api/menu-item-roles';

import { Menu, BaseWindow, WebContents, KeyboardEvent } from 'electron/main';

let nextCommandId = 0;

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
  this.overrideReadOnlyProperty('accelerator');
  this.overrideReadOnlyProperty('icon');
  this.overrideReadOnlyProperty('submenu');

  this.overrideProperty('label', roles.getDefaultLabel(this.role));
  this.overrideProperty('sublabel', '');
  this.overrideProperty('toolTip', '');
  this.overrideProperty('enabled', true);
  this.overrideProperty('visible', true);
  this.overrideProperty('checked', false);
  this.overrideProperty('acceleratorWorksWhenHidden', true);
  this.overrideProperty('registerAccelerator', roles.shouldRegisterAccelerator(this.role));

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
    if (!roles.shouldOverrideCheckStatus(this.role) &&
        (this.type === 'checkbox' || this.type === 'radio')) {
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

MenuItem.types = ['normal', 'separator', 'submenu', 'checkbox', 'radio'];

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
