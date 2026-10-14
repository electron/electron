import { BrowserWindow, ipcMain } from 'electron/main';

import { afterAll, afterEach, beforeEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as importedFs from 'node:fs';
import { createRequire } from 'node:module';
import * as os from 'node:os';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';
import * as url from 'node:url';
import { Worker } from 'node:worker_threads';

import { defer, getRemoteContext, ifdescribe, ifit, itremote, useRemoteContext } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

const require = createRequire(import.meta.url);

const features = process._linkedBinding('electron_common_features');

describe('asar package', () => {
  const fixtures = path.join(import.meta.dirname, 'fixtures');
  const asarDir = path.join(fixtures, 'test.asar');

  afterEach(closeAllWindows);

  describe('asar protocol', () => {
    it('sets __dirname correctly', async () => {
      afterAll(() => {
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
      expect(dirname).toBe(path.dirname(p));
    });

    it('loads script tag in html', async () => {
      afterAll(() => {
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
      expect(message).toBe('pong');
    });

    it('loads video tag in html', { timeout: 60000 }, async () => {
      afterAll(() => {
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
        expect(error).toBeNull();
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
      expect(state).toBe('completed');
      expect(item.getFilename()).toBe('ping.js');
      expect(importedFs.readFileSync(savePath, 'utf8')).toBe(importedFs.readFileSync(src, 'utf8'));
    });

    ifit(features.isPDFViewerEnabled())('saves a packed PDF from the PDF viewer', async () => {
      const w = new BrowserWindow({ show: false });
      const src = path.join(asarDir, 'pdf.asar', 'cat.pdf');
      const savePath = path.join(importedFs.mkdtempSync(path.join(os.tmpdir(), 'asar-pdf-')), 'saved.pdf');
      const willDownload = once(w.webContents.session, 'will-download');
      // The click loop below can start more than one download. Give the first
      // its save path here, synchronously, and refuse the rest, so none of them
      // ever falls through to the native Save dialog.
      let started = false;
      const onWillDownload = (event: Electron.Event, item: Electron.DownloadItem) => {
        if (started) {
          event.preventDefault();
        } else {
          started = true;
          item.savePath = savePath;
        }
      };
      w.webContents.session.on('will-download', onWillDownload);
      defer(() => w.webContents.session.off('will-download', onWillDownload));
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
      expect(downloading, 'the viewer never started a download').toBeInstanceOf(Array);
      const item = downloading![1] as Electron.DownloadItem;
      const [, state] = await once(item, 'done');
      expect(state).toBe('completed');
      expect(item.getFilename()).toBe('cat.pdf');
      expect(importedFs.readFileSync(savePath).equals(importedFs.readFileSync(path.join(fixtures, 'cat.pdf')))).toBe(
        true
      );
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
      expect(result).toBe('success');
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
      expect(result).toBe('success');
    });
  });

  describe('worker threads', () => {
    // DISABLED-FIXME(#38192): only disabled for ASan.
    ifit(!process.env.IS_ASAN)('should start worker thread from asar file', async () => {
      const p = path.join(asarDir, 'worker_threads.asar', 'worker.js');
      const w = new Worker(p);

      const [message] = await once(w, 'message');
      expect(message).toBe('ping');
      await w.terminate();
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

describe('asar package', () => {
  const fixtures = path.join(import.meta.dirname, 'fixtures');
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

        const { expect } = require(${JSON.stringify(path.join(import.meta.dirname, 'lib', 'vitest-cjs.cjs'))})
        expect(error).to.have.property('code').which.equals(code);
      }

      fs = require('node:fs')
      path = require('node:path')
      fixtures = ${JSON.stringify(fixtures)}
      asarDir = ${JSON.stringify(asarDir)}

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

  describe('node api', () => {
    itremote('supports paths specified as a Buffer', () => {
      const file = Buffer.from(path.join(asarDir, 'a.asar', 'file1'));
      expect(fs.existsSync(file)).toBe(true);
    });

    describe('fs.readFileSync', () => {
      itremote('does not leak fd', () => {
        let readCalls = 1;
        while (readCalls <= 10000) {
          fs.readFileSync(path.join(process.resourcesPath, 'default_app.asar', 'main.js'));
          readCalls++;
        }
      });

      itremote('reads a normal file', () => {
        const file1 = path.join(asarDir, 'a.asar', 'file1');
        expect(fs.readFileSync(file1).toString().trim()).toBe('file1');
        const file2 = path.join(asarDir, 'a.asar', 'file2');
        expect(fs.readFileSync(file2).toString().trim()).toBe('file2');
        const file3 = path.join(asarDir, 'a.asar', 'file3');
        expect(fs.readFileSync(file3).toString().trim()).toBe('file3');
      });

      itremote('reads from a empty file', () => {
        const file = path.join(asarDir, 'empty.asar', 'file1');
        const buffer = fs.readFileSync(file);
        expect(buffer).toHaveLength(0);
        expect(buffer.toString()).toBe('');
      });

      itremote('reads a linked file', () => {
        const p = path.join(asarDir, 'a.asar', 'link1');
        expect(fs.readFileSync(p).toString().trim()).toBe('file1');
      });

      itremote('reads a file from linked directory', () => {
        const p1 = path.join(asarDir, 'a.asar', 'link2', 'file1');
        expect(fs.readFileSync(p1).toString().trim()).toBe('file1');
        const p2 = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        expect(fs.readFileSync(p2).toString().trim()).toBe('file1');
      });

      itremote('throws ENOENT error when can not find file', () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.readFileSync(p);
        }).toThrow(/ENOENT/);
      });

      itremote('passes ENOENT error to callback when can not find file', () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        let async = false;
        fs.readFile(p, function (error) {
          expect(async).toBe(true);
          expect(error).toHaveProperty('code', 'ENOENT');
        });
        async = true;
      });

      itremote('reads a normal file with unpacked files', () => {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        expect(fs.readFileSync(p).toString().trim()).toBe('a');
      });

      itremote('reads a file in filesystem', () => {
        const p = path.resolve(asarDir, 'file');
        expect(fs.readFileSync(p).toString().trim()).toBe('file');
      });
    });

    describe('archives with self-referential link entries', () => {
      // Guard against a missing/renamed fixture silently passing the ENOENT
      // assertions below: a path inside a non-existent .asar would also throw
      // ENOENT. Assert the archive file itself is present first.
      itremote('has the link-cycle fixtures on disk', () => {
        // original-fs bypasses the asar wrapper so the archive file is stat'd
        // as a plain file rather than resolved as an archive root.
        const originalFs = require('original-fs') as typeof importedFs;
        for (const name of ['cyclic-link.asar', 'cyclic-link2.asar', 'cyclic-dir-link.asar']) {
          const archive = path.join(fixtures, 'asar', name);
          expect(originalFs.statSync(archive).isFile(), `${name} fixture missing`).toBe(true);
        }
      });

      itremote('throws instead of hanging on a self-linked file', () => {
        const p = path.join(fixtures, 'asar', 'cyclic-link.asar', 'a');
        expect(() => {
          fs.readFileSync(p);
        }).toThrow(/ENOENT/);
      });

      itremote('throws instead of hanging on a two-node link cycle', () => {
        const p = path.join(fixtures, 'asar', 'cyclic-link2.asar', 'a');
        expect(() => {
          fs.readFileSync(p);
        }).toThrow(/ENOENT/);
      });

      itremote('throws instead of hanging on a link that resolves through itself', () => {
        const p = path.join(fixtures, 'asar', 'cyclic-dir-link.asar', 'a', 'b');
        expect(() => {
          fs.readFileSync(p);
        }).toThrow(/ENOENT/);
      });

      itremote('reports the missing entry from statSync without hanging', () => {
        const p = path.join(fixtures, 'asar', 'cyclic-dir-link.asar', 'a', 'b');
        expect(() => {
          fs.statSync(p);
        }).toThrow(/ENOENT/);
      });
    });

    describe('fs.readFile', () => {
      itremote('reads a normal file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const content = await new Promise((resolve, reject) =>
          fs.readFile(p, (err, content) => {
            if (err) return reject(err);
            resolve(content);
          })
        );
        expect(String(content).trim()).toBe('file1');
      });

      itremote('reads from a empty file', async () => {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        const content = await new Promise((resolve, reject) =>
          fs.readFile(p, (err, content) => {
            if (err) return reject(err);
            resolve(content);
          })
        );
        expect(String(content)).toBe('');
      });

      itremote('reads from a empty file with encoding', async () => {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        const content = await new Promise((resolve, reject) =>
          fs.readFile(p, (err, content) => {
            if (err) return reject(err);
            resolve(content);
          })
        );
        expect(String(content)).toBe('');
      });

      itremote('reads a linked file', async () => {
        const p = path.join(asarDir, 'a.asar', 'link1');
        const content = await new Promise((resolve, reject) =>
          fs.readFile(p, (err, content) => {
            if (err) return reject(err);
            resolve(content);
          })
        );
        expect(String(content).trim()).toBe('file1');
      });

      itremote('reads a file from linked directory', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        const content = await new Promise((resolve, reject) =>
          fs.readFile(p, (err, content) => {
            if (err) return reject(err);
            resolve(content);
          })
        );
        expect(String(content).trim()).toBe('file1');
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.readFile(p, resolve));
        expect(err.code).toBe('ENOENT');
      });
    });

    describe('fs.promises.readFile', () => {
      itremote('reads a normal file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const content = await fs.promises.readFile(p);
        expect(String(content).trim()).toBe('file1');
      });

      itremote('reads from a empty file', async () => {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        const content = await fs.promises.readFile(p);
        expect(String(content)).toBe('');
      });

      itremote('reads from a empty file with encoding', async () => {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        const content = await fs.promises.readFile(p, 'utf8');
        expect(content).toBe('');
      });

      itremote('reads a linked file', async () => {
        const p = path.join(asarDir, 'a.asar', 'link1');
        const content = await fs.promises.readFile(p);
        expect(String(content).trim()).toBe('file1');
      });

      itremote('reads a file from linked directory', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        const content = await fs.promises.readFile(p);
        expect(String(content).trim()).toBe('file1');
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.readFile(p), 'ENOENT');
      });
    });

    describe('fs.copyFile', () => {
      itremote('copies a normal file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        const dest = temp.path();
        await new Promise<void>((resolve, reject) => {
          fs.copyFile(p, dest, (err) => {
            if (err) reject(err);
            else resolve();
          });
        });
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });

      itremote('copies a unpacked file', async () => {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const temp = require('temp').track();
        const dest = temp.path();
        await new Promise<void>((resolve, reject) => {
          fs.copyFile(p, dest, (err) => {
            if (err) reject(err);
            else resolve();
          });
        });
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });
    });

    describe('fs.promises.copyFile', () => {
      itremote('copies a normal file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        const dest = temp.path();
        await fs.promises.copyFile(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });

      itremote('copies a unpacked file', async () => {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const temp = require('temp').track();
        const dest = temp.path();
        await fs.promises.copyFile(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });
    });

    describe('fs.copyFileSync', () => {
      itremote('copies a normal file', () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        const dest = temp.path();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });

      itremote('copies a unpacked file', () => {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const temp = require('temp').track();
        const dest = temp.path();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });
    });

    describe('fs.cpSync', () => {
      itremote('copies a normal file', () => {
        if (!fs.cpSync) return;
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        const dest = temp.path();
        fs.cpSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });
    });

    describe('fs.cp', () => {
      itremote('copies a normal file', async () => {
        if (!fs.cp) return;
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        const dest = temp.path();
        await new Promise<void>((resolve, reject) => {
          fs.cp(p, dest, (err) => (err ? reject(err) : resolve()));
        });
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });
    });

    describe('fs.promises.cp', () => {
      itremote('copies a normal file', async () => {
        if (!fs.promises.cp) return;
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        const dest = temp.path();
        await fs.promises.cp(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });
    });

    describe('fs.lstatSync', () => {
      itremote('handles path with trailing slash correctly', () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        fs.lstatSync(p);
        fs.lstatSync(p + '/');
      });

      itremote('returns information of root', () => {
        const p = path.join(asarDir, 'a.asar');
        const stats = fs.lstatSync(p);
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(true);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(0);
      });

      itremote('returns stat properties with types matching a real file', () => {
        const asarStats = fs.lstatSync(path.join(asarDir, 'a.asar', 'file1'));
        const realStats = fs.lstatSync(path.join(fixtures, 'test.asar', 'a.asar'));
        for (const key of Object.keys(realStats) as (keyof typeof realStats)[]) {
          expect(typeof asarStats[key], `typeof stats.${key}`).toBe(typeof realStats[key]);
        }
      });

      itremote('returns information of root with stats as bigint', () => {
        const p = path.join(asarDir, 'a.asar');
        const stats = fs.lstatSync(p, { bigint: false });
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(true);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(0);
      });

      itremote('returns information of a normal file', () => {
        const ref2 = ['file1', 'file2', 'file3', path.join('dir1', 'file1'), path.join('link2', 'file1')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).toBe(true);
          expect(stats.isDirectory()).toBe(false);
          expect(stats.isSymbolicLink()).toBe(false);
          expect(stats.size).toBe(6);
        }
      });

      itremote('returns information of a normal directory', () => {
        const ref2 = ['dir1', 'dir2', 'dir3'];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).toBe(false);
          expect(stats.isDirectory()).toBe(true);
          expect(stats.isSymbolicLink()).toBe(false);
          expect(stats.size).toBe(0);
        }
      });

      itremote('returns information of a linked file', () => {
        const ref2 = ['link1', path.join('dir1', 'link1'), path.join('link2', 'link2')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).toBe(false);
          expect(stats.isDirectory()).toBe(false);
          expect(stats.isSymbolicLink()).toBe(true);
          expect(stats.size).toBe(0);
        }
      });

      itremote('returns information of a linked directory', () => {
        const ref2 = ['link2', path.join('dir1', 'link2'), path.join('link2', 'link2')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).toBe(false);
          expect(stats.isDirectory()).toBe(false);
          expect(stats.isSymbolicLink()).toBe(true);
          expect(stats.size).toBe(0);
        }
      });

      itremote('throws ENOENT error when can not find file', () => {
        const ref2 = ['file4', 'file5', path.join('dir1', 'file4')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          expect(() => {
            fs.lstatSync(p);
          }).toThrow(/ENOENT/);
        }
      });

      itremote('returns null when can not find file with throwIfNoEntry === false', () => {
        const ref2 = ['file4', 'file5', path.join('dir1', 'file4')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          expect(fs.lstatSync(p, { throwIfNoEntry: false })).toBe(null);
        }
      });
    });

    describe('fs.lstat', () => {
      itremote('handles path with trailing slash correctly', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        await promisify(fs.lstat)(p + '/');
      });

      itremote('returns information of root', async () => {
        const p = path.join(asarDir, 'a.asar');
        const stats = await promisify(fs.lstat)(p);
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(true);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(0);
      });

      itremote('returns information of root with stats as bigint', async () => {
        const p = path.join(asarDir, 'a.asar');
        const stats = await promisify(fs.lstat)(p, { bigint: false });
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(true);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(0);
      });

      itremote('returns information of a normal file', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'file1');
        const stats = await promisify(fs.lstat)(p);
        expect(stats.isFile()).toBe(true);
        expect(stats.isDirectory()).toBe(false);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(6);
      });

      itremote('returns information of a normal directory', async () => {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const stats = await promisify(fs.lstat)(p);
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(true);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(0);
      });

      itremote('returns information of a linked file', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link1');
        const stats = await promisify(fs.lstat)(p);
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(false);
        expect(stats.isSymbolicLink()).toBe(true);
        expect(stats.size).toBe(0);
      });

      itremote('returns information of a linked directory', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const stats = await promisify(fs.lstat)(p);
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(false);
        expect(stats.isSymbolicLink()).toBe(true);
        expect(stats.size).toBe(0);
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file4');
        const err = await new Promise<any>((resolve) => fs.lstat(p, resolve));
        expect(err.code).toBe('ENOENT');
      });
    });

    describe('fs.promises.lstat', () => {
      itremote('handles path with trailing slash correctly', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        await fs.promises.lstat(p + '/');
      });

      itremote('returns information of root', async () => {
        const p = path.join(asarDir, 'a.asar');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(true);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(0);
      });

      itremote('returns information of root with stats as bigint', async () => {
        const p = path.join(asarDir, 'a.asar');
        const stats = await fs.promises.lstat(p, { bigint: false });
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(true);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(0);
      });

      itremote('returns information of a normal file', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'file1');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).toBe(true);
        expect(stats.isDirectory()).toBe(false);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(6);
      });

      itremote('returns information of a normal directory', async () => {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(true);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(0);
      });

      itremote('returns information of a linked file', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link1');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(false);
        expect(stats.isSymbolicLink()).toBe(true);
        expect(stats.size).toBe(0);
      });

      itremote('returns information of a linked directory', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).toBe(false);
        expect(stats.isDirectory()).toBe(false);
        expect(stats.isSymbolicLink()).toBe(true);
        expect(stats.size).toBe(0);
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file4');
        await expectToThrowErrorWithCode(() => fs.promises.lstat(p), 'ENOENT');
      });
    });

    describe('fs.realpathSync', () => {
      itremote('returns real path root', () => {
        const parent = fs.realpathSync(asarDir);
        const p = 'a.asar';
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a normal file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a normal directory', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a linked file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).toBe(path.join(parent, 'a.asar', 'file1'));
      });

      itremote('returns real path of a linked directory', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).toBe(path.join(parent, 'a.asar', 'dir1'));
      });

      itremote('returns real path of an unpacked file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('throws ENOENT error when can not find file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'not-exist');
        expect(() => {
          fs.realpathSync(path.join(parent, p));
        }).toThrow(/ENOENT/);
      });
    });

    describe('fs.realpathSync.native', () => {
      itremote('returns real path root', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = 'a.asar';
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a normal file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a normal directory', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a linked file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).toBe(path.join(parent, 'a.asar', 'file1'));
      });

      itremote('returns real path of a linked directory', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).toBe(path.join(parent, 'a.asar', 'dir1'));
      });

      itremote('returns real path of an unpacked file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('throws ENOENT error when can not find file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'not-exist');
        expect(() => {
          fs.realpathSync.native(path.join(parent, p));
        }).toThrow(/ENOENT/);
      });
    });

    describe('fs.realpath', () => {
      itremote('returns real path root', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = 'a.asar';
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a normal file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a normal directory', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a linked file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).toBe(path.join(parent, 'a.asar', 'file1'));
      });

      itremote('returns real path of a linked directory', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).toBe(path.join(parent, 'a.asar', 'dir1'));
      });

      itremote('returns real path of an unpacked file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = await promisify(fs.realpath)(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.realpath(path.join(parent, p), resolve));
        expect(err.code).toBe('ENOENT');
      });
    });

    describe('fs.promises.realpath', () => {
      itremote('returns real path root', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = 'a.asar';
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a normal file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a normal directory', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a linked file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).toBe(path.join(parent, 'a.asar', 'file1'));
      });

      itremote('returns real path of a linked directory', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).toBe(path.join(parent, 'a.asar', 'dir1'));
      });

      itremote('returns real path of an unpacked file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
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
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a normal file', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a normal directory', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('returns real path of a linked file', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).toBe(path.join(parent, 'a.asar', 'file1'));
      });

      itremote('returns real path of a linked directory', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).toBe(path.join(parent, 'a.asar', 'dir1'));
      });

      itremote('returns real path of an unpacked file', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = await promisify(fs.realpath.native)(path.join(parent, p));
        expect(r).toBe(path.join(parent, p));
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.realpath.native(path.join(parent, p), resolve));
        expect(err.code).toBe('ENOENT');
      });
    });

    describe('fs.readdirSync', () => {
      itremote('reads dirs from root', () => {
        const p = path.join(asarDir, 'a.asar');
        const dirs = fs.readdirSync(p);
        expect(dirs).toEqual(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('supports recursive readdirSync withFileTypes', () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = fs.readdirSync(dir, { recursive: true, withFileTypes: true });

        expect(files).toHaveLength(24);

        for (const file of files) {
          expect(file).toBeInstanceOf(fs.Dirent);
        }

        const paths = files.map((a: any) => a.name);
        expect(paths).toHaveLength(24);
        expect(paths).toEqual(
          expect.arrayContaining([
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
          ])
        );
      });

      itremote('supports recursive readdirSync', () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = fs.readdirSync(dir, { recursive: true });
        expect(files).toHaveLength(24);
        expect(files).toEqual(
          expect.arrayContaining([
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
          ])
        );
      });

      itremote('reads dirs from a normal dir', () => {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const dirs = fs.readdirSync(p);
        expect(dirs).toEqual(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('supports withFileTypes', () => {
        const p = path.join(asarDir, 'a.asar');
        const dirs = fs.readdirSync(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir).toBeInstanceOf(fs.Dirent);
          expect(dir.parentPath).toBe(p);
        }
        const names = dirs.map((a) => a.name);
        expect(names).toEqual(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('supports withFileTypes for a deep directory', () => {
        const p = path.join(asarDir, 'a.asar', 'dir3');
        const dirs = fs.readdirSync(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir).toBeInstanceOf(fs.Dirent);
        }
        const names = dirs.map((a) => a.name);
        expect(names).toEqual(['file1', 'file2', 'file3']);
      });

      itremote('reads dirs from a linked dir', () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const dirs = fs.readdirSync(p);
        expect(dirs).toEqual(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('throws ENOENT error when can not find file', () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.readdirSync(p);
        }).toThrow(/ENOENT/);
      });
    });

    describe('fs.readdir', () => {
      itremote('reads dirs from root', async () => {
        const p = path.join(asarDir, 'a.asar');
        const dirs = await promisify(fs.readdir)(p);
        expect(dirs).toEqual(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('supports recursive readdirSync', async () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = await promisify(fs.readdir)(dir, { recursive: true });
        expect(files).toHaveLength(24);
        expect(files).toEqual(
          expect.arrayContaining([
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
          ])
        );
      });

      itremote('supports readdir withFileTypes', async () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = await promisify(fs.readdir)(dir, { recursive: true, withFileTypes: true });

        expect(files).toHaveLength(24);

        for (const file of files) {
          expect(file).toBeInstanceOf(fs.Dirent);
        }

        const paths = files.map((a: any) => a.name);
        expect(paths).toHaveLength(24);
        expect(paths).toEqual(
          expect.arrayContaining([
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
          ])
        );
      });

      itremote('supports withFileTypes', async () => {
        const p = path.join(asarDir, 'a.asar');

        const dirs = await promisify(fs.readdir)(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir).toBeInstanceOf(fs.Dirent);
          expect(dir.parentPath).toBe(p);
        }

        const names = dirs.map((a: any) => a.name);
        expect(names).toEqual(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('reads dirs from a normal dir', async () => {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const dirs = await promisify(fs.readdir)(p);
        expect(dirs).toEqual(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('reads dirs from a linked dir', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const dirs = await promisify(fs.readdir)(p);
        expect(dirs).toEqual(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.readdir(p, resolve));
        expect(err.code).toBe('ENOENT');
      });

      it('handles null for options', () =>
        new Promise<void>((resolve, reject) => {
          const done = (error?: unknown) => (error ? reject(error) : resolve());
          const p = path.join(asarDir, 'a.asar', 'dir1');
          fs.readdir(p, null, function (err, dirs) {
            try {
              expect(err).toBeNull();
              expect(dirs).toEqual(['file1', 'file2', 'file3', 'link1', 'link2']);
              done();
            } catch (e) {
              done(e);
            }
          });
        }));

      it('handles undefined for options', () =>
        new Promise<void>((resolve, reject) => {
          const done = (error?: unknown) => (error ? reject(error) : resolve());
          const p = path.join(asarDir, 'a.asar', 'dir1');
          fs.readdir(p, undefined, function (err, dirs) {
            try {
              expect(err).toBeNull();
              expect(dirs).toEqual(['file1', 'file2', 'file3', 'link1', 'link2']);
              done();
            } catch (e) {
              done(e);
            }
          });
        }));
    });

    describe('fs.promises.readdir', () => {
      itremote('reads dirs from root', async () => {
        const p = path.join(asarDir, 'a.asar');
        const dirs = await fs.promises.readdir(p);
        expect(dirs).toEqual(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('supports recursive readdir', async () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = await fs.promises.readdir(dir, { recursive: true });
        expect(files).toHaveLength(24);
        expect(files).toEqual(
          expect.arrayContaining([
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
          ])
        );
      });

      itremote('supports readdir withFileTypes', async () => {
        const dir = path.join(fixtures, 'recursive-asar');
        const files = await fs.promises.readdir(dir, { recursive: true, withFileTypes: true });

        expect(files).toHaveLength(24);

        for (const file of files) {
          expect(file).toBeInstanceOf(fs.Dirent);
        }

        const paths = files.map((a: any) => a.name);
        expect(paths).toHaveLength(24);
        expect(paths).toEqual(
          expect.arrayContaining([
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
          ])
        );
      });

      itremote('supports withFileTypes', async () => {
        const p = path.join(asarDir, 'a.asar');
        const dirs = await fs.promises.readdir(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir).toBeInstanceOf(fs.Dirent);
          expect(dir.parentPath).toBe(p);
        }
        const names = dirs.map((a) => a.name);
        expect(names).toEqual(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      itremote('reads dirs from a normal dir', async () => {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const dirs = await fs.promises.readdir(p);
        expect(dirs).toEqual(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('reads dirs from a linked dir', async () => {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const dirs = await fs.promises.readdir(p);
        expect(dirs).toEqual(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.readdir(p), 'ENOENT');
      });
    });

    describe('fs.globSync', () => {
      itremote('supports withFileTypes with a cwd inside an asar archive', () => {
        const cwd = path.join(asarDir, 'a.asar');
        const dirents = fs.globSync('*.js', { cwd, withFileTypes: true });
        expect(dirents).toHaveLength(1);
        expect(dirents[0]).toBeInstanceOf(fs.Dirent);
        expect(dirents[0].name).toBe('ping.js');
        expect(dirents[0].parentPath).toBe(cwd);
      });
    });

    describe('fs.glob', () => {
      itremote('supports withFileTypes with a cwd inside an asar archive', async () => {
        const cwd = path.join(asarDir, 'a.asar');
        const dirents = await promisify(fs.glob)('*.js', { cwd, withFileTypes: true });
        expect(dirents).toHaveLength(1);
        expect(dirents[0]).toBeInstanceOf(fs.Dirent);
        expect(dirents[0].name).toBe('ping.js');
        expect(dirents[0].parentPath).toBe(cwd);
      });
    });

    describe('fs.openSync', () => {
      itremote('opens a normal/linked/under-linked-directory file', () => {
        const ref2 = ['file1', 'link1', path.join('link2', 'file1')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const fd = fs.openSync(p, 'r');
          const buffer = Buffer.alloc(6);
          fs.readSync(fd, buffer, 0, 6, 0);
          expect(String(buffer).trim()).toBe('file1');
          fs.closeSync(fd);
        }
      });

      itremote('throws ENOENT error when can not find file', () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          (fs.openSync as any)(p);
        }).toThrow(/ENOENT/);
      });
    });

    describe('fs.open', () => {
      itremote('opens a normal file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = await promisify(fs.open)(p, 'r');
        const buffer = Buffer.alloc(6);
        await promisify(fs.read)(fd, buffer, 0, 6, 0);
        expect(String(buffer).trim()).toBe('file1');
        await promisify(fs.close)(fd);
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.open(p, 'r', resolve));
        expect(err.code).toBe('ENOENT');
      });
    });

    describe('fs.promises.open', () => {
      itremote('opens a normal file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fh = await fs.promises.open(p, 'r');
        const buffer = Buffer.alloc(6);
        await fh.read(buffer, 0, 6, 0);
        expect(String(buffer).trim()).toBe('file1');
        await fh.close();
      });

      itremote('throws ENOENT error when can not find file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.open(p, 'r'), 'ENOENT');
      });
    });

    describe('fs.mkdir', () => {
      itremote('throws error when calling inside asar archive', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.mkdir(p, resolve));
        expect(err.code).toBe('ENOTDIR');
      });
    });

    describe('fs.promises.mkdir', () => {
      itremote('throws error when calling inside asar archive', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.mkdir(p), 'ENOTDIR');
      });
    });

    describe('fs.mkdirSync', () => {
      itremote('throws error when calling inside asar archive', () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.mkdirSync(p);
        }).toThrow(/ENOTDIR/);
      });
    });

    describe('fs.exists', () => {
      itremote('handles an existing file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const exists = await new Promise((resolve) => fs.exists(p, resolve));
        expect(exists).toBe(true);
      });

      itremote('handles a non-existent file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const exists = await new Promise((resolve) => fs.exists(p, resolve));
        expect(exists).toBe(false);
      });

      itremote('promisified version handles an existing file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const exists = await require('node:util').promisify(fs.exists)(p);
        expect(exists).toBe(true);
      });

      itremote('promisified version handles a non-existent file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const exists = await require('node:util').promisify(fs.exists)(p);
        expect(exists).toBe(false);
      });
    });

    describe('fs.existsSync', () => {
      itremote('handles an existing file', () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(fs.existsSync(p)).toBe(true);
      });

      itremote('handles a non-existent file', () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(fs.existsSync(p)).toBe(false);
      });
    });

    describe('fs.access', () => {
      itremote('accesses a normal file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        await promisify(fs.access)(p);
      });

      itremote('throws an error when called with write mode', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const err = await new Promise<any>((resolve) => fs.access(p, fs.constants.R_OK | fs.constants.W_OK, resolve));
        expect(err.code).toBe('EACCES');
      });

      itremote('throws an error when called on non-existent file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        const err = await new Promise<any>((resolve) => fs.access(p, fs.constants.R_OK | fs.constants.W_OK, resolve));
        expect(err.code).toBe('ENOENT');
      });

      itremote('allows write mode for unpacked files', async () => {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        await promisify(fs.access)(p, fs.constants.R_OK | fs.constants.W_OK);
      });
    });

    describe('fs.promises.access', () => {
      itremote('accesses a normal file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        await fs.promises.access(p);
      });

      itremote('throws an error when called with write mode', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        await expectToThrowErrorWithCode(() => fs.promises.access(p, fs.constants.R_OK | fs.constants.W_OK), 'EACCES');
      });

      itremote('throws an error when called on non-existent file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.access(p), 'ENOENT');
      });

      itremote('allows write mode for unpacked files', async () => {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        await fs.promises.access(p, fs.constants.R_OK | fs.constants.W_OK);
      });
    });

    describe('fs.accessSync', () => {
      itremote('accesses a normal file', () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => {
          fs.accessSync(p);
        }).not.toThrow();
      });

      itremote('throws an error when called with write mode', () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => {
          fs.accessSync(p, fs.constants.R_OK | fs.constants.W_OK);
        }).toThrow(/EACCES/);
      });

      itremote('throws an error when called on non-existent file', () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.accessSync(p);
        }).toThrow(/ENOENT/);
      });

      itremote('allows write mode for unpacked files', () => {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        expect(() => {
          fs.accessSync(p, fs.constants.R_OK | fs.constants.W_OK);
        }).not.toThrow();
      });
    });

    function generateSpecs(childProcess: string) {
      describe(`${childProcess}.fork`, () => {
        itremote(
          'opens a normal js file',
          async function (childProcess: string) {
            const child = require(childProcess).fork(path.join(asarDir, 'a.asar', 'ping.js'));
            child.send('message');
            const msg = await new Promise((resolve) => child.once('message', resolve));
            expect(msg).toBe('message');
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
            expect(content).toBe(fs.readFileSync(file).toString());
          },
          [childProcess, fixtures]
        );
      });

      describe(`${childProcess}.exec`, () => {
        itremote(
          'should not try to extract the command if there is a reference to a file inside an .asar',
          async function (childProcess: string) {
            const echo = path.join(asarDir, 'echo.asar', 'echo');

            const stdout = await promisify(require(childProcess).exec)('echo ' + echo + ' foo bar');
            expect(stdout.toString().replaceAll('\r', '')).toBe(echo + ' foo bar\n');
          },
          [childProcess]
        );
      });

      describe(`${childProcess}.execSync`, () => {
        itremote(
          'should not try to extract the command if there is a reference to a file inside an .asar',
          async function (childProcess: string) {
            const echo = path.join(asarDir, 'echo.asar', 'echo');

            const stdout = require(childProcess).execSync('echo ' + echo + ' foo bar');
            expect(stdout.toString().replaceAll('\r', '')).toBe(echo + ' foo bar\n');
          },
          [childProcess]
        );
      });

      ifdescribe(process.platform === 'darwin' && process.arch !== 'arm64')(`${childProcess}.execFile`, () => {
        itremote(
          'executes binaries',
          async function (childProcess: string) {
            const echo = path.join(asarDir, 'echo.asar', 'echo');
            const stdout = await promisify(require(childProcess).execFile)(echo, ['test']);
            expect(stdout).toBe('test\n');
          },
          [childProcess]
        );

        itremote(
          'executes binaries without callback',
          async function (childProcess: string) {
            const echo = path.join(asarDir, 'echo.asar', 'echo');
            const process = require(childProcess).execFile(echo, ['test']);
            const code = await new Promise((resolve, reject) => {
              process.once('close', resolve);
              process.once('error', reject);
            });
            expect(code).toBe(0);
          },
          [childProcess]
        );

        itremote(
          'execFileSync executes binaries',
          function (childProcess: string) {
            const echo = path.join(asarDir, 'echo.asar', 'echo');
            const output = require(childProcess).execFileSync(echo, ['test']);
            expect(String(output)).toBe('test\n');
          },
          [childProcess]
        );
      });
    }

    generateSpecs('child_process');
    generateSpecs('node:child_process');

    describe('util.promisify', () => {
      itremote('can promisify all fs functions', () => {
        const originalFs = require('original-fs');
        const util = require('node:util');

        for (const [propertyName, originalValue] of Object.entries(originalFs)) {
          // Some properties exist but have a value of `undefined` on some platforms.
          // E.g. `fs.lchmod`, which in only available on MacOS, see
          // https://nodejs.org/docs/latest-v10.x/api/fs.html#fs_fs_lchmod_path_mode_callback
          // Also check for `null`s, `hasOwnProperty()` can't handle them.
          if (typeof originalValue === 'undefined' || originalValue === null) continue;

          if (Object.hasOwn(originalValue, util.promisify.custom)) {
            expect(fs).toHaveProperty([propertyName]);
            expect(Object.hasOwn((fs as any)[propertyName], util.promisify.custom), propertyName).toBe(true);
          }
        }
      });
    });

    describe('splitPath', () => {
      itremote('splits at the deepest .asar file component and normalizes the relative part', () => {
        const { splitPath } = process._linkedBinding('electron_common_asar');
        const archive = path.join(asarDir, 'a.asar');
        expect(splitPath(path.join(archive, 'dir1', 'file1'))).toEqual({
          isAsar: true,
          asarPath: archive,
          filePath: ['dir1', 'file1'].join(path.sep)
        });
        expect(splitPath(archive + path.sep + path.sep + 'dir1' + path.sep + path.sep + 'file1' + path.sep)).toEqual({
          isAsar: true,
          asarPath: archive,
          filePath: ['dir1', 'file1'].join(path.sep)
        });
        expect(splitPath(path.join(archive, 'nested.asar', 'x'))).toEqual({
          isAsar: true,
          asarPath: path.join(archive, 'nested.asar'),
          filePath: 'x'
        });
        expect(splitPath(archive)).toEqual({ isAsar: true, asarPath: archive, filePath: '' });
      });

      itremote('does not treat a real directory named like an archive as an archive', () => {
        const { splitPath } = process._linkedBinding('electron_common_asar');
        expect(splitPath(path.join(asarDir, 'file'))).toEqual({ isAsar: false });
        expect(splitPath(asarDir)).toEqual({ isAsar: false });
        expect(splitPath(path.join(fixtures, 'module', 'noop.js'))).toEqual({ isAsar: false });
      });

      itremote('matches the archive extension the same way base::FilePath does', () => {
        const { splitPath } = process._linkedBinding('electron_common_asar');
        const dir = path.join(fixtures, 'module');
        expect(splitPath(path.join(dir, 'X.ASAR', 'y')).isAsar).toBe(true);
        expect(splitPath(path.join(dir, '.asar', 'y')).isAsar).toBe(true);
        expect(splitPath(path.join(dir, 'x.asar.gz', 'y')).isAsar).toBe(false);
        expect(splitPath(path.join(dir, 'x.asarx', 'y')).isAsar).toBe(false);
        expect(splitPath(path.join(dir, 'asar', 'y')).isAsar).toBe(false);
      });
    });

    describe('process.noAsar', () => {
      const errorName = process.platform === 'win32' ? 'ENOENT' : 'ENOTDIR';

      beforeEach(async () => {
        return (await getRemoteContext()).webContents.executeJavaScript(`
          process.noAsar = true;
        `);
      });

      afterEach(async () => {
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
          }).toThrow(new RegExp(errorName));
          expect(() => {
            fs.lstatSync(file);
          }).toThrow(new RegExp(errorName));
          expect(() => {
            fs.realpathSync(file);
          }).toThrow(new RegExp(errorName));
          expect(() => {
            fs.readdirSync(dir);
          }).toThrow(new RegExp(errorName));
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
              expect(error?.code).toBe(errorName);
              fs.lstat(file, function (error) {
                expect(error?.code).toBe(errorName);
                fs.realpath(file, function (error) {
                  expect(error?.code).toBe(errorName);
                  fs.readdir(dir, function (error) {
                    expect(error?.code).toBe(errorName);
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
          await expect(fs.promises.readFile(file)).rejects.toThrow(new RegExp(errorName));
          await expect(fs.promises.lstat(file)).rejects.toThrow(new RegExp(errorName));
          await expect(fs.promises.realpath(file)).rejects.toThrow(new RegExp(errorName));
          await expect(fs.promises.readdir(dir)).rejects.toThrow(new RegExp(errorName));
        },
        [errorName]
      );

      itremote('treats *.asar as normal file', () => {
        const originalFs = require('original-fs');
        const asar = path.join(asarDir, 'a.asar');
        const content1 = fs.readFileSync(asar);
        const content2 = originalFs.readFileSync(asar);
        expect(content1.compare(content2)).toBe(0);
        expect(() => {
          fs.readdirSync(asar);
        }).toThrow(/ENOTDIR/);
      });

      itremote('is reset to its original value when execSync throws an error', () => {
        process.noAsar = false;
        expect(() => {
          require('node:child_process').execSync(path.join(__dirname, 'does-not-exist.txt'));
        }).toThrow();
        expect(process.noAsar).toBe(false);
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
            expect(stats.isFile).to.be.true;
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
            expect(stats.isFile).to.be.true;
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

  describe('asar protocol', () => {
    itremote('can request a file in package', async () => {
      const p = path.resolve(asarDir, 'a.asar', 'file1');
      const response = await fetch('file://' + p);
      const data = await response.text();
      expect(data.trim()).toBe('file1');
    });

    itremote('can request a file in package with unpacked files', async () => {
      const p = path.resolve(asarDir, 'unpack.asar', 'a.txt');
      const response = await fetch('file://' + p);
      const data = await response.text();
      expect(data.trim()).toBe('a');
    });

    itremote('can request a linked file in package', async () => {
      const p = path.resolve(asarDir, 'a.asar', 'link2', 'link1');
      const response = await fetch('file://' + p);
      const data = await response.text();
      expect(data.trim()).toBe('file1');
    });

    itremote('can request a file in filesystem', async () => {
      const p = path.resolve(asarDir, 'file');
      const response = await fetch('file://' + p);
      const data = await response.text();
      expect(data.trim()).toBe('file');
    });

    itremote('gets error when file is not found', async () => {
      const p = path.resolve(asarDir, 'a.asar', 'no-exist');
      try {
        const response = await fetch('file://' + p);
        expect(response.status).toBe(404);
      } catch (error: any) {
        expect(error.message).toBe('Failed to fetch');
      }
    });
  });

  describe('original-fs module', () => {
    itremote('treats .asar as file', () => {
      const file = path.join(asarDir, 'a.asar');
      const originalFs = require('original-fs');
      const stats = originalFs.statSync(file);
      expect(stats.isFile()).toBe(true);
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
      const originalFs = require('original-fs');
      originalFs.createReadStream(path.join(asarDir, 'a.asar'));
    });

    itremote('can recursively delete a directory with an asar file in itremote using rmdirSync', () => {
      const deleteDir = path.join(asarDir, 'deleteme');
      fs.mkdirSync(deleteDir);

      const originalFs = require('original-fs');
      originalFs.rmdirSync(deleteDir, { recursive: true });

      expect(fs.existsSync(deleteDir)).toBe(false);
    });

    itremote('can recursively delete a directory with an asar file in itremote using promises.rmdir', async () => {
      const deleteDir = path.join(asarDir, 'deleteme');
      fs.mkdirSync(deleteDir);

      const originalFs = require('original-fs');
      await originalFs.promises.rmdir(deleteDir, { recursive: true });

      expect(fs.existsSync(deleteDir)).toBe(false);
    });

    itremote('has the same APIs as fs', () => {
      expect(Object.keys(require('node:fs'))).toEqual(Object.keys(require('original-fs')));
      expect(Object.keys(require('node:fs').promises)).toEqual(Object.keys(require('original-fs').promises));
    });
  });

  describe('graceful-fs module', () => {
    itremote('recognize asar archives', () => {
      const gfs = require('graceful-fs');

      const p = path.join(asarDir, 'a.asar', 'link1');
      expect(gfs.readFileSync(p).toString().trim()).toBe('file1');
    });
    itremote('does not touch global fs object', () => {
      const gfs = require('graceful-fs');
      expect(fs.readdir).not.toBe(gfs.readdir);
    });
  });

  describe('mkdirp module', () => {
    itremote('throws error when calling inside asar archive', () => {
      const mkdirp = require('mkdirp');

      const p = path.join(asarDir, 'a.asar', 'not-exist');
      expect(() => {
        mkdirp.sync(p);
      }).toThrow(/ENOTDIR/);
    });
  });

  describe('native-image', () => {
    itremote('reads image from asar archive', () => {
      const p = path.join(asarDir, 'logo.asar', 'logo.png');
      const logo = require('electron').nativeImage.createFromPath(p);
      expect(logo.getSize()).toEqual({
        width: 55,
        height: 55
      });
    });

    itremote('reads image from asar archive with unpacked files', () => {
      const p = path.join(asarDir, 'unpack.asar', 'atom.png');
      const logo = require('electron').nativeImage.createFromPath(p);
      expect(logo.getSize()).toEqual({
        width: 1024,
        height: 1024
      });
    });
  });
});
