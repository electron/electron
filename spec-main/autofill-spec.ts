import { BrowserWindow } from 'electron';
import * as path from 'path';
import { delay } from './spec-helpers';
import { expect } from 'chai';
import { closeAllWindows } from './window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('autofill', () => {
  afterEach(closeAllWindows);

  it('can be selected via keyboard', async () => {
    const w = new BrowserWindow({ show: true });
    await w.loadFile(path.join(fixturesPath, 'pages', 'datalist.html'));
    w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Tab' });
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
