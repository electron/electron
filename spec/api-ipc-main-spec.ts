import { ipcMain, BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as path from 'node:path';

import { defer } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

describe('ipc main module', () => {
  const fixtures = path.join(__dirname, 'fixtures');

  afterEach(closeAllWindows);

  describe('ipc.sendSync', () => {
    afterEach(() => { ipcMain.removeAllListeners('send-sync-message'); });

    it('does not crash when reply is not sent and browser is destroyed', (done) => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      ipcMain.once('send-sync-message', (event) => {
        event.returnValue = null;
        done();
      });
      w.loadFile(path.join(fixtures, 'api', 'send-sync-message.html'));
    });

    it('does not crash when reply is sent by multiple listeners', (done) => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      ipcMain.on('send-sync-message', (event) => {
        event.returnValue = null;
      });
      ipcMain.on('send-sync-message', (event) => {
        event.returnValue = null;
        done();
      });
      w.loadFile(path.join(fixtures, 'api', 'send-sync-message.html'));
    });
  });

  describe('ipcMain.on', () => {
    it('is not used for internals', async () => {
      const appPath = path.join(fixtures, 'api', 'ipc-main-listeners');
      const electronPath = process.execPath;
      const appProcess = cp.spawn(electronPath, [appPath]);

      let output = '';
      appProcess.stdout.on('data', (data) => { output += data; });

      await once(appProcess.stdout, 'end');

      output = JSON.parse(output);
      expect(output).to.deep.equal(['error']);
    });

    it('can be replied to', async () => {
      ipcMain.on('test-echo', (e, arg) => {
        e.reply('test-echo', arg);
      });
      defer(() => {
        ipcMain.removeAllListeners('test-echo');
      });

      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      w.loadURL('about:blank');
      const v = await w.webContents.executeJavaScript(`new Promise((resolve, reject) => {
        const { ipcRenderer } = require('electron')
        ipcRenderer.send('test-echo', 'hello')
        ipcRenderer.on('test-echo', (e, v) => {
          resolve(v)
        })
      })`);
      expect(v).to.equal('hello');
    });
  });
});
