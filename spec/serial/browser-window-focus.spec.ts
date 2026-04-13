import { app, BrowserWindow } from 'electron/main';

import { expect } from 'chai';
import { afterEach, beforeEach, describe, it } from 'vitest';

import * as childProcess from 'node:child_process';
import { once } from 'node:events';
import { setTimeout } from 'node:timers/promises';

import { defer, ifdescribe, ifit, isWayland, dangerouslyIgnoreWebContentsLoadResult } from '../lib/spec-helpers';
import { closeAllWindows, closeWindow } from '../lib/window-helpers';

describe('BrowserWindow module (serial)', () => {
  afterEach(closeAllWindows);

  ifdescribe(!isWayland)('focus, blur, and z-order', () => {
    let w: BrowserWindow;
    beforeEach(() => {
      w = new BrowserWindow({ show: false });
    });
    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });

    describe('BrowserWindow.show()', () => {
      it('should focus on window', async () => {
        const p = once(w, 'focus');
        w.show();
        await p;
        expect(w.isFocused()).to.equal(true);
      });
      it('emits focus event and makes the window visible', async () => {
        const p = once(w, 'focus');
        w.show();
        await p;
        expect(w.isVisible()).to.equal(true);
      });
    });

    describe('BrowserWindow.hide()', () => {
      it('should defocus on window', () => {
        w.hide();
        expect(w.isFocused()).to.equal(false);
      });
    });

    describe('BrowserWindow.minimize()', () => {
      // TODO(codebytere): Enable for Linux once maximize/minimize events work in CI.
      ifit(process.platform !== 'linux')('should not be visible when the window is minimized', async () => {
        const minimize = once(w, 'minimize');
        w.minimize();
        await minimize;

        expect(w.isMinimized()).to.equal(true);
        expect(w.isVisible()).to.equal(false);
      });
    });

    describe('BrowserWindow.showInactive()', () => {
      it('should not focus on window', () => {
        w.showInactive();
        expect(w.isFocused()).to.equal(false);
      });

      // TODO(dsanders11): Enable for Linux once CI plays nice with these kinds of tests
      ifit(process.platform !== 'linux')('should not restore maximized windows', async () => {
        const maximize = once(w, 'maximize');
        const shown = once(w, 'show');
        w.maximize();
        // TODO(dsanders11): The maximize event isn't firing on macOS for a window initially hidden
        if (process.platform !== 'darwin') {
          await maximize;
        } else {
          await setTimeout(1000);
        }
        w.showInactive();
        await shown;
        expect(w.isMaximized()).to.equal(true);
      });

      ifit(process.platform === 'darwin')('should attach child window to parent', async () => {
        const wShow = once(w, 'show');
        w.show();
        await wShow;

        const c = new BrowserWindow({ show: false, parent: w });
        const cShow = once(c, 'show');
        c.showInactive();
        await cShow;

        // verifying by checking that the child tracks the parent's visibility
        const minimized = once(w, 'minimize');
        w.minimize();
        await minimized;

        expect(w.isVisible()).to.be.false('parent is visible');
        expect(c.isVisible()).to.be.false('child is visible');

        const restored = once(w, 'restore');
        w.restore();
        await restored;

        expect(w.isVisible()).to.be.true('parent is visible');
        expect(c.isVisible()).to.be.true('child is visible');

        await closeWindow(c);
      });
    });

    describe('BrowserWindow.focus()', () => {
      it('does not make the window become visible', () => {
        expect(w.isVisible()).to.equal(false);
        w.focus();
        expect(w.isVisible()).to.equal(false);
      });

      ifit(process.platform !== 'win32')('focuses a blurred window', async () => {
        {
          const isBlurred = once(w, 'blur');
          const isShown = once(w, 'show');
          w.show();
          w.blur();
          await isShown;
          await isBlurred;
        }
        expect(w.isFocused()).to.equal(false);
        w.focus();
        expect(w.isFocused()).to.equal(true);
      });

      ifit(process.platform !== 'linux')('acquires focus status from the other windows', async () => {
        const w1 = new BrowserWindow({ show: false });
        const w2 = new BrowserWindow({ show: false });
        const w3 = new BrowserWindow({ show: false });
        {
          const isFocused3 = once(w3, 'focus');
          const isShown1 = once(w1, 'show');
          const isShown2 = once(w2, 'show');
          const isShown3 = once(w3, 'show');
          w1.show();
          w2.show();
          w3.show();
          await isShown1;
          await isShown2;
          await isShown3;
          await isFocused3;
        }
        // TODO(RaisinTen): Investigate why this assertion fails only on Linux.
        expect(w1.isFocused()).to.equal(false);
        expect(w2.isFocused()).to.equal(false);
        expect(w3.isFocused()).to.equal(true);

        w1.focus();
        expect(w1.isFocused()).to.equal(true);
        expect(w2.isFocused()).to.equal(false);
        expect(w3.isFocused()).to.equal(false);

        w2.focus();
        expect(w1.isFocused()).to.equal(false);
        expect(w2.isFocused()).to.equal(true);
        expect(w3.isFocused()).to.equal(false);

        w3.focus();
        expect(w1.isFocused()).to.equal(false);
        expect(w2.isFocused()).to.equal(false);
        expect(w3.isFocused()).to.equal(true);

        {
          const isClosed1 = once(w1, 'closed');
          const isClosed2 = once(w2, 'closed');
          const isClosed3 = once(w3, 'closed');
          w1.destroy();
          w2.destroy();
          w3.destroy();
          await isClosed1;
          await isClosed2;
          await isClosed3;
        }
      });

      ifit(process.platform === 'darwin')('it does not activate the app if focusing an inactive panel', async () => {
        // Show to focus app, then remove existing window
        w.show();
        w.destroy();

        // We first need to resign app focus for this test to work
        const isInactive = once(app, 'did-resign-active');
        childProcess.execSync('osascript -e \'tell application "Finder" to activate\'');
        defer(() => childProcess.execSync('osascript -e \'tell application "Finder" to quit\''));
        await isInactive;

        // Create new window
        w = new BrowserWindow({
          type: 'panel',
          height: 200,
          width: 200,
          center: true,
          show: false
        });

        const isShow = once(w, 'show');
        const isFocus = once(w, 'focus');

        w.show();
        w.focus();

        await isShow;
        await isFocus;

        const getActiveAppOsa =
          'tell application "System Events" to get the name of the first process whose frontmost is true';
        const activeApp = childProcess.execSync(`osascript -e '${getActiveAppOsa}'`).toString().trim();

        expect(activeApp).to.equal('Finder');
      });
    });

    // TODO(RaisinTen): Make this work on Windows too.
    // Refs: https://github.com/electron/electron/issues/20464.
    ifdescribe(process.platform !== 'win32')('BrowserWindow.blur()', () => {
      it('removes focus from window', async () => {
        {
          const isFocused = once(w, 'focus');
          const isShown = once(w, 'show');
          w.show();
          await isShown;
          await isFocused;
        }
        expect(w.isFocused()).to.equal(true);
        w.blur();
        expect(w.isFocused()).to.equal(false);
      });

      ifit(process.platform !== 'linux')('transfers focus status to the next window', async () => {
        const w1 = new BrowserWindow({ show: false });
        const w2 = new BrowserWindow({ show: false });
        const w3 = new BrowserWindow({ show: false });
        {
          const isFocused3 = once(w3, 'focus');
          const isShown1 = once(w1, 'show');
          const isShown2 = once(w2, 'show');
          const isShown3 = once(w3, 'show');
          w1.show();
          w2.show();
          w3.show();
          await isShown1;
          await isShown2;
          await isShown3;
          await isFocused3;
        }
        // TODO(RaisinTen): Investigate why this assertion fails only on Linux.
        expect(w1.isFocused()).to.equal(false);
        expect(w2.isFocused()).to.equal(false);
        expect(w3.isFocused()).to.equal(true);

        w3.blur();
        expect(w1.isFocused()).to.equal(false);
        expect(w2.isFocused()).to.equal(true);
        expect(w3.isFocused()).to.equal(false);

        w2.blur();
        expect(w1.isFocused()).to.equal(true);
        expect(w2.isFocused()).to.equal(false);
        expect(w3.isFocused()).to.equal(false);

        w1.blur();
        expect(w1.isFocused()).to.equal(false);
        expect(w2.isFocused()).to.equal(false);
        expect(w3.isFocused()).to.equal(true);

        {
          const isClosed1 = once(w1, 'closed');
          const isClosed2 = once(w2, 'closed');
          const isClosed3 = once(w3, 'closed');
          w1.destroy();
          w2.destroy();
          w3.destroy();
          await isClosed1;
          await isClosed2;
          await isClosed3;
        }
      });
    });

    describe('BrowserWindow.getFocusedWindow()', () => {
      it('returns the opener window when dev tools window is focused', async () => {
        const p = once(w, 'focus');
        w.show();
        await p;
        w.webContents.openDevTools({ mode: 'undocked' });
        await once(w.webContents, 'devtools-focused');
        expect(BrowserWindow.getFocusedWindow()).to.equal(w);
      });
    });

    describe('BrowserWindow.moveTop()', () => {
      afterEach(closeAllWindows);

      it('should not steal focus', async () => {
        const posDelta = 50;
        const wShownInactive = once(w, 'show');
        w.showInactive();
        await wShownInactive;
        expect(w.isFocused()).to.equal(false);

        const otherWindow = new BrowserWindow({ show: false, title: 'otherWindow' });
        const otherWindowShown = once(otherWindow, 'show');
        const otherWindowFocused = once(otherWindow, 'focus');
        otherWindow.show();
        await otherWindowShown;
        await otherWindowFocused;
        expect(otherWindow.isFocused()).to.equal(true);

        w.moveTop();
        const wPos = w.getPosition();
        const wMoving = once(w, 'move');
        w.setPosition(wPos[0] + posDelta, wPos[1] + posDelta);
        await wMoving;
        expect(w.isFocused()).to.equal(false);
        expect(otherWindow.isFocused()).to.equal(true);

        const wFocused = once(w, 'focus');
        const otherWindowBlurred = once(otherWindow, 'blur');
        w.focus();
        await wFocused;
        await otherWindowBlurred;
        expect(w.isFocused()).to.equal(true);

        otherWindow.moveTop();
        const otherWindowPos = otherWindow.getPosition();
        const otherWindowMoving = once(otherWindow, 'move');
        otherWindow.setPosition(otherWindowPos[0] + posDelta, otherWindowPos[1] + posDelta);
        await otherWindowMoving;
        expect(otherWindow.isFocused()).to.equal(false);
        expect(w.isFocused()).to.equal(true);

        await closeWindow(otherWindow);
        expect(BrowserWindow.getAllWindows()).to.have.lengthOf(1);
      });

      it('should not crash when called on a modal child window', async () => {
        const shown = once(w, 'show');
        w.show();
        await shown;

        const child = new BrowserWindow({ modal: true, parent: w });
        expect(() => {
          child.moveTop();
        }).to.not.throw();
      });
    });

    describe('BrowserWindow.moveAbove(mediaSourceId)', () => {
      it('should throw an exception if wrong formatting', async () => {
        const fakeSourceIds = ['none', 'screen:0', 'window:fake', 'window:1234', 'foobar:1:2'];
        for (const sourceId of fakeSourceIds) {
          expect(() => {
            w.moveAbove(sourceId);
          }).to.throw(/Invalid media source id/);
        }
      });

      it('should throw an exception if wrong type', async () => {
        const fakeSourceIds = [null as any, 123 as any];
        for (const sourceId of fakeSourceIds) {
          expect(() => {
            w.moveAbove(sourceId);
          }).to.throw(/Error processing argument at index 0 */);
        }
      });

      it('should throw an exception if invalid window', async () => {
        // It is very unlikely that these window id exist.
        const fakeSourceIds = ['window:99999999:0', 'window:123456:1', 'window:123456:9'];
        for (const sourceId of fakeSourceIds) {
          expect(() => {
            w.moveAbove(sourceId);
          }).to.throw(/Invalid media source id/);
        }
      });

      it('should not throw an exception', async () => {
        const w2 = new BrowserWindow({ show: false, title: 'window2' });
        const w2Shown = once(w2, 'show');
        w2.show();
        await w2Shown;

        expect(() => {
          w.moveAbove(w2.getMediaSourceId());
        }).to.not.throw();

        await closeWindow(w2);
      });
    });

    describe('BrowserWindow.setFocusable()', () => {
      it('can set unfocusable window to focusable', async () => {
        const w2 = new BrowserWindow({ focusable: false });
        const w2Focused = once(w2, 'focus');
        w2.setFocusable(true);
        w2.focus();
        await w2Focused;
        await closeWindow(w2);
      });
    });

    describe('BrowserWindow.isFocusable()', () => {
      it('correctly returns whether a window is focusable', async () => {
        const w2 = new BrowserWindow({ focusable: false });
        expect(w2.isFocusable()).to.be.false();

        w2.setFocusable(true);
        expect(w2.isFocusable()).to.be.true();
        await closeWindow(w2);
      });
    });

    describe('window.webContents.focus()', () => {
      afterEach(closeAllWindows);
      it('focuses window', async () => {
        const w1 = new BrowserWindow({ x: 100, y: 300, width: 300, height: 200 });
        dangerouslyIgnoreWebContentsLoadResult(w1.loadURL('about:blank'));
        const w2 = new BrowserWindow({ x: 300, y: 300, width: 300, height: 200 });
        dangerouslyIgnoreWebContentsLoadResult(w2.loadURL('about:blank'));
        const w1Focused = once(w1, 'focus');
        w1.webContents.focus();
        await w1Focused;
        expect(w1.webContents.isFocused()).to.be.true('focuses window');
      });
    });
  });
});
