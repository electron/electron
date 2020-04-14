import { BrowserWindow } from 'electron';
import * as path from 'path';
import { delay } from './spec-helpers';
import { expect } from 'chai';

const fixturesPath = path.resolve(__dirname, '..', 'spec', 'fixtures');

describe('autofill', () => {
  let w: BrowserWindow = null as unknown as BrowserWindow;

  beforeEach(async () => {
    w = new BrowserWindow({
      show: true,
      webPreferences: {
        nodeIntegration: true
      }
    });

    await w.loadFile(path.join(fixturesPath, 'pages', 'datalist.html'));
  });

  afterEach(() => {
    w.destroy();
    w = null as unknown as BrowserWindow;
  });

  it('can be selected via keyboard', async () => {
    w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Tab' });
    await delay(100);
    const inputText = 'clap';
    for (const keyCode of inputText) {
      w.webContents.sendInputEvent({ type: 'char', keyCode });
      await delay(100);
    }

    w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Down' });
    w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Enter' });

    const value = await w.webContents.executeJavaScript("document.querySelector('input').value");
    expect(value).to.equal('Eric Clapton');
  });
});
