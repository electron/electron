import { BrowserWindow, Menu, screen } from 'electron/main';
import type { MenuItemConstructorOptions } from 'electron/main';

import { afterAll, afterEach, beforeAll, beforeEach, expect, it } from 'vitest';

import { once } from 'node:events';
import { createRequire } from 'node:module';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, isWayland, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

const require = createRequire(import.meta.url);

// robotjs is an optional native dependency; if it fails to load the suite is
// skipped below.
let robot!: typeof import('@hurdlegroup/robotjs');
try {
  robot = require('@hurdlegroup/robotjs');
} catch {
  // skip tests
}

const display = screen.getPrimaryDisplay();

// Delay after key presses that don't open or close a menu. robotjs can drop
// keystrokes that arrive too quickly for the nested menu message loop.
const KEY_DELAY = 350;

// A key name or a [key, modifiers] tuple.
type Key = string | [string, string[]];

type MenuEvent = 'menu-will-show' | 'menu-will-close';

// Menu-bar keyboard navigation uses the Views menu bar on Windows and Linux.
// robotjs injects input via XTest, which can't reach native Wayland windows.
ifdescribe(process.platform !== 'darwin' && !isWayland && !!robot?.keyTap)(
  'menu bar keyboard sibling switching',
  { tags: ['serial'], retry: 0 },
  () => {
    // Records the label of the last menu item activated via the keyboard
    let lastClicked: string | null = null;
    const record = (label: string) => {
      lastClicked = label;
    };

    // Three submenu headers, with optional items between Alpha and Beta.
    // Keyboard-opened dropdowns select their first item automatically.
    const buildMenu = (betweenMenus: MenuItemConstructorOptions[] = []) =>
      Menu.buildFromTemplate([
        {
          id: 'alpha',
          label: '&Alpha',
          submenu: [
            {
              label: 'Alpha Item',
              click: () => record('alpha-item')
            }
          ]
        },
        ...betweenMenus,
        {
          id: 'beta',
          label: '&Beta',
          submenu: [
            {
              label: 'Beta Leaf',
              click: () => record('beta-leaf')
            },
            {
              id: 'beta-more',
              label: 'Beta More',
              submenu: [
                {
                  label: 'Beta More Item',
                  click: () => record('beta-more-item')
                }
              ]
            }
          ]
        },
        {
          id: 'gamma',
          label: '&Gamma',
          submenu: [
            {
              label: 'Gamma Item',
              click: () => record('gamma-item')
            }
          ]
        }
      ]);

    const buildMenuWithCommands = () =>
      buildMenu([
        { type: 'separator' },
        { label: '&Command', click: () => record('command') },
        { type: 'separator' },
        { label: 'Disabled', enabled: false, click: () => record('disabled') },
        { label: 'Hidden', visible: false, click: () => record('hidden') }
      ]);

    let w: BrowserWindow;
    let currentMenu: Menu;

    // Submenus that are currently showing.
    const openMenus = new Set<Menu>();

    const trackOpenMenus = (menu: Menu) => {
      for (const { submenu } of menu.items) {
        if (!submenu) continue;
        submenu.on('menu-will-show', () => openMenus.add(submenu));
        submenu.on('menu-will-close', () => openMenus.delete(submenu));
        trackOpenMenus(submenu);
      }
    };

    const setMenu = (menu: Menu) => {
      trackOpenMenus(menu);
      currentMenu = menu;
      w.setMenu(menu);
    };

    const submenuById = (id: string) => currentMenu.getMenuItemById(id)!.submenu!;

    // robotjs works in physical screen pixels, so convert from DIPs first.
    const clickAt = (point: Electron.Point) => {
      const { x, y } = screen.dipToScreenPoint({ x: Math.round(point.x), y: Math.round(point.y) });
      robot.moveMouse(x, y);
      robot.mouseClick();
    };

    // Brings the shared window to the OS foreground so robotjs key events route to
    // it. A programmatic focus() isn't treated as real activation by every window
    // manager, so we also click the content area, then wait for focus.
    const focusWindow = async () => {
      w.show();
      w.moveTop();
      w.focus();
      const bounds = w.getContentBounds();
      clickAt({ x: bounds.x + bounds.width / 2, y: bounds.y + bounds.height / 2 });
      await waitUntil(() => w.isFocused());
    };

    const tap = (key: Key) => {
      if (Array.isArray(key)) {
        robot.keyTap(key[0], key[1]);
      } else {
        robot.keyTap(key);
      }
    };

    // Presses keys that don't open or close a menu, pausing after each one so
    // menu event handling can keep up.
    const pressKeys = async (keys: Key[]) => {
      for (const key of keys) {
        tap(key);
        await setTimeout(KEY_DELAY);
      }
    };

    // Presses a key and waits for the submenu with `id` to emit `event`, so that
    // following keys go to whichever menu is open afterwards. Switching menus is
    // asynchronous, so a fixed delay could let the next key reach the old menu.
    const pressKeyAndWait = async (key: Key, id: string, event: MenuEvent = 'menu-will-show') => {
      const done = once(submenuById(id), event);
      tap(key);
      await done;
    };

    // Presses Escape until every open menu has closed, one menu at a time.
    const closeMenus = async () => {
      while (openMenus.size > 0) {
        const count = openMenus.size;
        tap('escape');
        await waitUntil(() => openMenus.size < count);
      }
    };

    beforeAll(async () => {
      w = new BrowserWindow({
        x: 0,
        y: 0,
        width: Math.round(display.bounds.width / 2),
        height: Math.round(display.bounds.height / 2),
        autoHideMenuBar: false
      });
      setMenu(buildMenu());
      await w.loadURL('about:blank');
      await focusWindow();

      //  Warm up with a throwaway open/close cycle.
      await pressKeyAndWait(['a', ['alt']], 'alpha');
      await closeMenus();
    });

    beforeEach(async () => {
      lastClicked = null;
      setMenu(buildMenu());
      // Re-establish foreground focus
      await focusWindow();
    });

    afterEach(async () => {
      // Dismiss any open menu for a clean state.
      await closeMenus();
      openMenus.clear();
    });

    afterAll(async () => {
      await closeAllWindows();
    });

    it('switches to the next sibling menu when Right is pressed on a top-level leaf item', async () => {
      await pressKeyAndWait(['a', ['alt']], 'alpha'); // open Alpha (Alpha Item selected)
      await pressKeyAndWait('right', 'beta'); // open Beta automatically (Beta Leaf selected)
      await pressKeys(['enter']); // activate Beta's first item

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('beta-leaf');
    });

    it('switches to the first sibling menu when Left is pressed on the second menu', async () => {
      await pressKeyAndWait(['b', ['alt']], 'beta'); // open Beta (Beta Leaf selected)
      await pressKeyAndWait('left', 'alpha'); // open Alpha automatically (Alpha Item selected)
      await pressKeys(['enter']); // activate Alpha's first item

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('alpha-item');
    });

    it('wraps to the last sibling menu when Left is pressed on the first menu', async () => {
      await pressKeyAndWait(['a', ['alt']], 'alpha'); // open Alpha (Alpha Item selected)
      await pressKeyAndWait('left', 'gamma'); // wrap to Gamma and open it automatically
      await pressKeys(['enter']); // activate Gamma's first item

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('gamma-item');
    });

    it('wraps to the first sibling menu when Right is pressed on the last menu', async () => {
      await pressKeyAndWait(['g', ['alt']], 'gamma'); // open Gamma (Gamma Item selected)
      await pressKeyAndWait('right', 'alpha'); // wrap to Alpha and open it automatically
      await pressKeys(['enter']); // activate Alpha's first item

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('alpha-item');
    });

    for (const [mnemonic, direction, openId] of [
      ['a', 'right', 'alpha'],
      ['b', 'left', 'beta']
    ] as const) {
      it(`selects a top-level command without executing it when ${direction} is pressed`, async () => {
        setMenu(buildMenuWithCommands());
        await pressKeyAndWait([mnemonic, ['alt']], openId); // open the menu next to Command
        await pressKeyAndWait(direction, openId, 'menu-will-close'); // close it and select Command

        expect(lastClicked).toBeNull();
        await pressKeys(['up', 'down']); // must NOT execute Command
        expect(lastClicked).toBeNull();

        await pressKeys(['enter']); // execute Command
        await waitUntil(() => lastClicked !== null);
        expect(lastClicked).toBe('command');
      });
    }

    for (const [mnemonic, direction, openId, nextId, expected] of [
      ['a', 'right', 'alpha', 'beta', 'beta-leaf'],
      ['b', 'left', 'beta', 'alpha', 'alpha-item']
    ] as const) {
      it(`automatically opens the next submenu after navigating ${direction} through a command`, async () => {
        setMenu(buildMenuWithCommands());
        await pressKeyAndWait([mnemonic, ['alt']], openId); // open the menu next to Command
        await pressKeyAndWait(direction, openId, 'menu-will-close'); // close it and select Command
        await pressKeyAndWait(direction, nextId); // open the menu past Command automatically
        await pressKeys(['enter']); // activate its first item

        await waitUntil(() => lastClicked !== null);
        expect(lastClicked).toBe(expected);
      });

      it(`skips separators, disabled items, and hidden items when navigating ${direction}`, async () => {
        setMenu(
          buildMenu([
            { type: 'separator' },
            { label: 'Disabled', enabled: false, click: () => record('disabled') },
            { label: 'Hidden', visible: false, click: () => record('hidden') }
          ])
        );
        await pressKeyAndWait([mnemonic, ['alt']], openId); // open the first menu
        await pressKeyAndWait(direction, nextId); // skip the separator, disabled, and hidden items
        await pressKeys(['enter']); // activate its first item

        await waitUntil(() => lastClicked !== null);
        expect(lastClicked).toBe(expected);
      });
    }

    it('requires activation to open a submenu when navigating the menu bar after Alt alone', async () => {
      setMenu(buildMenuWithCommands());
      await pressKeys(['alt', 'right']); // focus the menu bar, then select Command
      expect(lastClicked).toBeNull();

      // Enter opens Beta here; only the following Enter executes Beta Leaf.
      await pressKeys(['right']); // select Beta without opening it
      await pressKeyAndWait('enter', 'beta'); // open Beta (Beta Leaf selected)
      expect(lastClicked).toBeNull();

      await pressKeys(['enter']); // activate Beta Leaf
      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('beta-leaf');
    });

    it('executes a top-level command using its mnemonic after navigating from an open submenu', async () => {
      setMenu(buildMenuWithCommands());
      await pressKeyAndWait(['a', ['alt']], 'alpha'); // open Alpha (Alpha Item selected)
      await pressKeyAndWait('right', 'alpha', 'menu-will-close'); // close Alpha and select Command
      await pressKeys(['c']); // execute Command by its mnemonic

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('command');
    });

    it('leaves open-menu navigation mode when Escape is pressed on a top-level command', async () => {
      setMenu(buildMenuWithCommands());
      await pressKeyAndWait(['a', ['alt']], 'alpha'); // open Alpha (Alpha Item selected)
      await pressKeyAndWait('right', 'alpha', 'menu-will-close'); // close Alpha and select Command
      await pressKeys([
        'escape', // leave the menu bar
        'alt', // focus the menu bar again (Alpha selected)
        'right', // select Command
        'right' // select Beta without opening it
      ]);
      await pressKeyAndWait('enter', 'beta'); // open Beta (Beta Leaf selected)
      expect(lastClicked).toBeNull();

      await pressKeys(['enter']); // activate Beta Leaf
      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('beta-leaf');
    });

    for (const direction of ['left', 'right']) {
      it(`keeps the current menu usable when ${direction} has no other menu-bar item to select`, async () => {
        setMenu(
          Menu.buildFromTemplate([
            {
              id: 'alpha',
              label: '&Alpha',
              submenu: [{ label: 'Alpha Item', click: () => record('alpha-item') }]
            },
            { type: 'separator' }
          ])
        );
        await pressKeyAndWait(['a', ['alt']], 'alpha'); // open Alpha (Alpha Item selected)
        await pressKeys([
          direction, // no other menu to switch to; Alpha stays open
          'enter' // activate Alpha Item
        ]);

        await waitUntil(() => lastClicked !== null);
        expect(lastClicked).toBe('alpha-item');
      });
    }

    for (const [steps, expected] of [
      [[['right', 'beta']], 'beta-leaf'],
      [[['left', 'gamma']], 'gamma-item'],
      [
        [
          ['right', 'beta'],
          ['left', 'alpha']
        ],
        'alpha-item'
      ]
    ] as const) {
      const keys = steps.map(([key]) => key).join(' then ');
      it(`switches sibling menus when ${keys} is pressed after opening a menu by mouse`, async () => {
        const bounds = w.getContentBounds();
        // The menu bar is immediately above the content area.
        const opened = once(submenuById('alpha'), 'menu-will-show');
        clickAt({ x: bounds.x + 20, y: bounds.y - 10 }); // open Alpha by clicking its header
        await opened;

        for (const [key, id] of steps) {
          await pressKeyAndWait(key, id); // switch to the sibling menu and open it automatically
        }
        await pressKeys(['enter']); // activate its first item

        await waitUntil(() => lastClicked !== null);
        expect(lastClicked).toBe(expected);
      });
    }

    it('opens a nested submenu instead of switching siblings when Right is pressed on a submenu item', async () => {
      await pressKeyAndWait(['b', ['alt']], 'beta'); // open Beta (Beta Leaf selected)
      await pressKeys(['down']); // select Beta More (has a submenu)
      await pressKeyAndWait('right', 'beta-more'); // open the Beta More submenu (NOT switch to Gamma)
      await pressKeys(['enter']); // activate Beta More Item

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('beta-more-item');
    });

    it('switches to the previous sibling menu when Left is pressed on a top-level submenu item', async () => {
      await pressKeyAndWait(['b', ['alt']], 'beta'); // open Beta (Beta Leaf selected)
      await pressKeys(['down']); // select Beta More (a submenu item, not open)
      await pressKeyAndWait('left', 'alpha'); // switch to Alpha like Win32 (Alpha Item selected)
      await pressKeys(['enter']); // activate Alpha's first item

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('alpha-item');
    });

    it('closes a nested submenu instead of switching siblings when Left is pressed inside it', async () => {
      await pressKeyAndWait(['b', ['alt']], 'beta'); // open Beta (Beta Leaf selected)
      await pressKeys(['down']); // select Beta More (a submenu item)
      await pressKeyAndWait('right', 'beta-more'); // open the Beta More submenu (Beta More Item selected)
      await pressKeyAndWait('left', 'beta-more', 'menu-will-close'); // close Beta More (NOT switch to Alpha)
      await pressKeys([
        'up', // back to Beta Leaf
        'enter' // activate Beta Leaf
      ]);

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('beta-leaf');
    });

    it('switches to the next sibling menu when Right is pressed on a leaf inside a nested submenu', async () => {
      await pressKeyAndWait(['b', ['alt']], 'beta'); // open Beta (Beta Leaf selected)
      await pressKeys(['down']); // select Beta More (a submenu item)
      await pressKeyAndWait('right', 'beta-more'); // open the Beta More submenu (Beta More Item selected)
      await pressKeyAndWait('right', 'gamma'); // switch to Gamma like Win32 (Gamma Item selected)
      await pressKeys(['enter']); // activate Gamma's first item

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('gamma-item');
    });
  }
);
