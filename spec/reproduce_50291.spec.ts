import { BrowserWindow, ipcMain } from 'electron/main';
import { expect } from 'chai';
import { closeAllWindows } from './lib/window-helpers';

describe('ipc SharedArrayBuffer reproduction', () => {
  afterEach(closeAllWindows);

  it('fails to send SharedArrayBuffer from renderer to main', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        contextIsolation: false
      }
    });
    await w.loadURL('about:blank');

    const promise = new Promise<void>((resolve, reject) => {
      ipcMain.once('sab-test', (event, arg) => {
        if (arg instanceof SharedArrayBuffer) {
          resolve();
        } else {
          reject(new Error('Received argument is not a SharedArrayBuffer'));
        }
      });
      ipcMain.once('sab-error', (event, error) => {
        reject(new Error(error));
      });
    });

    await w.webContents.executeJavaScript(`
      try {
        const sab = new SharedArrayBuffer(1024);
        require('electron').ipcRenderer.send('sab-test', sab);
      } catch (e) {
        require('electron').ipcRenderer.send('sab-error', e.message);
      }
    `);

    try {
      await promise;
    } catch (e) {
      expect((e as Error).message).to.equal('An object could not be cloned.');
    } finally {
      ipcMain.removeAllListeners('sab-test');
      ipcMain.removeAllListeners('sab-error');
    }
  });
});
