import { BrowserWindow, Menu, screen } from 'electron/main';

import { expect } from 'chai';

import { setTimeout } from 'node:timers/promises';

import { hasCapturableScreen } from './lib/screen-helpers';
import { ifdescribe } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

// These tests are skipped when robotjs or a capturable screen is unavailable
let robot: typeof import('@hurdlegroup/robotjs');
try {
  robot = require('@hurdlegroup/robotjs');
} catch {
  // skip tests
}

const display = screen.getPrimaryDisplay();

// Delay between key presses to give the native menu run loop time to process
// each event before the next one is sent. Kept generous because robotjs can
// drop keystrokes that arrive too quickly for the nested menu message loop.
const KEY_DELAY = 350;

// The sibling-menu switching feature only exists on the Views menu
// bar used on Windows and Linux
ifdescribe(process.platform !== 'darwin')('menu bar keyboard sibling switching', function () {
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

  const createWindow = async () => {
    const w = new BrowserWindow({
      x: 0,
      y: 0,
      width: Math.round(display.bounds.width / 2),
      height: Math.round(display.bounds.height / 2),
      autoHideMenuBar: false
    });
    w.setMenu(buildMenu());
    await w.loadURL('about:blank');
    w.show();
    w.moveTop();
    w.focus();
    // Give the compositor/window manager a moment to make the window active so
    // that robotjs key events are routed to it.
    await setTimeout(500);
    // Click the window's content area to force real foreground activation before we
    // start sending keys.
    const bounds = w.getContentBounds();
    robot.moveMouse(Math.round(bounds.x + bounds.width / 2), Math.round(bounds.y + bounds.height / 2));
    robot.mouseClick();
    await setTimeout(300);
    return w;
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

  before(async function () {
    if (!robot || !robot.keyTap || !hasCapturableScreen()) {
      this.skip();
      return;
    }

    // The first window may not properly receive events due to UI transitions or
    // focus management. Warm up with a throwaway open/close cycle.
    const w = await createWindow();
    await pressKeys([['a', ['alt']], 'escape', 'escape']);
    w.destroy();
  });

  beforeEach(() => {
    lastClicked = null;
  });

  afterEach(async () => {
    // Ensure any open menu is dismissed before the window is destroyed
    if (robot && robot.keyTap) {
      robot.keyTap('escape');
      robot.keyTap('escape');
      await setTimeout(KEY_DELAY);
    }
    await closeAllWindows();
  });

  it('switches to the next sibling menu when Right is pressed on a top-level leaf item', async () => {
    await createWindow();

    await pressKeys([
      ['a', ['alt']], // open Alpha (Alpha Item selected)
      'right', // switch to Beta (next sibling)
      'enter' // activate Beta's first item
    ]);

    await setTimeout(KEY_DELAY);
    expect(lastClicked).to.equal('beta-leaf');
  });

  it('wraps to the last sibling menu when Left is pressed on the first menu', async () => {
    await createWindow();

    await pressKeys([
      ['a', ['alt']], // open Alpha (Alpha Item selected)
      'left', // switch to Gamma (previous sibling, wrapping)
      'enter' // activate Gamma's first item
    ]);

    await setTimeout(KEY_DELAY);
    expect(lastClicked).to.equal('gamma-item');
  });

  it('wraps to the first sibling menu when Right is pressed on the last menu', async () => {
    await createWindow();

    await pressKeys([
      ['g', ['alt']], // open Gamma (Gamma Item selected)
      'right', // switch to Alpha (next sibling, wrapping)
      'enter' // activate Alpha's first item
    ]);

    await setTimeout(KEY_DELAY);
    expect(lastClicked).to.equal('alpha-item');
  });

  it('opens a nested submenu instead of switching siblings when Right is pressed on a submenu item', async () => {
    await createWindow();

    await pressKeys([
      ['b', ['alt']], // open Beta (Beta Leaf selected)
      'down', // select Beta More (has a submenu)
      'right', // open the Beta More submenu (NOT switch to Gamma)
      'enter' // activate Beta More Item
    ]);

    await setTimeout(KEY_DELAY);
    expect(lastClicked).to.equal('beta-more-item');
  });

  it('does not switch to a sibling menu when Left is pressed on a top-level submenu item', async () => {
    await createWindow();

    await pressKeys([
      ['b', ['alt']], // open Beta (Beta Leaf selected)
      'down', // select Beta More (a submenu item)
      'left', // must NOT switch to Alpha
      'up', // back to Beta Leaf
      'enter' // activate Beta Leaf
    ]);

    await setTimeout(KEY_DELAY);
    expect(lastClicked).to.equal('beta-leaf');
  });

  it('does not switch to a sibling menu when Right is pressed inside a nested submenu', async () => {
    await createWindow();

    await pressKeys([
      ['b', ['alt']], // open Beta (Beta Leaf selected)
      'down', // select Beta More (a submenu item)
      'right', // open the Beta More submenu (Beta More Item selected)
      'right', // must NOT switch to Gamma (still nested)
      'enter' // activate Beta More Item
    ]);

    await setTimeout(KEY_DELAY);
    expect(lastClicked).to.equal('beta-more-item');
  });
});
