import * as cp from 'child_process';
import * as path from 'path';
import { expect } from 'chai';
import { BrowserWindow, Menu, MenuItem } from 'electron/main';
import { sortMenuItems } from '../lib/browser/api/menu-utils';
import { emittedOnce } from './events-helpers';
import { ifit } from './spec-helpers';
import { closeWindow } from './window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('Menu module', function () {
  describe('Menu.buildFromTemplate', () => {
    it('should be able to attach extra fields', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'text',
          extra: 'field'
        } as MenuItem | Record<string, any>
      ]);
      expect((menu.items[0] as any).extra).to.equal('field');
    });

    it('should be able to accept only MenuItems', () => {
      const menu = Menu.buildFromTemplate([
        new MenuItem({ label: 'one' }),
        new MenuItem({ label: 'two' })
      ]);
      expect(menu.items[0].label).to.equal('one');
      expect(menu.items[1].label).to.equal('two');
    });

    it('should be able to accept only MenuItems in a submenu', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'one',
          submenu: [
            new MenuItem({ label: 'two' }) as any
          ]
        }
      ]);
      expect(menu.items[0].label).to.equal('one');
      expect(menu.items[0].submenu!.items[0].label).to.equal('two');
    });

    it('should be able to accept MenuItems and plain objects', () => {
      const menu = Menu.buildFromTemplate([
        new MenuItem({ label: 'one' }),
        { label: 'two' }
      ]);
      expect(menu.items[0].label).to.equal('one');
      expect(menu.items[1].label).to.equal('two');
    });

    it('does not modify the specified template', () => {
      const template = [{ label: 'text', submenu: [{ label: 'sub' }] }];
      const templateCopy = JSON.parse(JSON.stringify(template));
      Menu.buildFromTemplate(template);
      expect(template).to.deep.equal(templateCopy);
    });

    it('does not throw exceptions for undefined/null values', () => {
      expect(() => {
        Menu.buildFromTemplate([
          {
            label: 'text',
            accelerator: undefined
          },
          {
            label: 'text again',
            accelerator: null as any
          }
        ]);
      }).to.not.throw();
    });

    it('does throw exceptions for empty objects and null values', () => {
      expect(() => {
        Menu.buildFromTemplate([{}, null as any]);
      }).to.throw(/Invalid template for MenuItem: must have at least one of label, role or type/);
    });

    it('does throw exception for object without role, label, or type attribute', () => {
      expect(() => {
        Menu.buildFromTemplate([{ visible: true }]);
      }).to.throw(/Invalid template for MenuItem: must have at least one of label, role or type/);
    });

    it('does throw exception for undefined', () => {
      expect(() => {
        Menu.buildFromTemplate([undefined as any]);
      }).to.throw(/Invalid template for MenuItem: must have at least one of label, role or type/);
    });

    it('throws when an non-array is passed as a template', () => {
      expect(() => {
        Menu.buildFromTemplate('hello' as any);
      }).to.throw(/Invalid template for Menu: Menu template must be an array/);
    });

    describe('Menu sorting and building', () => {
      describe('sorts groups', () => {
        it('does a simple sort', () => {
          const items = [
            {
              label: 'two',
              id: '2',
              afterGroupContaining: ['1']
            },
            { type: 'separator' },
            {
              id: '1',
              label: 'one'
            }
          ];

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two',
              afterGroupContaining: ['1']
            }
          ];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });

        it('does a simple sort with MenuItems', () => {
          const firstItem = new MenuItem({ id: '1', label: 'one' });
          const secondItem = new MenuItem({
            label: 'two',
            id: '2',
            afterGroupContaining: ['1']
          });
          const sep = new MenuItem({ type: 'separator' });

          const items = [secondItem, sep, firstItem];
          const expected = [firstItem, sep, secondItem];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });

        it('resolves cycles by ignoring things that conflict', () => {
          const items = [
            {
              id: '2',
              label: 'two',
              afterGroupContaining: ['1']
            },
            { type: 'separator' },
            {
              id: '1',
              label: 'one',
              afterGroupContaining: ['2']
            }
          ];

          const expected = [
            {
              id: '1',
              label: 'one',
              afterGroupContaining: ['2']
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two',
              afterGroupContaining: ['1']
            }
          ];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });

        it('ignores references to commands that do not exist', () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two',
              afterGroupContaining: ['does-not-exist']
            }
          ];

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two',
              afterGroupContaining: ['does-not-exist']
            }
          ];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });

        it('only respects the first matching [before|after]GroupContaining rule in a given group', () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              beforeGroupContaining: ['1']
            },
            {
              id: '4',
              label: 'four',
              afterGroupContaining: ['2']
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            }
          ];

          const expected = [
            {
              id: '3',
              label: 'three',
              beforeGroupContaining: ['1']
            },
            {
              id: '4',
              label: 'four',
              afterGroupContaining: ['2']
            },
            { type: 'separator' },
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            }
          ];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });
      });

      describe('moves an item to a different group by merging groups', () => {
        it('can move a group of one item', () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              after: ['1']
            },
            { type: 'separator' }
          ];

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three',
              after: ['1']
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            }
          ];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });

        it("moves all items in the moving item's group", () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              after: ['1']
            },
            {
              id: '4',
              label: 'four'
            },
            { type: 'separator' }
          ];

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three',
              after: ['1']
            },
            {
              id: '4',
              label: 'four'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            }
          ];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });

        it("ignores positions relative to commands that don't exist", () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              after: ['does-not-exist']
            },
            {
              id: '4',
              label: 'four',
              after: ['1']
            },
            { type: 'separator' }
          ];

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three',
              after: ['does-not-exist']
            },
            {
              id: '4',
              label: 'four',
              after: ['1']
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            }
          ];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });

        it('can handle recursive group merging', () => {
          const items = [
            {
              id: '1',
              label: 'one',
              after: ['3']
            },
            {
              id: '2',
              label: 'two',
              before: ['1']
            },
            {
              id: '3',
              label: 'three'
            }
          ];

          const expected = [
            {
              id: '3',
              label: 'three'
            },
            {
              id: '2',
              label: 'two',
              before: ['1']
            },
            {
              id: '1',
              label: 'one',
              after: ['3']
            }
          ];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });

        it('can merge multiple groups when given a list of before/after commands', () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              after: ['1', '2']
            }
          ];

          const expected = [
            {
              id: '2',
              label: 'two'
            },
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three',
              after: ['1', '2']
            }
          ];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });

        it('can merge multiple groups based on both before/after commands', () => {
          const items = [
            {
              id: '1',
              label: 'one'
            },
            { type: 'separator' },
            {
              id: '2',
              label: 'two'
            },
            { type: 'separator' },
            {
              id: '3',
              label: 'three',
              after: ['1'],
              before: ['2']
            }
          ];

          const expected = [
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three',
              after: ['1'],
              before: ['2']
            },
            {
              id: '2',
              label: 'two'
            }
          ];

          expect(sortMenuItems(items)).to.deep.equal(expected);
        });
      });

      it('should position before existing item', () => {
        const menu = Menu.buildFromTemplate([
          {
            id: '2',
            label: 'two'
          }, {
            id: '3',
            label: 'three'
          }, {
            id: '1',
            label: 'one',
            before: ['2']
          }
        ]);

        expect(menu.items[0].label).to.equal('one');
        expect(menu.items[1].label).to.equal('two');
        expect(menu.items[2].label).to.equal('three');
      });

      it('should position after existing item', () => {
        const menu = Menu.buildFromTemplate([
          {
            id: '2',
            label: 'two',
            after: ['1']
          },
          {
            id: '1',
            label: 'one'
          }, {
            id: '3',
            label: 'three'
          }
        ]);

        expect(menu.items[0].label).to.equal('one');
        expect(menu.items[1].label).to.equal('two');
        expect(menu.items[2].label).to.equal('three');
      });

      it('should filter excess menu separators', () => {
        const menuOne = Menu.buildFromTemplate([
          {
            type: 'separator'
          }, {
            label: 'a'
          }, {
            label: 'b'
          }, {
            label: 'c'
          }, {
            type: 'separator'
          }
        ]);

        expect(menuOne.items).to.have.length(3);
        expect(menuOne.items[0].label).to.equal('a');
        expect(menuOne.items[1].label).to.equal('b');
        expect(menuOne.items[2].label).to.equal('c');

        const menuTwo = Menu.buildFromTemplate([
          {
            type: 'separator'
          }, {
            type: 'separator'
          }, {
            label: 'a'
          }, {
            label: 'b'
          }, {
            label: 'c'
          }, {
            type: 'separator'
          }, {
            type: 'separator'
          }
        ]);

        expect(menuTwo.items).to.have.length(3);
        expect(menuTwo.items[0].label).to.equal('a');
        expect(menuTwo.items[1].label).to.equal('b');
        expect(menuTwo.items[2].label).to.equal('c');
      });

      it('should continue inserting items at next index when no specifier is present', () => {
        const menu = Menu.buildFromTemplate([
          {
            id: '2',
            label: 'two'
          }, {
            id: '3',
            label: 'three'
          }, {
            id: '4',
            label: 'four'
          }, {
            id: '5',
            label: 'five'
          }, {
            id: '1',
            label: 'one',
            before: ['2']
          }
        ]);

        expect(menu.items[0].label).to.equal('one');
        expect(menu.items[1].label).to.equal('two');
        expect(menu.items[2].label).to.equal('three');
        expect(menu.items[3].label).to.equal('four');
        expect(menu.items[4].label).to.equal('five');
      });

      it('should continue inserting MenuItems at next index when no specifier is present', () => {
        const menu = Menu.buildFromTemplate([
          new MenuItem({
            id: '2',
            label: 'two'
          }), new MenuItem({
            id: '3',
            label: 'three'
          }), new MenuItem({
            id: '4',
            label: 'four'
          }), new MenuItem({
            id: '5',
            label: 'five'
          }), new MenuItem({
            id: '1',
            label: 'one',
            before: ['2']
          })
        ]);

        expect(menu.items[0].label).to.equal('one');
        expect(menu.items[1].label).to.equal('two');
        expect(menu.items[2].label).to.equal('three');
        expect(menu.items[3].label).to.equal('four');
        expect(menu.items[4].label).to.equal('five');
      });
    });
  });

  describe('Menu.getMenuItemById', () => {
    it('should return the item with the given id', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'View',
          submenu: [
            {
              label: 'Enter Fullscreen',
              accelerator: 'ControlCommandF',
              id: 'fullScreen'
            }
          ]
        }
      ]);
      const fsc = menu.getMenuItemById('fullScreen');
      expect(menu.items[0].submenu!.items[0]).to.equal(fsc);
    });

    it('should return the separator with the given id', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'Item 1',
          id: 'item_1'
        },
        {
          id: 'separator',
          type: 'separator'
        },
        {
          label: 'Item 2',
          id: 'item_2'
        }
      ]);
      const separator = menu.getMenuItemById('separator');
      expect(separator).to.be.an('object');
      expect(separator).to.equal(menu.items[1]);
    });
  });

  describe('Menu.insert', () => {
    it('should throw when attempting to insert at out-of-range indices', () => {
      const menu = Menu.buildFromTemplate([
        { label: '1' },
        { label: '2' },
        { label: '3' }
      ]);

      const item = new MenuItem({ label: 'badInsert' });

      expect(() => {
        menu.insert(9999, item);
      }).to.throw(/Position 9999 cannot be greater than the total MenuItem count/);

      expect(() => {
        menu.insert(-9999, item);
      }).to.throw(/Position -9999 cannot be less than 0/);
    });

    it('should store item in @items by its index', () => {
      const menu = Menu.buildFromTemplate([
        { label: '1' },
        { label: '2' },
        { label: '3' }
      ]);

      const item = new MenuItem({ label: 'inserted' });

      menu.insert(1, item);
      expect(menu.items[0].label).to.equal('1');
      expect(menu.items[1].label).to.equal('inserted');
      expect(menu.items[2].label).to.equal('2');
      expect(menu.items[3].label).to.equal('3');
    });
  });

  describe('Menu.append', () => {
    it('should add the item to the end of the menu', () => {
      const menu = Menu.buildFromTemplate([
        { label: '1' },
        { label: '2' },
        { label: '3' }
      ]);

      const item = new MenuItem({ label: 'inserted' });
      menu.append(item);

      expect(menu.items[0].label).to.equal('1');
      expect(menu.items[1].label).to.equal('2');
      expect(menu.items[2].label).to.equal('3');
      expect(menu.items[3].label).to.equal('inserted');
    });
  });

  describe('Menu.popup', () => {
    let w: BrowserWindow;
    let menu: Menu;

    beforeEach(() => {
      w = new BrowserWindow({ show: false, width: 200, height: 200 });
      menu = Menu.buildFromTemplate([
        { label: '1' },
        { label: '2' },
        { label: '3' }
      ]);
    });

    afterEach(async () => {
      menu.closePopup();
      menu.closePopup(w);
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });

    it('throws an error if options is not an object', () => {
      expect(() => {
        menu.popup('this is a string, not an object' as any);
      }).to.throw(/Options must be an object/);
    });

    it('allows for options to be optional', () => {
      expect(() => {
        menu.popup({});
      }).to.not.throw();
    });

    it('should emit menu-will-show event', (done) => {
      menu.on('menu-will-show', () => { done(); });
      menu.popup({ window: w });
    });

    it('should emit menu-will-close event', (done) => {
      menu.on('menu-will-close', () => { done(); });
      menu.popup({ window: w });
      // https://github.com/electron/electron/issues/19411
      setTimeout(() => {
        menu.closePopup();
      });
    });

    it('returns immediately', () => {
      const input = { window: w, x: 100, y: 101 };
      const output = menu.popup(input) as unknown as {x: number, y: number, browserWindow: BrowserWindow};
      expect(output.x).to.equal(input.x);
      expect(output.y).to.equal(input.y);
      expect(output.browserWindow).to.equal(input.window);
    });

    it('works without a given BrowserWindow and options', () => {
      const { browserWindow, x, y } = menu.popup({ x: 100, y: 101 }) as unknown as {x: number, y: number, browserWindow: BrowserWindow};

      expect(browserWindow.constructor.name).to.equal('BrowserWindow');
      expect(x).to.equal(100);
      expect(y).to.equal(101);
    });

    it('works with a given BrowserWindow, options and callback', (done) => {
      const { x, y } = menu.popup({
        window: w,
        x: 100,
        y: 101,
        callback: () => done()
      }) as unknown as {x: number, y: number};

      expect(x).to.equal(100);
      expect(y).to.equal(101);
      // https://github.com/electron/electron/issues/19411
      setTimeout(() => {
        menu.closePopup();
      });
    });

    it('works with a given BrowserWindow, no options, and a callback', (done) => {
      menu.popup({ window: w, callback: () => done() });
      // https://github.com/electron/electron/issues/19411
      setTimeout(() => {
        menu.closePopup();
      });
    });

    it('prevents menu from getting garbage-collected when popuping', (done) => {
      const menu = Menu.buildFromTemplate([{ role: 'paste' }]);
      menu.popup({ window: w });

      // Keep a weak reference to the menu.
      const v8Util = process.electronBinding('v8_util');
      const map = v8Util.createIDWeakMap<Electron.Menu>();
      map.set(0, menu);

      setTimeout(() => {
        // Do garbage collection, since |menu| is not referenced in this closure
        // it would be gone after next call.
        v8Util.requestGarbageCollectionForTesting();
        setTimeout(() => {
          // Try to receive menu from weak reference.
          if (map.has(0)) {
            map.get(0)!.closePopup();
            done();
          } else {
            done('Menu is garbage-collected while popuping');
          }
        });
      });
    });
  });

  describe('Menu.setApplicationMenu', () => {
    it('sets a menu', () => {
      const menu = Menu.buildFromTemplate([
        { label: '1' },
        { label: '2' }
      ]);

      Menu.setApplicationMenu(menu);
      expect(Menu.getApplicationMenu()).to.not.be.null('application menu');
    });

    // TODO(nornagon): this causes the focus handling tests to fail
    it.skip('unsets a menu with null', () => {
      Menu.setApplicationMenu(null);
      expect(Menu.getApplicationMenu()).to.be.null('application menu');
    });

    ifit(process.platform !== 'darwin')('does not override menu visibility on startup', async () => {
      const appPath = path.join(fixturesPath, 'api', 'test-menu-visibility');
      const appProcess = cp.spawn(process.execPath, [appPath]);

      let output = '';
      await new Promise((resolve) => {
        appProcess.stdout.on('data', data => {
          output += data;
          if (data.indexOf('Window has') > -1) {
            resolve();
          }
        });
      });
      expect(output).to.include('Window has no menu');
    });

    ifit(process.platform !== 'darwin')('does not override null menu on startup', async () => {
      const appPath = path.join(fixturesPath, 'api', 'test-menu-null');
      const appProcess = cp.spawn(process.execPath, [appPath]);

      let output = '';
      appProcess.stdout.on('data', data => { output += data; });
      appProcess.stderr.on('data', data => { output += data; });

      const [code] = await emittedOnce(appProcess, 'exit');
      if (!output.includes('Window has no menu')) {
        console.log(code, output);
      }
      expect(output).to.include('Window has no menu');
    });
  });
});
