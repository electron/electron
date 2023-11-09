import { BrowserWindow, app, Menu, MenuItem, MenuItemConstructorOptions } from 'electron/main';
import { expect } from 'chai';
import { ifdescribe } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';
import { roleList, execute } from '../lib/browser/api/menu-item-roles';

function keys<Key extends string, Value> (record: Record<Key, Value>) {
  return Object.keys(record) as Key[];
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
        click: () => { },
        enabled: true,
        visible: true,
        checked: false,
        type: 'normal',
        registerAccelerator: true,
        submenu: [{ role: 'about' }]
      });

      expect(item).to.have.property('id').that.is.a('string');
      expect(item).to.have.property('label').that.is.a('string').equal('hello');
      expect(item).to.have.property('sublabel').that.is.a('string').equal('goodbye');
      expect(item).to.have.property('accelerator').that.is.a('string').equal('CmdOrControl+Q');
      expect(item).to.have.property('click').that.is.a('function');
      expect(item).to.have.property('enabled').that.is.a('boolean').and.is.true('item is enabled');
      expect(item).to.have.property('visible').that.is.a('boolean').and.is.true('item is visible');
      expect(item).to.have.property('checked').that.is.a('boolean').and.is.false('item is not checked');
      expect(item).to.have.property('registerAccelerator').that.is.a('boolean').and.is.true('item can register accelerator');
      expect(item).to.have.property('type').that.is.a('string').equal('normal');
      expect(item).to.have.property('commandId').that.is.a('number');
      expect(item).to.have.property('toolTip').that.is.a('string');
      expect(item).to.have.property('role').that.is.a('string');
      expect(item).to.have.property('icon');
    });
  });

  describe('MenuItem.click', () => {
    it('should be called with the item object passed', done => {
      const menu = Menu.buildFromTemplate([{
        label: 'text',
        click: (item) => {
          try {
            expect(item.constructor.name).to.equal('MenuItem');
            expect(item.label).to.equal('text');
            done();
          } catch (e) {
            done(e);
          }
        }
      }]);
      menu._executeCommand({}, menu.items[0].commandId);
    });
  });

  describe('MenuItem with checked/radio property', () => {
    it('clicking an checkbox item should flip the checked property', () => {
      const menu = Menu.buildFromTemplate([{
        label: 'text',
        type: 'checkbox'
      }]);

      expect(menu.items[0].checked).to.be.false('menu item checked');
      menu._executeCommand({}, menu.items[0].commandId);
      expect(menu.items[0].checked).to.be.true('menu item checked');
    });

    it('clicking an radio item should always make checked property true', () => {
      const menu = Menu.buildFromTemplate([{
        label: 'text',
        type: 'radio'
      }]);

      menu._executeCommand({}, menu.items[0].commandId);
      expect(menu.items[0].checked).to.be.true('menu item checked');
      menu._executeCommand({}, menu.items[0].commandId);
      expect(menu.items[0].checked).to.be.true('menu item checked');
    });

    describe('MenuItem group properties', () => {
      const template: MenuItemConstructorOptions[] = [];

      const findRadioGroups = (template: MenuItemConstructorOptions[]) => {
        const groups = [];
        let cur: { begin?: number, end?: number } | null = null;
        for (let i = 0; i <= template.length; i++) {
          if (cur && ((i === template.length) || (template[i].type !== 'radio'))) {
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
          expect(findChecked(menu.items, g.begin!, g.end!)).to.deep.equal([g.begin]);
        }
      });

      it('should assign groupId automatically', () => {
        const menu = Menu.buildFromTemplate(template);

        const usedGroupIds = new Set();
        const groups = findRadioGroups(template);
        for (const g of groups) {
          const groupId = (menu.items[g.begin!] as any).groupId;

          // groupId should be previously unused
          // expect(usedGroupIds.has(groupId)).to.be.false('group id present')
          expect(usedGroupIds).not.to.contain(groupId);
          usedGroupIds.add(groupId);

          // everything in the group should have the same id
          for (let i = g.begin!; i < g.end!; ++i) {
            expect((menu.items[i] as any).groupId).to.equal(groupId);
          }
        }
      });

      it("setting 'checked' should flip other items' 'checked' property", () => {
        const menu = Menu.buildFromTemplate(template);

        const groups = findRadioGroups(template);
        for (const g of groups) {
          expect(findChecked(menu.items, g.begin!, g.end!)).to.deep.equal([]);

          menu.items[g.begin!].checked = true;
          expect(findChecked(menu.items, g.begin!, g.end!)).to.deep.equal([g.begin!]);

          menu.items[g.end! - 1].checked = true;
          expect(findChecked(menu.items, g.begin!, g.end!)).to.deep.equal([g.end! - 1]);
        }
      });
    });
  });

  describe('MenuItem role execution', () => {
    afterEach(closeAllWindows);
    it('does not try to execute roles without a valid role property', () => {
      const win = new BrowserWindow({ show: false, width: 200, height: 200 });
      const item = new MenuItem({ role: 'asdfghjkl' as any });

      const canExecute = execute(item.role as any, win, win.webContents);
      expect(canExecute).to.be.false('can execute');
    });

    it('executes roles with native role functions', () => {
      const win = new BrowserWindow({ show: false, width: 200, height: 200 });
      const item = new MenuItem({ role: 'reload' });

      const canExecute = execute(item.role as any, win, win.webContents);
      expect(canExecute).to.be.true('can execute');
    });

    it('execute roles with non-native role functions', () => {
      const win = new BrowserWindow({ show: false, width: 200, height: 200 });
      const item = new MenuItem({ role: 'resetZoom' });

      const canExecute = execute(item.role as any, win, win.webContents);
      expect(canExecute).to.be.true('can execute');
    });
  });

  describe('MenuItem command id', () => {
    it('cannot be overwritten', () => {
      const item = new MenuItem({ label: 'item' });

      const commandId = item.commandId;
      expect(commandId).to.not.be.undefined('command id');
      expect(() => {
        item.commandId = `${commandId}-modified` as any;
      }).to.throw(/Cannot assign to read only property/);
      expect(item.commandId).to.equal(commandId);
    });
  });

  describe('MenuItem with invalid type', () => {
    it('throws an exception', () => {
      expect(() => {
        Menu.buildFromTemplate([{
          label: 'text',
          type: 'not-a-type' as any
        }]);
      }).to.throw(/Unknown menu item type: not-a-type/);
    });
  });

  describe('MenuItem with submenu type and missing submenu', () => {
    it('throws an exception', () => {
      expect(() => {
        Menu.buildFromTemplate([{
          label: 'text',
          type: 'submenu'
        }]);
      }).to.throw(/Invalid submenu/);
    });
  });

  describe('MenuItem role', () => {
    it('returns undefined for items without default accelerator', () => {
      const list = keys(roleList).filter(key => !roleList[key].accelerator);

      for (const role of list) {
        const item = new MenuItem({ role: role as any });
        expect(item.getDefaultRoleAccelerator()).to.be.undefined('default accelerator');
      }
    });

    it('returns the correct default label', () => {
      for (const role of keys(roleList)) {
        const item = new MenuItem({ role: role as any });
        const label: string = roleList[role].label;
        expect(item.label).to.equal(label);
      }
    });

    it('returns the correct default accelerator', () => {
      const list = keys(roleList).filter(key => roleList[key].accelerator);

      for (const role of list) {
        const item = new MenuItem({ role: role as any });
        const accelerator = roleList[role].accelerator;
        expect(item.getDefaultRoleAccelerator()).to.equal(accelerator);
      }
    });

    it('allows a custom accelerator and label to be set', () => {
      const item = new MenuItem({
        role: 'close',
        label: 'Custom Close!',
        accelerator: 'D'
      });

      expect(item.label).to.equal('Custom Close!');
      expect(item.accelerator).to.equal('D');
      expect(item.getDefaultRoleAccelerator()).to.equal('CommandOrControl+W');
    });
  });

  ifdescribe(process.platform === 'darwin')('MenuItem appMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'appMenu' });

      expect(item.label).to.equal(app.name);
      expect(item.submenu!.items[0].role).to.equal('about');
      expect(item.submenu!.items[1].type).to.equal('separator');
      expect(item.submenu!.items[2].role).to.equal('services');
      expect(item.submenu!.items[3].type).to.equal('separator');
      expect(item.submenu!.items[4].role).to.equal('hide');
      expect(item.submenu!.items[5].role).to.equal('hideothers');
      expect(item.submenu!.items[6].role).to.equal('unhide');
      expect(item.submenu!.items[7].type).to.equal('separator');
      expect(item.submenu!.items[8].role).to.equal('quit');
    });

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'appMenu',
        submenu: [{
          role: 'close'
        }]
      });
      expect(item.label).to.equal(app.name);
      expect(item.submenu!.items[0].role).to.equal('close');
    });
  });

  describe('MenuItem fileMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'fileMenu' });

      expect(item.label).to.equal('File');
      if (process.platform === 'darwin') {
        expect(item.submenu!.items[0].role).to.equal('close');
      } else {
        expect(item.submenu!.items[0].role).to.equal('quit');
      }
    });

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'fileMenu',
        submenu: [{
          role: 'about'
        }]
      });
      expect(item.label).to.equal('File');
      expect(item.submenu!.items[0].role).to.equal('about');
    });
  });

  describe('MenuItem editMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'editMenu' });

      expect(item.label).to.equal('Edit');
      expect(item.submenu!.items[0].role).to.equal('undo');
      expect(item.submenu!.items[1].role).to.equal('redo');
      expect(item.submenu!.items[2].type).to.equal('separator');
      expect(item.submenu!.items[3].role).to.equal('cut');
      expect(item.submenu!.items[4].role).to.equal('copy');
      expect(item.submenu!.items[5].role).to.equal('paste');

      if (process.platform === 'darwin') {
        expect(item.submenu!.items[6].role).to.equal('pasteandmatchstyle');
        expect(item.submenu!.items[7].role).to.equal('delete');
        expect(item.submenu!.items[8].role).to.equal('selectall');
        expect(item.submenu!.items[9].type).to.equal('separator');
        expect(item.submenu!.items[10].label).to.equal('Substitutions');
        expect(item.submenu!.items[10].submenu!.items[0].role).to.equal('showsubstitutions');
        expect(item.submenu!.items[10].submenu!.items[1].type).to.equal('separator');
        expect(item.submenu!.items[10].submenu!.items[2].role).to.equal('togglesmartquotes');
        expect(item.submenu!.items[10].submenu!.items[3].role).to.equal('togglesmartdashes');
        expect(item.submenu!.items[10].submenu!.items[4].role).to.equal('toggletextreplacement');
        expect(item.submenu!.items[11].label).to.equal('Speech');
        expect(item.submenu!.items[11].submenu!.items[0].role).to.equal('startspeaking');
        expect(item.submenu!.items[11].submenu!.items[1].role).to.equal('stopspeaking');
      } else {
        expect(item.submenu!.items[6].role).to.equal('delete');
        expect(item.submenu!.items[7].type).to.equal('separator');
        expect(item.submenu!.items[8].role).to.equal('selectall');
      }
    });

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'editMenu',
        submenu: [{
          role: 'close'
        }]
      });
      expect(item.label).to.equal('Edit');
      expect(item.submenu!.items[0].role).to.equal('close');
    });
  });

  describe('MenuItem viewMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'viewMenu' });

      expect(item.label).to.equal('View');
      expect(item.submenu!.items[0].role).to.equal('reload');
      expect(item.submenu!.items[1].role).to.equal('forcereload');
      expect(item.submenu!.items[2].role).to.equal('toggledevtools');
      expect(item.submenu!.items[3].type).to.equal('separator');
      expect(item.submenu!.items[4].role).to.equal('resetzoom');
      expect(item.submenu!.items[5].role).to.equal('zoomin');
      expect(item.submenu!.items[6].role).to.equal('zoomout');
      expect(item.submenu!.items[7].type).to.equal('separator');
      expect(item.submenu!.items[8].role).to.equal('togglefullscreen');
    });

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'viewMenu',
        submenu: [{
          role: 'close'
        }]
      });
      expect(item.label).to.equal('View');
      expect(item.submenu!.items[0].role).to.equal('close');
    });
  });

  describe('MenuItem windowMenu', () => {
    it('includes a default submenu layout when submenu is empty', () => {
      const item = new MenuItem({ role: 'windowMenu' });

      expect(item.label).to.equal('Window');
      expect(item.submenu!.items[0].role).to.equal('minimize');
      expect(item.submenu!.items[1].role).to.equal('zoom');

      if (process.platform === 'darwin') {
        expect(item.submenu!.items[2].type).to.equal('separator');
        expect(item.submenu!.items[3].role).to.equal('front');
      } else {
        expect(item.submenu!.items[2].role).to.equal('close');
      }
    });

    it('overrides default layout when submenu is specified', () => {
      const item = new MenuItem({
        role: 'windowMenu',
        submenu: [{ role: 'copy' }]
      });

      expect(item.label).to.equal('Window');
      expect(item.submenu!.items[0].role).to.equal('copy');
    });
  });

  describe('MenuItem with custom properties in constructor', () => {
    it('preserves the custom properties', () => {
      const template = [{
        label: 'menu 1',
        customProp: 'foo',
        submenu: []
      }];

      const menu = Menu.buildFromTemplate(template);
      menu.items[0].submenu!.append(new MenuItem({
        label: 'item 1',
        customProp: 'bar',
        overrideProperty: 'oops not allowed'
      } as any));

      expect((menu.items[0] as any).customProp).to.equal('foo');
      expect(menu.items[0].submenu!.items[0].label).to.equal('item 1');
      expect((menu.items[0].submenu!.items[0] as any).customProp).to.equal('bar');
      expect((menu.items[0].submenu!.items[0] as any).overrideProperty).to.be.a('function');
    });
  });

  describe('MenuItem accelerators', () => {
    const isDarwin = () => {
      return (process.platform === 'darwin');
    };

    it('should display modifiers correctly for simple keys', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'CmdOrCtrl+A' },
        { label: 'text', accelerator: 'Shift+A' },
        { label: 'text', accelerator: 'Alt+A' }
      ]);

      expect(menu._getAcceleratorTextAt(0)).to.equal(isDarwin() ? 'Command+A' : 'Ctrl+A');
      expect(menu._getAcceleratorTextAt(1)).to.equal('Shift+A');
      expect(menu._getAcceleratorTextAt(2)).to.equal('Alt+A');
    });

    it('should display modifiers correctly for special keys', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'CmdOrCtrl+Tab' },
        { label: 'text', accelerator: 'Shift+Tab' },
        { label: 'text', accelerator: 'Alt+Tab' }
      ]);

      expect(menu._getAcceleratorTextAt(0)).to.equal(isDarwin() ? 'Command+Tab' : 'Ctrl+Tab');
      expect(menu._getAcceleratorTextAt(1)).to.equal('Shift+Tab');
      expect(menu._getAcceleratorTextAt(2)).to.equal('Alt+Tab');
    });

    it('should not display modifiers twice', () => {
      const menu = Menu.buildFromTemplate([
        { label: 'text', accelerator: 'Shift+Shift+A' },
        { label: 'text', accelerator: 'Shift+Shift+Tab' }
      ]);

      expect(menu._getAcceleratorTextAt(0)).to.equal('Shift+A');
      expect(menu._getAcceleratorTextAt(1)).to.equal('Shift+Tab');
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

      expect(menu._getAcceleratorTextAt(0)).to.equal('Ctrl+Shift+=');
      expect(menu._getAcceleratorTextAt(1)).to.equal('Ctrl++');
      expect(menu._getAcceleratorTextAt(2)).to.equal('Ctrl+Shift+3');
      expect(menu._getAcceleratorTextAt(3)).to.equal('Ctrl+#');
      expect(menu._getAcceleratorTextAt(4)).to.equal('Ctrl+Shift+/');
      expect(menu._getAcceleratorTextAt(5)).to.equal('Ctrl+?');
    });
  });
});
