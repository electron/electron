import { BrowserWindow } from 'electron';
import * as path from 'node:path';
import { expect } from 'chai';
import { closeAllWindows } from './lib/window-helpers';
import { setTimeout } from 'node:timers/promises';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('autofill', () => {
  afterEach(closeAllWindows);

  it('can be selected via keyboard for a <datalist> with text type', async () => {
    const w = new BrowserWindow({ show: true });
    await w.loadFile(path.join(fixturesPath, 'pages', 'datalist-text.html'));
    w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Tab' });

    const inputText = 'clap';
    for (const keyCode of inputText) {
      w.webContents.sendInputEvent({ type: 'char', keyCode });
      await setTimeout(100);
    }

    w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Down' });
    w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Enter' });

    const value = await w.webContents.executeJavaScript("document.querySelector('input').value");
    expect(value).to.equal('Eric Clapton');
  });

  it('can be selected via keyboard for a <datalist> with time type', async () => {
    const w = new BrowserWindow({ show: true });
    await w.loadFile(path.join(fixturesPath, 'pages', 'datalist-time.html'));

    const inputText = '11P'; // 1:01 PM
    for (const keyCode of inputText) {
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Tab' });
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode });
      w.webContents.sendInputEvent({ type: 'char', keyCode });
      await setTimeout(100);
    }

    w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Tab' });

    const value = await w.webContents.executeJavaScript("document.querySelector('input').value");
    expect(value).to.equal('13:01');
  });
});
