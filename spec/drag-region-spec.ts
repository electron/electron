import { BrowserWindow, screen } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';
import { pathToFileURL } from 'node:url';

import { hasCapturableScreen } from './lib/screen-helpers';
import { closeAllWindows } from './lib/window-helpers';

const display = screen.getPrimaryDisplay();

const fixtures = path.resolve(__dirname, 'fixtures');

// Try to load robotjs
let robot: typeof import('@hurdlegroup/robotjs');
try {
  robot = require('@hurdlegroup/robotjs');
} catch {
  // ignore. tests are skipped below if this is undefined.
}

const draggablePageURL = pathToFileURL(
  path.join(fixtures, 'pages', 'draggable-page.html')
);
const iframePageURL = pathToFileURL(
  path.join(fixtures, 'pages', 'iframe.html')
);
const webviewPageURL = pathToFileURL(
  path.join(fixtures, 'pages', 'webview.html')
);

const testWindowOpts: Electron.BrowserWindowConstructorOptions = {
  x: 0,
  y: 0,
  width: Math.round(display.bounds.width / 2),
  height: Math.round(display.bounds.height / 2),
  frame: false
};

const center = (rect: Electron.Rectangle): Electron.Point => ({
  x: Math.round(rect.x + rect.width / 2),
  y: Math.round(rect.y + rect.height / 2)
});

const performDrag = async (
  w: BrowserWindow
): Promise<{
  start: [number, number];
  end: [number, number];
}> => {
  const winBounds = w.getBounds();
  const winCenter = center(winBounds);
  const screenCenter = center(display.bounds);

  const start = w.getPosition() as [number, number];
  const moved = once(w, 'move');

  // Extra events based on research from https://github.com/octalmage/robotjs/issues/389
  robot.moveMouse(winCenter.x, winCenter.y);
  robot.mouseToggle('down', 'left');
  robot.moveMouse(winCenter.x + 2, winCenter.y + 2); // extra
  await setTimeout(200); // extra
  robot.dragMouse(screenCenter.x, screenCenter.y);
  robot.mouseToggle('up', 'left');

  await Promise.race([moved, setTimeout(1000)]);

  const end = w.getPosition() as [number, number];
  return { start, end };
};

const loadDraggableSubframe = async (w: BrowserWindow): Promise<void> => {
  let selector: string;
  let eventName: string;
  if (w.getURL() === iframePageURL.href) {
    selector = 'iframe';
    eventName = 'load';
  } else if (w.getURL() === webviewPageURL.href) {
    selector = 'webview';
    eventName = 'did-finish-load';
  } else {
    throw new Error('Unexpected page loaded');
  }

  await w.webContents.executeJavaScript(`
    new Promise((resolve) => {
      const frame = document.querySelector('${selector}');
      frame.addEventListener(
        '${eventName}',
        () => resolve(),
        { once: true }
      );
      frame.src = '${draggablePageURL.href}';
    })
  `);
};

describe('draggable regions', function () {
  before(async function () {
    if (!robot || !robot.moveMouse || !hasCapturableScreen()) {
      this.skip();
    }

    // The first window may not properly receive events due to UI transitions or
    // focus management. To mitigate this, warm up with a test run.
    const w = new BrowserWindow(testWindowOpts);
    await w.loadURL(draggablePageURL.href);
    await performDrag(w);
    w.destroy();
  });

  afterEach(closeAllWindows);

  describe('main window', () => {
    let w: BrowserWindow;

    beforeEach(() => {
      w = new BrowserWindow(testWindowOpts);
    });

    it('drags with app-region: drag', async () => {
      await w.loadURL(draggablePageURL.href);

      const { start, end } = await performDrag(w);

      expect(start).to.not.deep.equal(end);
    });

    it('does not drag when app-region: no-drag overlaps drag region', async () => {
      const noDragURL = new URL(draggablePageURL.href);
      noDragURL.searchParams.set('no-drag', '1');
      await w.loadURL(noDragURL.href);

      const { start, end } = await performDrag(w);

      expect(start).to.deep.equal(end);
    });

    it('drags after navigation', async () => {
      await w.loadFile(path.join(fixtures, 'pages', 'base-page.html'));
      await w.loadURL(draggablePageURL.href);

      const { start, end } = await performDrag(w);

      expect(start).to.not.deep.equal(end);
    });

    it('drags after in-page navigation', async () => {
      await w.loadURL(draggablePageURL.href);

      const didNavigate = once(w.webContents, 'did-navigate-in-page');
      await w.webContents.executeJavaScript(`
        window.history.pushState({}, '', '/new-path');
      `);
      await didNavigate;

      const { start, end } = await performDrag(w);

      expect(start).to.not.deep.equal(end);
    });
  });

  describe('child windows (window.open)', () => {
    let childWindow: BrowserWindow;

    beforeEach(async () => {
      const parentWindow = new BrowserWindow({ show: false });
      await parentWindow.loadFile(
        path.join(fixtures, 'pages', 'base-page.html')
      );

      parentWindow.webContents.setWindowOpenHandler(() => ({
        action: 'allow',
        overrideBrowserWindowOptions: testWindowOpts
      }));

      const newBrowserWindow = once(parentWindow.webContents, 'did-create-window');

      await parentWindow.webContents.executeJavaScript(
        `void window.open('${draggablePageURL.href}', '_blank');`
      );

      [childWindow] = await newBrowserWindow;
      await once(childWindow, 'ready-to-show');
    });

    it('drags with app-region: drag', async () => {
      const { start, end } = await performDrag(childWindow);

      expect(start).to.not.deep.equal(end);
    });

    it('drags after navigation', async () => {
      await childWindow.loadURL(draggablePageURL.href);

      const { start, end } = await performDrag(childWindow);

      expect(start).to.not.deep.equal(end);
    });
  });

  for (const frameType of ['webview', 'iframe'] as const) {
    // FIXME: this behavior is broken before the tests were added
    // See: https://github.com/electron/electron/issues/49256
    describe.skip(`child frames (${frameType})`, () => {
      const subframePageURL = frameType === 'webview' ? webviewPageURL : iframePageURL;
      let w: BrowserWindow;

      beforeEach(() => {
        w = new BrowserWindow({
          ...testWindowOpts,
          webPreferences: frameType === 'webview'
            ? {
                webviewTag: true
              }
            : {}
        });
      });

      it('drags in subframe with app-region: drag', async () => {
        await w.loadURL(subframePageURL.href);
        await loadDraggableSubframe(w);

        const { start, end } = await performDrag(w);

        expect(start).to.not.deep.equal(end);
      });

      it('drags after subframe navigation', async () => {
        await w.loadURL(subframePageURL.href);
        await loadDraggableSubframe(w);
        await loadDraggableSubframe(w);

        const { start, end } = await performDrag(w);

        expect(start).to.not.deep.equal(end);
      });

      it('does not drag after host page navigation without draggable region', async () => {
        await w.loadURL(subframePageURL.href);
        await loadDraggableSubframe(w);

        await w.loadFile(
          path.join(fixtures, 'pages', 'base-page.html')
        );

        const { start, end } = await performDrag(w);

        expect(start).to.deep.equal(end);
      });

      it('drags after host page navigation', async () => {
        await w.loadURL(subframePageURL.href);
        await loadDraggableSubframe(w);
        await w.loadURL(subframePageURL.href);
        await loadDraggableSubframe(w);

        const { start, end } = await performDrag(w);

        expect(start).to.not.deep.equal(end);
      });
    });
  }
});
