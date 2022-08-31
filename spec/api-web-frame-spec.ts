import { expect } from 'chai';
import * as path from 'path';
import { BrowserWindow, ipcMain, WebContents } from 'electron/main';
import { emittedOnce } from './events-helpers';
import { defer } from './spec-helpers';

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
    const isSafe = emittedOnce(ipcMain, 'executejs-safe');
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
    const execError = emittedOnce(ipcMain, 'executejs-safe');
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
      await win.loadURL('about:blank');
      w = win.webContents;
      await w.executeJavaScript('webFrame = require(\'electron\').webFrame; null');
    });
    it('top is self for top frame', async () => {
      const equal = await w.executeJavaScript('webFrame.top.context === webFrame.context');
      expect(equal).to.be.true();
    });

    it('opener is null for top frame', async () => {
      const equal = await w.executeJavaScript('webFrame.opener === null');
      expect(equal).to.be.true();
    });

    it('firstChild is null for top frame', async () => {
      const equal = await w.executeJavaScript('webFrame.firstChild === null');
      expect(equal).to.be.true();
    });

    it('getFrameForSelector() does not crash when not found', async () => {
      const equal = await w.executeJavaScript('webFrame.getFrameForSelector(\'unexist-selector\') === null');
      expect(equal).to.be.true();
    });

    it('findFrameByName() does not crash when not found', async () => {
      const equal = await w.executeJavaScript('webFrame.findFrameByName(\'unexist-name\') === null');
      expect(equal).to.be.true();
    });

    it('findFrameByRoutingId() does not crash when not found', async () => {
      const equal = await w.executeJavaScript('webFrame.findFrameByRoutingId(-1) === null');
      expect(equal).to.be.true();
    });

    describe('executeJavaScript', () => {
      before(() => {
        w.executeJavaScript(`
          childFrameElement = document.createElement('iframe');
          document.body.appendChild(childFrameElement);
          childFrame = webFrame.firstChild;
          null
        `);
      });

      after(() => {
        w.executeJavaScript(`
          childFrameElement.remove();
          null
        `);
      });

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
