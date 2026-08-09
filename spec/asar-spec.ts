import { BrowserWindow, ipcMain } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as importedFs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';
import * as url from 'node:url';
import { Worker } from 'node:worker_threads';

import { getRemoteContext, ifdescribe, ifit, itremote, useRemoteContext } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const features = process._linkedBinding('electron_common_features');

describe('asar package', () => {
  const fixtures = path.join(__dirname, 'fixtures');
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
      const dirnameEvent = once(ipcMain, 'dirname');
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
      const ping = once(ipcMain, 'ping');
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
      const [, message, error] = await once(ipcMain, 'asar-video');
      if (message === 'ended') {
        expect(error).to.be.null();
      } else if (message === 'error') {
        throw new Error(error);
      }
    });
  });

  describe('downloads', () => {
    const fileUrl = (p: string) => url.pathToFileURL(p).toString();

    it('downloads a packed file through webContents.downloadURL()', async () => {
      const w = new BrowserWindow({ show: false });
      const src = path.join(asarDir, 'a.asar', 'ping.js');
      const savePath = path.join(importedFs.mkdtempSync(path.join(os.tmpdir(), 'asar-dl-')), 'saved.js');
      const willDownload = once(w.webContents.session, 'will-download');
      w.webContents.downloadURL(fileUrl(src));
      const [, item] = (await willDownload) as [unknown, Electron.DownloadItem];
      item.savePath = savePath;
      const [, state] = await once(item, 'done');
      expect(state).to.equal('completed');
      expect(item.getFilename()).to.equal('ping.js');
      expect(importedFs.readFileSync(savePath, 'utf8')).to.equal(importedFs.readFileSync(src, 'utf8'));
    });

    ifit(features.isPDFViewerEnabled())('saves a packed PDF from the PDF viewer', async () => {
      const w = new BrowserWindow({ show: false });
      const src = path.join(asarDir, 'pdf.asar', 'cat.pdf');
      const savePath = path.join(importedFs.mkdtempSync(path.join(os.tmpdir(), 'asar-pdf-')), 'saved.pdf');
      const willDownload = once(w.webContents.session, 'will-download');
      await w.loadURL(fileUrl(src));
      // Click the viewer's download button once its plugin is up; an
      // unedited document is saved through the browser as a download.
      const clickSave = `new Promise((resolve) => { const tick = () => {
        const button = document.querySelector('#viewer')?.shadowRoot?.querySelector('#toolbar')
          ?.shadowRoot?.querySelector('#downloads')?.shadowRoot?.querySelector('#save');
        if (button) { button.click(); resolve(true); } else { setTimeout(tick, 100); } }; tick(); })`;
      const viewerFrame = () =>
        w.webContents.mainFrame.framesInSubtree.find((f) => f.url.startsWith('chrome-extension://'));
      const deadline = Date.now() + 20000;
      let downloading: unknown[] | undefined;
      while (!downloading && Date.now() < deadline) {
        const frame = viewerFrame();
        if (frame) await frame.executeJavaScript(clickSave, true).catch(() => {});
        downloading = await Promise.race([willDownload, setTimeout(500).then(() => undefined)]);
      }
      expect(downloading, 'the viewer never started a download').to.be.an('array');
      const item = downloading![1] as Electron.DownloadItem;
      item.savePath = savePath;
      const [, state] = await once(item, 'done');
      expect(state).to.equal('completed');
      expect(item.getFilename()).to.equal('cat.pdf');
      expect(
        importedFs.readFileSync(savePath).equals(importedFs.readFileSync(path.join(fixtures, 'cat.pdf')))
      ).to.equal(true);
    });
  });

  describe('worker', () => {
    it('Worker can load asar file', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixtures, 'workers', 'load_worker.html'));

      const workerUrl = url.format({
        pathname: path.resolve(fixtures, 'workers', 'workers.asar', 'worker.js').replaceAll('\\', '/'),
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
        pathname: path.resolve(fixtures, 'workers', 'workers.asar', 'shared_worker.js').replaceAll('\\', '/'),
        protocol: 'file',
        slashes: true
      });
      const result = await w.webContents.executeJavaScript(`loadSharedWorker('${workerUrl}')`);
      expect(result).to.equal('success');
    });
  });

  describe('worker threads', function () {
    // DISABLED-FIXME(#38192): only disabled for ASan.
    ifit(!process.env.IS_ASAN)('should start worker thread from asar file', function (callback) {
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

// eslint-disable-next-line @typescript-eslint/no-unused-vars
async function expectToThrowErrorWithCode(_func: Function, _code: string) {
  /* dummy for typescript */
}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
function promisify(_f: Function): any {
  /* dummy for typescript */
}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
function tempPath(): string {
  /* dummy for typescript */
  return '';
}

describe('asar package', function () {
  const fixtures = path.join(__dirname, 'fixtures');
  const asarDir = path.join(fixtures, 'test.asar');
  const fs = require('node:fs') as typeof importedFs; // dummy, to fool typescript

  useRemoteContext({
    url: url.pathToFileURL(path.join(fixtures, 'pages', 'blank.html')),
    setup: `
      async function expectToThrowErrorWithCode (func, code) {
        let error;
        try {
          await func();
        } catch (e) {
          error = e;
        }

        const chai = require('chai')
        chai.expect(error).to.have.property('code').which.equals(code);
      }

      fs = require('node:fs')
      path = require('node:path')
      fixtures = ${JSON.stringify(fixtures)}
      asarDir = ${JSON.stringify(asarDir)}

      // Returns a path to a not-yet-existing file inside a fresh temp directory.
      tempPath = () => path.join(fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'electron-asar-spec-')), 'file')

      // This is used instead of util.promisify for some tests to dodge the
      // util.promisify.custom behavior.
      promisify = (f) => {
        return (...args) => new Promise((resolve, reject) => {
          f(...args, (err, result) => {
            if (err) reject(err)
            else resolve(result)
          })
        })
      }

      null
    `
  });

  describe('node api', function () {
    itremote('supports paths specified as a Buffer', function () {
      const file = Buffer.from(path.join(asarDir, 'a.asar', 'file1'));
      expect(fs.existsSync(file)).to.be.true();
    });

    describe('fs.readFileSync', function () {
      itremote('does not leak fd', function () {
        let readCalls = 1;
        while (readCalls <= 10000) {
          fs.readFileSync(path.join(process.resourcesPath, 'default_app.asar', 'main.js'));
          readCalls++;
        }
      });

      itremote('reads a normal file', function () {
        const file1 = path.join(asarDir, 'a.asar', 'file1');
        expect(fs.readFileSync(file1).toString().trim()).to.equal('file1');
        const file2 = path.join(asarDir, 'a.asar', 'file2');
        expect(fs.readFileSync(file2).toString().trim()).to.equal('file2');
        const file3 = path.join(asarDir, 'a.asar', 'file3');
        expect(fs.readFileSync(file3).toString().trim()).to.equal('file3');
      });

      itremote('reads from a empty file', function () {
        const file = path.join(asarDir, 'empty.asar', 'file1');
        const buffer = fs.readFileSync(file);
        expect(buffer).to.be.empty();
        expect(buffer.toString()).to.equal('');
      });

      itremote('reads a linked file', function () {
        const p = path.join(asarDir, 'a.asar', 'link1');
        expect(fs.readFileSync(p).toString().trim()).to.equal('file1');
      });

      itremote('reads a file from linked directory', function () {
        const p1 = path.join(asarDir, 'a.asar', 'link2', 'file1');
        expect(fs.readFileSync(p1).toString().trim()).to.equal('file1');
        const p2 = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        expect(fs.readFileSync(p2).toString().trim()).to.equal('file1');
      });

      itremote('throws ENOENT error when can not find file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.readFileSync(p);
        }).to.throw(/ENOENT/);
      });

      itremote('passes ENOENT error to callback when can not find file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        let async = false;
        fs.readFile(p, function (error) {
          expect(async).to.be.true();
          expect(error).to.match(/ENOENT/);
        });
        async = true;
      });

      itremote('reads a normal file with unpacked files', function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        expect(fs.readFileSync(p).toString().trim()).to.equal('a');
      });

      itremote('reads a file in filesystem', function () {
        const p = path.resolve(asarDir, 'file');
        expect(fs.readFileSync(p).toString().trim()).to.equal('file');
      });
    });

    describe('archives with self-referential link entries', function () {
      // Guard against a missing/renamed fixture silently passing the ENOENT
      // assertions below: a path inside a non-existent .asar would also throw
      // ENOENT. Assert the archive file itself is present first.
      itremote('has the link-cycle fixtures on disk', function () {
        // original-fs bypasses the asar wrapper so the archive file is stat'd
        // as a plain file rather than resolved as an archive root.
        const originalFs = require('original-fs') as typeof importedFs;
        for (const name of ['cyclic-link.asar', 'cyclic-link2.asar', 'cyclic-dir-link.asar']) {
          const archive = path.join(fixtures, 'asar', name);
          expect(originalFs.statSync(archive).isFile(), `${name} fixture missing`).to.equal(true);
        }
      });

      itremote('throws instead of hanging on a self-linked file', function () {
        const p = path.join(fixtures, 'asar', 'cyclic-link.asar', 'a');
        expect(() => {
          fs.readFileSync(p);
        }).to.throw(/ENOENT/);
      });

      itremote('throws instead of hanging on a two-node link cycle', function () {
        const p = path.join(fixtures, 'asar', 'cyclic-link2.asar', 'a');
        expect(() => {
          fs.readFileSync(p);
        }).to.throw(/ENOENT/);
      });

      itremote('throws instead of hanging on a link that resolves through itself', function () {
        const p = path.join(fixtures, 'asar', 'cyclic-dir-link.asar', 'a', 'b');
        expect(() => {
          fs.readFileSync(p);
        }).to.throw(/ENOENT/);
      });

      itremote('reports the missing entry from statSync without hanging', function () {
        const p = path.join(fixtures, 'asar', 'cyclic-dir-link.asar', 'a', 'b');
        expect(() => {
          fs.statSync(p);
        }).to.throw(/ENOENT/);
      });
    });

    describe('fs.readFile', function () {
      itremote('reads a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const content = await new Promise((resolve, reject) =>
          fs.readFile(p, (err, content) => {
            if (err) return reject(err);
            resolve(content);
          })
        );
        expect(String(content).trim()).to.equal('file1');
      });

      itremote('reads from a empty file', async function () {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        const content = await new Promise((resolve, reject) =>
          fs.readFile(p, (err, content) => {
            if (err) return reject(err);
            resolve(content);
          })
        );
        expect(String(content)).to.equal('');
      });

      itremote('reads from a empty file with encoding', async function () {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        const content = await new Promise((resolve, reject) =>
          fs.readFile(p, (err, content) => {
            if (err) return reject(err);
            resolve(content);
          })
        );
        expect(String(content)).to.equal('');
      });

      itremote('reads a linked file', async function () {
        const p = path.join(asarDir, 'a.asar', 'link1');
        const content = await new Promise((resolve, reject) =>
          fs.readFile(p, (err, content) => {
            if (err) return reject(err);
            resolve(content);
          })
        );
        expect(String(content).trim()).to.equal('file1');
      });

      itremote('reads a file from linked directory', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        const content = await new Promise((resolve, reject) =>
          fs.readFile(p, (err, content) => {
            if (err) return reject(err);
            resolve(content);
          })
        );
        expect(String(content).trim()).to.equal('file1');
      });

      itremote('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.readFile(p, resolve));
        expect(err.code).to.equal('ENOENT');
      });
    });

    describe('fs.promises.readFile', function () {
      itremote('reads a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const content = await fs.promises.readFile(p);
        expect(String(content).trim()).to.equal('file1');
      });

      itremote('reads from a empty file', async function () {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        const content = await fs.promises.readFile(p);
        expect(String(content)).to.equal('');
      });

      itremote('reads from a empty file with encoding', async function () {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        const content = await fs.promises.readFile(p, 'utf8');
        expect(content).to.equal('');
      });

      itremote('reads a linked file', async function () {
        const p = path.join(asarDir, 'a.asar', 'link1');
        const content = await fs.promises.readFile(p);
        expect(String(content).trim()).to.equal('file1');
      });

      itremote('reads a file from linked directory', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        const content = await fs.promises.readFile(p);
        expect(String(content).trim()).to.equal('file1');
      });

      itremote('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.readFile(p), 'ENOENT');
      });
    });

    describe('fs.copyFile', function () {
      itremote('copies a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
        await new Promise<void>((resolve, reject) => {
          fs.copyFile(p, dest, (err) => {
            if (err) reject(err);
            else resolve();
          });
        });
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });

      itremote('copies a unpacked file', async function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const dest = tempPath();
        await new Promise<void>((resolve, reject) => {
          fs.copyFile(p, dest, (err) => {
            if (err) reject(err);
            else resolve();
          });
        });
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.promises.copyFile', function () {
      itremote('copies a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
        await fs.promises.copyFile(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });

      itremote('copies a unpacked file', async function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const dest = tempPath();
        await fs.promises.copyFile(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.copyFileSync', function () {
      itremote('copies a normal file', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });

      itremote('copies a unpacked file', function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const dest = tempPath();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.cpSync', function () {
      itremote('copies a normal file', function () {
        if (!fs.cpSync) return;
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
        fs.cpSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.cp', function () {
      itremote('copies a normal file', async function () {
        if (!fs.cp) return;
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
        await new Promise<void>((resolve, reject) => {
          fs.cp(p, dest, (err) => (err ? reject(err) : resolve()));
        });
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.promises.cp', function () {
      itremote('copies a normal file', async function () {
        if (!fs.promises.cp) return;
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
        await fs.promises.cp(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.lstatSync', function () {
      itremote('handles path with trailing slash correctly', function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        fs.lstatSync(p);
        fs.lstatSync(p + '/');
      });

      itremote('returns information of root', function () {
        const p = path.join(asarDir, 'a.asar');
        const stats = fs.lstatSync(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      itremote('returns stat properties with types matching a real file', function () {
        const asarStats = fs.lstatSync(path.join(asarDir, 'a.asar', 'file1'));
        const realStats = fs.lstatSync(path.join(fixtures, 'test.asar', 'a.asar'));
        for (const key of Object.keys(realStats) as (keyof typeof realStats)[]) {
          expect(typeof asarStats[key]).to.equal(typeof realStats[key], `typeof stats.${key}`);
        }
      });

      itremote('returns information of root with stats as bigint', function () {
        const p = path.join(asarDir, 'a.asar');
        const stats = fs.lstatSync(p, { bigint: false });
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      itremote('returns information of a normal file', function () {
        const ref2 = ['file1', 'file2', 'file3', path.join('dir1', 'file1'), path.join('link2', 'file1')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).to.be.true();
          expect(stats.isDirectory()).to.be.false();
          expect(stats.isSymbolicLink()).to.be.false();
          expect(stats.size).to.equal(6);
        }
      });

      itremote('returns information of a normal directory', function () {
        const ref2 = ['dir1', 'dir2', 'dir3'];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).to.be.false();
          expect(stats.isDirectory()).to.be.true();
          expect(stats.isSymbolicLink()).to.be.false();
          expect(stats.size).to.equal(0);
        }
      });

      itremote('returns information of a linked file', function () {
        const ref2 = ['link1', path.join('dir1', 'link1'), path.join('link2', 'link2')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).to.be.false();
          expect(stats.isDirectory()).to.be.false();
          expect(stats.isSymbolicLink()).to.be.true();
          expect(stats.size).to.equal(0);
        }
      });

      itremote('returns information of a linked directory', function () {
        const ref2 = ['link2', path.join('dir1', 'link2'), path.join('link2', 'link2')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).to.be.false();
          expect(stats.isDirectory()).to.be.false();
          expect(stats.isSymbolicLink()).to.be.true();
          expect(stats.size).to.equal(0);
        }
      });

      itremote('throws ENOENT error when can not find file', function () {
        const ref2 = ['file4', 'file5', path.join('dir1', 'file4')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          expect(() => {
            fs.lstatSync(p);
          }).to.throw(/ENOENT/);
        }
      });

      itremote('returns null when can not find file with throwIfNoEntry === false', function () {
        const ref2 = ['file4', 'file5', path.join('dir1', 'file4')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          expect(fs.lstatSync(p, { throwIfNoEntry: false })).to.equal(null);
        }
      });
    });

    describe('fs.lstat', function () {
      itremote('handles path with trailing slash correctly', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        await promisify(fs.lstat)(p + '/');
      });

      itremote('returns information of root', async function () {
        const p = path.join(asarDir, 'a.asar');
        const stats = await promisify(fs.lstat)(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      itremote('returns information of root with stats as bigint', async function () {
        const p = path.join(asarDir, 'a.asar');
        const stats = await promisify(fs.lstat)(p, { bigint: false });
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      itremote('returns information of a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'file1');
        const stats = await promisify(fs.lstat)(p);
        expect(stats.isFile()).to.be.true();
        expect(stats.isDirectory()).to.be.false();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(6);
      });

      itremote('returns information of a normal directory', async function () {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const stats = await promisify(fs.lstat)(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      itremote('returns information of a linked file', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link1');
        const stats = await promisify(fs.lstat)(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.false();
        expect(stats.isSymbolicLink()).to.be.true();
        expect(stats.size).to.equal(0);
      });

      itremote('returns information of a linked directory', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const stats = await promisify(fs.lstat)(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.false();
        expect(stats.isSymbolicLink()).to.be.true();
        expect(stats.size).to.equal(0);
      });

      itremote('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file4');
        const err = await new Promise<any>((resolve) => fs.lstat(p, resolve));
        expect(err.code).to.equal('ENOENT');
      });
    });

    describe('fs.promises.lstat', function () {
      itremote('handles path with trailing slash correctly', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        await fs.promises.lstat(p + '/');
      });

      itremote('returns information of root', async function () {
        const p = path.join(asarDir, 'a.asar');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      itremote('returns information of root with stats as bigint', async function () {
        const p = path.join(asarDir, 'a.asar');
        const stats = await fs.promises.lstat(p, { bigint: false });
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      itremote('returns information of a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'file1');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).to.be.true();
        expect(stats.isDirectory()).to.be.false();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(6);
      });

      itremote('returns information of a normal directory', async function () {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      itremote('returns information of a linked file', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link1');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.false();
        expect(stats.isSymbolicLink()).to.be.true();
        expect(stats.size).to.equal(0);
      });

      itremote('returns information of a linked directory', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.false();
        expect(stats.isSymbolicLink()).to.be.true();
        expect(stats.size).to.equal(0);
      });

      itremote('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file4');
        await expectToThrowErrorWithCode(() => fs.promises.lstat(p), 'ENOENT');
      });
    });

    describe('fs.realpathSync', () => {
      itremote('returns real path root', () => {
        const parent = fs.realpathSync(asarDir);
        const p = 'a.asar';
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a normal file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a normal directory', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a linked file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'file1'));
      });

      itremote('returns real path of a linked directory', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'dir1'));
      });

      itremote('returns real path of an unpacked file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('throws ENOENT error when can not find file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'not-exist');
        expect(() => {
          fs.realpathSync(path.join(parent, p));
        }).to.throw(/ENOENT/);
      });
    });

    describe('fs.realpathSync.native', () => {
      itremote('returns real path root', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = 'a.asar';
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a normal file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a normal directory', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a linked file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'file1'));
      });

      itremote('returns real path of a linked directory', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'dir1'));
      });

      itremote('returns real path of an unpacked file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('throws ENOENT error when can not find file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'not-exist');
        expect(() => {
          fs.realpathSync.native(path.join(parent, p));
        }).to.throw(/ENOENT/);
      });
    });

    describe('fs.realpath', () => {
      itremote('returns real path root', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = 'a.asar';
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a normal file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a normal directory', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a linked file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'file1'));
      });

      itremote('returns real path of a linked directory', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'dir1'));
      });

      itremote('returns real path of an unpacked file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.realpath(path.join(parent, p), resolve));
        expect(err.code).to.equal('ENOENT');
      });
    });

    describe('fs.promises.realpath', () => {
      itremote('returns real path root', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = 'a.asar';
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a normal file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a normal directory', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a linked file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'file1'));
      });

      itremote('returns real path of a linked directory', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'dir1'));
      });

      itremote('returns real path of an unpacked file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.realpath(path.join(parent, p)), 'ENOENT');
      });
    });

    describe('fs.realpath.native', () => {
      itremote('returns real path root', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = 'a.asar';
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a normal file', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a normal directory', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('returns real path of a linked file', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'file1'));
      });

      itremote('returns real path of a linked directory', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'dir1'));
      });

      itremote('returns real path of an unpacked file', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.realpath.native(path.join(parent, p), resolve));
        expect(err.code).to.equal('ENOENT');
      });
    });

    describe('fs.readdirSync', function () {
      itremote('reads dirs from root', function () {
        const p = path.join(asarDir, 'a.asar');
        const dirs = fs.readdirSync(p);
        expect(dirs).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('supports recursive readdirSync withFileTypes', () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = fs.readdirSync(dir, { recursive: true, withFileTypes: true });

        expect(files).to.have.length(24);

        for (const file of files) {
          expect(file).to.be.an.instanceOf(fs.Dirent);
        }

        const paths = files.map((a: any) => a.name);
        expect(paths).to.have.members([
          'a.asar',
          'nested',
          'test.txt',
          'dir1',
          'dir2',
          'dir3',
          'file1',
          'file2',
          'file3',
          'link1',
          'link2',
          'ping.js',
          'hello.txt',
          'file1',
          'file2',
          'file3',
          'link1',
          'link2',
          'file1',
          'file2',
          'file3',
          'file1',
          'file2',
          'file3'
        ]);
      });

      itremote('supports recursive readdirSync', () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = fs.readdirSync(dir, { recursive: true });
        expect(files).to.have.members([
          'a.asar',
          'nested',
          'test.txt',
          path.join('a.asar', 'dir1'),
          path.join('a.asar', 'dir2'),
          path.join('a.asar', 'dir3'),
          path.join('a.asar', 'file1'),
          path.join('a.asar', 'file2'),
          path.join('a.asar', 'file3'),
          path.join('a.asar', 'link1'),
          path.join('a.asar', 'link2'),
          path.join('a.asar', 'ping.js'),
          path.join('nested', 'hello.txt'),
          path.join('a.asar', 'dir1', 'file1'),
          path.join('a.asar', 'dir1', 'file2'),
          path.join('a.asar', 'dir1', 'file3'),
          path.join('a.asar', 'dir1', 'link1'),
          path.join('a.asar', 'dir1', 'link2'),
          path.join('a.asar', 'dir2', 'file1'),
          path.join('a.asar', 'dir2', 'file2'),
          path.join('a.asar', 'dir2', 'file3'),
          path.join('a.asar', 'dir3', 'file1'),
          path.join('a.asar', 'dir3', 'file2'),
          path.join('a.asar', 'dir3', 'file3')
        ]);
      });

      itremote('reads dirs from a normal dir', function () {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const dirs = fs.readdirSync(p);
        expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('supports withFileTypes', function () {
        const p = path.join(asarDir, 'a.asar');
        const dirs = fs.readdirSync(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir).to.be.an.instanceof(fs.Dirent);
          expect(dir.parentPath).to.equal(p);
        }
        const names = dirs.map((a) => a.name);
        expect(names).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('supports withFileTypes for a deep directory', function () {
        const p = path.join(asarDir, 'a.asar', 'dir3');
        const dirs = fs.readdirSync(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir).to.be.an.instanceof(fs.Dirent);
        }
        const names = dirs.map((a) => a.name);
        expect(names).to.deep.equal(['file1', 'file2', 'file3']);
      });

      itremote('reads dirs from a linked dir', function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const dirs = fs.readdirSync(p);
        expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('throws ENOENT error when can not find file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.readdirSync(p);
        }).to.throw(/ENOENT/);
      });
    });

    describe('fs.readdir', function () {
      itremote('reads dirs from root', async () => {
        const p = path.join(asarDir, 'a.asar');
        const dirs = await promisify(fs.readdir)(p);
        expect(dirs).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('supports recursive readdirSync', async () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = await promisify(fs.readdir)(dir, { recursive: true });
        expect(files).to.have.members([
          'a.asar',
          'nested',
          'test.txt',
          path.join('a.asar', 'dir1'),
          path.join('a.asar', 'dir2'),
          path.join('a.asar', 'dir3'),
          path.join('a.asar', 'file1'),
          path.join('a.asar', 'file2'),
          path.join('a.asar', 'file3'),
          path.join('a.asar', 'link1'),
          path.join('a.asar', 'link2'),
          path.join('a.asar', 'ping.js'),
          path.join('nested', 'hello.txt'),
          path.join('a.asar', 'dir1', 'file1'),
          path.join('a.asar', 'dir1', 'file2'),
          path.join('a.asar', 'dir1', 'file3'),
          path.join('a.asar', 'dir1', 'link1'),
          path.join('a.asar', 'dir1', 'link2'),
          path.join('a.asar', 'dir2', 'file1'),
          path.join('a.asar', 'dir2', 'file2'),
          path.join('a.asar', 'dir2', 'file3'),
          path.join('a.asar', 'dir3', 'file1'),
          path.join('a.asar', 'dir3', 'file2'),
          path.join('a.asar', 'dir3', 'file3')
        ]);
      });

      itremote('supports readdir withFileTypes', async () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = await promisify(fs.readdir)(dir, { recursive: true, withFileTypes: true });

        expect(files).to.have.length(24);

        for (const file of files) {
          expect(file).to.be.an.instanceOf(fs.Dirent);
        }

        const paths = files.map((a: any) => a.name);
        expect(paths).to.have.members([
          'a.asar',
          'nested',
          'test.txt',
          'dir1',
          'dir2',
          'dir3',
          'file1',
          'file2',
          'file3',
          'link1',
          'link2',
          'ping.js',
          'hello.txt',
          'file1',
          'file2',
          'file3',
          'link1',
          'link2',
          'file1',
          'file2',
          'file3',
          'file1',
          'file2',
          'file3'
        ]);
      });

      itremote('supports withFileTypes', async () => {
        const p = path.join(asarDir, 'a.asar');

        const dirs = await promisify(fs.readdir)(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir).to.be.an.instanceof(fs.Dirent);
          expect(dir.parentPath).to.equal(p);
        }

        const names = dirs.map((a: any) => a.name);
        expect(names).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('reads dirs from a normal dir', async () => {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const dirs = await promisify(fs.readdir)(p);
        expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('reads dirs from a linked dir', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const dirs = await promisify(fs.readdir)(p);
        expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.readdir(p, resolve));
        expect(err.code).to.equal('ENOENT');
      });

      it('handles null for options', function (done) {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        fs.readdir(p, null, function (err, dirs) {
          try {
            expect(err).to.be.null();
            expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('handles undefined for options', function (done) {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        fs.readdir(p, undefined, function (err, dirs) {
          try {
            expect(err).to.be.null();
            expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.promises.readdir', function () {
      itremote('reads dirs from root', async function () {
        const p = path.join(asarDir, 'a.asar');
        const dirs = await fs.promises.readdir(p);
        expect(dirs).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('supports recursive readdir', async () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = await fs.promises.readdir(dir, { recursive: true });
        expect(files).to.have.members([
          'a.asar',
          'nested',
          'test.txt',
          path.join('a.asar', 'dir1'),
          path.join('a.asar', 'dir2'),
          path.join('a.asar', 'dir3'),
          path.join('a.asar', 'file1'),
          path.join('a.asar', 'file2'),
          path.join('a.asar', 'file3'),
          path.join('a.asar', 'link1'),
          path.join('a.asar', 'link2'),
          path.join('a.asar', 'ping.js'),
          path.join('nested', 'hello.txt'),
          path.join('a.asar', 'dir1', 'file1'),
          path.join('a.asar', 'dir1', 'file2'),
          path.join('a.asar', 'dir1', 'file3'),
          path.join('a.asar', 'dir1', 'link1'),
          path.join('a.asar', 'dir1', 'link2'),
          path.join('a.asar', 'dir2', 'file1'),
          path.join('a.asar', 'dir2', 'file2'),
          path.join('a.asar', 'dir2', 'file3'),
          path.join('a.asar', 'dir3', 'file1'),
          path.join('a.asar', 'dir3', 'file2'),
          path.join('a.asar', 'dir3', 'file3')
        ]);
      });

      itremote('supports readdir withFileTypes', async () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = await fs.promises.readdir(dir, { recursive: true, withFileTypes: true });

        expect(files).to.have.length(24);

        for (const file of files) {
          expect(file).to.be.an.instanceOf(fs.Dirent);
        }

        const paths = files.map((a: any) => a.name);
        expect(paths).to.have.members([
          'a.asar',
          'nested',
          'test.txt',
          'dir1',
          'dir2',
          'dir3',
          'file1',
          'file2',
          'file3',
          'link1',
          'link2',
          'ping.js',
          'hello.txt',
          'file1',
          'file2',
          'file3',
          'link1',
          'link2',
          'file1',
          'file2',
          'file3',
          'file1',
          'file2',
          'file3'
        ]);
      });

      itremote('supports withFileTypes', async function () {
        const p = path.join(asarDir, 'a.asar');
        const dirs = await fs.promises.readdir(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir).to.be.an.instanceof(fs.Dirent);
          expect(dir.parentPath).to.equal(p);
        }
        const names = dirs.map((a) => a.name);
        expect(names).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('reads dirs from a normal dir', async function () {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const dirs = await fs.promises.readdir(p);
        expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('reads dirs from a linked dir', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const dirs = await fs.promises.readdir(p);
        expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.readdir(p), 'ENOENT');
      });
    });

    describe('fs.globSync', function () {
      itremote('supports withFileTypes with a cwd inside an asar archive', function () {
        const cwd = path.join(asarDir, 'a.asar');
        const dirents = fs.globSync('*.js', { cwd, withFileTypes: true });
        expect(dirents).to.have.lengthOf(1);
        expect(dirents[0]).to.be.an.instanceof(fs.Dirent);
        expect(dirents[0].name).to.equal('ping.js');
        expect(dirents[0].parentPath).to.equal(cwd);
      });
    });

    describe('fs.glob', function () {
      itremote('supports withFileTypes with a cwd inside an asar archive', async function () {
        const cwd = path.join(asarDir, 'a.asar');
        const dirents = await promisify(fs.glob)('*.js', { cwd, withFileTypes: true });
        expect(dirents).to.have.lengthOf(1);
        expect(dirents[0]).to.be.an.instanceof(fs.Dirent);
        expect(dirents[0].name).to.equal('ping.js');
        expect(dirents[0].parentPath).to.equal(cwd);
      });
    });

    describe('fs.openSync', function () {
      itremote('opens a normal/linked/under-linked-directory file', function () {
        const ref2 = ['file1', 'link1', path.join('link2', 'file1')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const fd = fs.openSync(p, 'r');
          const buffer = Buffer.alloc(6);
          fs.readSync(fd, buffer, 0, 6, 0);
          expect(String(buffer).trim()).to.equal('file1');
          fs.closeSync(fd);
        }
      });

      itremote('throws ENOENT error when can not find file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          (fs.openSync as any)(p);
        }).to.throw(/ENOENT/);
      });
    });

    describe('fs.open', function () {
      itremote('opens a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = await promisify(fs.open)(p, 'r');
        const buffer = Buffer.alloc(6);
        await promisify(fs.read)(fd, buffer, 0, 6, 0);
        expect(String(buffer).trim()).to.equal('file1');
        await promisify(fs.close)(fd);
      });

      itremote('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.open(p, 'r', resolve));
        expect(err.code).to.equal('ENOENT');
      });
    });

    describe('fs.promises.open', function () {
      itremote('opens a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fh = await fs.promises.open(p, 'r');
        const buffer = Buffer.alloc(6);
        await fh.read(buffer, 0, 6, 0);
        expect(String(buffer).trim()).to.equal('file1');
        await fh.close();
      });

      itremote('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.open(p, 'r'), 'ENOENT');
      });
    });

    // The tests below cover the fs APIs that read packed archive entries
    // through file descriptors, streams, handles and copies (i.e. without
    // extracting them to a temporary file first), plus the directory and
    // symlink APIs.  Where a corresponding Node.js test exists
    // (test/parallel/test-fs-*), its expectations are mirrored here.

    describe('fs.openSync (fd semantics)', function () {
      itremote('returns a usable, closable file descriptor', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        expect(fd).to.be.a('number').that.is.greaterThan(2);
        fs.closeSync(fd);
        expect(() => fs.fstatSync(fd)).to.throw(/EBADF/);
        expect(() => fs.closeSync(fd)).to.throw(/EBADF/);
      });

      itremote('accepts default, string and numeric read-only flags', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        for (const flags of [
          undefined,
          'r',
          'rs',
          fs.constants.O_RDONLY,
          fs.constants.O_RDONLY | fs.constants.O_CREAT
        ]) {
          const fd = flags === undefined ? fs.openSync(p, 'r') : fs.openSync(p, flags as any);
          const buffer = Buffer.alloc(6);
          expect(fs.readSync(fd, buffer, 0, 6, 0)).to.equal(6);
          expect(String(buffer).trim()).to.equal('file1');
          fs.closeSync(fd);
        }
      });

      itremote('accepts a mode argument', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r', 0o644);
        fs.closeSync(fd);
      });

      itremote('refuses to open a packed file for writing', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const writeFlags = [
          'w',
          'w+',
          'wx',
          'a',
          'a+',
          'ax',
          'r+',
          'rs+',
          fs.constants.O_WRONLY,
          fs.constants.O_RDWR,
          fs.constants.O_RDONLY | fs.constants.O_TRUNC,
          fs.constants.O_RDONLY | fs.constants.O_APPEND
        ];
        for (const flags of writeFlags) {
          expect(() => fs.openSync(p, flags as any), String(flags)).to.throw(/EACCES/);
        }
        // Nothing was written or corrupted.
        expect(fs.readFileSync(p).toString().trim()).to.equal('file1');
      });

      itremote('throws EEXIST for O_CREAT | O_EXCL on an existing packed file', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => fs.openSync(p, fs.constants.O_RDONLY | fs.constants.O_CREAT | fs.constants.O_EXCL)).to.throw(
          /EEXIST/
        );
      });

      itremote('throws EISDIR when opening a directory', function () {
        expect(() => fs.openSync(path.join(asarDir, 'a.asar', 'dir1'), 'r')).to.throw(/EISDIR/);
        expect(() => fs.openSync(path.join(asarDir, 'a.asar'), 'r')).to.throw(/EISDIR/);
        // ...including through a symbolic link to a directory.
        expect(() => fs.openSync(path.join(asarDir, 'a.asar', 'link2'), 'r')).to.throw(/EISDIR/);
        expect(() => fs.openSync(path.join(asarDir, 'a.asar', 'link2', 'link2'), 'r')).to.throw(/EISDIR/);
      });

      itremote('opens files with an empty size', function () {
        const fd = fs.openSync(path.join(asarDir, 'empty.asar', 'file1'), 'r');
        expect(fs.fstatSync(fd).size).to.equal(0);
        expect(fs.readSync(fd, Buffer.alloc(8), 0, 8, 0)).to.equal(0);
        expect(fs.readFileSync(fd).length).to.equal(0);
        fs.closeSync(fd);
      });

      itremote('opens unpacked files as regular files', function () {
        const originalFs = require('node:original-fs');
        const fd = fs.openSync(path.join(asarDir, 'unpack.asar', 'a.txt'), 'r');
        const stats = fs.fstatSync(fd);
        const realStats = originalFs.statSync(path.join(asarDir, 'unpack.asar.unpacked', 'a.txt'));
        expect(stats.ino).to.equal(realStats.ino);
        expect(stats.size).to.equal(realStats.size);
        fs.closeSync(fd);
      });

      itremote('hands out a descriptor that is not usable outside of fs', function () {
        // The number identifies the entry to fs only; it must never expose the
        // archive's bytes to code that reads the raw descriptor, and that code
        // should fail loudly rather than get data from the wrong offset.
        const originalFs = require('node:original-fs');
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        const buffer = Buffer.alloc(16);
        expect(() => originalFs.readSync(fd, buffer, 0, 16, 0)).to.throw(/EBADF|EPERM|EACCES/);
        expect(() => originalFs.readSync(fd, buffer, 0, 16, null)).to.throw(/EBADF|EPERM|EACCES/);
        expect(originalFs.fstatSync(fd).isFile()).to.be.false();
        expect(originalFs.fstatSync(fd).size).to.not.equal(fs.fstatSync(fd).size);
        expect(buffer.equals(Buffer.alloc(16))).to.be.true();
        // ...while fs itself still serves the entry.
        expect(fs.readSync(fd, buffer, 0, 6, 0)).to.equal(6);
        expect(buffer.subarray(0, 6).toString()).to.equal('file1\n');
        fs.closeSync(fd);
      });

      itremote('does not leak file descriptors', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const first = fs.openSync(p, 'r');
        fs.closeSync(first);
        for (let i = 0; i < 2000; i++) {
          const fd = fs.openSync(p, 'r');
          fs.closeSync(fd);
        }
        const last = fs.openSync(p, 'r');
        fs.closeSync(last);
        // With no leak the descriptor number is reused.
        expect(last).to.equal(first);
      });

      itremote('gives every open its own file position', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd1 = fs.openSync(p, 'r');
        const fd2 = fs.openSync(p, 'r');
        const b1 = Buffer.alloc(3);
        const b2 = Buffer.alloc(3);
        fs.readSync(fd1, b1, 0, 3, null);
        fs.readSync(fd2, b2, 0, 3, null);
        expect(b1.toString()).to.equal('fil');
        expect(b2.toString()).to.equal('fil');
        fs.readSync(fd1, b1, 0, 3, null);
        expect(b1.toString()).to.equal('e1\n');
        fs.closeSync(fd1);
        fs.closeSync(fd2);
      });
    });

    describe('fs.open (callback forms)', function () {
      itremote('supports (path, cb), (path, flags, cb) and (path, flags, mode, cb)', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fds = [
          await new Promise<number>((resolve, reject) => fs.open(p, (e, fd) => (e ? reject(e) : resolve(fd)))),
          await new Promise<number>((resolve, reject) => fs.open(p, 'r', (e, fd) => (e ? reject(e) : resolve(fd)))),
          await new Promise<number>((resolve, reject) =>
            fs.open(p, 'r', 0o666, (e, fd) => (e ? reject(e) : resolve(fd)))
          )
        ];
        for (const fd of fds) {
          const buffer = Buffer.alloc(6);
          await promisify(fs.read)(fd, buffer, 0, 6, 0);
          expect(String(buffer).trim()).to.equal('file1');
          await promisify(fs.close)(fd);
        }
      });

      itremote('reports EACCES for write flags asynchronously', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const err = await new Promise<any>((resolve) => fs.open(p, 'w', resolve));
        expect(err.code).to.equal('EACCES');
      });

      itremote('reports EISDIR for directories asynchronously', async function () {
        const err = await new Promise<any>((resolve) => fs.open(path.join(asarDir, 'a.asar', 'dir1'), 'r', resolve));
        expect(err.code).to.equal('EISDIR');
      });
    });

    describe('fs.readSync / fs.read on packed files', function () {
      // Mirrors test-fs-read.js.
      itremote('reads into Buffers and Uint8Arrays with an explicit position', async function () {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        const expected = Buffer.from('file1\n');
        for (const make of [() => Buffer.allocUnsafe(expected.length), () => new Uint8Array(expected.length)]) {
          const bufferSync = make();
          expect(fs.readSync(fd, bufferSync, 0, expected.length, 0)).to.equal(expected.length);
          expect(Buffer.from(bufferSync).equals(expected)).to.be.true();
          const bufferAsync = make();
          const bytesRead = await new Promise<number>((resolve, reject) =>
            fs.read(fd, bufferAsync, 0, expected.length, 0, (e, n) => (e ? reject(e) : resolve(n)))
          );
          expect(bytesRead).to.equal(expected.length);
          expect(Buffer.from(bufferAsync).equals(expected)).to.be.true();
        }
        fs.closeSync(fd);
      });

      itremote('returns 0 bytes when reading beyond the end of the file', async function () {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        expect(fs.readSync(fd, Buffer.alloc(1), 0, 1, 6)).to.equal(0);
        expect(fs.readSync(fd, Buffer.alloc(1), 0, 1, 0xffffffff + 1)).to.equal(0);
        const n = await new Promise<number>((resolve, reject) =>
          fs.read(fd, Buffer.alloc(1), 0, 1, 0xffffffff + 1, (e, n) => (e ? reject(e) : resolve(n)))
        );
        expect(n).to.equal(0);
        fs.closeSync(fd);
      });

      itremote('clamps reads that run past the end of the entry', function () {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        const buffer = Buffer.alloc(100);
        expect(fs.readSync(fd, buffer, 0, 100, 4)).to.equal(2);
        expect(buffer.subarray(0, 2).toString()).to.equal('1\n');
        // Untouched bytes stay untouched.
        expect(buffer[2]).to.equal(0);
        fs.closeSync(fd);
      });

      itremote('honours a null position by using and advancing the file position', function () {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        const chunk = Buffer.alloc(2);
        const parts: string[] = [];
        let n;
        while ((n = fs.readSync(fd, chunk, 0, 2, null)) > 0) parts.push(chunk.subarray(0, n).toString());
        expect(parts.join('')).to.equal('file1\n');
        // An explicit position does not disturb the file position.
        const fd2 = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        const b = Buffer.alloc(3);
        fs.readSync(fd2, b, 0, 3, null);
        fs.readSync(fd2, b, 0, 3, 0);
        expect(b.toString()).to.equal('fil');
        fs.readSync(fd2, b, 0, 3, null);
        expect(b.toString()).to.equal('e1\n');
        fs.closeSync(fd);
        fs.closeSync(fd2);
      });

      itremote('accepts a bigint position', function () {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        const buffer = Buffer.alloc(2);
        expect(fs.readSync(fd, buffer, 0, 2, 4n as any)).to.equal(2);
        expect(buffer.toString()).to.equal('1\n');
        fs.closeSync(fd);
      });

      // Mirrors test-fs-read-optional-params.js and test-fs-readSync-optional-params.js.
      itremote('supports the options-object forms', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const expected = Buffer.from('file1\n');
        {
          const fd = fs.openSync(p, 'r');
          const buffer = Buffer.alloc(expected.length);
          expect(fs.readSync(fd, buffer, { offset: 0, length: expected.length, position: 0 })).to.equal(
            expected.length
          );
          expect(buffer.equals(expected)).to.be.true();
          // The explicit position above did not move the file position.
          buffer.fill(0);
          expect(fs.readSync(fd, buffer, {})).to.equal(expected.length);
          expect(buffer.equals(expected)).to.be.true();
          expect(fs.readSync(fd, buffer, {})).to.equal(0);
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          const buffer = Buffer.alloc(expected.length);
          // cursor read via options without position
          expect(fs.readSync(fd, buffer, { offset: 0, length: expected.length })).to.equal(expected.length);
          expect(buffer.equals(expected)).to.be.true();
          fs.closeSync(fd);
        }
        for (const options of [undefined, null, {}, { offset: 0, position: 0 }]) {
          const fd = fs.openSync(p, 'r');
          const { bytesRead, buffer } = await new Promise<{ bytesRead: number; buffer: Buffer }>((resolve, reject) => {
            const cb = (e: any, bytesRead: number, buffer: Buffer) => (e ? reject(e) : resolve({ bytesRead, buffer }));
            if (options === undefined) fs.read(fd, cb);
            else fs.read(fd, options as any, cb);
          });
          expect(bytesRead).to.equal(expected.length);
          expect(buffer.subarray(0, bytesRead).equals(expected)).to.be.true();
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          const buffer = Buffer.alloc(expected.length);
          const bytesRead = await new Promise<number>((resolve, reject) =>
            fs.read(fd, buffer, { position: 0 }, (e, n) => (e ? reject(e) : resolve(n)))
          );
          expect(bytesRead).to.equal(expected.length);
          expect(buffer.equals(expected)).to.be.true();
          fs.closeSync(fd);
        }
      });

      // Mirrors test-fs-read-zero-length.js and test-fs-read-empty-buffer.js.
      itremote('handles zero-length reads and rejects empty buffers like Node', async function () {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        expect(fs.readSync(fd, Buffer.alloc(4), 0, 0, 0)).to.equal(0);
        const n = await new Promise<number>((resolve, reject) =>
          fs.read(fd, Buffer.alloc(4), 0, 0, 0, (e, n) => (e ? reject(e) : resolve(n)))
        );
        expect(n).to.equal(0);
        expect(() => fs.readSync(fd, Buffer.alloc(0), 0, 1, 0))
          .to.throw()
          .with.property('code')
          .that.is.oneOf(['ERR_INVALID_ARG_VALUE', 'ERR_OUT_OF_RANGE']);
        fs.closeSync(fd);
      });

      itremote('validates arguments like Node', function () {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        expect(() => (fs.read as any)(fd, Buffer.alloc(1), 0, 1, 0))
          .to.throw()
          .with.property('code', 'ERR_INVALID_ARG_TYPE');
        expect(() => fs.readSync(fd, Buffer.alloc(1), 0, 2, 0))
          .to.throw()
          .with.property('code', 'ERR_OUT_OF_RANGE');
        expect(() => fs.readSync(fd, Buffer.alloc(1), 0, 1, -2))
          .to.throw()
          .with.property('code', 'ERR_OUT_OF_RANGE');
        fs.closeSync(fd);
      });

      itremote('reads a larger packed file back byte-for-byte in odd sized chunks', function () {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const expected = fs.readFileSync(p);
        expect(expected.length).to.be.greaterThan(100000);
        const fd = fs.openSync(p, 'r');
        const out = Buffer.alloc(expected.length);
        let read = 0;
        while (read < expected.length) {
          const n = fs.readSync(fd, out, read, Math.min(4097, expected.length - read), null);
          if (n === 0) break;
          read += n;
        }
        fs.closeSync(fd);
        expect(read).to.equal(expected.length);
        expect(out.equals(expected)).to.be.true();
      });
    });

    // Mirrors test-fs-readv-sync.js / test-fs-readv.js / test-fs-readv-promises.js.
    describe('fs.readvSync / fs.readv / FileHandle.readv on packed files', function () {
      itremote('reads into an array of buffers with and without a position', async function () {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const expected = fs.readFileSync(p);
        const allocate = () => [
          Buffer.alloc(Math.floor(expected.length / 2)),
          Buffer.alloc(Math.ceil(expected.length / 2))
        ];
        {
          const fd = fs.openSync(p, 'r');
          expect(fs.readvSync(fd, [Buffer.from('')], 0)).to.equal(0);
          const buffers = allocate();
          expect(fs.readvSync(fd, buffers, 0)).to.equal(expected.length);
          expect(Buffer.concat(buffers).equals(expected)).to.be.true();
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          expect(fs.readvSync(fd, [Buffer.from('')])).to.equal(0);
          const buffers = allocate();
          expect(fs.readvSync(fd, buffers)).to.equal(expected.length);
          expect(Buffer.concat(buffers).equals(expected)).to.be.true();
          expect(fs.readvSync(fd, allocate())).to.equal(0);
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          const buffers = allocate();
          const bytesRead = await new Promise<number>((resolve, reject) =>
            fs.readv(fd, buffers, 0, (e, n) => (e ? reject(e) : resolve(n)))
          );
          expect(bytesRead).to.equal(expected.length);
          expect(Buffer.concat(buffers).equals(expected)).to.be.true();
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          const buffers = allocate();
          const bytesRead = await new Promise<number>((resolve, reject) =>
            fs.readv(fd, buffers, (e, n) => (e ? reject(e) : resolve(n)))
          );
          expect(bytesRead).to.equal(expected.length);
          expect(Buffer.concat(buffers).equals(expected)).to.be.true();
          fs.closeSync(fd);
        }
        {
          const handle = await fs.promises.open(p, 'r');
          const buffers = allocate();
          const { bytesRead } = await handle.readv(buffers, 0);
          expect(bytesRead).to.equal(expected.length);
          expect(Buffer.concat(buffers).equals(expected)).to.be.true();
          await handle.close();
        }
      });

      itremote('skips zero-length buffers and clamps at the end of the entry like preadv', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1'); // 6 bytes: 'file1\n'
        {
          // Zero-length buffers do not stop the read, whether or not the tail is clamped.
          const fd = fs.openSync(p, 'r');
          const buffers = [Buffer.alloc(0), Buffer.alloc(3), Buffer.alloc(0), Buffer.alloc(3)];
          expect(fs.readvSync(fd, buffers, 0)).to.equal(6);
          expect(Buffer.concat(buffers).toString()).to.equal('file1\n');
          const clamped = [Buffer.alloc(0), Buffer.alloc(20)];
          expect(fs.readvSync(fd, clamped, 2)).to.equal(4);
          expect(clamped[1].subarray(0, 4).toString()).to.equal('le1\n');
          expect(clamped[1][4]).to.equal(0);
          const clampedMore = [Buffer.alloc(2), Buffer.alloc(0), Buffer.alloc(2), Buffer.alloc(100)];
          expect(fs.readvSync(fd, clampedMore, 3)).to.equal(3);
          expect(Buffer.concat(clampedMore).subarray(0, 3).toString()).to.equal('e1\n');
          fs.closeSync(fd);
        }
        {
          // Same through the file position and the async / promise forms.
          const fd = fs.openSync(p, 'r');
          const first = [Buffer.alloc(0), Buffer.alloc(4)];
          expect(await promisify(fs.readv)(fd, first)).to.equal(4);
          expect(first[1].toString()).to.equal('file');
          const rest = [Buffer.alloc(0), Buffer.alloc(10)];
          expect(await promisify(fs.readv)(fd, rest)).to.equal(2);
          expect(rest[1].subarray(0, 2).toString()).to.equal('1\n');
          expect(await promisify(fs.readv)(fd, [Buffer.alloc(0), Buffer.alloc(10)])).to.equal(0);
          fs.closeSync(fd);
          const handle = await fs.promises.open(p, 'r');
          const buffers = [Buffer.alloc(0), Buffer.alloc(3), Buffer.alloc(0), Buffer.alloc(30)];
          const { bytesRead } = await handle.readv(buffers, 1);
          expect(bytesRead).to.equal(5);
          expect(Buffer.concat(buffers).subarray(0, 5).toString()).to.equal('ile1\n');
          await handle.close();
        }
      });

      itremote('rejects invalid buffer arguments like Node', function () {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        for (const wrong of [false, 'test', {}, [{}], ['sdf'], null, undefined]) {
          expect(() => (fs.readvSync as any)(fd, wrong, null))
            .to.throw()
            .with.property('code', 'ERR_INVALID_ARG_TYPE');
        }
        fs.closeSync(fd);
      });
    });

    describe('fs.fstat on packed files', function () {
      itremote('describes the entry', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        const stats = fs.fstatSync(fd);
        expect(stats.isFile()).to.be.true();
        expect(stats.isDirectory()).to.be.false();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(6);
        expect(stats.mtime).to.be.an.instanceOf(Date);
        const bigint = fs.fstatSync(fd, { bigint: true });
        expect(bigint.size).to.equal(6n);
        expect(typeof bigint.mtimeMs).to.equal('bigint');
        const async = await promisify(fs.fstat)(fd);
        expect(async.size).to.equal(6);
        const asyncBig = await new Promise<any>((resolve, reject) =>
          fs.fstat(fd, { bigint: true }, (e, s) => (e ? reject(e) : resolve(s)))
        );
        expect(asyncBig.size).to.equal(6n);
        // Stable across calls on the same descriptor.
        expect(fs.fstatSync(fd).ino).to.equal(stats.ino);
        fs.closeSync(fd);
      });

      itremote('reports executable entries as executable', function () {
        const fd = fs.openSync(path.join(asarDir, 'echo.asar', 'echo'), 'r');
        const stats = fs.fstatSync(fd);
        expect(stats.mode & 0o111).to.not.equal(0);
        fs.closeSync(fd);
        const fd2 = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        expect(fs.fstatSync(fd2).mode & 0o111).to.equal(0);
        fs.closeSync(fd2);
      });

      itremote('reports conventional permission bits for stat/lstat', function () {
        const a = path.join(asarDir, 'a.asar');
        expect(fs.statSync(path.join(a, 'file1')).mode & 0o777).to.equal(0o644);
        expect(fs.statSync(path.join(asarDir, 'echo.asar', 'echo')).mode & 0o777).to.equal(0o755);
        expect(fs.lstatSync(path.join(asarDir, 'echo.asar', 'echo')).mode & 0o777).to.equal(0o755);
        expect(fs.statSync(path.join(a, 'dir1')).mode & 0o777).to.equal(0o755);
        expect(fs.statSync(a).mode & 0o777).to.equal(0o755);
        expect(fs.lstatSync(path.join(a, 'link1')).mode & 0o777).to.equal(0o777);
      });
    });

    // Mirrors test-fs-readfile-fd.js.
    describe('fs.readFile / fs.readFileSync with a packed file descriptor', function () {
      itremote('reads from the current file position', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        {
          const fd = fs.openSync(p, 'r');
          expect(fs.readFileSync(fd).toString()).to.equal('file1\n');
          expect(fs.readFileSync(fd).toString()).to.equal('');
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          fs.readSync(fd, Buffer.alloc(2), 0, 2, null);
          expect(fs.readFileSync(fd, 'utf8')).to.equal('le1\n');
          expect(fs.readFileSync(fd, 'utf8')).to.equal('');
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          fs.readSync(fd, Buffer.alloc(3), 0, 3, null);
          expect((await promisify(fs.readFile)(fd)).toString()).to.equal('e1\n');
          expect(await promisify(fs.readFile)(fd, 'utf8')).to.equal('');
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          expect(await promisify(fs.readFile)(fd, { encoding: 'utf8' })).to.equal('file1\n');
          fs.closeSync(fd);
        }
      });

      itremote('reads an empty file through a descriptor', async function () {
        const fd = fs.openSync(path.join(asarDir, 'empty.asar', 'file1'), 'r');
        expect(fs.readFileSync(fd, 'utf8')).to.equal('');
        expect((await promisify(fs.readFile)(fd)).length).to.equal(0);
        fs.closeSync(fd);
      });
    });

    describe('fs.close / fs.closeSync of packed file descriptors', function () {
      itremote('closes and rejects further use', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        await promisify(fs.close)(fd);
        expect(() => fs.readSync(fd, Buffer.alloc(1), 0, 1, 0)).to.throw(/EBADF/);
        const err = await new Promise<any>((resolve) => fs.close(fd, resolve));
        expect(err.code).to.equal('EBADF');
      });
    });

    describe('fchmod / fchown / futimes on packed file descriptors', function () {
      itremote('are refused so the archive itself cannot be modified through them', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        expect(() => fs.fchmodSync(fd, 0o777)).to.throw(/EACCES/);
        expect(() => fs.futimesSync(fd, new Date(), new Date())).to.throw(/EACCES/);
        if (process.platform !== 'win32') {
          expect(() => fs.fchownSync(fd, process.getuid!(), process.getgid!())).to.throw(/EACCES/);
        }
        const err = await new Promise<any>((resolve) => fs.fchmod(fd, 0o777, resolve));
        expect(err.code).to.equal('EACCES');
        // Every mutation through the descriptor is refused.
        expect(() => fs.writeSync(fd, 'x')).to.throw(/EACCES/);
        expect(() => fs.writeSync(fd, Buffer.from('x'))).to.throw(/EACCES/);
        expect(() => fs.writevSync(fd, [Buffer.from('x')])).to.throw(/EACCES/);
        expect(() => fs.ftruncateSync(fd, 0)).to.throw(/EACCES/);
        const werr = await new Promise<any>((resolve) => fs.write(fd, 'x', resolve));
        expect(werr.code).to.equal('EACCES');
        const terr = await new Promise<any>((resolve) => fs.ftruncate(fd, 0, resolve));
        expect(terr.code).to.equal('EACCES');
        fs.closeSync(fd);
        // The archive is intact.
        expect(fs.readFileSync(p).toString().trim()).to.equal('file1');
      });
    });

    // Mirrors test-fs-read-stream.js, test-fs-read-stream-pos.js,
    // test-fs-read-stream-fd.js and test-fs-read-stream-file-handle.js.
    describe('fs.createReadStream on packed files', function () {
      itremote('streams a whole file, emitting open/ready/end/close', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const events: string[] = [];
        const content = await new Promise<Buffer>((resolve, reject) => {
          const chunks: Buffer[] = [];
          const stream = fs.createReadStream(p);
          stream.on('open', (fd) => {
            events.push('open');
            expect(fd).to.be.a('number');
          });
          stream.on('ready', () => events.push('ready'));
          stream.on('data', (chunk) => chunks.push(chunk as Buffer));
          stream.on('error', reject);
          stream.on('end', () => events.push('end'));
          stream.on('close', () => {
            events.push('close');
            expect(stream.bytesRead).to.equal(6);
            resolve(Buffer.concat(chunks));
          });
        });
        expect(content.toString()).to.equal('file1\n');
        expect(events).to.deep.equal(['open', 'ready', 'end', 'close']);
      });

      itremote('honours encoding, highWaterMark, start and end', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const read = (options: any) =>
          new Promise<any[]>((resolve, reject) => {
            const chunks: any[] = [];
            fs.createReadStream(p, options)
              .on('data', (c) => chunks.push(c))
              .on('error', reject)
              .on('end', () => resolve(chunks));
          });
        expect((await read({ encoding: 'utf8' })).join('')).to.equal('file1\n');
        const small = await read({ highWaterMark: 2 });
        expect(small.length).to.equal(3);
        expect(Buffer.concat(small).toString()).to.equal('file1\n');
        expect(Buffer.concat(await read({ start: 1, end: 3 })).toString()).to.equal('ile');
        expect(Buffer.concat(await read({ start: 4 })).toString()).to.equal('1\n');
        expect(Buffer.concat(await read({ end: 0 })).toString()).to.equal('f');
        expect(Buffer.concat(await read({ start: 5, end: 100 })).toString()).to.equal('\n');
        expect(Buffer.concat(await read({ start: 6 })).toString()).to.equal('');
      });

      itremote('streams a larger file identically to readFileSync', async function () {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const expected = fs.readFileSync(p);
        for (const highWaterMark of [1024, 16 * 1024, 1024 * 1024]) {
          const content = await new Promise<Buffer>((resolve, reject) => {
            const chunks: Buffer[] = [];
            fs.createReadStream(p, { highWaterMark })
              .on('data', (c) => chunks.push(c as Buffer))
              .on('error', reject)
              .on('end', () => resolve(Buffer.concat(chunks)));
          });
          expect(content.equals(expected)).to.be.true();
        }
      });

      itremote('streams linked files and files in linked directories', async function () {
        for (const file of ['link1', path.join('link2', 'file1'), path.join('link2', 'link2', 'file1')]) {
          const content = await new Promise<Buffer>((resolve, reject) => {
            const chunks: Buffer[] = [];
            fs.createReadStream(path.join(asarDir, 'a.asar', file))
              .on('data', (c) => chunks.push(c as Buffer))
              .on('error', reject)
              .on('end', () => resolve(Buffer.concat(chunks)));
          });
          expect(content.toString().trim()).to.equal('file1');
        }
      });

      itremote('streams unpacked files', async function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const content = await new Promise<Buffer>((resolve, reject) => {
          const chunks: Buffer[] = [];
          fs.createReadStream(p)
            .on('data', (c) => chunks.push(c as Buffer))
            .on('error', reject)
            .on('end', () => resolve(Buffer.concat(chunks)));
        });
        expect(content.equals(fs.readFileSync(p))).to.be.true();
      });

      itremote('emits errors for missing files and directories', async function () {
        const errorFor = (p: string) =>
          new Promise<any>((resolve) => {
            fs.createReadStream(p)
              .on('error', resolve)
              .on('data', () => {});
          });
        expect((await errorFor(path.join(asarDir, 'a.asar', 'not-exist'))).code).to.equal('ENOENT');
        expect((await errorFor(path.join(asarDir, 'a.asar', 'dir1'))).code).to.equal('EISDIR');
      });

      itremote('can be created from an already open packed fd', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        const content = await new Promise<Buffer>((resolve, reject) => {
          const chunks: Buffer[] = [];
          fs.createReadStream(null as any, { fd, autoClose: false })
            .on('data', (c) => chunks.push(c as Buffer))
            .on('error', reject)
            .on('end', () => resolve(Buffer.concat(chunks)));
        });
        expect(content.toString()).to.equal('file1\n');
        // Not closed by the stream, and the position was consumed.
        expect(fs.readSync(fd, Buffer.alloc(1), 0, 1, null)).to.equal(0);
        expect(fs.readSync(fd, Buffer.alloc(1), 0, 1, 0)).to.equal(1);
        fs.closeSync(fd);
        // With autoClose the stream closes the descriptor.
        const fd2 = fs.openSync(p, 'r');
        await new Promise<void>((resolve, reject) => {
          fs.createReadStream(null as any, { fd: fd2 })
            .on('error', reject)
            .on('data', () => {})
            .on('close', resolve);
        });
        expect(() => fs.fstatSync(fd2)).to.throw(/EBADF/);
      });

      itremote('can be created from a FileHandle', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const handle = await fs.promises.open(p, 'r');
        const content = await new Promise<Buffer>((resolve, reject) => {
          const chunks: Buffer[] = [];
          fs.createReadStream(null as any, { fd: handle })
            .on('data', (c) => chunks.push(c as Buffer))
            .on('error', reject)
            .on('end', () => resolve(Buffer.concat(chunks)));
        });
        expect(content.toString()).to.equal('file1\n');
      });

      itremote('supports many concurrent streams of the same entry', async function () {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const expected = fs.readFileSync(p);
        const streams = [];
        for (let i = 0; i < 50; i++) {
          streams.push(
            new Promise<Buffer>((resolve, reject) => {
              const chunks: Buffer[] = [];
              fs.createReadStream(p, { highWaterMark: 1000 + i })
                .on('data', (c) => chunks.push(c as Buffer))
                .on('error', reject)
                .on('end', () => resolve(Buffer.concat(chunks)));
            })
          );
        }
        for (const content of await Promise.all(streams)) expect(content.equals(expected)).to.be.true();
      });

      itremote('works with pipe()', async function () {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const os = require('node:os');
        const dest = path.join(os.tmpdir(), `asar-pipe-${process.pid}-${Date.now()}`);
        await new Promise<void>((resolve, reject) => {
          fs.createReadStream(p)
            .on('error', reject)
            .pipe(fs.createWriteStream(dest))
            .on('error', reject)
            .on('finish', resolve);
        });
        expect(fs.readFileSync(dest).equals(fs.readFileSync(p))).to.be.true();
        fs.unlinkSync(dest);
      });
    });

    describe('fs.createWriteStream on packed files', function () {
      itremote('fails with EACCES', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const err = await new Promise<any>((resolve) => {
          fs.createWriteStream(p).on('error', resolve).end('x');
        });
        expect(err.code).to.equal('EACCES');
        expect(fs.readFileSync(p).toString().trim()).to.equal('file1');
      });
    });

    describe('write APIs on packed files', function () {
      itremote('fail without touching the archive', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => fs.writeFileSync(p, 'x')).to.throw(/EACCES/);
        expect(() => fs.appendFileSync(p, 'x')).to.throw(/EACCES/);
        await expectToThrowErrorWithCode(() => fs.promises.writeFile(p, 'x'), 'EACCES');
        await expectToThrowErrorWithCode(() => fs.promises.appendFile(p, 'x'), 'EACCES');
        const err = await new Promise<any>((resolve) => fs.writeFile(p, 'x', resolve));
        expect(err.code).to.equal('EACCES');
        expect(fs.readFileSync(p).toString().trim()).to.equal('file1');
      });
    });

    // Mirrors test-fs-promises-file-handle-read.js, -readFile.js, -stat.js,
    // -readLines.mjs, -stream.js, -close.js, -dispose.js and -chmod.js.
    describe('fs.promises.open FileHandle on packed files', function () {
      itremote('exposes a numeric fd and supports all read forms', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const handle = await fs.promises.open(p, 'r');
        expect(handle.fd).to.be.a('number').that.is.greaterThan(2);

        let r = await handle.read(Buffer.alloc(3), 0, 3, 0);
        expect(r.bytesRead).to.equal(3);
        expect(r.buffer.toString()).to.equal('fil');

        r = await handle.read({ buffer: Buffer.alloc(3), offset: 0, length: 3, position: 3 });
        expect(r.bytesRead).to.equal(3);
        expect(r.buffer.toString()).to.equal('e1\n');

        r = await handle.read(Buffer.alloc(3), { offset: 0, length: 3, position: 1 });
        expect(r.buffer.toString()).to.equal('ile');

        // Default arguments read from the current position into a fresh buffer.
        r = await handle.read();
        expect(r.bytesRead).to.equal(6);
        expect(r.buffer.subarray(0, 6).toString()).to.equal('file1\n');
        r = await handle.read();
        expect(r.bytesRead).to.equal(0);

        // Null position advances; explicit position does not.
        const h2 = await fs.promises.open(p, 'r');
        expect((await h2.read(Buffer.alloc(2), 0, 2, null)).buffer.toString()).to.equal('fi');
        expect((await h2.read(Buffer.alloc(2), 0, 2, 0)).buffer.toString()).to.equal('fi');
        expect((await h2.read(Buffer.alloc(2), 0, 2, null)).buffer.toString()).to.equal('le');
        await h2.close();

        await handle.close();
      });

      itremote('supports readFile, stat and readLines', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const handle = await fs.promises.open(p, 'r');
        const stats = await handle.stat();
        expect(stats.isFile()).to.be.true();
        expect(stats.size).to.equal(6);
        expect((await handle.stat({ bigint: true })).size).to.equal(6n);
        expect((await handle.readFile()).toString()).to.equal('file1\n');
        // Like Node, readFile continues from the current position.
        expect(await handle.readFile('utf8')).to.equal('');
        await handle.close();

        const h2 = await fs.promises.open(p, 'r');
        expect(await fs.promises.readFile(h2, 'utf8')).to.equal('file1\n');
        await h2.close();

        const h3 = await fs.promises.open(path.join(asarDir, 'a.asar', 'ping.js'), 'r');
        const lines: string[] = [];
        for await (const line of h3.readLines()) lines.push(line);
        expect(lines.join('\n')).to.equal(fs.readFileSync(path.join(asarDir, 'a.asar', 'ping.js'), 'utf8').trimEnd());
      });

      itremote('supports createReadStream and readableWebStream', async function () {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const expected = fs.readFileSync(p);
        const handle = await fs.promises.open(p, 'r');
        const chunks: Buffer[] = [];
        for await (const chunk of handle.createReadStream({ start: 10, end: 1009, autoClose: false })) {
          chunks.push(chunk);
        }
        expect(Buffer.concat(chunks).equals(expected.subarray(10, 1010))).to.be.true();
        let total = 0;
        for await (const chunk of handle.readableWebStream()) total += (chunk as Uint8Array).byteLength;
        expect(total).to.equal(expected.length);
        await handle.close();
      });

      itremote('stops routing a descriptor number as soon as close() is requested', async function () {
        // FileHandle#close() runs close(2) on the threadpool; until then a real
        // file opened right after may be handed the same number. It must never
        // be served from the archive-backed reader.
        const temp = require('temp').track();
        const real = temp.path();
        fs.writeFileSync(real, 'real-file-content');
        const p = path.join(asarDir, 'a.asar', 'file1');
        for (let i = 0; i < 200; i++) {
          const handle = await fs.promises.open(p, 'r');
          const closing = handle.close();
          const fd = fs.openSync(real, 'r');
          const buffer = Buffer.alloc(17);
          expect(fs.readSync(fd, buffer, 0, 17, 0)).to.equal(17);
          expect(buffer.toString()).to.equal('real-file-content');
          expect(fs.fstatSync(fd).size).to.equal(17);
          fs.closeSync(fd);
          await closing;
        }
      });

      itremote('does not capture a descriptor number that was closed behind its back', async function () {
        // Something outside fs (a native FileHandle built on the number, a
        // handle moved to a worker, ...) can close a packed descriptor
        // without going through fs.close. Simulate that with original-fs and
        // check the next owner of the number is served natively.
        const originalFs = require('node:original-fs');
        const temp = require('temp').track();
        const real = temp.path();
        fs.writeFileSync(real, 'real-file-content');
        const p = path.join(asarDir, 'a.asar', 'file1');
        for (const variant of ['sync', 'async', 'handle']) {
          const stale = fs.openSync(p, 'r');
          originalFs.closeSync(stale);
          let fd: number;
          if (variant === 'sync') fd = fs.openSync(real, 'r');
          else if (variant === 'async') fd = await promisify(fs.open)(real, 'r');
          else fd = (await fs.promises.open(real, 'r')).fd;
          const buffer = Buffer.alloc(17);
          expect(fs.readSync(fd, buffer, 0, 17, 0)).to.equal(17);
          expect(buffer.toString()).to.equal('real-file-content');
          expect(fs.fstatSync(fd).size).to.equal(17);
          if (variant !== 'handle') fs.closeSync(fd);
          // A packed open that gets the same number again works too.
          const again = fs.openSync(p, 'r');
          expect(fs.readFileSync(again).toString()).to.equal('file1\n');
          fs.closeSync(again);
        }
      });

      itremote('close semantics match Node', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const handle = await fs.promises.open(p, 'r');
        await handle.close();
        expect(handle.fd).to.equal(-1);
        await expectToThrowErrorWithCode(() => handle.read(Buffer.alloc(1), 0, 1, 0), 'EBADF');
        // Closing twice resolves (Node returns the same promise / a resolved one).
        await handle.close();

        const disposable = await fs.promises.open(p, 'r');
        await disposable[Symbol.asyncDispose]();
        expect(disposable.fd).to.equal(-1);
      });

      itremote('refuses metadata and write operations', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const handle = await fs.promises.open(p, 'r');
        await expectToThrowErrorWithCode(() => handle.chmod(0o777), 'EACCES');
        await expectToThrowErrorWithCode(() => handle.utimes(new Date(), new Date()), 'EACCES');
        if (process.platform !== 'win32') {
          await expectToThrowErrorWithCode(() => handle.chown(process.getuid!(), process.getgid!()), 'EACCES');
        }
        await expectToThrowErrorWithCode(() => handle.write('x'), 'EACCES');
        await expectToThrowErrorWithCode(() => handle.write(Buffer.from('x')), 'EACCES');
        await expectToThrowErrorWithCode(() => handle.writev([Buffer.from('x')]), 'EACCES');
        await expectToThrowErrorWithCode(() => handle.writeFile('x'), 'EACCES');
        await expectToThrowErrorWithCode(() => handle.appendFile('x'), 'EACCES');
        await expectToThrowErrorWithCode(() => handle.truncate(0), 'EACCES');
        await handle.close();
        expect(fs.readFileSync(p).toString().trim()).to.equal('file1');
      });

      itremote('rejects write flags, missing files and directories', async function () {
        await expectToThrowErrorWithCode(() => fs.promises.open(path.join(asarDir, 'a.asar', 'file1'), 'w'), 'EACCES');
        await expectToThrowErrorWithCode(() => fs.promises.open(path.join(asarDir, 'a.asar', 'file1'), 'a+'), 'EACCES');
        await expectToThrowErrorWithCode(() => fs.promises.open(path.join(asarDir, 'a.asar', 'dir1'), 'r'), 'EISDIR');
        await expectToThrowErrorWithCode(() => fs.promises.open(path.join(asarDir, 'a.asar', 'nope'), 'r'), 'ENOENT');
      });

      itremote('opens unpacked files as regular handles', async function () {
        const originalFs = require('node:original-fs');
        const handle = await fs.promises.open(path.join(asarDir, 'unpack.asar', 'a.txt'), 'r');
        const stats = await handle.stat();
        expect(stats.ino).to.equal(originalFs.statSync(path.join(asarDir, 'unpack.asar.unpacked', 'a.txt')).ino);
        await handle.close();
      });

      itremote('lets many handles coexist and be read concurrently', async function () {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const expected = fs.readFileSync(p);
        const handles = await Promise.all(Array.from({ length: 40 }, () => fs.promises.open(p, 'r')));
        const contents = await Promise.all(handles.map((h) => h.readFile()));
        for (const c of contents) expect(c.equals(expected)).to.be.true();
        await Promise.all(handles.map((h) => h.close()));
      });
    });

    // Mirrors test-fs-copyfile.js.
    describe('fs.copyFile flags and errors', function () {
      itremote('honours COPYFILE_EXCL and overwrites otherwise', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        const dest = temp.path();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(dest).toString()).to.equal('file1\n');
        expect(() => fs.copyFileSync(p, dest, fs.constants.COPYFILE_EXCL)).to.throw(/EEXIST/);
        await expectToThrowErrorWithCode(() => fs.promises.copyFile(p, dest, fs.constants.COPYFILE_EXCL), 'EEXIST');
        const err = await new Promise<any>((resolve) => fs.copyFile(p, dest, fs.constants.COPYFILE_EXCL, resolve));
        expect(err.code).to.equal('EEXIST');
        // Overwrite a longer existing file: no trailing garbage.
        fs.writeFileSync(dest, 'a much longer file than the source');
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(dest).toString()).to.equal('file1\n');
        // FICLONE is best-effort, FICLONE_FORCE cannot be honoured.
        fs.copyFileSync(p, dest, fs.constants.COPYFILE_FICLONE);
        expect(fs.readFileSync(dest).toString()).to.equal('file1\n');
        expect(() => fs.copyFileSync(p, dest, fs.constants.COPYFILE_FICLONE_FORCE)).to.throw(/ENOTSUP/);
      });

      itremote('lets native fs decide about copy-on-write clones of unpacked files', function () {
        const originalFs = require('node:original-fs');
        const temp = require('temp').track();
        const unpacked = path.join(asarDir, 'unpack.asar', 'a.txt');
        const real = path.join(asarDir, 'unpack.asar.unpacked', 'a.txt');
        const outcome = (fn: () => void) => {
          try {
            fn();
            return 'ok';
          } catch (e: any) {
            return e.code;
          }
        };
        const viaAsar = temp.path();
        const viaOriginal = temp.path();
        // Whatever the filesystem says about reflinks, the answer for an unpacked
        // entry must match the answer for the real file behind it.
        expect(outcome(() => fs.copyFileSync(unpacked, viaAsar, fs.constants.COPYFILE_FICLONE_FORCE))).to.equal(
          outcome(() => originalFs.copyFileSync(real, viaOriginal, fs.constants.COPYFILE_FICLONE_FORCE))
        );
        if (fs.existsSync(viaAsar)) expect(fs.readFileSync(viaAsar).equals(fs.readFileSync(real))).to.be.true();
      });

      itremote('validates the mode argument like Node', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        expect(() => fs.copyFileSync(p, temp.path(), 8))
          .to.throw()
          .with.property('code', 'ERR_OUT_OF_RANGE');
        expect(() => (fs.copyFileSync as any)(p, temp.path(), 'x'))
          .to.throw()
          .with.property('code', 'ERR_INVALID_ARG_TYPE');
      });

      itremote('reports ENOENT / EISDIR for bad sources', async function () {
        const temp = require('temp').track();
        expect(() => fs.copyFileSync(path.join(asarDir, 'a.asar', 'nope'), temp.path())).to.throw(/ENOENT/);
        expect(() => fs.copyFileSync(path.join(asarDir, 'a.asar', 'dir1'), temp.path())).to.throw(/EISDIR/);
        await expectToThrowErrorWithCode(
          () => fs.promises.copyFile(path.join(asarDir, 'a.asar', 'nope'), temp.path()),
          'ENOENT'
        );
        const err = await new Promise<any>((resolve) =>
          fs.copyFile(path.join(asarDir, 'a.asar', 'dir1'), temp.path(), resolve)
        );
        expect(err.code).to.equal('EISDIR');
      });

      itremote('copies linked files, larger files and executable bits', async function () {
        const temp = require('temp').track();
        const link = path.join(asarDir, 'a.asar', 'link1');
        const d1 = temp.path();
        fs.copyFileSync(link, d1);
        expect(fs.readFileSync(d1).toString()).to.equal('file1\n');

        const big = path.join(asarDir, 'video.asar', 'video.mp4');
        const d2 = temp.path();
        await fs.promises.copyFile(big, d2);
        expect(fs.readFileSync(d2).equals(fs.readFileSync(big))).to.be.true();
        const d3 = temp.path();
        await promisify(fs.copyFile)(big, d3);
        expect(fs.readFileSync(d3).equals(fs.readFileSync(big))).to.be.true();

        if (process.platform !== 'win32') {
          const d4 = temp.path();
          fs.copyFileSync(path.join(asarDir, 'echo.asar', 'echo'), d4);
          expect(fs.statSync(d4).mode & 0o111).to.not.equal(0);
          const d5 = temp.path();
          fs.copyFileSync(path.join(asarDir, 'a.asar', 'file1'), d5);
          expect(fs.statSync(d5).mode & 0o111).to.equal(0);
          // cp chmods the destination to the source's stat mode afterwards, so
          // stat must agree with what copyFile preserved.
          const d6 = temp.path();
          fs.cpSync(path.join(asarDir, 'echo.asar', 'echo'), d6);
          expect(fs.statSync(d6).mode & 0o111).to.not.equal(0);
          const d7 = temp.path();
          await fs.promises.cp(path.join(asarDir, 'echo.asar', 'echo'), d7);
          expect(fs.statSync(d7).mode & 0o111).to.not.equal(0);
          const d8 = temp.path();
          fs.cpSync(path.join(asarDir, 'echo.asar'), d8, { recursive: true, filter: () => true });
          expect(fs.statSync(path.join(d8, 'echo')).mode & 0o111).to.not.equal(0);
        }
      });

      itremote('fails when the destination is inside an archive', async function () {
        const temp = require('temp').track();
        const src = temp.path();
        fs.writeFileSync(src, 'x');
        expect(() => fs.copyFileSync(src, path.join(asarDir, 'a.asar', 'new-file'))).to.throw(/ENOTDIR|EACCES|ENOENT/);
        expect(() =>
          fs.copyFileSync(path.join(asarDir, 'a.asar', 'file1'), path.join(asarDir, 'a.asar', 'file2'))
        ).to.throw(/ENOTDIR|EACCES|ENOENT/);
      });
    });

    // Mirrors the test-fs-cp-* family (subset applicable to read-only sources).
    describe('fs.cp / fs.cpSync / fs.promises.cp from archives', function () {
      itremote('copies a directory tree recursively (sync, callback, promises)', async function () {
        const temp = require('temp').track();
        const src = path.join(asarDir, 'a.asar');
        const expectedTop = ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js'];
        for (const variant of ['sync', 'callback', 'promises']) {
          const dest = temp.path();
          if (variant === 'sync') fs.cpSync(src, dest, { recursive: true });
          else if (variant === 'callback') await promisify(fs.cp)(src, dest, { recursive: true });
          else await fs.promises.cp(src, dest, { recursive: true });
          expect(fs.readdirSync(dest).sort(), variant).to.deep.equal(expectedTop);
          for (const dir of ['dir1', 'dir2', 'dir3']) {
            expect(fs.statSync(path.join(dest, dir)).isDirectory(), variant).to.be.true();
            for (const file of ['file1', 'file2', 'file3']) {
              expect(
                fs.readFileSync(path.join(dest, dir, file)).equals(fs.readFileSync(path.join(src, dir, file))),
                `${variant} ${dir}/${file}`
              ).to.be.true();
            }
          }
          expect(
            fs.readFileSync(path.join(dest, 'ping.js')).equals(fs.readFileSync(path.join(src, 'ping.js')))
          ).to.be.true();
          // Like Node's cp of a real tree, symlinks are preserved and (without
          // verbatimSymlinks) made absolute, i.e. they point back into the archive.
          expect(fs.lstatSync(path.join(dest, 'link1')).isSymbolicLink(), variant).to.be.true();
          expect(fs.readlinkSync(path.join(dest, 'link1')), variant).to.equal(path.join(src, 'file1'));
          expect(fs.lstatSync(path.join(dest, 'link2')).isSymbolicLink(), variant).to.be.true();
          expect(fs.readlinkSync(path.join(dest, 'link2')), variant).to.equal(path.join(src, 'dir1'));
          expect(fs.lstatSync(path.join(dest, 'dir1', 'link2')).isSymbolicLink(), variant).to.be.true();
          expect(fs.readlinkSync(path.join(dest, 'dir1', 'link2')), variant).to.equal(path.join(src, 'dir1'));
        }
      });

      itremote('copies symlinks verbatim when asked, producing a self-contained tree', async function () {
        const temp = require('temp').track();
        const src = path.join(asarDir, 'a.asar');
        for (const variant of ['sync', 'promises']) {
          const dest = temp.path();
          if (variant === 'sync') fs.cpSync(src, dest, { recursive: true, verbatimSymlinks: true });
          else await fs.promises.cp(src, dest, { recursive: true, verbatimSymlinks: true });
          expect(fs.readlinkSync(path.join(dest, 'link1')), variant).to.equal('file1');
          expect(fs.readlinkSync(path.join(dest, 'link2')), variant).to.equal('dir1');
          expect(fs.readlinkSync(path.join(dest, 'dir1', 'link1')), variant).to.equal(path.join('..', 'file1'));
          expect(fs.readlinkSync(path.join(dest, 'dir1', 'link2')), variant).to.equal('.');
          expect(fs.readFileSync(path.join(dest, 'dir1', 'link1')).toString(), variant).to.equal('file1\n');
          // ...which resolve on the real filesystem.
          expect(fs.readFileSync(path.join(dest, 'link1')).toString(), variant).to.equal('file1\n');
          expect(fs.readdirSync(path.join(dest, 'link2')).sort(), variant).to.deep.equal(
            fs.readdirSync(path.join(src, 'dir1')).sort()
          );
          expect(fs.readFileSync(path.join(dest, 'link2', 'link2', 'file1')).toString(), variant).to.equal('file1\n');
        }
      });

      itremote('requires recursive for directories', async function () {
        const temp = require('temp').track();
        const src = path.join(asarDir, 'a.asar');
        // Node reports this as the ERR_FS_EISDIR system error (with EISDIR as its info code).
        expect(() => fs.cpSync(src, temp.path()))
          .to.throw()
          .with.property('code', 'ERR_FS_EISDIR');
        await expectToThrowErrorWithCode(() => fs.promises.cp(src, temp.path()), 'ERR_FS_EISDIR');
        const err = await new Promise<any>((resolve) => fs.cp(src, temp.path(), resolve));
        expect(err.code).to.equal('ERR_FS_EISDIR');
        expect(err.info.code).to.equal('EISDIR');
      });

      itremote('honours errorOnExist and force', async function () {
        const temp = require('temp').track();
        const src = path.join(asarDir, 'a.asar', 'file1');
        const dest = temp.path();
        fs.writeFileSync(dest, 'existing');
        // force defaults to true
        fs.cpSync(src, dest);
        expect(fs.readFileSync(dest).toString()).to.equal('file1\n');
        fs.writeFileSync(dest, 'existing');
        fs.cpSync(src, dest, { force: false });
        expect(fs.readFileSync(dest).toString()).to.equal('existing');
        expect(() => fs.cpSync(src, dest, { force: false, errorOnExist: true }))
          .to.throw()
          .with.property('code', 'ERR_FS_CP_EEXIST');
        await expectToThrowErrorWithCode(
          () => fs.promises.cp(src, dest, { force: false, errorOnExist: true }),
          'ERR_FS_CP_EEXIST'
        );
        await fs.promises.cp(src, dest, { force: false });
        expect(fs.readFileSync(dest).toString()).to.equal('existing');
        await fs.promises.cp(src, dest);
        expect(fs.readFileSync(dest).toString()).to.equal('file1\n');

        // Directory variants
        const dir = temp.path();
        fs.mkdirSync(path.join(dir, 'dir1'), { recursive: true });
        fs.writeFileSync(path.join(dir, 'dir1', 'file1'), 'existing');
        fs.cpSync(path.join(asarDir, 'a.asar'), dir, { recursive: true, force: false });
        expect(fs.readFileSync(path.join(dir, 'dir1', 'file1')).toString()).to.equal('existing');
        expect(fs.readFileSync(path.join(dir, 'dir1', 'file2')).toString()).to.equal('file2\n');
        expect(() =>
          fs.cpSync(path.join(asarDir, 'a.asar'), dir, { recursive: true, force: false, errorOnExist: true })
        ).to.throw(/EEXIST/);
        fs.cpSync(path.join(asarDir, 'a.asar'), dir, { recursive: true });
        expect(fs.readFileSync(path.join(dir, 'dir1', 'file1')).toString()).to.equal('file1\n');
      });

      itremote('applies filter functions', async function () {
        const temp = require('temp').track();
        const src = path.join(asarDir, 'a.asar');
        const dest = temp.path();
        fs.cpSync(src, dest, { recursive: true, filter: (p: string) => !p.endsWith('file2') });
        expect(fs.existsSync(path.join(dest, 'file1'))).to.be.true();
        expect(fs.existsSync(path.join(dest, 'file2'))).to.be.false();
        expect(fs.existsSync(path.join(dest, 'dir1', 'file2'))).to.be.false();
        const dest2 = temp.path();
        await fs.promises.cp(src, dest2, {
          recursive: true,
          filter: async (p: string) => !path.basename(p).startsWith('dir')
        });
        expect(fs.readdirSync(dest2).sort()).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
        // Symlink to a filtered-out directory still gets created as a link.
        expect(fs.lstatSync(path.join(dest2, 'link2')).isSymbolicLink()).to.be.true();
      });

      itremote('dereferences symlinks when asked', async function () {
        const temp = require('temp').track();
        const src = path.join(asarDir, 'a.asar');
        // Note: a.asar's directory links form a cycle (dir1/link2 -> dir1), so a
        // dereferencing copy of the whole tree cannot terminate (Node behaves
        // the same on a real tree); dereference the acyclic parts instead.
        for (const variant of ['sync', 'promises']) {
          const single = temp.path();
          if (variant === 'sync') fs.cpSync(path.join(src, 'link1'), single, { dereference: true });
          else await fs.promises.cp(path.join(src, 'link1'), single, { dereference: true });
          expect(fs.lstatSync(single).isFile(), variant).to.be.true();
          expect(fs.readFileSync(single).toString(), variant).to.equal('file1\n');

          const dir = temp.path();
          if (variant === 'sync') fs.cpSync(path.join(src, 'dir2'), dir, { recursive: true, dereference: true });
          else await fs.promises.cp(path.join(src, 'dir2'), dir, { recursive: true, dereference: true });
          expect(fs.readdirSync(dir).sort(), variant).to.deep.equal(['file1', 'file2', 'file3']);
        }
      });

      itremote('errors on non-existent sources and file-to-directory mismatches', async function () {
        const temp = require('temp').track();
        expect(() => fs.cpSync(path.join(asarDir, 'a.asar', 'nope'), temp.path())).to.throw(/ENOENT/);
        await expectToThrowErrorWithCode(
          () => fs.promises.cp(path.join(asarDir, 'a.asar', 'nope'), temp.path()),
          'ENOENT'
        );
        const dir = temp.path();
        fs.mkdirSync(dir);
        expect(() => fs.cpSync(path.join(asarDir, 'a.asar', 'file1'), dir))
          .to.throw()
          .with.property('code', 'ERR_FS_CP_NON_DIR_TO_DIR');
        await expectToThrowErrorWithCode(
          () => fs.promises.cp(path.join(asarDir, 'a.asar', 'file1'), dir),
          'ERR_FS_CP_NON_DIR_TO_DIR'
        );
        const file = temp.path();
        fs.writeFileSync(file, 'x');
        expect(() => fs.cpSync(path.join(asarDir, 'a.asar'), file, { recursive: true }))
          .to.throw()
          .with.property('code', 'ERR_FS_CP_DIR_TO_NON_DIR');
        await expectToThrowErrorWithCode(
          () => fs.promises.cp(path.join(asarDir, 'a.asar'), file, { recursive: true }),
          'ERR_FS_CP_DIR_TO_NON_DIR'
        );
      });

      itremote('copies unpacked files and mixed archives', async function () {
        const temp = require('temp').track();
        const dest = temp.path();
        fs.cpSync(path.join(asarDir, 'unpack.asar'), dest, { recursive: true });
        expect(
          fs.readFileSync(path.join(dest, 'a.txt')).equals(fs.readFileSync(path.join(asarDir, 'unpack.asar', 'a.txt')))
        ).to.be.true();
        expect(
          fs
            .readFileSync(path.join(dest, 'atom.png'))
            .equals(fs.readFileSync(path.join(asarDir, 'unpack.asar', 'atom.png')))
        ).to.be.true();
      });
    });

    // Mirrors test-fs-opendir.js.
    describe('fs.opendir / fs.opendirSync / fs.promises.opendir on archives', function () {
      itremote('lists entries with correct types (sync)', function () {
        const dir = fs.opendirSync(path.join(asarDir, 'a.asar'));
        expect(dir.path).to.equal(path.join(asarDir, 'a.asar'));
        const entries: Record<string, string> = {};
        let dirent;
        while ((dirent = dir.readSync()) !== null) {
          expect(dirent).to.be.an.instanceOf(fs.Dirent);
          expect(dirent.parentPath).to.equal(path.join(asarDir, 'a.asar'));
          entries[dirent.name] = dirent.isDirectory()
            ? 'dir'
            : dirent.isSymbolicLink()
              ? 'link'
              : dirent.isFile()
                ? 'file'
                : '?';
        }
        dir.closeSync();
        expect(entries).to.deep.equal({
          dir1: 'dir',
          dir2: 'dir',
          dir3: 'dir',
          file1: 'file',
          file2: 'file',
          file3: 'file',
          link1: 'link',
          link2: 'link',
          'ping.js': 'file'
        });
        expect(() => dir.readSync())
          .to.throw()
          .with.property('code', 'ERR_DIR_CLOSED');
        expect(() => dir.closeSync())
          .to.throw()
          .with.property('code', 'ERR_DIR_CLOSED');
      });

      itremote('lists entries via callbacks, promises and async iteration', async function () {
        const p = path.join(asarDir, 'a.asar');
        const expected = fs.readdirSync(p).sort();

        const dir1 = await promisify(fs.opendir)(p);
        const names1: string[] = [];
        for (;;) {
          const dirent = await promisify(dir1.read.bind(dir1))();
          if (dirent === null) break;
          names1.push(dirent.name);
        }
        await promisify(dir1.close.bind(dir1))();
        expect(names1.sort()).to.deep.equal(expected);

        const dir2 = await fs.promises.opendir(p);
        const names2: string[] = [];
        for await (const dirent of dir2) names2.push(dirent.name);
        expect(names2.sort()).to.deep.equal(expected);
        await expectToThrowErrorWithCode(() => dir2.close(), 'ERR_DIR_CLOSED');

        const dir3 = await fs.promises.opendir(p, { bufferSize: 2 });
        const names3: string[] = [];
        let dirent;
        while ((dirent = await dir3.read()) !== null) names3.push(dirent.name);
        await dir3.close();
        expect(names3.sort()).to.deep.equal(expected);

        // Breaking out of iteration closes the handle.
        const dir4 = await fs.promises.opendir(p);
        // eslint-disable-next-line no-unreachable-loop
        for await (const _ of dir4) {
          expect(_).to.be.an.instanceOf(fs.Dirent);
          break;
        }
        await expectToThrowErrorWithCode(() => dir4.read(), 'ERR_DIR_CLOSED');
      });

      itremote('supports recursive iteration', async function () {
        const p = path.join(asarDir, 'a.asar');
        const dir = await fs.promises.opendir(p, { recursive: true });
        const found: string[] = [];
        for await (const dirent of dir) found.push(path.relative(p, path.join(dirent.parentPath, dirent.name)));
        expect(found).to.include(path.join('dir1', 'file1'));
        expect(found).to.include(path.join('dir3', 'file3'));
        expect(found).to.include('link2');
        // Symlinked directories are not followed (like Node).
        expect(found).to.not.include(path.join('link2', 'file1'));
        const sync = fs.opendirSync(p, { recursive: true });
        const foundSync: string[] = [];
        let dirent;
        while ((dirent = sync.readSync()) !== null) {
          foundSync.push(path.relative(p, path.join(dirent.parentPath, dirent.name)));
        }
        sync.closeSync();
        expect(foundSync.sort()).to.deep.equal(found.sort());
      });

      itremote('lists sub directories, linked directories and empty archives', async function () {
        expect(
          Array.from({ length: 5 }, () => fs.opendirSync(path.join(asarDir, 'a.asar', 'dir1')).readSync()!.name).sort()
        ).to.deep.equal(['file1', 'file1', 'file1', 'file1', 'file1']);
        const linked = fs.opendirSync(path.join(asarDir, 'a.asar', 'link2'));
        const names: string[] = [];
        let dirent;
        while ((dirent = linked.readSync()) !== null) names.push(dirent.name);
        linked.closeSync();
        expect(names.sort()).to.deep.equal(fs.readdirSync(path.join(asarDir, 'a.asar', 'dir1')).sort());
        const empty = fs.opendirSync(path.join(asarDir, 'empty.asar'));
        expect(empty.readSync()!.name).to.equal('file1');
        expect(empty.readSync()).to.equal(null);
        empty.closeSync();
      });

      itremote('supports the buffer and other name encodings', function () {
        const dir = fs.opendirSync(path.join(asarDir, 'a.asar'), { encoding: 'buffer' as any });
        const dirent = dir.readSync()!;
        expect(Buffer.isBuffer(dirent.name)).to.be.true();
        dir.closeSync();
        const hex = fs.opendirSync(path.join(asarDir, 'a.asar'), { encoding: 'hex' as any });
        const names: string[] = [];
        let d;
        while ((d = hex.readSync()) !== null) names.push(d.name as any);
        hex.closeSync();
        expect(names.sort()).to.deep.equal(
          fs
            .readdirSync(path.join(asarDir, 'a.asar'))
            .map((n) => Buffer.from(n).toString('hex'))
            .sort()
        );
      });

      itremote('reports ENOTDIR and ENOENT', async function () {
        expect(() => fs.opendirSync(path.join(asarDir, 'a.asar', 'file1'))).to.throw(/ENOTDIR/);
        expect(() => fs.opendirSync(path.join(asarDir, 'a.asar', 'nope'))).to.throw(/ENOENT/);
        await expectToThrowErrorWithCode(() => fs.promises.opendir(path.join(asarDir, 'a.asar', 'file1')), 'ENOTDIR');
        const err = await new Promise<any>((resolve) => fs.opendir(path.join(asarDir, 'a.asar', 'nope'), resolve));
        expect(err.code).to.equal('ENOENT');
        expect(() => fs.opendirSync(path.join(asarDir, 'a.asar'), { bufferSize: 0 }))
          .to.throw()
          .with.property('code', 'ERR_OUT_OF_RANGE');
      });
    });

    describe('fs.readlink / fs.readlinkSync / fs.promises.readlink on archives', function () {
      itremote('returns link targets relative to the link, like a real filesystem', async function () {
        const a = path.join(asarDir, 'a.asar');
        // In this fixture every link points at a root entry (dir1/link1 -> /file1,
        // dir1/link2 -> /dir1), and asar stores targets relative to the archive
        // root; readlink reports them relative to the link's directory.
        expect(fs.readlinkSync(path.join(a, 'link1'))).to.equal('file1');
        expect(fs.readlinkSync(path.join(a, 'link2'))).to.equal('dir1');
        expect(fs.readlinkSync(path.join(a, 'dir1', 'link1'))).to.equal(path.join('..', 'file1'));
        expect(fs.readlinkSync(path.join(a, 'dir1', 'link2'))).to.equal('.');
        expect(fs.readlinkSync(path.join(a, 'link2', 'link1'))).to.equal(path.join('..', 'file1'));
        expect(await fs.promises.readlink(path.join(a, 'link1'))).to.equal('file1');
        expect(await promisify(fs.readlink)(path.join(a, 'link1'))).to.equal('file1');
        const asBuffer = fs.readlinkSync(path.join(a, 'link1'), 'buffer');
        expect(Buffer.isBuffer(asBuffer)).to.be.true();
        expect(asBuffer.toString()).to.equal('file1');
        expect(fs.readlinkSync(path.join(a, 'link1'), { encoding: 'utf8' })).to.equal('file1');
        expect(fs.readlinkSync(path.join(a, 'link1'), 'hex')).to.equal(Buffer.from('file1').toString('hex'));
        expect(fs.readlinkSync(path.join(a, 'link1'), { encoding: 'base64' })).to.equal(
          Buffer.from('file1').toString('base64')
        );
        // Resolving the target against the link's directory gives a readable path.
        const resolve = (link: string) => path.resolve(path.dirname(link), fs.readlinkSync(link));
        expect(fs.readFileSync(resolve(path.join(a, 'link1'))).toString()).to.equal('file1\n');
        expect(fs.readdirSync(resolve(path.join(a, 'link2'))).sort()).to.deep.equal(
          fs.readdirSync(path.join(a, 'dir1')).sort()
        );
        expect(fs.readFileSync(resolve(path.join(a, 'link2', 'link1'))).toString()).to.equal('file1\n');
      });

      itremote('reports EINVAL for non-links and ENOENT for missing paths', async function () {
        const a = path.join(asarDir, 'a.asar');
        expect(() => fs.readlinkSync(path.join(a, 'file1'))).to.throw(/EINVAL/);
        expect(() => fs.readlinkSync(path.join(a, 'dir1'))).to.throw(/EINVAL/);
        expect(() => fs.readlinkSync(path.join(a, 'nope'))).to.throw(/ENOENT/);
        await expectToThrowErrorWithCode(() => fs.promises.readlink(path.join(a, 'file1')), 'EINVAL');
        const err = await new Promise<any>((resolve) => fs.readlink(path.join(a, 'nope'), resolve));
        expect(err.code).to.equal('ENOENT');
      });
    });

    describe('fs.stat follows symbolic links inside archives', function () {
      itremote('reports the link target for stat and the link itself for lstat', async function () {
        const a = path.join(asarDir, 'a.asar');
        for (const [link, expected] of [
          ['link1', 'file'],
          ['link2', 'dir'],
          [path.join('dir1', 'link1'), 'file'],
          [path.join('dir1', 'link2'), 'dir'],
          [path.join('link2', 'link1'), 'file'],
          [path.join('link2', 'link2'), 'dir'],
          [path.join('link2', 'link2', 'link1'), 'file']
        ] as [string, string][]) {
          const p = path.join(a, link);
          for (const stats of [
            fs.statSync(p),
            await promisify(fs.stat)(p),
            await fs.promises.stat(p),
            fs.statSync(p, { bigint: true })
          ]) {
            expect(stats.isSymbolicLink(), link).to.be.false();
            expect(stats.isFile(), link).to.equal(expected === 'file');
            expect(stats.isDirectory(), link).to.equal(expected === 'dir');
            if (expected === 'file') expect(Number(stats.size), link).to.equal(6);
          }
          expect(fs.lstatSync(p).isSymbolicLink(), link).to.be.true();
          expect((await fs.promises.lstat(p)).isSymbolicLink(), link).to.be.true();
        }
        expect(fs.statSync(path.join(a, 'link1'), { throwIfNoEntry: false })!.isFile()).to.be.true();
        expect(fs.statSync(path.join(a, 'nope'), { throwIfNoEntry: false })).to.equal(null as any);
        expect(() => fs.statSync(path.join(a, 'nope'))).to.throw(/ENOENT/);
        await expectToThrowErrorWithCode(() => fs.promises.stat(path.join(a, 'nope')), 'ENOENT');
      });

      itremote('resolves paths through symlinked directories for module loading', function () {
        // require() relies on internalModuleStat, which must follow links to
        // recognise directories.
        expect(fs.readdirSync(path.join(asarDir, 'a.asar', 'link2'))).to.include('file1');
        expect(fs.readFileSync(path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1')).toString()).to.equal('file1\n');
        expect(fs.existsSync(path.join(asarDir, 'a.asar', 'link2', 'link1'))).to.be.true();
      });
    });

    describe('fs.stat / fs.lstat with bigint on archives', function () {
      itremote('returns BigIntStats when requested', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const s = fs.statSync(p, { bigint: true });
        expect(typeof s.size).to.equal('bigint');
        expect(s.size).to.equal(6n);
        expect(typeof s.mtimeMs).to.equal('bigint');
        expect(typeof s.mtimeNs).to.equal('bigint');
        expect(s.isFile()).to.be.true();
        expect(fs.lstatSync(path.join(asarDir, 'a.asar', 'link1'), { bigint: true }).isSymbolicLink()).to.be.true();
        expect(fs.lstatSync(path.join(asarDir, 'a.asar', 'dir1'), { bigint: true }).isDirectory()).to.be.true();
        expect((await fs.promises.stat(p, { bigint: true })).size).to.equal(6n);
        expect((await fs.promises.lstat(p, { bigint: true })).size).to.equal(6n);
        const cb = await new Promise<any>((resolve, reject) =>
          fs.stat(p, { bigint: true }, (e, s) => (e ? reject(e) : resolve(s)))
        );
        expect(cb.size).to.equal(6n);
        // Plain stats are still numbers.
        expect(typeof fs.statSync(p).size).to.equal('number');
        expect(fs.statSync(p).mtime).to.be.an.instanceOf(Date);
        expect(fs.statSync(p, { bigint: false }).size).to.equal(6);
      });
    });

    describe('fs.exists on an invalid archive', function () {
      itremote('reports false rather than an error object', async function () {
        const p = path.join(asarDir, 'not-an-archive.asar', 'file');
        expect(fs.existsSync(p)).to.be.false();
        // eslint-disable-next-line n/no-deprecated-api
        const exists = await new Promise((resolve) => fs.exists(p, resolve));
        expect(exists).to.be.false();
      });
    });

    describe('fs.readFileSync options handling', function () {
      itremote('returns a Buffer for empty files when options is an object without an encoding', function () {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        expect(Buffer.isBuffer(fs.readFileSync(p, { flag: 'r' }))).to.be.true();
        expect(Buffer.isBuffer(fs.readFileSync(p, { encoding: null }))).to.be.true();
        expect(fs.readFileSync(p, { encoding: 'utf8' })).to.equal('');
      });
    });

    describe('original-fs is not affected by the archive-aware fs overrides', function () {
      itremote('treats archives as plain files through every fd, stream, copy and dir API', async function () {
        const originalFs = require('node:original-fs');
        const archive = path.join(asarDir, 'a.asar');
        const rawSize = originalFs.statSync(archive).size;
        expect(originalFs.statSync(archive).isFile()).to.be.true();
        expect(rawSize).to.be.greaterThan(1000);
        const raw = originalFs.readFileSync(archive);
        expect(raw.length).to.equal(rawSize);
        expect(raw.subarray(0, 4).readUInt32LE(0)).to.equal(4); // asar pickle header

        // fd-based access reads the archive itself.
        const fd = originalFs.openSync(archive, 'r');
        expect(originalFs.fstatSync(fd).size).to.equal(rawSize);
        const head = Buffer.alloc(16);
        expect(originalFs.readSync(fd, head, 0, 16, 0)).to.equal(16);
        expect(head.equals(raw.subarray(0, 16))).to.be.true();
        expect(originalFs.readFileSync(fd).length).to.equal(rawSize);
        expect(originalFs.readvSync(fd, [Buffer.alloc(8)], 0)).to.equal(8);
        originalFs.closeSync(fd);

        const handle = await originalFs.promises.open(archive, 'r');
        expect((await handle.stat()).size).to.equal(rawSize);
        expect((await handle.readFile()).length).to.equal(rawSize);
        await handle.close();

        const streamed = await new Promise<Buffer>((resolve, reject) => {
          const chunks: Buffer[] = [];
          originalFs
            .createReadStream(archive)
            .on('data', (c: Buffer) => chunks.push(c))
            .on('error', reject)
            .on('end', () => resolve(Buffer.concat(chunks)));
        });
        expect(streamed.equals(raw)).to.be.true();

        // Paths "inside" the archive are just non-existent for original-fs.
        expect(() => originalFs.openSync(path.join(archive, 'file1'), 'r')).to.throw(/ENOTDIR|ENOENT/);
        expect(() => originalFs.readlinkSync(path.join(archive, 'link1'))).to.throw(/ENOTDIR|ENOENT/);
        expect(() => originalFs.opendirSync(path.join(archive, 'dir1'))).to.throw(/ENOTDIR|ENOENT/);
        expect(() => originalFs.opendirSync(archive)).to.throw(/ENOTDIR/);
        await expectToThrowErrorWithCode(
          () => originalFs.promises.open(path.join(archive, 'file1'), 'r'),
          process.platform === 'win32' ? 'ENOENT' : 'ENOTDIR'
        );

        // Copies copy the archive file itself.
        const temp = require('temp').track();
        const d1 = temp.path();
        originalFs.copyFileSync(archive, d1);
        expect(originalFs.readFileSync(d1).equals(raw)).to.be.true();
        const d2 = temp.path();
        await originalFs.promises.copyFile(archive, d2);
        expect(originalFs.readFileSync(d2).equals(raw)).to.be.true();
        const d3 = temp.path();
        originalFs.cpSync(archive, d3);
        expect(originalFs.readFileSync(d3).equals(raw)).to.be.true();
        const d4 = temp.path();
        await originalFs.promises.cp(archive, d4);
        expect(originalFs.readFileSync(d4).equals(raw)).to.be.true();

        // And a directory listing of the fixtures dir shows the archive as a file.
        const dir = originalFs.opendirSync(asarDir);
        let dirent;
        let seen = false;
        while ((dirent = dir.readSync()) !== null) {
          if (dirent.name === 'a.asar') {
            seen = true;
            expect(dirent.isFile()).to.be.true();
          }
        }
        dir.closeSync();
        expect(seen).to.be.true();

        // Writing through original-fs to a path that merely mentions .asar works.
        const scratch = path.join(temp.mkdirSync('asar-original-fs'), 'not-really.asar');
        originalFs.writeFileSync(scratch, 'hello');
        expect(originalFs.readFileSync(scratch, 'utf8')).to.equal('hello');
        const wfd = originalFs.openSync(scratch, 'w');
        originalFs.writeSync(wfd, 'bye');
        originalFs.closeSync(wfd);
        expect(originalFs.readFileSync(scratch, 'utf8')).to.equal('bye');
      });
    });

    describe('fs.mkdir', function () {
      itremote('throws error when calling inside asar archive', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.mkdir(p, resolve));
        expect(err.code).to.equal('ENOTDIR');
      });
    });

    describe('fs.promises.mkdir', function () {
      itremote('throws error when calling inside asar archive', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.mkdir(p), 'ENOTDIR');
      });
    });

    describe('fs.mkdirSync', function () {
      itremote('throws error when calling inside asar archive', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.mkdirSync(p);
        }).to.throw(/ENOTDIR/);
      });

      itremote('throws error when calling recursively inside asar archive', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.mkdirSync(p, { recursive: true });
        }).to.throw(/ENOTDIR/);
      });
    });

    describe('fs.exists', function () {
      itremote('handles an existing file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        // eslint-disable-next-line n/no-deprecated-api
        const exists = await new Promise((resolve) => fs.exists(p, resolve));
        expect(exists).to.be.true();
      });

      itremote('handles a non-existent file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        // eslint-disable-next-line n/no-deprecated-api
        const exists = await new Promise((resolve) => fs.exists(p, resolve));
        expect(exists).to.be.false();
      });

      itremote('promisified version handles an existing file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        // eslint-disable-next-line n/no-deprecated-api
        const exists = await require('node:util').promisify(fs.exists)(p);
        expect(exists).to.be.true();
      });

      itremote('promisified version handles a non-existent file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        // eslint-disable-next-line n/no-deprecated-api
        const exists = await require('node:util').promisify(fs.exists)(p);
        expect(exists).to.be.false();
      });
    });

    describe('fs.existsSync', function () {
      itremote('handles an existing file', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(fs.existsSync(p)).to.be.true();
      });

      itremote('handles a non-existent file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(fs.existsSync(p)).to.be.false();
      });
    });

    describe('fs.access', function () {
      itremote('accesses a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        await promisify(fs.access)(p);
      });

      itremote('throws an error when called with write mode', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const err = await new Promise<any>((resolve) => fs.access(p, fs.constants.R_OK | fs.constants.W_OK, resolve));
        expect(err.code).to.equal('EACCES');
      });

      itremote('throws an error when called on non-existent file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.access(p, fs.constants.R_OK | fs.constants.W_OK, resolve));
        expect(err.code).to.equal('ENOENT');
      });

      itremote('allows write mode for unpacked files', async function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        await promisify(fs.access)(p, fs.constants.R_OK | fs.constants.W_OK);
      });
    });

    describe('fs.promises.access', function () {
      itremote('accesses a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        await fs.promises.access(p);
      });

      itremote('throws an error when called with write mode', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        await expectToThrowErrorWithCode(() => fs.promises.access(p, fs.constants.R_OK | fs.constants.W_OK), 'EACCES');
      });

      itremote('throws an error when called on non-existent file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.access(p), 'ENOENT');
      });

      itremote('allows write mode for unpacked files', async function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        await fs.promises.access(p, fs.constants.R_OK | fs.constants.W_OK);
      });
    });

    describe('fs.accessSync', function () {
      itremote('accesses a normal file', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => {
          fs.accessSync(p);
        }).to.not.throw();
      });

      itremote('throws an error when called with write mode', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => {
          fs.accessSync(p, fs.constants.R_OK | fs.constants.W_OK);
        }).to.throw(/EACCES/);
      });

      itremote('throws an error when called on non-existent file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.accessSync(p);
        }).to.throw(/ENOENT/);
      });

      itremote('allows write mode for unpacked files', function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        expect(() => {
          fs.accessSync(p, fs.constants.R_OK | fs.constants.W_OK);
        }).to.not.throw();
      });
    });

    function generateSpecs(childProcess: string) {
      describe(`${childProcess}.fork`, function () {
        itremote(
          'opens a normal js file',
          async function (childProcess: string) {
            const child = require(childProcess).fork(path.join(asarDir, 'a.asar', 'ping.js'));
            child.send('message');
            const msg = await new Promise((resolve) => child.once('message', resolve));
            expect(msg).to.equal('message');
          },
          [childProcess]
        );

        itremote(
          'supports asar in the forked js',
          async function (childProcess: string, fixtures: string) {
            const file = path.join(asarDir, 'a.asar', 'file1');
            const child = require(childProcess).fork(path.join(fixtures, 'module', 'asar.js'));
            child.send(file);
            const content = await new Promise((resolve) => child.once('message', resolve));
            expect(content).to.equal(fs.readFileSync(file).toString());
          },
          [childProcess, fixtures]
        );
      });

      describe(`${childProcess}.exec`, function () {
        itremote(
          'should not try to extract the command if there is a reference to a file inside an .asar',
          async function (childProcess: string) {
            const echo = path.join(asarDir, 'echo.asar', 'echo');

            const stdout = await promisify(require(childProcess).exec)('echo ' + echo + ' foo bar');
            expect(stdout.toString().replaceAll('\r', '')).to.equal(echo + ' foo bar\n');
          },
          [childProcess]
        );
      });

      describe(`${childProcess}.execSync`, function () {
        itremote(
          'should not try to extract the command if there is a reference to a file inside an .asar',
          async function (childProcess: string) {
            const echo = path.join(asarDir, 'echo.asar', 'echo');

            const stdout = require(childProcess).execSync('echo ' + echo + ' foo bar');
            expect(stdout.toString().replaceAll('\r', '')).to.equal(echo + ' foo bar\n');
          },
          [childProcess]
        );
      });

      ifdescribe(process.platform === 'darwin' && process.arch !== 'arm64')(`${childProcess}.execFile`, function () {
        itremote(
          'executes binaries',
          async function (childProcess: string) {
            const echo = path.join(asarDir, 'echo.asar', 'echo');
            const stdout = await promisify(require(childProcess).execFile)(echo, ['test']);
            expect(stdout).to.equal('test\n');
          },
          [childProcess]
        );

        itremote(
          'executes binaries without callback',
          async function (childProcess: string) {
            const echo = path.join(asarDir, 'echo.asar', 'echo');
            const process = require(childProcess).execFile(echo, ['test']);
            const code = await new Promise((resolve) => process.once('close', resolve));
            expect(code).to.equal(0);
            process.on('error', function () {
              throw new Error('error');
            });
          },
          [childProcess]
        );

        itremote(
          'execFileSync executes binaries',
          function (childProcess: string) {
            const echo = path.join(asarDir, 'echo.asar', 'echo');
            const output = require(childProcess).execFileSync(echo, ['test']);
            expect(String(output)).to.equal('test\n');
          },
          [childProcess]
        );
      });
    }

    generateSpecs('child_process');
    generateSpecs('node:child_process');

    describe('util.promisify', function () {
      itremote('can promisify all fs functions', function () {
        const originalFs = require('node:original-fs');
        const util = require('node:util');

        for (const [propertyName, originalValue] of Object.entries(originalFs)) {
          // Some properties exist but have a value of `undefined` on some platforms.
          // E.g. `fs.lchmod`, which in only available on MacOS, see
          // https://nodejs.org/docs/latest-v10.x/api/fs.html#fs_fs_lchmod_path_mode_callback
          // Also check for `null`s, `hasOwnProperty()` can't handle them.
          if (typeof originalValue === 'undefined' || originalValue === null) continue;

          if (Object.hasOwn(originalValue, util.promisify.custom)) {
            expect(fs).to.have.own.property(propertyName).that.has.own.property(util.promisify.custom);
          }
        }
      });
    });

    describe('splitPath', function () {
      itremote('splits at the deepest .asar file component and normalizes the relative part', function () {
        const { splitPath } = process._linkedBinding('electron_common_asar');
        const archive = path.join(asarDir, 'a.asar');
        expect(splitPath(path.join(archive, 'dir1', 'file1'))).to.deep.equal({
          isAsar: true,
          asarPath: archive,
          filePath: ['dir1', 'file1'].join(path.sep)
        });
        expect(
          splitPath(archive + path.sep + path.sep + 'dir1' + path.sep + path.sep + 'file1' + path.sep)
        ).to.deep.equal({ isAsar: true, asarPath: archive, filePath: ['dir1', 'file1'].join(path.sep) });
        expect(splitPath(path.join(archive, 'nested.asar', 'x'))).to.deep.equal({
          isAsar: true,
          asarPath: path.join(archive, 'nested.asar'),
          filePath: 'x'
        });
        expect(splitPath(archive)).to.deep.equal({ isAsar: true, asarPath: archive, filePath: '' });
      });

      itremote('does not treat a real directory named like an archive as an archive', function () {
        const { splitPath } = process._linkedBinding('electron_common_asar');
        expect(splitPath(path.join(asarDir, 'file'))).to.deep.equal({ isAsar: false });
        expect(splitPath(asarDir)).to.deep.equal({ isAsar: false });
        expect(splitPath(path.join(fixtures, 'module', 'noop.js'))).to.deep.equal({ isAsar: false });
      });

      itremote('matches the archive extension the same way base::FilePath does', function () {
        const { splitPath } = process._linkedBinding('electron_common_asar');
        const dir = path.join(fixtures, 'module');
        expect(splitPath(path.join(dir, 'X.ASAR', 'y')).isAsar).to.equal(true);
        expect(splitPath(path.join(dir, '.asar', 'y')).isAsar).to.equal(true);
        expect(splitPath(path.join(dir, 'x.asar.gz', 'y')).isAsar).to.equal(false);
        expect(splitPath(path.join(dir, 'x.asarx', 'y')).isAsar).to.equal(false);
        expect(splitPath(path.join(dir, 'asar', 'y')).isAsar).to.equal(false);
      });
    });

    describe('process.noAsar', function () {
      const errorName = process.platform === 'win32' ? 'ENOENT' : 'ENOTDIR';

      beforeEach(async function () {
        return (await getRemoteContext()).webContents.executeJavaScript(`
          process.noAsar = true;
        `);
      });

      afterEach(async function () {
        return (await getRemoteContext()).webContents.executeJavaScript(`
          process.noAsar = false;
        `);
      });

      itremote(
        'disables asar support in sync API',
        function (errorName: string) {
          const file = path.join(asarDir, 'a.asar', 'file1');
          const dir = path.join(asarDir, 'a.asar', 'dir1');
          console.log(1);
          expect(() => {
            fs.readFileSync(file);
          }).to.throw(new RegExp(errorName));
          expect(() => {
            fs.lstatSync(file);
          }).to.throw(new RegExp(errorName));
          expect(() => {
            fs.realpathSync(file);
          }).to.throw(new RegExp(errorName));
          expect(() => {
            fs.readdirSync(dir);
          }).to.throw(new RegExp(errorName));
        },
        [errorName]
      );

      itremote(
        'disables asar support in async API',
        async function (errorName: string) {
          const file = path.join(asarDir, 'a.asar', 'file1');
          const dir = path.join(asarDir, 'a.asar', 'dir1');
          await new Promise<void>((resolve) => {
            fs.readFile(file, function (error) {
              expect(error?.code).to.equal(errorName);
              fs.lstat(file, function (error) {
                expect(error?.code).to.equal(errorName);
                fs.realpath(file, function (error) {
                  expect(error?.code).to.equal(errorName);
                  fs.readdir(dir, function (error) {
                    expect(error?.code).to.equal(errorName);
                    resolve();
                  });
                });
              });
            });
          });
        },
        [errorName]
      );

      itremote(
        'disables asar support in promises API',
        async function (errorName: string) {
          const file = path.join(asarDir, 'a.asar', 'file1');
          const dir = path.join(asarDir, 'a.asar', 'dir1');
          await expect(fs.promises.readFile(file)).to.be.eventually.rejectedWith(Error, new RegExp(errorName));
          await expect(fs.promises.lstat(file)).to.be.eventually.rejectedWith(Error, new RegExp(errorName));
          await expect(fs.promises.realpath(file)).to.be.eventually.rejectedWith(Error, new RegExp(errorName));
          await expect(fs.promises.readdir(dir)).to.be.eventually.rejectedWith(Error, new RegExp(errorName));
        },
        [errorName]
      );

      itremote('treats *.asar as normal file', function () {
        const originalFs = require('node:original-fs');
        const asar = path.join(asarDir, 'a.asar');
        const content1 = fs.readFileSync(asar);
        const content2 = originalFs.readFileSync(asar);
        expect(content1.compare(content2)).to.equal(0);
        expect(() => {
          fs.readdirSync(asar);
        }).to.throw(/ENOTDIR/);
      });

      itremote('is reset to its original value when execSync throws an error', function () {
        process.noAsar = false;
        expect(() => {
          require('node:child_process').execSync(path.join(__dirname, 'does-not-exist.txt'));
        }).to.throw();
        expect(process.noAsar).to.be.false();
      });
    });

    /*
    describe('process.env.ELECTRON_NO_ASAR', function () {
      itremote('disables asar support in forked processes', function (done) {
        const forked = ChildProcess.fork(path.join(__dirname, 'fixtures', 'module', 'no-asar.js'), [], {
          env: {
            ELECTRON_NO_ASAR: true
          }
        });
        forked.on('message', function (stats) {
          try {
            expect(stats.isFile).to.be.true();
            expect(stats.size).to.equal(3458);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      itremote('disables asar support in spawned processes', function (done) {
        const spawned = ChildProcess.spawn(process.execPath, [path.join(__dirname, 'fixtures', 'module', 'no-asar.js')], {
          env: {
            ELECTRON_NO_ASAR: true,
            ELECTRON_RUN_AS_NODE: true
          }
        });

        let output = '';
        spawned.stdout.on('data', function (data) {
          output += data;
        });
        spawned.stdout.on('close', function () {
          try {
            const stats = JSON.parse(output);
            expect(stats.isFile).to.be.true();
            expect(stats.size).to.equal(3458);
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });
    */
  });

  describe('asar protocol', function () {
    itremote('can request a file in package', async function () {
      const p = path.resolve(asarDir, 'a.asar', 'file1');
      const response = await fetch('file://' + p);
      const data = await response.text();
      expect(data.trim()).to.equal('file1');
    });

    itremote('can request a file in package with unpacked files', async function () {
      const p = path.resolve(asarDir, 'unpack.asar', 'a.txt');
      const response = await fetch('file://' + p);
      const data = await response.text();
      expect(data.trim()).to.equal('a');
    });

    itremote('can request a linked file in package', async function () {
      const p = path.resolve(asarDir, 'a.asar', 'link2', 'link1');
      const response = await fetch('file://' + p);
      const data = await response.text();
      expect(data.trim()).to.equal('file1');
    });

    itremote('can request a file in filesystem', async function () {
      const p = path.resolve(asarDir, 'file');
      const response = await fetch('file://' + p);
      const data = await response.text();
      expect(data.trim()).to.equal('file');
    });

    itremote('gets error when file is not found', async function () {
      const p = path.resolve(asarDir, 'a.asar', 'no-exist');
      try {
        const response = await fetch('file://' + p);
        expect(response.status).to.equal(404);
      } catch (error: any) {
        expect(error.message).to.equal('Failed to fetch');
      }
    });
  });

  describe('original-fs module', function () {
    itremote('treats .asar as file', function () {
      const file = path.join(asarDir, 'a.asar');
      const originalFs = require('node:original-fs');
      const stats = originalFs.statSync(file);
      expect(stats.isFile()).to.be.true();
    });

    /*
    it('is available in forked scripts', async function () {
      const child = ChildProcess.fork(path.join(fixtures, 'module', 'original-fs.js'));
      const message = once(child, 'message');
      child.send('message');
      const [msg] = await message;
      expect(msg).to.equal('object');
    });
    */

    itremote('can be used with streams', () => {
      const originalFs = require('node:original-fs');
      originalFs.createReadStream(path.join(asarDir, 'a.asar'));
    });

    itremote('can recursively delete a directory with an asar file in itremote using rmdirSync', () => {
      const deleteDir = path.join(asarDir, 'deleteme');
      fs.mkdirSync(deleteDir);

      const originalFs = require('node:original-fs');
      originalFs.rmdirSync(deleteDir, { recursive: true });

      expect(fs.existsSync(deleteDir)).to.be.false();
    });

    itremote('can recursively delete a directory with an asar file in itremote using promises.rmdir', async () => {
      const deleteDir = path.join(asarDir, 'deleteme');
      fs.mkdirSync(deleteDir);

      const originalFs = require('node:original-fs');
      await originalFs.promises.rmdir(deleteDir, { recursive: true });

      expect(fs.existsSync(deleteDir)).to.be.false();
    });

    itremote('has the same APIs as fs', function () {
      expect(Object.keys(require('node:fs'))).to.deep.equal(Object.keys(require('node:original-fs')));
      expect(Object.keys(require('node:fs').promises)).to.deep.equal(Object.keys(require('node:original-fs').promises));
    });
  });

  describe('graceful-fs module', function () {
    itremote('recognize asar archives', function () {
      const gfs = require('graceful-fs');

      const p = path.join(asarDir, 'a.asar', 'link1');
      expect(gfs.readFileSync(p).toString().trim()).to.equal('file1');
    });
    itremote('does not touch global fs object', function () {
      const gfs = require('graceful-fs');
      expect(fs.readdir).to.not.equal(gfs.readdir);
    });
  });

  describe('native-image', function () {
    itremote('reads image from asar archive', function () {
      const p = path.join(asarDir, 'logo.asar', 'logo.png');
      const logo = require('electron').nativeImage.createFromPath(p);
      expect(logo.getSize()).to.deep.equal({
        width: 55,
        height: 55
      });
    });

    itremote('reads image from asar archive with unpacked files', function () {
      const p = path.join(asarDir, 'unpack.asar', 'atom.png');
      const logo = require('electron').nativeImage.createFromPath(p);
      expect(logo.getSize()).to.deep.equal({
        width: 1024,
        height: 1024
      });
    });
  });
});
