import { expect } from 'chai';
import * as path from 'path';
import * as url from 'url';
import { Worker } from 'worker_threads';
import { BrowserWindow, ipcMain } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import { emittedOnce } from './events-helpers';

describe('asar package', () => {
  const fixtures = path.join(__dirname, '..', 'spec', 'fixtures');
  const asarDir = path.join(fixtures, 'test.asar');

  afterEach(closeAllWindows);

  describe('asar protocol', () => {
    it('sets __dirname correctly', async function () {
      after(function () {
        ipcMain.removeAllListeners('dirname');
      });

      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      const p = path.resolve(asarDir, 'web.asar', 'index.html');
      const dirnameEvent = emittedOnce(ipcMain, 'dirname');
      w.loadFile(p);
      const [, dirname] = await dirnameEvent;
      expect(dirname).to.equal(path.dirname(p));
    });

    it('loads script tag in html', async function () {
      after(function () {
        ipcMain.removeAllListeners('ping');
      });

      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      const p = path.resolve(asarDir, 'script.asar', 'index.html');
      const ping = emittedOnce(ipcMain, 'ping');
      w.loadFile(p);
      const [, message] = await ping;
      expect(message).to.equal('pong');
    });

    it('loads video tag in html', async function () {
      this.timeout(60000);

      after(function () {
        ipcMain.removeAllListeners('asar-video');
      });

      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      const p = path.resolve(asarDir, 'video.asar', 'index.html');
      w.loadFile(p);
      const [, message, error] = await emittedOnce(ipcMain, 'asar-video');
      if (message === 'ended') {
        expect(error).to.be.null();
      } else if (message === 'error') {
        throw new Error(error);
      }
    });
  });

  describe('worker', () => {
    it('Worker can load asar file', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixtures, 'workers', 'load_worker.html'));

      const workerUrl = url.format({
        pathname: path.resolve(fixtures, 'workers', 'workers.asar', 'worker.js').replace(/\\/g, '/'),
        protocol: 'file',
        slashes: true
      });
      const result = await w.webContents.executeJavaScript(`loadWorker('${workerUrl}')`);
      expect(result).to.equal('success');
    });

    it('SharedWorker can load asar file', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixtures, 'workers', 'load_shared_worker.html'));

      const workerUrl = url.format({
        pathname: path.resolve(fixtures, 'workers', 'workers.asar', 'shared_worker.js').replace(/\\/g, '/'),
        protocol: 'file',
        slashes: true
      });
      const result = await w.webContents.executeJavaScript(`loadSharedWorker('${workerUrl}')`);
      expect(result).to.equal('success');
    });
  });

  describe('worker threads', function () {
    it('should start worker thread from asar file', function (callback) {
      const p = path.join(asarDir, 'worker_threads.asar', 'worker.js');
      const w = new Worker(p);

      w.on('error', (err) => callback(err));
      w.on('message', (message) => {
        expect(message).to.equal('ping');
        w.terminate();

        callback(null);
      });
    });
  });
});
