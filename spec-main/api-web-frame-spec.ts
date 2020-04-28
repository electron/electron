import { expect } from 'chai';
import * as path from 'path';
import { BrowserWindow, ipcMain } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import { emittedOnce } from './events-helpers';

describe('webFrame module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures');

  afterEach(closeAllWindows);

  it('calls a spellcheck provider', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true
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

  it('enable text zoom in webview', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        webviewTag: true
      }
    });
    const initalTextZoomEventPromise = emittedOnce(ipcMain, 'initalTextZoomFactor');
    w.loadFile(path.join(fixtures, 'pages', 'webview-text-zoom-factor.html'));

    const [, initalTextZoom] = await initalTextZoomEventPromise;
    expect(initalTextZoom).to.equal(2);
  });
});
