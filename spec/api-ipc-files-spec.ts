import { expect } from 'chai';
import { BrowserWindow } from 'electron/main';
import { closeWindow } from './window-helpers';

describe('files module', () => {
  let w: BrowserWindow;
  before(async () => {
    w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
    await w.loadURL('about:blank');
  });
  after(async () => {
    await closeWindow(w);
    w = null as unknown as BrowserWindow;
  });

  describe('getPath()', () => {
    it('throws error when not given a File', async () => {
      await expect(w.webContents.executeJavaScript(`{
        const { files } = require('electron')
        files.getPath('abc')
      }`)).to.eventually.be.rejected();
    });
  });
});
