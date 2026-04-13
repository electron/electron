import { BrowserWindow, WebContents, ipcMain } from 'electron/main';

import { expect } from 'chai';
import { afterEach, beforeEach, describe, it } from 'vitest';

import { once } from 'node:events';
import * as path from 'node:path';

const fixturesPath = path.resolve(__dirname, '..', 'fixtures');

describe('focus handling (serial)', () => {
  let webviewContents: WebContents;
  let w: BrowserWindow;

  beforeEach(async () => {
    w = new BrowserWindow({
      alwaysOnTop: true,
      show: true,
      webPreferences: {
        nodeIntegration: true,
        webviewTag: true,
        contextIsolation: false
      }
    });

    const webviewReady = once(w.webContents, 'did-attach-webview') as Promise<[any, WebContents]>;
    await w.loadFile(path.join(fixturesPath, 'pages', 'tab-focus-loop-elements.html'));
    const [, wvContents] = await webviewReady;
    webviewContents = wvContents;
    await once(webviewContents, 'did-finish-load');
    w.focus();
  });

  afterEach(() => {
    webviewContents = null as unknown as WebContents;
    w.destroy();
    w = null as unknown as BrowserWindow;
  });

  const expectFocusChange = async () => {
    const [, focusedElementId] = await once(ipcMain, 'focus-changed');
    return focusedElementId;
  };

  describe('a TAB press', () => {
    const tabPressEvent: any = {
      type: 'keyDown',
      keyCode: 'Tab'
    };

    it('moves focus to the next focusable item', async () => {
      let focusChange = expectFocusChange();
      w.webContents.sendInputEvent(tabPressEvent);
      let focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-element-1',
        `should start focused in element-1, it's instead in ${focusedElementId}`
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(tabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-element-2',
        `focus should've moved to element-2, it's instead in ${focusedElementId}`
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(tabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-wv-element-1',
        `focus should've moved to the webview's element-1, it's instead in ${focusedElementId}`
      );

      focusChange = expectFocusChange();
      webviewContents.sendInputEvent(tabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-wv-element-2',
        `focus should've moved to the webview's element-2, it's instead in ${focusedElementId}`
      );

      focusChange = expectFocusChange();
      webviewContents.sendInputEvent(tabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-element-3',
        `focus should've moved to element-3, it's instead in ${focusedElementId}`
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(tabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-element-1',
        `focus should've looped back to element-1, it's instead in ${focusedElementId}`
      );
    });
  });

  describe('a SHIFT + TAB press', () => {
    const shiftTabPressEvent: any = {
      type: 'keyDown',
      modifiers: ['Shift'],
      keyCode: 'Tab'
    };

    it('moves focus to the previous focusable item', async () => {
      let focusChange = expectFocusChange();
      w.webContents.sendInputEvent(shiftTabPressEvent);
      let focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-element-3',
        `should start focused in element-3, it's instead in ${focusedElementId}`
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(shiftTabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-wv-element-2',
        `focus should've moved to the webview's element-2, it's instead in ${focusedElementId}`
      );

      focusChange = expectFocusChange();
      webviewContents.sendInputEvent(shiftTabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-wv-element-1',
        `focus should've moved to the webview's element-1, it's instead in ${focusedElementId}`
      );

      focusChange = expectFocusChange();
      webviewContents.sendInputEvent(shiftTabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-element-2',
        `focus should've moved to element-2, it's instead in ${focusedElementId}`
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(shiftTabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-element-1',
        `focus should've moved to element-1, it's instead in ${focusedElementId}`
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(shiftTabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId).to.equal(
        'BUTTON-element-3',
        `focus should've looped back to element-3, it's instead in ${focusedElementId}`
      );
    });
  });
});
