import { BrowserWindow, ipcMain, WebContents } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as path from 'node:path';

import { defer } from './lib/spec-helpers';

describe('webFrame module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');

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
    expect(wasSafe).to.equal(true);
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
    expect(error).to.not.equal(null, 'Error should not be null');
    expect(error).to.have.property('message', 'Uncaught Error: An object could not be cloned.');
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

    const spellCheckerFeedback =
      new Promise<[string[], boolean]>(resolve => {
        ipcMain.on('spec-spell-check', (e, words, callbackDefined) => {
          if (words.length === 5) {
            // The API calls the provider after every completed word.
            // The promise is resolved only after this event is received with all words.
            resolve([words, callbackDefined]);
          }
        });
      });
    const inputText = 'spleling test you\'re ';
    for (const keyCode of inputText) {
      w.webContents.sendInputEvent({ type: 'char', keyCode });
    }
    const [words, callbackDefined] = await spellCheckerFeedback;
    expect(words.sort()).to.deep.equal(['spleling', 'test', 'you\'re', 'you', 're'].sort());
    expect(callbackDefined).to.be.true();
  });

  describe('api', () => {
    let w: WebContents;
    before(async () => {
      const win = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false, nodeIntegration: true } });
      await win.loadURL('data:text/html,<iframe name="test"></iframe>');
      w = win.webContents;
      await w.executeJavaScript(`
        var { webFrame } = require('electron');
        var isSameWebFrame = (a, b) => a.context === b.context;
        childFrame = webFrame.firstChild;
        null
      `);
    });

    describe('top', () => {
      it('is self for top frame', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(webFrame.top, webFrame)');
        expect(equal).to.be.true();
      });

      it('is self for child frame', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(childFrame.top, webFrame)');
        expect(equal).to.be.true();
      });
    });

    describe('opener', () => {
      it('is null for top frame', async () => {
        const equal = await w.executeJavaScript('webFrame.opener === null');
        expect(equal).to.be.true();
      });
    });

    describe('parent', () => {
      it('is null for top frame', async () => {
        const equal = await w.executeJavaScript('webFrame.parent === null');
        expect(equal).to.be.true();
      });

      it('is top frame for child frame', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(childFrame.parent, webFrame)');
        expect(equal).to.be.true();
      });
    });

    describe('firstChild', () => {
      it('is child frame for top frame', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(webFrame.firstChild, childFrame)');
        expect(equal).to.be.true();
      });

      it('is null for child frame', async () => {
        const equal = await w.executeJavaScript('childFrame.firstChild === null');
        expect(equal).to.be.true();
      });
    });

    describe('getFrameForSelector()', () => {
      it('does not crash when not found', async () => {
        const equal = await w.executeJavaScript('webFrame.getFrameForSelector("unexist-selector") === null');
        expect(equal).to.be.true();
      });

      it('returns the webFrame when found', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(webFrame.getFrameForSelector("iframe"), childFrame)');
        expect(equal).to.be.true();
      });
    });

    describe('findFrameByName()', () => {
      it('does not crash when not found', async () => {
        const equal = await w.executeJavaScript('webFrame.findFrameByName("unexist-name") === null');
        expect(equal).to.be.true();
      });

      it('returns the webFrame when found', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(webFrame.findFrameByName("test"), childFrame)');
        expect(equal).to.be.true();
      });
    });

    describe('findFrameByRoutingId()', () => {
      it('does not crash when not found', async () => {
        const equal = await w.executeJavaScript('webFrame.findFrameByRoutingId(-1) === null');
        expect(equal).to.be.true();
      });

      it('returns the webFrame when found', async () => {
        const equal = await w.executeJavaScript('isSameWebFrame(webFrame.findFrameByRoutingId(childFrame.routingId), childFrame)');
        expect(equal).to.be.true();
      });
    });

    describe('setZoomFactor()', () => {
      it('works', async () => {
        const equal = await w.executeJavaScript('childFrame.setZoomFactor(2.0); childFrame.getZoomFactor() === 2.0');
        expect(equal).to.be.true();
      });
    });

    describe('setZoomLevel()', () => {
      it('works', async () => {
        const equal = await w.executeJavaScript('childFrame.setZoomLevel(5); childFrame.getZoomLevel() === 5');
        expect(equal).to.be.true();
      });
    });

    describe('getResourceUsage()', () => {
      it('works', async () => {
        const result = await w.executeJavaScript('childFrame.getResourceUsage()');
        expect(result).to.have.property('images').that.is.an('object');
        expect(result).to.have.property('scripts').that.is.an('object');
        expect(result).to.have.property('cssStyleSheets').that.is.an('object');
        expect(result).to.have.property('xslStyleSheets').that.is.an('object');
        expect(result).to.have.property('fonts').that.is.an('object');
        expect(result).to.have.property('other').that.is.an('object');
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

        expect(callbackResult).to.equal(2);
        expect(callbackError).to.be.undefined();
        expect(result).to.equal(2);
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

        expect(callbackResult).to.equal(2);
        expect(callbackError).to.be.undefined();
        expect(result).to.equal(2);
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

        expect(callbackResult).to.be.undefined();
        expect(callbackError).to.be.an('error');
        expect(error).to.be.an('error');
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

        expect(callbackResult).to.be.undefined();
        expect(callbackError).to.be.an('error');
        expect(error).to.be.an('error');
      });

      it('executeJavaScript(InIsolatedWorld) can be used without a callback', async () => {
        expect(await w.executeJavaScript('webFrame.executeJavaScript(\'1 + 1\')')).to.equal(2);
        expect(await w.executeJavaScript('webFrame.executeJavaScriptInIsolatedWorld(999, [{code: \'1 + 1\'}])')).to.equal(2);
      });
    });
  });
});
