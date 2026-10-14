import {
  BaseWindow,
  BrowserWindow,
  WebContentsView,
  app,
  Menu,
  MenuItem,
  type MenuItemConstructorOptions
} from 'electron/main';

import { afterEach, beforeEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import { createRequire } from 'node:module';

import { ifit, ifdescribe } from './lib/spec-helpers.ts';
import { closeAllWindows, cleanupWebContents } from './lib/window-helpers.ts';

// See deprecate.spec.ts for why this is require()d rather than imported.
const { roleList }: typeof import('../lib/browser/api/menu-item-roles.ts') = createRequire(import.meta.url)(
  '../lib/browser/api/menu-item-roles.ts'
);

function keys<Key extends string, Value>(record: Record<Key, Value>) {
  return Object.keys(record) as Key[];
}

// Helper to execute a menu item by its role
function executeByRole(role: MenuItemConstructorOptions['role'], win: BaseWindow, wc: Electron.WebContents) {
  let handledByRole = true;
  const menu = Menu.buildFromTemplate([
    {
      role,
      click: () => {
        handledByRole = false;
      }
    }
  ]);
  menu.items[0].click({}, win, wc);
  return handledByRole;
}

describe('MenuItems', () => {
  describe('MenuItem instance properties', () => {
    it('should have default MenuItem properties', () => {
      const item = new MenuItem({
        id: '1',
        label: 'hello',
        role: 'close',
        sublabel: 'goodbye',
        accelerator: 'CmdOrControl+Q',
        click: () => {},
        enabled: true,
        visible: true,
        checked: false,
        type: 'normal',
        registerAccelerator: true,
        submenu: [{ role: 'about' }]
      });

      expect(item).toHaveProperty('id', expect.any(String));
      expect(item).toHaveProperty('label');
      expect(item.label).toBeTypeOf('string');
      expect(item.label).toBe('hello');
      expect(item).toHaveProperty('sublabel');
      expect(item.sublabel).toBeTypeOf('string');
      expect(item.sublabel).toBe('goodbye');
      expect(item).toHaveProperty('accelerator');
      expect(item.accelerator).toBeTypeOf('string');
      expect(item.accelerator).toBe('CmdOrControl+Q');
      expect(item).toHaveProperty('click', expect.any(Function));
      expect(item, 'item is enabled').toHaveProperty('enabled');
      expect(item.enabled, 'item is enabled').toBeTypeOf('boolean');
      expect(item.enabled, 'item is enabled').toBe(true);
      expect(item, 'item is visible').toHaveProperty('visible');
      expect(item.visible, 'item is visible').toBeTypeOf('boolean');
      expect(item.visible, 'item is visible').toBe(true);
      expect(item, 'item is not checked').toHaveProperty('checked');
      expect(item.checked, 'item is not checked').toBeTypeOf('boolean');
      expect(item.checked, 'item is not checked').toBe(false);
      expect(item, 'item can register accelerator').toHaveProperty('registerAccelerator');
      expect(item.registerAccelerator, 'item can register accelerator').toBeTypeOf('boolean');
      expect(item.registerAccelerator, 'item can register accelerator').toBe(true);
      expect(item).toHaveProperty('type');
      expect(item.type).toBeTypeOf('string');
      expect(item.type).toBe('normal');
      expect(item).toHaveProperty('commandId', expect.any(Number));
      expect(item).toHaveProperty('toolTip', expect.any(String));
      expect(item).toHaveProperty('role', expect.any(String));
      expect(item).toHaveProperty('icon');
    });

    it('should have a default accelerator for certain roles', () => {
      const items: Record<string, Electron.MenuItem['accelerator']> = {
        undo: 'CommandOrControl+Z',
        redo: process.platform === 'win32' ? 'Control+Y' : 'Shift+CommandOrControl+Z',
        cut: 'CommandOrControl+X',
        copy: 'CommandOrControl+C',
        paste: 'CommandOrControl+V',
        pasteAndMatchStyle: process.platform === 'darwin' ? 'Cmd+Option+Shift+V' : 'Shift+CommandOrControl+V',
        delete: null,
        selectAll: 'CommandOrControl+A',
        reload: 'CmdOrCtrl+R',
        forceReload: 'Shift+CmdOrCtrl+R',
        toggleDevTools: process.platform === 'darwin' ? 'Alt+Command+I' : 'Ctrl+Shift+I',
        resetZoom: 'CommandOrControl+0',
        zoomIn: 'CommandOrControl+Plus',
        zoomOut: 'CommandOrControl+-',
        toggleSpellChecker: null,
        togglefullscreen: process.platform === 'darwin' ? 'Control+Command+F' : 'F11',
        window: null,
        minimize: 'CommandOrControl+M',
        close: 'CommandOrControl+W',
        help: null,
        about: null,
        services: null,
        hide: 'Command+H',
        hideOthers: 'Command+Alt+H',
        unhide: null,
        quit: process.platform === 'win32' ? null : 'CommandOrControl+Q',
        showSubstitutions: null,
        toggleSmartQuotes: null,
        toggleSmartDashes: null,
        toggleTextReplacement: null,
        startSpeaking: null,
        stopSpeaking: null,
        zoom: null,
        front: null,
        appMenu: null,
        fileMenu: null,
        editMenu: null,
        viewMenu: null,
        shareMenu: null,
        recentDocuments: null,
        toggleTabBar: null,
        selectNextTab: null,
        selectPreviousTab: null,
        showAllTabs: null,
        mergeAllWindows: null,
        clearRecentDocuments: null,
        moveTabToNewWindow: null,
        windowMenu: null
      };

      for (const role in items) {
        if (!Object.hasOwn(items, role)) continue;
        const item = new MenuItem({ role: role as any });
        expect(item.accelerator).toBe(items[role]);
      }
    });
  });

  describe('MenuItem.click', () => {
    it('should be called with the item object passed', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const menu = Menu.buildFromTemplate([
          {
            label: 'text',
            click: (item) => {
              try {
                expect(item.constructor.name).toBe('MenuItem');
                expect(item.label).toBe('text');
                done();
              } catch (e) {
                done(e);
              }
            }
          }
        ]);
        menu._executeCommand({}, menu.items[0].commandId);
      }));
  });

  describe('MenuItem with checked/radio property', () => {
    it('clicking an checkbox item should flip the checked property', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'text',
          type: 'checkbox'
        }
      ]);

      expect(menu.items[0].checked, 'menu item checked').toBe(false);
      menu._executeCommand({}, menu.items[0].commandId);
      expect(menu.items[0].checked, 'menu item checked').toBe(true);
    });

    it('clicking an radio item should always make checked property true', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'text',
          type: 'radio'
        }
      ]);

      menu._executeCommand({}, menu.items[0].commandId);
      expect(menu.items[0].checked, 'menu item checked').toBe(true);
      menu._executeCommand({}, menu.items[0].commandId);
      expect(menu.items[0].checked, 'menu item checked').toBe(true);
    });

    describe('MenuItem group properties', () => {
      const template: MenuItemConstructorOptions[] = [];

      const findRadioGroups = (template: MenuItemConstructorOptions[]) => {
        const groups = [];
        let cur: { begin?: number; end?: number } | null = null;
        for (let i = 0; i <= template.length; i++) {
          if (cur && (i === template.length || template[i].type !== 'radio')) {
            cur.end = i;
            groups.push(cur);
            cur = null;
          } else if (!cur && i < template.length && template[i].type === 'radio') {
            cur = { begin: i };
          }
        }
        return groups;
      };

      // returns array of checked menuitems in [begin,end)
      const findChecked = (menuItems: MenuItem[], begin: number, end: number) => {
        const checked = [];
        for (let i = begin; i < end; i++) {
          if (menuItems[i].checked) checked.push(i);
        }
        return checked;
      };

      beforeEach(() => {
        for (let i = 0; i <= 10; i++) {
          template.push({
            label: `${i}`,
            type: 'radio'
          });
        }

        template.push({ type: 'separator' });

        for (let i = 12; i <= 20; i++) {
          template.push({
            label: `${i}`,
            type: 'radio'
          });
        }
      });

      it('at least have one item checked in each group', () => {
        const menu = Menu.buildFromTemplate(template);
        menu._menuWillShow();

        const groups = findRadioGroups(template);

        for (const g of groups) {
          expect(findChecked(menu.items, g.begin!, g.end!)).toEqual([g.begin]);
        }
      });

      it('should assign groupId automatically', () => {
        const menu = Menu.buildFromTemplate(template);

        const usedGroupIds = new Set();
        const groups = findRadioGroups(template);
        for (const g of groups) {
          const groupId = (menu.items[g.begin!] as any).groupId;

          // groupId should be previously unused
          // expect(usedGroupIds.has(groupId)).to.be.false
          expect(usedGroupIds).not.toContain(groupId);
          usedGroupIds.add(groupId);

          // everything in the group should have the same id
          for (let i = g.begin!; i < g.end!; ++i) {
            expect((menu.items[i] as any).groupId).toBe(groupId);
          }
        }
      });

      it("setting 'checked' should flip other items' 'checked' property", () => {
        const menu = Menu.buildFromTemplate(template);

        const groups = findRadioGroups(template);
        for (const g of groups) {
          expect(findChecked(menu.items, g.begin!, g.end!)).toEqual([]);

          menu.items[g.begin!].checked = true;
          expect(findChecked(menu.items, g.begin!, g.end!)).toEqual([g.begin!]);

          menu.items[g.end! - 1].checked = true;
          expect(findChecked(menu.items, g.begin!, g.end!)).toEqual([g.end! - 1]);
        }
      });
    });
  });

  describe('MenuItem role execution', () => {
    afterEach(closeAllWindows);
    it('does not try to execute roles without a valid role property', () => {
      const win = new BrowserWindow({ show: false, width: 200, height: 200 });
      const item = new MenuItem({ role: 'asdfghjkl' as any });

      const canExecute = executeByRole(item.role as any, win, win.webContents);
      expect(canExecute, 'can execute').toBe(false);
    });

    it('executes roles with native role functions', () => {
      const win = new BrowserWindow({ show: false, width: 200, height: 200 });
      const item = new MenuItem({ role: 'reload' });

      const canExecute = executeByRole(item.role as any, win, win.webContents);
      expect(canExecute, 'can execute').toBe(true);
    });

    it('execute roles with non-native role functions', () => {
      const win = new BrowserWindow({ show: false, width: 200, height: 200 });
      const item = new MenuItem({ role: 'resetZoom' });

      const canExecute = executeByRole(item.role as any, win, win.webContents);
      expect(canExecute, 'can execute').toBe(true);
    });

    ifit(process.platform === 'win32')('does not execute minimize role when minimizable false', () => {
      const win = new BrowserWindow({ minimizable: false });
      const menu = Menu.buildFromTemplate([
        {
          label: 'text',
          role: 'minimize'
        }
      ]);

      Menu.setApplicationMenu(menu);
      menu._executeCommand({}, menu.items[0].commandId);
      expect(win.isMinimized()).toBe(false);

      win.setMinimizable(true);
      menu._executeCommand({}, menu.items[0].commandId);
      expect(win.isMinimized()).toBe(true);
    });
  });

  describe('MenuItem command id', () => {
    it('cannot be overwritten', () => {
      const item = new MenuItem({ label: 'item' });

      const commandId = item.commandId;
      expect(commandId, 'command id').not.toBeUndefined();
      expect(() => {
        item.commandId = `${commandId}-modified` as any;
      }).toThrow(/Cannot assign to read only property/);
      expect(item.commandId).toBe(commandId);
    });
  });

  describe('MenuItem with invalid type', () => {
    it('throws an exception', () => {
      expect(() => {
        Menu.buildFromTemplate([
          {
            label: 'text',
            type: 'not-a-type' as any
          }
        ]);
      }).toThrow(/Unknown menu item type: not-a-type/);
    });
  });

  describe('MenuItem with submenu type and missing submenu', () => {
    it('throws an exception', () => {
      expect(() => {
        Menu.buildFromTemplate([
          {
            label: 'text',
            type: 'submenu'
          }
        ]);
      }).toThrow(/Invalid submenu/);
    });
  });

  describe('MenuItem role', () => {
    it('returns undefined for items without default accelerator', () => {
      const list = keys(roleList).filter((key) => !roleList[key].accelerator);

      for (const role of list) {
        const item = new MenuItem({ role: role as any });
        expect(item.getDefaultRoleAccelerator(), 'default accelerator').toBeUndefined();
      }
    });

    it('returns the correct default label', () => {
      for (const role of keys(roleList)) {
        const item = new MenuItem({ role: role as any });
        const label: string = roleList[role].label;
        expect(item.label).toBe(label);
      }
    });

    it('returns the correct default accelerator', () => {
      const list = keys(roleList).filter((key) => roleList[key].accelerator);

      for (const role of list) {
        const item = new MenuItem({ role: role as any });
        const accelerator = roleList[role].accelerator;
        expect(item.getDefaultRoleAccelerator()).toBe(accelerator);
      }
    });

    it('allows a custom accelerator and label to be set', () => {
      const item = new MenuItem({
        role: 'close',
        label: 'Custom Close!',
        accelerator: 'D'
      });

      expect(item.label).toBe('Custom Close!');
      expect(item.accelerator).toBe('D');
      expect(item.getDefaultRoleAccelerator()).toBe('CommandOrControl+W');
    });
  });

  ifdescribe(process.platform === 'darwin')('MenuItem appMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'appMenu' });

      expect(item.label).toBe(app.name);
      expect(item.submenu!.items[0].role).toBe('about');
      expect(item.submenu!.items[1].type).toBe('separator');
      expect(item.submenu!.items[2].role).toBe('services');
      expect(item.submenu!.items[3].type).toBe('separator');
      expect(item.submenu!.items[4].role).toBe('hide');
      expect(item.submenu!.items[5].role).toBe('hideothers');
      expect(item.submenu!.items[6].role).toBe('unhide');
      expect(item.submenu!.items[7].type).toBe('separator');
      expect(item.submenu!.items[8].role).toBe('quit');
    });

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'appMenu',
        submenu: [
          {
            role: 'close'
          }
        ]
      });
      expect(item.label).toBe(app.name);
      expect(item.submenu!.items[0].role).toBe('close');
    });
  });

  describe('MenuItem fileMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'fileMenu' });

      expect(item.label).toBe('File');
      if (process.platform === 'darwin') {
        expect(item.submenu!.items[0].role).toBe('close');
      } else {
        expect(item.submenu!.items[0].role).toBe('quit');
      }
    });

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'fileMenu',
        submenu: [
          {
            role: 'about'
          }
        ]
      });
      expect(item.label).toBe('File');
      expect(item.submenu!.items[0].role).toBe('about');
    });
  });

  describe('MenuItem editMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'editMenu' });

      expect(item.label).toBe('Edit');
      expect(item.submenu!.items[0].role).toBe('undo');
      expect(item.submenu!.items[1].role).toBe('redo');
      expect(item.submenu!.items[2].type).toBe('separator');
      expect(item.submenu!.items[3].role).toBe('cut');
      expect(item.submenu!.items[4].role).toBe('copy');
      expect(item.submenu!.items[5].role).toBe('paste');

      if (process.platform === 'darwin') {
        expect(item.submenu!.items[6].role).toBe('pasteandmatchstyle');
        expect(item.submenu!.items[7].role).toBe('delete');
        expect(item.submenu!.items[8].role).toBe('selectall');
        expect(item.submenu!.items[9].type).toBe('separator');
        expect(item.submenu!.items[10].label).toBe('Substitutions');
        expect(item.submenu!.items[10].submenu!.items[0].role).toBe('showsubstitutions');
        expect(item.submenu!.items[10].submenu!.items[1].type).toBe('separator');
        expect(item.submenu!.items[10].submenu!.items[2].role).toBe('togglesmartquotes');
        expect(item.submenu!.items[10].submenu!.items[3].role).toBe('togglesmartdashes');
        expect(item.submenu!.items[10].submenu!.items[4].role).toBe('toggletextreplacement');
        expect(item.submenu!.items[11].label).toBe('Speech');
        expect(item.submenu!.items[11].submenu!.items[0].role).toBe('startspeaking');
        expect(item.submenu!.items[11].submenu!.items[1].role).toBe('stopspeaking');
      } else {
        expect(item.submenu!.items[6].role).toBe('delete');
        expect(item.submenu!.items[7].type).toBe('separator');
        expect(item.submenu!.items[8].role).toBe('selectall');
      }
    });

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'editMenu',
        submenu: [
          {
            role: 'close'
          }
        ]
      });
      expect(item.label).toBe('Edit');
      expect(item.submenu!.items[0].role).toBe('close');
    });
  });

  describe('MenuItem viewMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'viewMenu' });

      expect(item.label).toBe('View');
      expect(item.submenu!.items[0].role).toBe('reload');
      expect(item.submenu!.items[1].role).toBe('forcereload');
      expect(item.submenu!.items[2].role).toBe('toggledevtools');
      expect(item.submenu!.items[3].type).toBe('separator');
      expect(item.submenu!.items[4].role).toBe('resetzoom');
      expect(item.submenu!.items[5].role).toBe('zoomin');
      expect(item.submenu!.items[6].role).toBe('zoomout');
      expect(item.submenu!.items[7].type).toBe('separator');
      expect(item.submenu!.items[8].role).toBe('togglefullscreen');
    });

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'viewMenu',
        submenu: [
          {
            role: 'close'
          }
        ]
      });
      expect(item.label).toBe('View');
      expect(item.submenu!.items[0].role).toBe('close');
    });
  });

  describe('MenuItem toggleDevTools role', () => {
    afterEach(closeAllWindows);
    afterEach(cleanupWebContents);

    it('toggles devtools on the focused webContents', async () => {
      const w = new BaseWindow({ show: false });
      const wcv = new WebContentsView();
      w.setContentView(wcv);
      await wcv.webContents.loadURL('about:blank');

      const opened = once(wcv.webContents, 'devtools-opened');
      expect(executeByRole('toggleDevTools', w, wcv.webContents)).toBe(true);
      await opened;
      expect(wcv.webContents.isDevToolsOpened()).toBe(true);

      const closed = once(wcv.webContents, 'devtools-closed');
      executeByRole('toggleDevTools', w, wcv.webContents);
      await closed;
      expect(wcv.webContents.isDevToolsOpened()).toBe(false);
    });

    it('toggles parent devtools when invoked with the devtools webContents', async () => {
      const w = new BaseWindow({ show: false });
      const wcv = new WebContentsView();
      w.setContentView(wcv);
      await wcv.webContents.loadURL('about:blank');

      const opened = once(wcv.webContents, 'devtools-opened');
      wcv.webContents.openDevTools({ mode: 'bottom' });
      await opened;
      expect(wcv.webContents.isDevToolsOpened()).toBe(true);

      const devToolsWc = wcv.webContents.devToolsWebContents!;
      expect(devToolsWc).not.toBeNull();

      const closed = once(wcv.webContents, 'devtools-closed');
      expect(executeByRole('toggleDevTools', w, devToolsWc)).toBe(true);
      await closed;
      expect(wcv.webContents.isDevToolsOpened()).toBe(false);
    });
  });

  describe('MenuItem reload roles', () => {
    afterEach(closeAllWindows);
    afterEach(cleanupWebContents);

    const title = 'TEST';
    const htmlUrl = `data:text/html,<html><head><title>${title}</title></head><body></body></html>`;

    const changeTitle = async (wcv: WebContentsView) => {
      const changedTitle = 'CHANGED';
      await wcv.webContents.executeJavaScript(`document.title = '${changedTitle}'`);
      expect(wcv.webContents.getTitle()).toBe(changedTitle);
    };

    const testReloadRole = async (role: 'reload' | 'forceReload') => {
      const w = new BaseWindow({ show: false });
      const wcv = new WebContentsView();
      w.contentView.addChildView(wcv);

      await wcv.webContents.loadURL(htmlUrl);
      await changeTitle(wcv);

      const didStartLoading = once(wcv.webContents, 'did-start-loading');
      expect(executeByRole(role, w, wcv.webContents)).toBe(true);
      await didStartLoading;
      await once(wcv.webContents, 'did-finish-load');

      // If the page was reloaded, the title should have been reset.
      expect(wcv.webContents.getTitle()).toBe(title);
    };

    const testReloadRoleOnDevToolsWebContents = async (role: 'reload' | 'forceReload') => {
      const w = new BaseWindow({ show: false });
      const wcv = new WebContentsView();
      w.contentView.addChildView(wcv);

      await wcv.webContents.loadURL(htmlUrl);
      await changeTitle(wcv);

      const opened = once(wcv.webContents, 'devtools-opened');
      wcv.webContents.openDevTools();
      await opened;

      const didStartLoading = once(wcv.webContents, 'did-start-loading');
      expect(executeByRole(role, w, wcv.webContents.devToolsWebContents!)).toBe(true);
      await didStartLoading;
      await once(wcv.webContents, 'did-finish-load');

      // If the page was reloaded, the title should have been reset.
      expect(wcv.webContents.getTitle()).toBe(title);
    };

    describe('reload role', () => {
      it('reloads the focused webContents', async () => {
        await testReloadRole('reload');
      });

      it('reloads the parent webContents when called on DevTools webContents', async () => {
        await testReloadRoleOnDevToolsWebContents('reload');
      });
    });

    describe('forceReload role', () => {
      it('reloads the focused webContents', async () => {
        await testReloadRole('forceReload');
      });

      it('reloads the parent webContents when called on DevTools webContents', async () => {
        await testReloadRoleOnDevToolsWebContents('forceReload');
      });
    });
  });

  describe('MenuItem windowMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'windowMenu' });

      expect(item.label).toBe('Window');
      expect(item.submenu!.items[0].role).toBe('minimize');
      expect(item.submenu!.items[1].role).toBe('zoom');

      if (process.platform === 'darwin') {
        expect(item.submenu!.items[2].type).toBe('separator');
        expect(item.submenu!.items[3].role).toBe('front');
      } else {
        expect(item.submenu!.items[2].role).toBe('close');
      }
    });

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'windowMenu',
        submenu: [{ role: 'copy' }]
      });

      expect(item.label).toBe('Window');
      expect(item.submenu!.items[0].role).toBe('copy');
    });
  });

  describe('MenuItem with custom properties in constructor', () => {
    it('preserves the custom properties', () => {
      const template = [
        {
          label: 'menu 1',
          customProp: 'foo',
          submenu: []
        }
      ];

      const menu = Menu.buildFromTemplate(template);
      menu.items[0].submenu!.append(
        new MenuItem({
          label: 'item 1',
          customProp: 'bar',
          overrideProperty: 'oops not allowed'
        } as any)
      );

      expect((menu.items[0] as any).customProp).toBe('foo');
      expect(menu.items[0].submenu!.items[0].label).toBe('item 1');
      expect((menu.items[0].submenu!.items[0] as any).customProp).toBe('bar');
      expect((menu.items[0].submenu!.items[0] as any).overrideProperty).toBeTypeOf('function');
    });
  });

  describe('MenuItem accelerators', () => {
    const isDarwin = () => {
      return process.platform === 'darwin';
    };

    it('should display modifiers correctly for simple keys', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'CommandOrControl+A' },
        { label: 'text', accelerator: 'Shift+A' },
        { label: 'text', accelerator: 'Alt+A' }
      ]);

      expect(menu._getAcceleratorTextAt(0)).toBe(isDarwin() ? 'Command+A' : 'Ctrl+A');
      expect(menu._getAcceleratorTextAt(1)).toBe('Shift+A');
      expect(menu._getAcceleratorTextAt(2)).toBe('Alt+A');
    });

    it('should display modifiers correctly for special keys', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'CommandOrControl+Tab' },
        { label: 'text', accelerator: 'Shift+Tab' },
        { label: 'text', accelerator: 'Alt+Tab' }
      ]);

      expect(menu._getAcceleratorTextAt(0)).toBe(isDarwin() ? 'Command+Tab' : 'Ctrl+Tab');
      expect(menu._getAcceleratorTextAt(1)).toBe('Shift+Tab');
      expect(menu._getAcceleratorTextAt(2)).toBe('Alt+Tab');
    });

    it('should not display modifiers twice', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'Shift+Shift+A' },
        { label: 'text', accelerator: 'Shift+Shift+Tab' }
      ]);

      expect(menu._getAcceleratorTextAt(0)).toBe('Shift+A');
      expect(menu._getAcceleratorTextAt(1)).toBe('Shift+Tab');
    });

    it('should display correctly for shifted keys', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'Control+Shift+=' },
        { label: 'text', accelerator: 'Control+Plus' },
        { label: 'text', accelerator: 'Control+Shift+3' },
        { label: 'text', accelerator: 'Control+#' },
        { label: 'text', accelerator: 'Control+Shift+/' },
        { label: 'text', accelerator: 'Control+?' }
      ]);

      expect(menu._getAcceleratorTextAt(0)).toBe('Ctrl+Shift+=');
      expect(menu._getAcceleratorTextAt(1)).toBe('Ctrl++');
      expect(menu._getAcceleratorTextAt(2)).toBe('Ctrl+Shift+3');
      expect(menu._getAcceleratorTextAt(3)).toBe('Ctrl+#');
      expect(menu._getAcceleratorTextAt(4)).toBe('Ctrl+Shift+/');
      expect(menu._getAcceleratorTextAt(5)).toBe('Ctrl+?');
    });
  });

  ifdescribe(process.platform === 'darwin')('MenuItem.badge', () => {
    it('should set badge from constructor options', () => {
      const item = new MenuItem({
        label: 'test',
        badge: { type: 'alerts', count: 3 }
      });

      expect(item.badge).toEqual({ type: 'alerts', count: 3 });
    });

    it('should support all badge types', () => {
      const alerts = new MenuItem({ label: 'a', badge: { type: 'alerts', count: 1 } });
      const updates = new MenuItem({ label: 'b', badge: { type: 'updates', count: 2 } });
      const newItems = new MenuItem({ label: 'c', badge: { type: 'new-items', count: 5 } });
      const custom = new MenuItem({ label: 'd', badge: { type: 'none', content: 'Custom' } });

      expect(alerts.badge).toEqual({ type: 'alerts', count: 1 });
      expect(updates.badge).toEqual({ type: 'updates', count: 2 });
      expect(newItems.badge).toEqual({ type: 'new-items', count: 5 });
      expect(custom.badge).toEqual({ type: 'none', content: 'Custom' });
    });

    it('should allow dynamic badge updates', () => {
      const item = new MenuItem({
        label: 'test',
        badge: { type: 'alerts', count: 1 }
      });

      item.badge = { type: 'updates', count: 10 };
      expect(item.badge).toEqual({ type: 'updates', count: 10 });
    });

    it('should have undefined badge when not set', () => {
      const item = new MenuItem({ label: 'test' });
      expect(item.badge).toBeUndefined();
    });

    it('should set badge on items added to a menu', () => {
      const menu = Menu.buildFromTemplate([{ label: 'test', badge: { type: 'alerts', count: 3 } }]);

      expect(menu.items[0].badge).toEqual({ type: 'alerts', count: 3 });
    });

    it('should update badge after item is added to a menu', () => {
      const menu = Menu.buildFromTemplate([{ label: 'test', badge: { type: 'alerts', count: 1 } }]);

      menu.items[0].badge = { type: 'updates', count: 5 };
      expect(menu.items[0].badge).toEqual({ type: 'updates', count: 5 });
    });

    it('should remove an existing badge', () => {
      const item = new MenuItem({ label: 'test', badge: { type: 'alerts', count: 3 } });
      item.badge = undefined;
      expect(item.badge).toBeUndefined();

      const menu = Menu.buildFromTemplate([{ label: 'test', badge: { type: 'none', content: 'New' } }]);
      menu.items[0].badge = undefined;
      expect(menu.items[0].badge).toBeUndefined();
      menu.items[0].badge = { type: 'new-items', count: 2 };
      expect(menu.items[0].badge).toEqual({ type: 'new-items', count: 2 });
    });

    it('should default the badge type to none', () => {
      const item = new MenuItem({ label: 'test', badge: { content: 'Beta' } });
      expect(item.badge).toEqual({ content: 'Beta' });
    });

    it('should throw on an invalid badge', () => {
      expect(() => new MenuItem({ label: 'test', badge: { type: 'bogus', count: 1 } as any })).toThrow(
        /Invalid badge type/
      );
      expect(() => new MenuItem({ label: 'test', badge: { type: 'alerts' } })).toThrow(
        /badge.count must be a non-negative integer/
      );
      expect(() => new MenuItem({ label: 'test', badge: { type: 'updates', count: -1 } })).toThrow(
        /badge.count must be a non-negative integer/
      );
      expect(() => new MenuItem({ label: 'test', badge: { type: 'alerts', count: 1, content: 'x' } })).toThrow(
        /badge.content can only be used/
      );
      expect(() => new MenuItem({ label: 'test', badge: { type: 'none' } })).toThrow(/badge.content must be a string/);
      expect(() => new MenuItem({ label: 'test', badge: { type: 'none', content: 'x', count: 1 } })).toThrow(
        /badge.count cannot be used/
      );

      const item = new MenuItem({ label: 'test' });
      expect(() => {
        item.badge = { type: 'bogus' } as any;
      }).toThrow(/Invalid badge type/);
      expect(item.badge).toBeUndefined();
    });
  });
});
