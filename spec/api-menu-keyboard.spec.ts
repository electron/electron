import { BrowserWindow, Menu, screen } from 'electron/main';

import { afterAll, afterEach, beforeAll, beforeEach, expect, it } from 'vitest';

import { createRequire } from 'node:module';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

const require = createRequire(import.meta.url);

// TODO(@mlaurencin): Find a robotjs-free approach that avoids flakes and works on CI.

// robotjs is an optional native dependency; if it fails to load the suite is
// skipped below.
let robot!: typeof import('@hurdlegroup/robotjs');
try {
  robot = require('@hurdlegroup/robotjs');
} catch {
  // skip tests
}

const display = screen.getPrimaryDisplay();

// Delay between key presses. robotjs can drop keystrokes that arrive
// too quickly for the nested menu message loop.
const KEY_DELAY = 350;

// The sibling-menu switching feature only exists on the Views menu
// bar used on Windows and Linux
ifdescribe(process.platform !== 'darwin' && !!robot?.keyTap)(
  'menu bar keyboard sibling switching',
  { tags: ['serial'] },
  () => {
    // Records the label of the last menu item activated via the keyboard
    let lastClicked: string | null = null;
    const record = (label: string) => {
      lastClicked = label;
    };

    // A menu bar with three top-level menus. Opening a menu via the keyboard auto-selects the first item.
    const buildMenu = () =>
      Menu.buildFromTemplate([
        {
          label: '&Alpha',
          submenu: [
            {
              label: 'Alpha Item',
              click: () => record('alpha-item')
            }
          ]
        },
        {
          label: '&Beta',
          submenu: [
            {
              label: 'Beta Leaf',
              click: () => record('beta-leaf')
            },
            {
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
          label: '&Gamma',
          submenu: [
            {
              label: 'Gamma Item',
              click: () => record('gamma-item')
            }
          ]
        }
      ]);

    let w: BrowserWindow;

    // Brings the shared window to the OS foreground so robotjs key events route to
    // it. A programmatic focus() isn't treated as real activation by every window
    // manager, so we also click the content area, then wait for focus.
    const focusWindow = async () => {
      w.show();
      w.moveTop();
      w.focus();
      const bounds = w.getContentBounds();
      robot.moveMouse(Math.round(bounds.x + bounds.width / 2), Math.round(bounds.y + bounds.height / 2));
      robot.mouseClick();
      await waitUntil(() => w.isFocused());
    };

    // Presses a sequence of keys with a delay between each so the native menu can
    // keep up. Each entry is either a key name or a [key, modifiers] tuple.
    const pressKeys = async (keys: Array<string | [string, string[]]>) => {
      for (const key of keys) {
        if (Array.isArray(key)) {
          robot.keyTap(key[0], key[1]);
        } else {
          robot.keyTap(key);
        }
        await setTimeout(KEY_DELAY);
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
      w.setMenu(buildMenu());
      await w.loadURL('about:blank');
      await focusWindow();

      //  Warm up with a throwaway open/close cycle.
      await pressKeys([['a', ['alt']], 'escape', 'escape']);
    });

    beforeEach(async () => {
      lastClicked = null;
      // Re-establish foreground focus
      await focusWindow();
    });

    afterEach(async () => {
      // Dismiss any open menu for a clean state.
      if (robot && robot.keyTap) {
        robot.keyTap('escape');
        robot.keyTap('escape');
        await setTimeout(KEY_DELAY);
      }
    });

    afterAll(async () => {
      await closeAllWindows();
    });

    it('switches to the next sibling menu when Right is pressed on a top-level leaf item', async () => {
      await pressKeys([
        ['a', ['alt']], // open Alpha (Alpha Item selected)
        'right', // switch to Beta (next sibling)
        'enter' // activate Beta's first item
      ]);

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('beta-leaf');
    });

    it('switches to the first sibling menu when Left is pressed on the second menu', async () => {
      await pressKeys([['b', ['alt']], 'left', 'enter']);

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('alpha-item');
    });

    it('wraps to the last sibling menu when Left is pressed on the first menu', async () => {
      await pressKeys([
        ['a', ['alt']], // open Alpha (Alpha Item selected)
        'left', // switch to Gamma (previous sibling, wrapping)
        'enter' // activate Gamma's first item
      ]);

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('gamma-item');
    });

    it('wraps to the first sibling menu when Right is pressed on the last menu', async () => {
      await pressKeys([
        ['g', ['alt']], // open Gamma (Gamma Item selected)
        'right', // switch to Alpha (next sibling, wrapping)
        'enter' // activate Alpha's first item
      ]);

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('alpha-item');
    });

    for (const [keys, expected] of [
      [['right'], 'beta-leaf'],
      [['left'], 'gamma-item'],
      [['right', 'left'], 'alpha-item']
    ] as const) {
      it(`switches sibling menus when ${keys.join(' then ')} is pressed after opening a menu by mouse`, async () => {
        const bounds = w.getContentBounds();
        // The menu bar is immediately above the content area.
        const point = { x: Math.round(bounds.x + 20), y: Math.round(bounds.y - 10) };
        const screenPoint = process.platform === 'win32' ? screen.dipToScreenPoint(point) : point;
        robot.moveMouse(screenPoint.x, screenPoint.y);
        robot.mouseClick();
        await setTimeout(KEY_DELAY);

        await pressKeys([...keys, 'enter']);

        await waitUntil(() => lastClicked !== null);
        expect(lastClicked).toBe(expected);
      });
    }

    it('opens a nested submenu instead of switching siblings when Right is pressed on a submenu item', async () => {
      await pressKeys([
        ['b', ['alt']], // open Beta (Beta Leaf selected)
        'down', // select Beta More (has a submenu)
        'right', // open the Beta More submenu (NOT switch to Gamma)
        'enter' // activate Beta More Item
      ]);

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('beta-more-item');
    });

    it('does not switch to a sibling menu when Left is pressed on a top-level submenu item', async () => {
      await pressKeys([
        ['b', ['alt']], // open Beta (Beta Leaf selected)
        'down', // select Beta More (a submenu item)
        'left', // must NOT switch to Alpha
        'up', // back to Beta Leaf
        'enter' // activate Beta Leaf
      ]);

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('beta-leaf');
    });

    it('does not switch to a sibling menu when Right is pressed inside a nested submenu', async () => {
      await pressKeys([
        ['b', ['alt']], // open Beta (Beta Leaf selected)
        'down', // select Beta More (a submenu item)
        'right', // open the Beta More submenu (Beta More Item selected)
        'right', // must NOT switch to Gamma (still nested)
        'enter' // activate Beta More Item
      ]);

      await waitUntil(() => lastClicked !== null);
      expect(lastClicked).toBe('beta-more-item');
    });
  }
);
