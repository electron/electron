import { BrowserWindow, ipcMain, type WebContents } from 'electron/main';

import { afterAll, beforeAll, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as path from 'node:path';

import { defer } from './lib/spec-helpers.ts';

describe('webFrame module', () => {
  const fixtures = path.resolve(import.meta.dirname, 'fixtures');

  it('can use executeJavaScript', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        contextIsolation: true,
        preload: path.join(fixtures, 'pages', 'world-safe-preload.js')
      }
    });
    defer(() => w.close());
    const isSafe = once(ipcMain, 'executejs-safe');
    w.loadURL('about:blank');
    const [, wasSafe] = await isSafe;
    expect(wasSafe).toBe(true);
  });

  it('can use executeJavaScript and catch conversion errors', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        contextIsolation: true,
        preload: path.join(fixtures, 'pages', 'world-safe-preload-error.js')
      }
    });
    defer(() => w.close());
    const execError = once(ipcMain, 'executejs-safe');
    w.loadURL('about:blank');
    const [, error] = await execError;
    expect(error, 'Error should not be null').not.toBe(null);
    expect(error).toHaveProperty('message', 'Uncaught Error: An object could not be cloned.');
  });

  it('calls a spellcheck provider', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        contextIsolation: false
      }
    });
    defer(() => w.close());
    await w.loadFile(path.join(fixtures, 'pages', 'webframe-spell-check.html'));
    w.focus();
    await w.webContents.executeJavaScript('document.querySelector("input").focus()', true);

    const spellCheckerFeedback = new Promise<[string[], boolean]>((resolve) => {
      ipcMain.on('spec-spell-check', (e, words, callbackDefined) => {
        if (words.length === 5) {
          // The API calls the provider after every completed word.
          // The promise is resolved only after this event is received with all words.
          resolve([words, callbackDefined]);
        }
      });
    });
    const inputText = "spleling test you're ";
    for (const keyCode of inputText) {
      w.webContents.sendInputEvent({ type: 'char', keyCode });
    }
    const [words, callbackDefined] = await spellCheckerFeedback;
    expect(words.sort()).toEqual(['spleling', 'test', "you're", 'you', 're'].sort());
    expect(callbackDefined).toBe(true);
  });

  describe('api', () => {
    let w: WebContents;
    let win: BrowserWindow;
    beforeAll(async () => {
      win = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false, nodeIntegration: true } });
      await win.loadURL('data:text/html,<iframe name="test"></iframe>');
      w = win.webContents;
      await w.executeJavaScript(`
        var { webFrame } = require('electron');
        var isSameWebFrame = (a, b) => a.context === b.context;
        childFrame = webFrame.firstChild;
        null
      `);
    });

    afterAll(() => {
      win.close();
      win = null as unknown as BrowserWindow;
    });

    describe('top', () => {
      it('is self for top frame', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(webFrame.top, webFrame)');
        expect(equal).toBe(true);
      });

      it('is self for child frame', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(childFrame.top, webFrame)');
        expect(equal).toBe(true);
      });
    });

    describe('opener', () => {
      it('is null for top frame', async () => {
        const equal = await w.executeJavaScript('webFrame.opener === null');
        expect(equal).toBe(true);
      });
    });

    describe('parent', () => {
      it('is null for top frame', async () => {
        const equal = await w.executeJavaScript('webFrame.parent === null');
        expect(equal).toBe(true);
      });

      it('is top frame for child frame', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(childFrame.parent, webFrame)');
        expect(equal).toBe(true);
      });
    });

    describe('firstChild', () => {
      it('is child frame for top frame', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(webFrame.firstChild, childFrame)');
        expect(equal).toBe(true);
      });

      it('is null for child frame', async () => {
        const equal = await w.executeJavaScript('childFrame.firstChild === null');
        expect(equal).toBe(true);
      });
    });

    describe('getFrameForSelector()', () => {
      it('does not crash when not found', async () => {
        const equal = await w.executeJavaScript('webFrame.getFrameForSelector("unexist-selector") === null');
        expect(equal).toBe(true);
      });

      it('returns the webFrame when found', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(webFrame.getFrameForSelector("iframe"), childFrame)');
        expect(equal).toBe(true);
      });
    });

    describe('findFrameByName()', () => {
      it('does not crash when not found', async () => {
        const equal = await w.executeJavaScript('webFrame.findFrameByName("unexist-name") === null');
        expect(equal).toBe(true);
      });

      it('returns the webFrame when found', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(webFrame.findFrameByName("test"), childFrame)');
        expect(equal).toBe(true);
      });
    });

    describe('findFrameByToken()', () => {
      it('does not crash when not found', async () => {
        const equal = await w.executeJavaScript('webFrame.findFrameByToken("unknown") === null');
        expect(equal).toBe(true);
      });

      it('returns the webFrame when found', async () => {
        const equal = await w.executeJavaScript(
          'isSameWebFrame(webFrame.findFrameByToken(childFrame.frameToken), childFrame)'
        );
        expect(equal).toBe(true);
      });
    });

    describe('setZoomFactor()', () => {
      it('works', async () => {
        const zoom = await w.executeJavaScript('childFrame.setZoomFactor(2.0); childFrame.getZoomFactor()');
        expect(zoom).toBe(2.0);
      });
    });

    describe('setZoomLevel()', () => {
      it('works', async () => {
        const zoom = await w.executeJavaScript('childFrame.setZoomLevel(5); childFrame.getZoomLevel()');
        expect(zoom).toBe(5);
      });
    });

    describe('getResourceUsage()', () => {
      it('works', async () => {
        const result = await w.executeJavaScript('childFrame.getResourceUsage()');
        expect(result).toHaveProperty('images', expect.any(Object));
        expect(result).toHaveProperty('scripts', expect.any(Object));
        expect(result).toHaveProperty('cssStyleSheets', expect.any(Object));
        expect(result).toHaveProperty('xslStyleSheets', expect.any(Object));
        expect(result).toHaveProperty('fonts', expect.any(Object));
        expect(result).toHaveProperty('other', expect.any(Object));
      });
    });

    describe('executeJavaScript', () => {
      it('executeJavaScript() yields results via a promise and a sync callback', async () => {
        const { callbackResult, callbackError, result } = await w.executeJavaScript(`new Promise(resolve => {
          let callbackResult, callbackError;
          childFrame
            .executeJavaScript('1 + 1', (result, error) => {
              callbackResult = result;
              callbackError = error;
            }).then(result => resolve({callbackResult, callbackError, result}))
        })`);

        expect(callbackResult).toBe(2);
        expect(callbackError).toBeUndefined();
        expect(result).toBe(2);
      });

      it('executeJavaScriptInIsolatedWorld() yields results via a promise and a sync callback', async () => {
        const { callbackResult, callbackError, result } = await w.executeJavaScript(`new Promise(resolve => {
          let callbackResult, callbackError;
          childFrame
            .executeJavaScriptInIsolatedWorld(999, [{code: '1 + 1'}], (result, error) => {
              callbackResult = result;
              callbackError = error;
            }).then(result => resolve({callbackResult, callbackError, result}))
        })`);

        expect(callbackResult).toBe(2);
        expect(callbackError).toBeUndefined();
        expect(result).toBe(2);
      });

      it('executeJavaScript() yields errors via a promise and a sync callback', async () => {
        const { callbackResult, callbackError, error } = await w.executeJavaScript(`new Promise(resolve => {
          let callbackResult, callbackError;
          childFrame
            .executeJavaScript('thisShouldProduceAnError()', (result, error) => {
              callbackResult = result;
              callbackError = error;
            }).then(result => {throw new Error}, error => resolve({callbackResult, callbackError, error}))
        })`);

        expect(callbackResult).toBeUndefined();
        expect(callbackError).toBeInstanceOf(Error);
        expect(error).toBeInstanceOf(Error);
      });

      it('executeJavaScriptInIsolatedWorld() yields errors via a promise and a sync callback', async () => {
        const { callbackResult, callbackError, error } = await w.executeJavaScript(`new Promise(resolve => {
          let callbackResult, callbackError;
          childFrame
            .executeJavaScriptInIsolatedWorld(999, [{code: 'thisShouldProduceAnError()'}], (result, error) => {
              callbackResult = result;
              callbackError = error;
            }).then(result => {throw new Error}, error => resolve({callbackResult, callbackError, error}))
        })`);

        expect(callbackResult).toBeUndefined();
        expect(callbackError).toBeInstanceOf(Error);
        expect(error).toBeInstanceOf(Error);
      });

      it('executeJavaScript(InIsolatedWorld) can be used without a callback', async () => {
        expect(await w.executeJavaScript("webFrame.executeJavaScript('1 + 1')")).toBe(2);
        expect(await w.executeJavaScript("webFrame.executeJavaScriptInIsolatedWorld(999, [{code: '1 + 1'}])")).toBe(2);
      });

      it('executeJavaScriptInIsolatedWorld() rejects when worldId is not an integer', async () => {
        const result = await w.executeJavaScript(`
          webFrame.executeJavaScriptInIsolatedWorld('1234', [{code: '1 + 1'}]).then(
            () => ({ rejected: false }),
            (error) => ({ rejected: true, name: error.name, message: error.message })
          )
        `);
        expect(result.rejected).toBe(true);
        expect(result.name).toBe('TypeError');
        expect(result.message).toBe('worldId must be an integer');
      });
    });
  });
});
