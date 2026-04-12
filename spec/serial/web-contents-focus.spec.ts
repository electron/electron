import { BrowserWindow, webContents } from 'electron/main';

import { assert, expect } from 'chai';
import { afterEach, describe, it } from 'vitest';

import { once } from 'node:events';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifit } from '../lib/spec-helpers';
import { closeAllWindows } from '../lib/window-helpers';

describe('webContents module (serial)', () => {
  afterEach(closeAllWindows);

  describe('getFocusedWebContents() API', () => {
    afterEach(closeAllWindows);

    // FIXME
    ifit(!(process.platform === 'win32' && process.arch === 'arm64'))('returns the focused web contents', async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadFile(path.join(__dirname, '..', 'fixtures', 'blank.html'));
      expect(webContents.getFocusedWebContents()?.id).to.equal(w.webContents.id);

      const devToolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools();
      await devToolsOpened;
      expect(webContents.getFocusedWebContents()?.id).to.equal(w.webContents.devToolsWebContents!.id);
      const devToolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devToolsClosed;
      expect(webContents.getFocusedWebContents()?.id).to.equal(w.webContents.id);
    });

    it('does not crash when called on a detached dev tools window', async () => {
      const w = new BrowserWindow({ show: true });

      w.webContents.openDevTools({ mode: 'detach' });
      w.webContents.inspectElement(100, 100);

      // For some reason we have to wait for two focused events...?
      await once(w.webContents, 'devtools-focused');

      expect(() => {
        webContents.getFocusedWebContents();
      }).to.not.throw();

      // Work around https://github.com/electron/electron/issues/19985
      await setTimeout();

      const devToolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devToolsClosed;
      expect(() => {
        webContents.getFocusedWebContents();
      }).to.not.throw();
    });

    it('Inspect activates detached devtools window', async () => {
      const window = new BrowserWindow({ show: true });
      await window.loadURL('about:blank');
      const webContentsBeforeOpenedDevtools = webContents.getAllWebContents();

      const windowWasBlurred = once(window, 'blur');
      window.webContents.openDevTools({ mode: 'detach' });
      await windowWasBlurred;

      let devToolsWebContents = null;
      for (const newWebContents of webContents.getAllWebContents()) {
        const oldWebContents = webContentsBeforeOpenedDevtools.find((oldWebContents) => {
          return newWebContents.id === oldWebContents.id;
        });
        if (oldWebContents !== null) {
          devToolsWebContents = newWebContents;
          break;
        }
      }
      assert(devToolsWebContents !== null);

      const windowFocused = once(window, 'focus');
      const devToolsBlurred = once(devToolsWebContents, 'blur');
      window.focus();
      await Promise.all([windowFocused, devToolsBlurred]);

      expect(devToolsWebContents.isFocused()).to.be.false();
      const devToolsWebContentsFocused = once(devToolsWebContents, 'focus');
      const windowBlurred = once(window, 'blur');
      window.webContents.inspectElement(100, 100);
      await Promise.all([devToolsWebContentsFocused, windowBlurred]);

      expect(devToolsWebContents.isFocused()).to.be.true();
      expect(window.isFocused()).to.be.false();
    });
  });

  describe('isFocused() API', () => {
    afterEach(closeAllWindows);
    it('returns false when the window is hidden', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      expect(w.isVisible()).to.be.false();
      expect(w.webContents.isFocused()).to.be.false();
    });
  });

  describe('openDevTools() API', () => {
    afterEach(closeAllWindows);
    it('can show window with activation', async () => {
      const w = new BrowserWindow({ show: false });
      const focused = once(w, 'focus');
      w.show();
      await focused;
      expect(w.isFocused()).to.be.true();
      const blurred = once(w, 'blur');
      w.webContents.openDevTools({ mode: 'detach', activate: true });
      await Promise.all([once(w.webContents, 'devtools-opened'), once(w.webContents, 'devtools-focused')]);
      await blurred;
      expect(w.isFocused()).to.be.false();
    });

    it('can show window without activation', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false });
      await devtoolsOpened;
      expect(w.webContents.isDevToolsOpened()).to.be.true();
    });

    it('can show a DevTools window with custom title', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false, title: 'myTitle' });
      await devtoolsOpened;
      expect(w.webContents.getDevToolsTitle()).to.equal('myTitle');
    });

    it('can re-open devtools', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: true });
      await devtoolsOpened;
      expect(w.webContents.isDevToolsOpened()).to.be.true();

      const devtoolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devtoolsClosed;
      expect(w.webContents.isDevToolsOpened()).to.be.false();

      const devtoolsOpened2 = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: true });
      await devtoolsOpened2;
      expect(w.webContents.isDevToolsOpened()).to.be.true();
    });
  });

  describe('focus APIs', () => {
    describe('focus()', () => {
      afterEach(closeAllWindows);
      it('does not blur the focused window when the web contents is hidden', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
        w.show();
        await w.loadURL('about:blank');
        w.focus();
        const child = new BrowserWindow({ show: false });
        child.loadURL('about:blank');
        child.webContents.focus();
        const currentFocused = w.isFocused();
        const childFocused = child.isFocused();
        child.close();
        expect(currentFocused).to.be.true();
        expect(childFocused).to.be.false();
      });

      it('does not crash when focusing a WebView webContents', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            webviewTag: true
          }
        });

        w.show();
        await w.loadURL('data:text/html,<webview src="data:text/html,hi"></webview>');

        const wc = webContents.getAllWebContents().find((wc) => wc.getType() === 'webview')!;
        expect(() => wc.focus()).to.not.throw();
      });
    });

    const moveFocusToDevTools = async (win: BrowserWindow) => {
      const devToolsOpened = once(win.webContents, 'devtools-opened');
      win.webContents.openDevTools({ mode: 'right' });
      await devToolsOpened;
      win.webContents.devToolsWebContents!.focus();
    };

    describe('focus event', () => {
      afterEach(closeAllWindows);

      it('is triggered when web contents is focused', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadURL('about:blank');
        await moveFocusToDevTools(w);
        const focusPromise = once(w.webContents, 'focus');
        w.webContents.focus();
        await expect(focusPromise).to.eventually.be.fulfilled();
      });
    });

    describe('blur event', () => {
      afterEach(closeAllWindows);
      it('is triggered when web contents is blurred', async () => {
        const w = new BrowserWindow({ show: true });
        await w.loadURL('about:blank');
        w.webContents.focus();
        const blurPromise = once(w.webContents, 'blur');
        await moveFocusToDevTools(w);
        await expect(blurPromise).to.eventually.be.fulfilled();
      });
    });

    describe('focusOnNavigation webPreference', () => {
      afterEach(closeAllWindows);

      it('focuses the webContents on navigation by default', async () => {
        const w = new BrowserWindow({ show: true });
        await once(w, 'focus');
        await w.loadURL('about:blank');
        await moveFocusToDevTools(w);
        expect(w.webContents.isFocused()).to.be.false();
        await w.loadURL('data:text/html,<body>test</body>');
        expect(w.webContents.isFocused()).to.be.true();
      });

      it('does not focus the webContents on navigation when focusOnNavigation is false', async () => {
        const w = new BrowserWindow({
          show: true,
          webPreferences: {
            focusOnNavigation: false
          }
        });
        await once(w, 'focus');
        await w.loadURL('about:blank');
        await moveFocusToDevTools(w);
        expect(w.webContents.isFocused()).to.be.false();
        await w.loadURL('data:text/html,<body>test</body>');
        expect(w.webContents.isFocused()).to.be.false();
      });
    });
  });
});
