import { expect } from 'chai';
import * as path from 'path';
import { BrowserWindow, ipcMain } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import { emittedOnce } from './events-helpers';
import { executeJavaScriptInPreloadContext } from './spec-helpers';

describe('webFrame module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures');

  afterEach(closeAllWindows);

  it('can use executeJavaScript', async () => {
    const w = new BrowserWindow({
      show: true,
      webPreferences: {
        nodeIntegration: true,
        contextIsolation: true,
        preload: path.join(fixtures, 'pages', 'world-safe-preload.js')
      }
    });
    const isSafe = emittedOnce(ipcMain, 'executejs-safe');
    w.loadURL('about:blank');
    const [, wasSafe] = await isSafe;
    expect(wasSafe).to.equal(true);
  });

  it('can use executeJavaScript and catch conversion errors', async () => {
    const w = new BrowserWindow({
      show: true,
      webPreferences: {
        nodeIntegration: true,
        contextIsolation: true,
        preload: path.join(fixtures, 'pages', 'world-safe-preload-error.js')
      }
    });
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

  describe('WebFrame.windowFeatures', () => {
    let w: Electron.BrowserWindow;
    const windowOptions = {
      show: false,
      webPreferences: {
        sandbox: true,
        nodeIntegration: true,
        contextIsolation: true,
        preload: path.join(fixtures, 'pages', 'test-preload.js')
      }
    };

    beforeEach(async () => {
      w = new BrowserWindow(windowOptions);
      w.webContents.setWindowOpenHandler(() => {
        return {
          action: 'allow',
          overrideBrowserWindowOptions: windowOptions
        };
      });
      await w.loadURL('about:blank');
    });

    it('returns blank for primary window', async () => {
      const windowFeatures = await executeJavaScriptInPreloadContext(w.webContents, 'requireForTest("electron").webFrame.windowFeatures');
      expect(windowFeatures).to.equal('');
    });

    it('returns feature list for child window', async () => {
      const customFeatures = 'left=20,custom=true';
      const windowPromise = emittedOnce(w.webContents, 'did-create-window');
      await w.webContents.executeJavaScript(`window.open("about:blank", null, "${customFeatures}"); void 0;`, true);
      const [childWindow] = await windowPromise;
      const windowFeatures = await executeJavaScriptInPreloadContext(childWindow.webContents, 'requireForTest("electron").webFrame.windowFeatures');
      expect(windowFeatures).to.equal(customFeatures);
    });
  });
});
