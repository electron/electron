import { BrowserWindow, Menu, MenuItem } from 'electron/main';

import { afterEach, assert, beforeEach, describe, expect, it } from 'vitest';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { singleModifierCombinations } from './lib/accelerator-helpers.ts';
import { defer, ifit, isWayland, spawnAndWait } from './lib/spec-helpers.ts';
import { closeWindow } from './lib/window-helpers.ts';

const fixturesPath = path.resolve(import.meta.dirname, 'fixtures');

describe('Menu module', () => {
  it('sets the correct class name on the prototype', () => {
    expect(Menu.prototype.constructor.name).toBe('Menu');
  });

  describe('Menu.buildFromTemplate', () => {
    it('should be able to attach extra fields', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'text',
          extra: 'field'
        } as MenuItem | Record<string, any>
      ]);
      expect((menu.items[0] as any).extra).toBe('field');
    });

    it('should be able to accept only MenuItems', () => {
      const menu = Menu.buildFromTemplate([new MenuItem({ label: 'one' }), new MenuItem({ label: 'two' })]);
      expect(menu.items[0].label).toBe('one');
      expect(menu.items[1].label).toBe('two');
    });

    it('should be able to accept only MenuItems in a submenu', () => {
      const menu = Menu.buildFromTemplate([
        {
          label: 'one',
          submenu: [new MenuItem({ label: 'two' }) as any]
        }
      ]);
      expect(menu.items[0].label).toBe('one');
      expect(menu.items[0].submenu!.items[0].label).toBe('two');
    });

    it('should be able to accept MenuItems and plain objects', () => {
      const menu = Menu.buildFromTemplate([new MenuItem({ label: 'one' }), { label: 'two' }]);
      expect(menu.items[0].label).toBe('one');
      expect(menu.items[1].label).toBe('two');
    });

    it('does not modify the specified template', () => {
      const template = [{ label: 'text', submenu: [{ label: 'sub' }] }];
      const templateCopy = JSON.parse(JSON.stringify(template));
      Menu.buildFromTemplate(template);
      expect(template).toEqual(templateCopy);
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
      }).not.toThrow();
    });

    it('does throw exceptions for empty objects and null values', () => {
      expect(() => {
        Menu.buildFromTemplate([{}, null as any]);
      }).toThrow(/Invalid template for MenuItem: must have at least one of label, role or type/);
    });

    it('does throw exception for object without role, label, or type attribute', () => {
      expect(() => {
        Menu.buildFromTemplate([{ visible: true }]);
      }).toThrow(/Invalid template for MenuItem: must have at least one of label, role or type/);
    });

    it('does throw exception for undefined', () => {
      expect(() => {
        Menu.buildFromTemplate([undefined as any]);
      }).toThrow(/Invalid template for MenuItem: must have at least one of label, role or type/);
    });

    it('throws when an non-array is passed as a template', () => {
      expect(() => {
        Menu.buildFromTemplate('hello' as any);
      }).toThrow(/Invalid template for Menu: Menu template must be an array/);
    });

    describe('Menu sorting and building', () => {
      describe('sorts groups', () => {
        it('does a simple sort', () => {
          const menu = Menu.buildFromTemplate([
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
          ]);

          const expected: Electron.MenuItemConstructorOptions[] = [
            {
              id: '1',
              label: 'one',
              type: 'normal'
            },
            {
              id: undefined,
              label: '',
              type: 'separator'
            },
            {
              id: '2',
              label: 'two',
              type: 'normal'
            }
          ];

          expect(menu.items.map(({ id, label, type }) => ({ id, label, type }))).toEqual(expected);
        });

        it('does a simple sort with MenuItems', () => {
          const firstItem = new MenuItem({ id: '1', label: 'one' });
          const secondItem = new MenuItem({
            label: 'two',
            id: '2',
            afterGroupContaining: ['1']
          });
          const sep = new MenuItem({ type: 'separator' });

          const menu = Menu.buildFromTemplate([secondItem, sep, firstItem]);
          const expected = [firstItem, sep, secondItem];

          expect(menu.items).toEqual(expected);
        });

        it('resolves cycles by ignoring things that conflict', () => {
          const menu = Menu.buildFromTemplate([
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
          ]);

          const expected: Electron.MenuItemConstructorOptions[] = [
            {
              id: '1',
              label: 'one',
              type: 'normal'
            },
            {
              id: undefined,
              label: '',
              type: 'separator'
            },
            {
              id: '2',
              label: 'two',
              type: 'normal'
            }
          ];

          expect(menu.items.map(({ id, label, type }) => ({ id, label, type }))).toEqual(expected);
        });

        it('ignores references to commands that do not exist', () => {
          const menu = Menu.buildFromTemplate([
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
          ]);

          const expected: Electron.MenuItemConstructorOptions[] = [
            {
              id: '1',
              label: 'one',
              type: 'normal'
            },
            {
              id: undefined,
              label: '',
              type: 'separator'
            },
            {
              id: '2',
              label: 'two',
              type: 'normal'
            }
          ];

          expect(menu.items.map(({ id, label, type }) => ({ id, label, type }))).toEqual(expected);
        });

        it('only respects the first matching [before|after]GroupContaining rule in a given group', () => {
          const menu = Menu.buildFromTemplate([
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
          ]);

          const expected: Electron.MenuItemConstructorOptions[] = [
            {
              id: '3',
              label: 'three',
              type: 'normal'
            },
            {
              id: '4',
              label: 'four',
              type: 'normal'
            },
            {
              id: undefined,
              label: '',
              type: 'separator'
            },
            {
              id: '1',
              label: 'one',
              type: 'normal'
            },
            {
              id: undefined,
              label: '',
              type: 'separator'
            },
            {
              id: '2',
              label: 'two',
              type: 'normal'
            }
          ];

          expect(menu.items.map(({ id, label, type }) => ({ id, label, type }))).toEqual(expected);
        });
      });

      describe('moves an item to a different group by merging groups', () => {
        it('can move a group of one item', () => {
          const menu = Menu.buildFromTemplate([
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
          ]);

          const expected: Electron.MenuItemConstructorOptions[] = [
            {
              id: '1',
              label: 'one',
              type: 'normal'
            },
            {
              id: '3',
              label: 'three',
              type: 'normal'
            },
            {
              id: undefined,
              label: '',
              type: 'separator'
            },
            {
              id: '2',
              label: 'two',
              type: 'normal'
            }
          ];

          expect(menu.items.map(({ id, label, type }) => ({ id, label, type }))).toEqual(expected);
        });

        it("moves all items in the moving item's group", () => {
          const menu = Menu.buildFromTemplate([
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
          ]);

          const expected: Electron.MenuItemConstructorOptions[] = [
            {
              id: '1',
              label: 'one',
              type: 'normal'
            },
            {
              id: '3',
              label: 'three',
              type: 'normal'
            },
            {
              id: '4',
              label: 'four',
              type: 'normal'
            },
            {
              id: undefined,
              label: '',
              type: 'separator'
            },
            {
              id: '2',
              label: 'two',
              type: 'normal'
            }
          ];

          expect(menu.items.map(({ id, label, type }) => ({ id, label, type }))).toEqual(expected);
        });

        it("ignores positions relative to commands that don't exist", () => {
          const menu = Menu.buildFromTemplate([
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
          ]);

          const expected: Electron.MenuItemConstructorOptions[] = [
            {
              id: '1',
              label: 'one',
              type: 'normal'
            },
            {
              id: '3',
              label: 'three',
              type: 'normal'
            },
            {
              id: '4',
              label: 'four',
              type: 'normal'
            },
            {
              id: undefined,
              label: '',
              type: 'separator'
            },
            {
              id: '2',
              label: 'two',
              type: 'normal'
            }
          ];

          expect(menu.items.map(({ id, label, type }) => ({ id, label, type }))).toEqual(expected);
        });

        it('can handle recursive group merging', () => {
          const menu = Menu.buildFromTemplate([
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
          ]);

          const expected: Electron.MenuItemConstructorOptions[] = [
            {
              id: '3',
              label: 'three'
            },
            {
              id: '2',
              label: 'two'
            },
            {
              id: '1',
              label: 'one'
            }
          ];

          expect(menu.items.map(({ id, label }) => ({ id, label }))).toEqual(expected);
        });

        it('can merge multiple groups when given a list of before/after commands', () => {
          const menu = Menu.buildFromTemplate([
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
          ]);

          const expected: Electron.MenuItemConstructorOptions[] = [
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
              label: 'three'
            }
          ];

          expect(menu.items.map(({ id, label }) => ({ id, label }))).toEqual(expected);
        });

        it('can merge multiple groups based on both before/after commands', () => {
          const menu = Menu.buildFromTemplate([
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
          ]);

          const expected: Electron.MenuItemConstructorOptions[] = [
            {
              id: '1',
              label: 'one'
            },
            {
              id: '3',
              label: 'three'
            },
            {
              id: '2',
              label: 'two'
            }
          ];

          expect(menu.items.map(({ id, label }) => ({ id, label }))).toEqual(expected);
        });
      });

      it('should position before existing item', () => {
        const menu = Menu.buildFromTemplate([
          {
            id: '2',
            label: 'two'
          },
          {
            id: '3',
            label: 'three'
          },
          {
            id: '1',
            label: 'one',
            before: ['2']
          }
        ]);

        expect(menu.items[0].label).toBe('one');
        expect(menu.items[1].label).toBe('two');
        expect(menu.items[2].label).toBe('three');
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
          },
          {
            id: '3',
            label: 'three'
          }
        ]);

        expect(menu.items[0].label).toBe('one');
        expect(menu.items[1].label).toBe('two');
        expect(menu.items[2].label).toBe('three');
      });

      it('should filter excess menu separators', () => {
        const menuOne = Menu.buildFromTemplate([
          {
            type: 'separator'
          },
          {
            label: 'a'
          },
          {
            label: 'b'
          },
          {
            label: 'c'
          },
          {
            type: 'separator'
          }
        ]);

        expect(menuOne.items).toHaveLength(3);
        expect(menuOne.items[0].label).toBe('a');
        expect(menuOne.items[1].label).toBe('b');
        expect(menuOne.items[2].label).toBe('c');

        const menuTwo = Menu.buildFromTemplate([
          {
            type: 'separator'
          },
          {
            type: 'separator'
          },
          {
            label: 'a'
          },
          {
            label: 'b'
          },
          {
            label: 'c'
          },
          {
            type: 'separator'
          },
          {
            type: 'separator'
          }
        ]);

        expect(menuTwo.items).toHaveLength(3);
        expect(menuTwo.items[0].label).toBe('a');
        expect(menuTwo.items[1].label).toBe('b');
        expect(menuTwo.items[2].label).toBe('c');
      });

      it('should only filter excess menu separators AFTER the re-ordering for before/after is done', () => {
        const menuOne = Menu.buildFromTemplate([
          {
            type: 'separator'
          },
          {
            type: 'normal',
            label: 'Foo',
            id: 'foo'
          },
          {
            type: 'normal',
            label: 'Bar',
            id: 'bar'
          },
          {
            type: 'separator',
            before: ['bar']
          }
        ]);

        expect(menuOne.items).toHaveLength(3);
        expect(menuOne.items[0].label).toBe('Foo');
        expect(menuOne.items[1].type).toBe('separator');
        expect(menuOne.items[2].label).toBe('Bar');
      });

      it('should continue inserting items at next index when no specifier is present', () => {
        const menu = Menu.buildFromTemplate([
          {
            id: '2',
            label: 'two'
          },
          {
            id: '3',
            label: 'three'
          },
          {
            id: '4',
            label: 'four'
          },
          {
            id: '5',
            label: 'five'
          },
          {
            id: '1',
            label: 'one',
            before: ['2']
          }
        ]);

        expect(menu.items[0].label).toBe('one');
        expect(menu.items[1].label).toBe('two');
        expect(menu.items[2].label).toBe('three');
        expect(menu.items[3].label).toBe('four');
        expect(menu.items[4].label).toBe('five');
      });

      it('should continue inserting MenuItems at next index when no specifier is present', () => {
        const menu = Menu.buildFromTemplate([
          new MenuItem({
            id: '2',
            label: 'two'
          }),
          new MenuItem({
            id: '3',
            label: 'three'
          }),
          new MenuItem({
            id: '4',
            label: 'four'
          }),
          new MenuItem({
            id: '5',
            label: 'five'
          }),
          new MenuItem({
            id: '1',
            label: 'one',
            before: ['2']
          })
        ]);

        expect(menu.items[0].label).toBe('one');
        expect(menu.items[1].label).toBe('two');
        expect(menu.items[2].label).toBe('three');
        expect(menu.items[3].label).toBe('four');
        expect(menu.items[4].label).toBe('five');
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
      expect(menu.items[0].submenu!.items[0]).toBe(fsc);
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
      expect(separator).toBeTypeOf('object');
      expect(separator).toBe(menu.items[1]);
    });
  });

  describe('Menu.insert', () => {
    it('should throw when attempting to insert at out-of-range indices', () => {
      const menu = Menu.buildFromTemplate([{ label: '1' }, { label: '2' }, { label: '3' }]);

      const item = new MenuItem({ label: 'badInsert' });

      expect(() => {
        menu.insert(9999, item);
      }).toThrow(/Position 9999 cannot be greater than the total MenuItem count/);

      expect(() => {
        menu.insert(-9999, item);
      }).toThrow(/Position -9999 cannot be less than 0/);
    });

    it('should store item in @items by its index', () => {
      const menu = Menu.buildFromTemplate([{ label: '1' }, { label: '2' }, { label: '3' }]);

      const item = new MenuItem({ label: 'inserted' });

      menu.insert(1, item);
      expect(menu.items[0].label).toBe('1');
      expect(menu.items[1].label).toBe('inserted');
      expect(menu.items[2].label).toBe('2');
      expect(menu.items[3].label).toBe('3');
    });
  });

  describe('Menu.append', () => {
    it('should add the item to the end of the menu', () => {
      const menu = Menu.buildFromTemplate([{ label: '1' }, { label: '2' }, { label: '3' }]);

      const item = new MenuItem({ label: 'inserted' });
      menu.append(item);

      expect(menu.items[0].label).toBe('1');
      expect(menu.items[1].label).toBe('2');
      expect(menu.items[2].label).toBe('3');
      expect(menu.items[3].label).toBe('inserted');
    });
  });

  describe('Menu.popup', { tags: ['serial'] }, () => {
    let w: BrowserWindow;
    let menu: Menu;

    beforeEach(() => {
      w = new BrowserWindow({ show: false, width: 200, height: 200 });
      menu = Menu.buildFromTemplate([{ label: '1' }, { label: '2' }, { label: '3' }]);
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
      }).toThrow(/Options must be an object/);
    });

    it('allows for options to be optional', () => {
      expect(() => {
        menu.popup({});
      }).not.toThrow();
    });

    it('should emit menu-will-show event', async () => {
      const menuWillShow = once(menu, 'menu-will-show');
      menu.popup({ window: w });
      await menuWillShow;
    });

    it('should emit menu-will-close event', async () => {
      const menuWillClose = once(menu, 'menu-will-close');
      menu.popup({ window: w });
      menu.closePopup();
      await menuWillClose;
    });

    it('returns immediately', () => {
      const input = { window: w, x: 100, y: 101 };
      const output = menu.popup(input) as unknown as { x: number; y: number; browserWindow: BrowserWindow };
      expect(output.x).toBe(input.x);
      expect(output.y).toBe(input.y);
      expect(output.browserWindow).toBe(input.window);
    });

    it('works without a given BrowserWindow and options', () => {
      const { browserWindow, x, y } = menu.popup({ x: 100, y: 101 }) as unknown as {
        x: number;
        y: number;
        browserWindow: BrowserWindow;
      };

      expect(browserWindow.constructor.name).toBe('BrowserWindow');
      expect(x).toBe(100);
      expect(y).toBe(101);
    });

    it('works with a given BrowserWindow, options and callback', () =>
      new Promise<void>((resolve) => {
        const { x, y } = menu.popup({
          window: w,
          x: 100,
          y: 101,
          callback: () => resolve()
        }) as unknown as { x: number; y: number };

        expect(x).toBe(100);
        expect(y).toBe(101);
        menu.closePopup();
      }));

    it('works with a given BrowserWindow, no options, and a callback', () =>
      new Promise<void>((resolve) => {
        menu.popup({ window: w, callback: () => resolve() });
        menu.closePopup();
      }));

    it('prevents menu from getting garbage-collected when popuping', async () => {
      const menu = Menu.buildFromTemplate([{ role: 'paste' }]);
      menu.popup({ window: w });

      // Keep a weak reference to the menu.
      const wr = new WeakRef(menu);

      await setTimeout();

      // Do garbage collection, since |menu| is not referenced in this closure
      // it would be gone after next call.
      const v8Util = process._linkedBinding('electron_common_v8_util');
      v8Util.requestGarbageCollectionForTesting();

      await setTimeout();

      // Try to receive menu from weak reference.
      if (wr.deref()) {
        wr.deref()!.closePopup();
      } else {
        throw new Error('Menu is garbage-collected while popuping');
      }
    });

    it('does not crash with an offscreen frame', async () => {
      const osrWindow = new BrowserWindow({
        show: false,
        width: 200,
        height: 200,
        webPreferences: {
          offscreen: true
        }
      });

      try {
        await osrWindow.loadURL('about:blank');

        const menuWillShow = once(menu, 'menu-will-show');
        menu.popup({ window: osrWindow, frame: osrWindow.webContents.mainFrame });
        await menuWillShow;

        const menuWillClose = once(menu, 'menu-will-close');
        menu.closePopup(osrWindow);
        await menuWillClose;
      } finally {
        await closeWindow(osrWindow, { assertNotWindows: false });
      }
    });

    // https://github.com/electron/electron/issues/35724
    // Maximizing window is enough to trigger the bug
    // FIXME(dsanders11): Test always passes on CI, even pre-fix
    ifit(process.platform === 'linux' && !process.env.CI)(
      'does not trigger issue #35724',
      () =>
        new Promise<void>((resolve, reject) => {
          const done = (error?: unknown) => (error ? reject(error) : resolve());
          const showAndCloseMenu = async () => {
            await setTimeout(1000);
            menu.popup({ window: w, x: 50, y: 50 });
            await setTimeout(500);
            const closed = once(menu, 'menu-will-close');
            menu.closePopup();
            await closed;
          };

          const failOnEvent = () => {
            done(new Error('Menu closed prematurely'));
          };

          assert(!w.isVisible());
          w.on('show', async () => {
            assert(!w.isMaximized());
            // Show the menu once, then maximize window
            await showAndCloseMenu();
            // NOTE - 'maximize' event never fires on CI for Linux
            const maximized = once(w, 'maximize');
            w.maximize();
            await maximized;

            // Bug only seems to trigger programmatically after showing the menu once more
            await showAndCloseMenu();

            // Now ensure the menu stays open until we close it
            await setTimeout(500);
            menu.once('menu-will-close', failOnEvent);
            menu.popup({ window: w, x: 50, y: 50 });
            await setTimeout(1500);
            menu.off('menu-will-close', failOnEvent);
            menu.once('menu-will-close', () => done());
            menu.closePopup();
          });
          w.show();
        })
    );

    const chunkSize = 10;
    let chunkCount = 0;
    const totalChunks = Math.ceil(singleModifierCombinations.length / chunkSize);
    for (let i = 0; i < singleModifierCombinations.length; i += chunkSize) {
      const chunk = singleModifierCombinations.slice(i, i + chunkSize);
      it(`does not crash when rendering menu item with single accelerator combinations ${++chunkCount}/${totalChunks}`, async () => {
        const menu = Menu.buildFromTemplate([
          ...chunk.map((combination) => ({
            label: `Test ${combination}`,
            accelerator: combination
          }))
        ]);
        menu.popup({ window: w });
        menu.closePopup();
      });
    }
  });

  ifit(process.platform === 'darwin')('emits menu close event even if submenu closes first', async () => {
    const menu = Menu.buildFromTemplate([
      {
        label: 'parent',
        submenu: [
          {
            label: 'child'
          }
        ]
      }
    ]);

    const menuWillClose = once(menu, 'menu-will-close');
    (menu as any)._simulateSubmenuCloseSequenceForTesting();

    await Promise.race([
      menuWillClose,
      setTimeout(1000).then(() => {
        throw new Error('menu-will-close was not emitted');
      })
    ]);
  });

  describe('Menu.setApplicationMenu', () => {
    beforeEach(() => {
      const menu = Menu.getApplicationMenu();
      defer(() => Menu.setApplicationMenu(menu));
    });

    it('sets a menu', () => {
      const menu = Menu.buildFromTemplate([{ label: '1' }, { label: '2' }]);

      Menu.setApplicationMenu(menu);
      expect(Menu.getApplicationMenu(), 'application menu').not.toBeNull();
    });

    it('unsets a menu with null', () => {
      Menu.setApplicationMenu(Menu.buildFromTemplate([{ label: '1' }]));
      Menu.setApplicationMenu(null);
      expect(Menu.getApplicationMenu(), 'application menu').toBeNull();
    });

    ifit(isWayland)('creates and refreshes a menu when GTK cannot initialize', async () => {
      const appPath = path.join(fixturesPath, 'api', 'test-menu-no-gtk');
      const schemaDir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-menu-no-gtk-'));
      defer(() => fs.rmSync(schemaDir, { recursive: true, force: true }));
      fs.copyFileSync(
        path.join(appPath, 'org.gnome.desktop.interface.gschema.xml'),
        path.join(schemaDir, 'org.gnome.desktop.interface.gschema.xml')
      );

      const compiled = await spawnAndWait('glib-compile-schemas', ['--strict', schemaDir], { timeout: 10000 });
      expect(compiled, compiled.stderr).toMatchObject({ code: 0, signal: null });

      const { code, signal, stdout, stderr } = await spawnAndWait(
        process.execPath,
        [appPath, '--ozone-platform=wayland', `--user-data-dir=${path.join(schemaDir, 'profile')}`],
        {
          env: { ...process.env, GSETTINGS_SCHEMA_DIR: schemaDir, GSETTINGS_BACKEND: 'memory' },
          timeout: 10000
        }
      );
      expect(stderr).toContain('Schema org.gnome.desktop.interface does not have key font-antialiasing');
      expect({ code, signal }, stderr).toEqual({ code: 0, signal: null });
      expect(stdout).toContain('Menu created and refreshed without GTK');
    });

    ifit(process.platform !== 'darwin')('does not override menu visibility on startup', async () => {
      const appPath = path.join(fixturesPath, 'api', 'test-menu-visibility');
      const appProcess = cp.spawn(process.execPath, [appPath]);

      let output = '';
      await new Promise<void>((resolve) => {
        appProcess.stdout.on('data', (data) => {
          output += data;
          if (data.includes('Window has')) {
            resolve();
          }
        });
      });
      expect(output).toContain('Window has no menu');
    });

    ifit(process.platform !== 'darwin')('does not override null menu on startup', async () => {
      const appPath = path.join(fixturesPath, 'api', 'test-menu-null');
      const appProcess = cp.spawn(process.execPath, [appPath]);

      let output = '';
      appProcess.stdout.on('data', (data) => {
        output += data;
      });
      appProcess.stderr.on('data', (data) => {
        output += data;
      });

      const [code] = await once(appProcess, 'close');
      if (!output.includes('Window has no menu')) {
        console.log(code, output);
      }
      expect(output).toContain('Window has no menu');
    });
  });
});
