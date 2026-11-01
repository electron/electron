import { BrowserWindow, ipcMain, type WebContents } from 'electron/main';

import { afterAll, beforeAll, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';
import { pathToFileURL } from 'node:url';

import { defer, listen } from './lib/spec-helpers.ts';

describe('webFrame module', () => {
  const fixtures = path.resolve(import.meta.dirname, 'fixtures');

  const createServer = async () => {
    const server = http.createServer((req, res) => {
      if (req.url === '/one') {
        res.end('<h1>one</h1>');
        return;
      }

      if (req.url === '/two') {
        res.end('<h1>two</h1>');
        return;
      }

      res.end('<h1>after</h1>');
    });
    const serverUrl = (await listen(server)).url;
    return {
      server,
      url: serverUrl,
      crossOriginUrl: serverUrl.replace('127.0.0.1', 'localhost')
    };
  };

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

  it('recreates isolated world discovery after same-renderer navigation', async () => {
    const { server, url } = await createServer();
    defer(() => server.close());

    const win = new BrowserWindow({
      show: false,
      webPreferences: {
        contextIsolation: false,
        nodeIntegration: true,
        preload: path.join(fixtures, 'pages', 'isolated-world-navigation-preload.js')
      }
    });
    defer(() => win.close());

    await win.loadURL(`${url}/one`);
    const pidBeforeNavigation = win.webContents.getOSProcessId();
    const worldsBeforeNavigation = await win.webContents.executeJavaScript(`
      const { webFrame } = require('electron');
      webFrame.executeJavaScriptInIsolatedWorld(1234, [{ code: 'window.__marker = "page-one"' }])
        .then(() => webFrame.getIsolatedWorlds());
    `);
    expect(worldsBeforeNavigation).toContain(1234);

    await win.loadURL(`${url}/two`);
    expect(win.webContents.getOSProcessId()).toBe(pidBeforeNavigation);

    const navigationState = once(ipcMain, 'isolated-world-navigation-state');
    win.webContents.send('get-isolated-world-navigation-state');
    const [, state] = (await navigationState) as [
      unknown,
      {
        startupWorlds: number[];
        createdWorlds: number[];
        isolatedWorlds: number[];
      }
    ];

    expect(state.startupWorlds).toEqual([]);
    expect(state.createdWorlds).toContain(1234);
    expect(state.isolatedWorlds).toContain(1234);

    const valueAfterNavigation = await win.webContents.executeJavaScript(`
      require('electron').webFrame.executeJavaScriptInIsolatedWorld(1234, [{ code: 'window.__marker' }]);
    `);
    expect(valueAfterNavigation).toBe(undefined);
  });

  it('clears isolated world discovery after renderer-swap navigation', async () => {
    const { server, url, crossOriginUrl } = await createServer();
    defer(() => server.close());

    const win = new BrowserWindow({
      show: false,
      webPreferences: {
        contextIsolation: false,
        nodeIntegration: true
      }
    });
    defer(() => win.close());

    await win.loadURL(`${url}/one`);
    const frameTokenBeforeNavigation = win.webContents.mainFrame.frameToken;
    const worldsBeforeNavigation = await win.webContents.executeJavaScript(`
      const { webFrame } = require('electron');
      webFrame.executeJavaScriptInIsolatedWorld(1234, [{ code: 'void 0' }])
        .then(() => webFrame.getIsolatedWorlds());
    `);
    expect(worldsBeforeNavigation).toContain(1234);

    await win.loadURL(`${crossOriginUrl}/two`);
    expect(win.webContents.mainFrame.frameToken).not.toBe(frameTokenBeforeNavigation);

    const worldsAfterNavigation = await win.webContents.executeJavaScript(`
      require('electron').webFrame.getIsolatedWorlds();
    `);
    expect(worldsAfterNavigation).toEqual([]);
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

    describe('removeInsertedCSS()', () => {
      const getBackgroundRepeat = 'getComputedStyle(document.body).backgroundRepeat';

      it('removes CSS inserted with the author origin', async () => {
        const inserted = await w.executeJavaScript(`
          cssKey = webFrame.insertCSS('body { background-repeat: round !important; }');
          ${getBackgroundRepeat}
        `);
        expect(inserted).toBe('round');
        const removed = await w.executeJavaScript(`webFrame.removeInsertedCSS(cssKey); ${getBackgroundRepeat}`);
        expect(removed).toBe('repeat');
      });

      it('removes CSS inserted with the user origin', async () => {
        const inserted = await w.executeJavaScript(`
          cssKey = webFrame.insertCSS('body { background-repeat: round !important; }', { cssOrigin: 'user' });
          ${getBackgroundRepeat}
        `);
        expect(inserted).toBe('round');
        const removed = await w.executeJavaScript(`webFrame.removeInsertedCSS(cssKey); ${getBackgroundRepeat}`);
        expect(removed).toBe('repeat');
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

    describe('isolated world discovery', () => {
      it('getIsolatedWorlds() tracks worlds per frame', async () => {
        const { mainWorlds, childWorlds } = await w.executeJavaScript(`
          childFrame.executeJavaScriptInIsolatedWorld(1104, [{ code: 'void 0' }]).then(() => ({
            mainWorlds: webFrame.getIsolatedWorlds(),
            childWorlds: childFrame.getIsolatedWorlds()
          }))
        `);

        expect(mainWorlds).not.toContain(1104);
        expect(childWorlds).toContain(1104);
      });

      it('emits isolated-world-created when a world is created', async () => {
        const createdWorld = await w.executeJavaScript(`new Promise(resolve => {
          webFrame.once('isolated-world-created', (worldId) => resolve(worldId));
          webFrame.executeJavaScriptInIsolatedWorld(1105, [{ code: 'void 0' }]);
        })`);

        expect(createdWorld).toBe(1105);
      });

      it('emits isolated-world-created on child frames', async () => {
        const createdWorld = await w.executeJavaScript(`new Promise(resolve => {
          childFrame.once('isolated-world-created', (worldId) => resolve(worldId));
          childFrame.executeJavaScriptInIsolatedWorld(1106, [{ code: 'void 0' }]);
        })`);

        expect(createdWorld).toBe(1106);
      });

      it('does not include main world (0) and Electron isolated world (999)', async () => {
        const worlds = await w.executeJavaScript(`
          webFrame.executeJavaScriptInIsolatedWorld(999, [{ code: 'void 0' }]).then(() =>
            webFrame.getIsolatedWorlds()
          )
        `);
        expect(worlds).not.toContain(0);
        expect(worlds).not.toContain(999);
      });

      it('does not crash when a listener registers more listeners while being notified', async () => {
        const worlds = await w.executeJavaScript(`new Promise(resolve => {
          const mk = () => webFrame.findFrameByToken(webFrame.frameToken);
          const seen = [];
          const a = mk();
          const b = mk();
          a.once('isolated-world-created', (worldId) => {
            seen.push(worldId);
            for (let i = 0; i < 256; i++) mk().on('isolated-world-created', () => {});
          });
          b.once('isolated-world-created', (worldId) => {
            seen.push(worldId);
            resolve(seen);
          });
          webFrame.executeJavaScriptInIsolatedWorld(1107, [{ code: 'void 0' }]);
        })`);
        expect(worlds).toEqual([1107, 1107]);
      });
    });
  });

  describe('frame tree', () => {
    it('nextSibling walks sibling frames in document order', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false, nodeIntegration: true } });
      defer(() => w.close());
      await w.loadURL('data:text/html,<iframe name="a"></iframe><iframe name="b"></iframe><iframe name="c"></iframe>');
      const result = await w.webContents.executeJavaScript(`(() => {
        const { webFrame } = require('electron');
        const first = webFrame.firstChild;
        const second = first.nextSibling;
        const third = second.nextSibling;
        return {
          first: first.frameToken === webFrame.findFrameByName('a').frameToken,
          second: second.frameToken === webFrame.findFrameByName('b').frameToken,
          third: third.frameToken === webFrame.findFrameByName('c').frameToken,
          afterLast: third.nextSibling,
          topNextSibling: webFrame.nextSibling
        };
      })()`);
      expect(result).toEqual({ first: true, second: true, third: true, afterLast: null, topNextSibling: null });
    });
  });

  describe('insertText()', () => {
    it('inserts text into the focused element', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false, nodeIntegration: true } });
      defer(() => w.close());
      await w.loadURL('data:text/html,<input id="field" value="">');
      const value = await w.webContents.executeJavaScript(`(() => {
        const field = document.getElementById('field');
        field.focus();
        require('electron').webFrame.insertText('hello world');
        return field.value;
      })()`);
      expect(value).toBe('hello world');
    });
  });

  describe('insertCSS() and removeInsertedCSS()', () => {
    let w: BrowserWindow;
    beforeAll(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false, nodeIntegration: true } });
      await w.loadURL(
        'data:text/html,<style>%23important { color: rgb(255, 0, 0) !important; }</style>' +
          '<div id="important"></div><div id="plain"></div>'
      );
      await w.webContents.executeJavaScript(`
        var { webFrame } = require('electron');
        var colorOf = (id) => getComputedStyle(document.getElementById(id)).color;
        null
      `);
    });

    afterAll(() => {
      w.close();
      w = null as unknown as BrowserWindow;
    });

    it('applies author-origin CSS by default and removes it by key', async () => {
      const result = await w.webContents.executeJavaScript(`(() => {
        const before = colorOf('plain');
        const key = webFrame.insertCSS('#plain { color: rgb(0, 128, 0); }');
        const inserted = colorOf('plain');
        webFrame.removeInsertedCSS(key);
        return { before, key, inserted, after: colorOf('plain') };
      })()`);
      expect(result.key).toEqual(expect.any(String));
      expect(result.key).not.toBe('');
      expect(result.inserted).toBe('rgb(0, 128, 0)');
      expect(result.after).toBe(result.before);
    });

    it('applies user-origin CSS when cssOrigin is "user"', async () => {
      // An !important declaration from the user origin wins over an !important
      // declaration from the author origin, but an author-origin one does not.
      const result = await w.webContents.executeJavaScript(`(() => {
        const authorKey = webFrame.insertCSS('#important { color: rgb(0, 0, 255) !important; }', { cssOrigin: 'author' });
        const withAuthor = colorOf('important');
        webFrame.removeInsertedCSS(authorKey);
        webFrame.insertCSS('#important { color: rgb(0, 0, 255) !important; }', { cssOrigin: 'user' });
        return { withAuthor, withUser: colorOf('important') };
      })()`);
      expect(result.withAuthor).toBe('rgb(255, 0, 0)');
      expect(result.withUser).toBe('rgb(0, 0, 255)');
    });
  });

  describe('setVisualZoomLevelLimits()', () => {
    it('sets the range that the page scale is clamped to', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false, nodeIntegration: true } });
      defer(() => w.close());
      await w.loadURL('about:blank');
      w.webContents.debugger.attach();
      const pageScaleAfterPinchTo = async (scale: number) => {
        await w.webContents.debugger.sendCommand('Emulation.setPageScaleFactor', { pageScaleFactor: scale });
        return w.webContents.executeJavaScript('window.visualViewport.scale');
      };

      expect(await pageScaleAfterPinchTo(2)).toBe(1);
      await w.webContents.executeJavaScript("require('electron').webFrame.setVisualZoomLevelLimits(1, 3)");
      expect(await pageScaleAfterPinchTo(2)).toBe(2);
      expect(await pageScaleAfterPinchTo(5)).toBe(3);
    });
  });

  describe('clearCache()', () => {
    it('frees cached resources that are no longer in use', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false, nodeIntegration: true } });
      defer(() => w.close());
      await w.loadFile(path.join(fixtures, 'pages', 'blank.html'));
      const result = await w.webContents.executeJavaScript(`(async () => {
        const { webFrame } = require('electron');
        const img = new Image();
        img.src = ${JSON.stringify(pathToFileURL(path.join(fixtures, 'assets', 'capybara.png')).href)};
        await img.decode();
        img.src = '';
        const before = webFrame.getResourceUsage().images;
        webFrame.clearCache();
        const after = webFrame.getResourceUsage().images;
        return { before, after };
      })()`);
      // The image stays in Blink's memory cache after it is no longer used,
      // until clearCache() evicts it.
      expect(result.before.count).toBeGreaterThan(0);
      expect(result.after.count).toBeLessThan(result.before.count);
      expect(result.after.size).toBeLessThan(result.before.size);
    });
  });

  describe('executeJavaScriptInIsolatedWorld() with invalid code', () => {
    it('rejects and passes the error to the callback when a script has no string code', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false, nodeIntegration: true } });
      defer(() => w.close());
      await w.loadURL('about:blank');
      const result = await w.webContents.executeJavaScript(`new Promise(resolve => {
        let callbackResult, callbackError;
        require('electron').webFrame
          .executeJavaScriptInIsolatedWorld(999, [{ code: 42 }], (result, error) => {
            callbackResult = result;
            callbackError = error;
          })
          .then(
            () => resolve({ rejected: false }),
            (error) => resolve({
              rejected: true,
              message: error.message,
              callbackResult,
              callbackErrorMessage: callbackError && callbackError.message
            })
          );
      })`);
      expect(result).toEqual({
        rejected: true,
        message: "Invalid 'code'",
        callbackResult: undefined,
        callbackErrorMessage: "Invalid 'code'"
      });
    });
  });
});
