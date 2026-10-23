import { BrowserWindow, ipcMain } from 'electron/main';

import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as importedFs from 'node:fs';
import { createRequire } from 'node:module';
import * as os from 'node:os';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';
import * as url from 'node:url';
import { Worker } from 'node:worker_threads';

import {
  defer,
  getRemoteContext,
  ifdescribe,
  ifit,
  itremote,
  spawnAndWait,
  useRemoteContext
} from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

const require = createRequire(import.meta.url);

const features = process._linkedBinding('electron_common_features');

describe('asar package', () => {
  const fixtures = path.join(import.meta.dirname, 'fixtures');
  const asarDir = path.join(fixtures, 'test.asar');

  afterEach(closeAllWindows);

  describe('asar protocol', () => {
    // Regression test for https://github.com/electron/electron/pull/52283:
    // contents used to be re-read from whatever file was at the archive's path,
    // using offsets from the header read at startup, so replacing the archive
    // under a running app served the wrong bytes or failed outright.
    // Windows does not let the archive be renamed over while it is held open.
    ifit(process.platform !== 'win32')(
      'keeps serving the archive the app started with after it is replaced on disk',
      async () => {
        const appPath = path.join(fixtures, 'apps', 'asar-replace');
        const initial = path.join(asarDir, 'a.asar');
        const replacement = path.join(asarDir, 'web.asar');
        const { code, stdout, stderr } = await spawnAndWait(
          process.execPath,
          [appPath, initial, replacement, 'file1'],
          { timeout: 20000 }
        );
        const result = stdout.split('\n').find((line) => line.startsWith('{'));
        expect(result, `exit code ${code}, stderr: ${stderr}`).toBeTypeOf('string');
        const { before, after } = JSON.parse(result!);
        const expected = importedFs.readFileSync(path.join(initial, 'file1'), 'utf8');
        expect(before).toEqual({ ok: true, body: expected });
        expect(after).toEqual(before);
        expect(code).toBe(0);
      }
    );

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

  describe('internals', () => {
    const asarBinding: NodeJS.AsarBinding = process._linkedBinding('electron_common_asar');
    const { splitPath } = asarBinding;
    const fs = importedFs;
    // The wrapped fs refuses to create files named *.asar; the fixtures are
    // written through the unwrapped module.
    const originalFs = require('original-fs') as typeof importedFs;
    const sep = path.sep;
    const j = (...parts: string[]) => parts.join(sep);

    // Writes a minimal asar archive: |tree| maps names to file contents
    // (string/Buffer), nested trees (directories) or { link } entries.
    const writeAsar = (file: string, tree: Record<string, any>, patch?: (files: Record<string, any>) => void) => {
      const chunks: Buffer[] = [];
      let offset = 0;
      const build = (node: Record<string, any>): any => {
        const files: Record<string, any> = {};
        for (const [name, value] of Object.entries(node)) {
          if (typeof value === 'string' || Buffer.isBuffer(value)) {
            const data = Buffer.from(value);
            files[name] = { size: data.length, offset: String(offset) };
            if (name.endsWith('.sh')) files[name].executable = true;
            offset += data.length;
            chunks.push(data);
          } else if (typeof value.link === 'string') {
            files[name] = { link: value.link };
          } else {
            files[name] = { files: build(value) };
          }
        }
        return files;
      };
      const files = build(tree);
      patch?.(files);
      const json = Buffer.from(JSON.stringify({ files }));
      const padded = (json.length + 3) & ~3;
      const headerPickle = Buffer.alloc(8 + padded);
      headerPickle.writeUInt32LE(4 + padded, 0);
      headerPickle.writeUInt32LE(json.length, 4);
      json.copy(headerPickle, 8);
      const sizePickle = Buffer.alloc(8);
      sizePickle.writeUInt32LE(4, 0);
      sizePickle.writeUInt32LE(headerPickle.length, 4);
      originalFs.writeFileSync(file, Buffer.concat([sizePickle, headerPickle, ...chunks]));
    };

    const unicodeDir = 'ünï\u{1F642}'; // "ünï🙂"
    const unicodeName = 'ファイル.txt'; // "ファイル.txt"
    const astralName = '\u{1F642}.txt'; // "🙂.txt"
    const tree = {
      'a.txt': 'alpha',
      'run.sh': '#!/bin/sh\n',
      empty: '',
      dir: {
        'b.txt': 'bravo',
        sub: { 'c.txt': 'charlie', deeper: { 'd.txt': 'delta' } },
        'up.lnk': { link: 'a.txt' },
        'self.lnk': { link: 'dir' }
      },
      'file.lnk': { link: 'a.txt' },
      'dir.lnk': { link: 'dir' },
      'chain.lnk': { link: 'file.lnk' },
      'dangling.lnk': { link: 'missing' },
      'loop1.lnk': { link: 'loop2.lnk' },
      'loop2.lnk': { link: 'loop1.lnk' },
      'inner.asar': 'not really an archive',
      [unicodeDir]: { [unicodeName]: 'unicode-content', [astralName]: 'astral' },
      '.hidden': 'dot',
      'a.txt.bak': 'backup'
    };

    let tmp: string;
    let archive: string; // <tmp>/plain/app.asar
    let unicodeArchive: string; // <tmp>/<unicodeDir>/app.asar
    let upperArchive: string; // <tmp>/plain/APP.ASAR
    let dirNamedAsar: string; // <tmp>/looks.asar (a real directory)
    let archiveInDirNamedAsar: string; // <tmp>/looks.asar/real.asar
    let bogusArchive: string; // <tmp>/plain/bogus.asar (a file that is not an archive)

    beforeAll(() => {
      tmp = importedFs.realpathSync(importedFs.mkdtempSync(path.join(os.tmpdir(), 'electron-asar-internals-')));
      originalFs.mkdirSync(j(tmp, 'plain'));
      originalFs.mkdirSync(j(tmp, unicodeDir));
      originalFs.mkdirSync(j(tmp, 'looks.asar'));
      archive = j(tmp, 'plain', 'app.asar');
      unicodeArchive = j(tmp, unicodeDir, 'app.asar');
      upperArchive = j(tmp, 'plain', 'APP.ASAR');
      dirNamedAsar = j(tmp, 'looks.asar');
      archiveInDirNamedAsar = j(tmp, 'looks.asar', 'real.asar');
      bogusArchive = j(tmp, 'plain', 'bogus.asar');
      for (const file of [archive, unicodeArchive, upperArchive, archiveInDirNamedAsar]) writeAsar(file, tree);
      originalFs.writeFileSync(bogusArchive, 'just some bytes');
      importedFs.writeFileSync(j(dirNamedAsar, 'plain.txt'), 'plain');
    });

    afterAll(() => {
      try {
        originalFs.rmSync(tmp, { recursive: true, force: true });
      } catch (error: any) {
        // Every archive this suite has touched stays open for the rest of the
        // process: the asar layer caches its Archive objects, and Archive opens
        // the file without FILE_SHARE_DELETE. Windows will not delete an open
        // file, so the directory can only go once this process has exited.
        if (process.platform !== 'win32' || !['EBUSY', 'EPERM'].includes(error.code)) throw error;
      }
    });

    // Skipped on Windows, where an open archive cannot be renamed.
    ifit(process.platform !== 'win32')(
      'keeps reading an opened archive after its path is replaced by a directory',
      () => {
        const swapped = j(tmp, 'swap', 'app.asar');
        originalFs.mkdirSync(j(tmp, 'swap'));
        writeAsar(swapped, { 'a.txt': 'from the archive' });
        expect(new asarBinding.Archive(swapped)).toBeTruthy();
        originalFs.renameSync(swapped, swapped + '.orig');
        originalFs.mkdirSync(swapped);
        originalFs.writeFileSync(j(swapped, 'a.txt'), 'from a plain directory');
        expect(fs.readFileSync(j(swapped, 'a.txt'), 'utf8')).toBe('from the archive');
      }
    );

    // Skipped on Windows, where an open archive cannot be renamed.
    ifit(process.platform !== 'win32')(
      'keeps treating an opened archive as an archive after an earlier directory probe',
      () => {
        const swapped = j(tmp, 'previously-directory.asar');
        const entry = j(swapped, 'a.txt');

        originalFs.mkdirSync(swapped);
        expect(splitPath(entry, true)).toBe(-1);
        originalFs.rmdirSync(swapped);

        writeAsar(swapped, { 'a.txt': 'from the archive' });
        expect(new asarBinding.Archive(swapped)).toBeTruthy();

        originalFs.renameSync(swapped, swapped + '.orig');
        originalFs.mkdirSync(swapped);
        originalFs.writeFileSync(entry, 'from a plain directory');

        expect(splitPath(entry, true)).toBe(swapped.length);
        expect(fs.readFileSync(entry, 'utf8')).toBe('from the archive');
      }
    );

    describe('splitPath (native archive prefix detection)', () => {
      it('finds the archive component and returns its length in JS string units', () => {
        const cases: [string, number][] = [
          [j(archive, 'a.txt'), archive.length],
          [j(archive, 'dir', 'sub', 'deeper', 'd.txt'), archive.length],
          [archive, archive.length],
          [archive + sep, archive.length],
          [archive + sep + sep, archive.length],
          [j(unicodeArchive, unicodeDir, astralName), unicodeArchive.length],
          [unicodeArchive, unicodeArchive.length],
          [j(upperArchive, 'a.txt'), upperArchive.length],
          [j(archiveInDirNamedAsar, 'dir', 'b.txt'), archiveInDirNamedAsar.length]
        ];
        for (const [input, expected] of cases) {
          expect(splitPath(input, true), input).toBe(expected);
          expect(splitPath(input, false), input).toBe(expected);
        }
      });

      it('returns -1 for paths that are not inside an archive', () => {
        for (const input of [
          j(tmp, 'plain'),
          j(tmp, 'plain', 'nope.txt'),
          j(dirNamedAsar, 'plain.txt'), // a real directory named *.asar
          dirNamedAsar,
          dirNamedAsar + sep,
          archive + '.unpacked' + sep + 'a.txt',
          archive + 'x' + sep + 'a.txt', // "app.asarx"
          j(tmp, 'plain', 'app.asar.gz', 'a.txt'),
          j(tmp, 'plain', 'asar', 'a.txt'),
          j(tmp, 'plain', 'xasar'),
          '',
          sep,
          'asar'
        ]) {
          expect(splitPath(input, true), JSON.stringify(input)).toBe(-1);
        }
      });

      it('leaves paths with control characters to the real filesystem, like Archive::New', () => {
        if (process.platform !== 'win32') {
          const odd = j(tmp, 'tab\there');
          originalFs.mkdirSync(odd);
          originalFs.copyFileSync(archive, j(odd, 'app.asar'));
          expect(splitPath(j(odd, 'app.asar', 'a.txt'), true)).toBe(-1);
          expect(fs.statSync(j(odd, 'app.asar')).isFile()).toBe(true);
          expect(fs.readFileSync(j(odd, 'app.asar')).length).toBeGreaterThan(8);
        }
        expect(splitPath(j(archive, 'a\u0000b'), true)).toBe(-1);
        expect(() => fs.readFileSync(j(archive, 'a\u0000b'))).toThrow(
          /must be .* without null bytes|ERR_INVALID_ARG_VALUE/
        );
      });

      it('ignores non-string arguments', () => {
        for (const input of [undefined, null, 42, {}, Buffer.from(archive), [archive]]) {
          expect(splitPath(input as any, true)).toBe(-1);
        }
      });

      it('picks the deepest *.asar component that is not a directory on disk', () => {
        // An entry named *.asar inside an archive cannot be a directory on
        // disk, so it becomes the archive path (and later fails to open):
        // nested archives are not supported, matching GetAsarArchivePath().
        const nested = j(archive, 'inner.asar', 'x');
        expect(splitPath(nested, true)).toBe(j(archive, 'inner.asar').length);
        // A *.asar that does not exist at all is still "not a directory".
        const ghost = j(tmp, 'plain', 'ghost.asar', 'x');
        expect(splitPath(ghost, true)).toBe(j(tmp, 'plain', 'ghost.asar').length);
        // A plain file named *.asar is accepted here; opening it is what fails.
        expect(splitPath(j(bogusArchive, 'x'), true)).toBe(bogusArchive.length);
        expect(() => fs.readFileSync(j(bogusArchive, 'x'))).toThrow(/Invalid package/);
      });

      it('matches the extension case-insensitively, like base::FilePath', () => {
        const dir = j(tmp, 'plain');
        for (const name of ['x.ASAR', 'x.Asar', 'x.aSaR', '.asar', 'x.y.asar', '..asar']) {
          expect(splitPath(j(dir, name, 'f'), true), name).toBe(j(dir, name).length);
        }
        for (const name of ['x.asar.', 'x.asar ', 'x.asa', 'x.tasar', 'asar.x']) {
          expect(splitPath(j(dir, name, 'f'), true), name).toBe(-1);
        }
      });

      it('reports paths that need lexical normalization instead of guessing', () => {
        const needs = [
          archive + sep + sep + 'a.txt',
          j(archive, '.', 'a.txt'),
          j(archive, 'dir', '..', 'a.txt'),
          archive + sep + '.',
          archive + sep + '..',
          j(tmp, 'plain', '.', 'app.asar', 'a.txt'),
          j(tmp, 'plain', '..', 'plain', 'app.asar'),
          '.' + sep + 'x.asar' + sep + 'f',
          '..' + sep + 'x.asar'
        ];
        for (const input of needs) {
          expect(splitPath(input, true), input).toBe(-2);
          expect(splitPath(input, false), input).toBeGreaterThan(0);
        }
        // No archive component at all: never -2, whatever the shape.
        expect(splitPath(j(tmp, '.', 'plain', '..', 'x'), true)).toBe(-1);
        // Dots inside names are not dot components.
        expect(splitPath(j(archive, '.hidden'), true)).toBe(archive.length);
        expect(splitPath(j(archive, 'a.txt.bak'), true)).toBe(archive.length);
        expect(splitPath(j(archive, '...'), true)).toBe(archive.length);
      });

      it('handles relative paths against the current directory', () => {
        const cwd = process.cwd();
        try {
          process.chdir(j(tmp, 'plain'));
          expect(splitPath(j('app.asar', 'a.txt'), true)).toBe('app.asar'.length);
          expect(fs.readFileSync(j('app.asar', 'a.txt'), 'utf8')).toBe('alpha');
          process.chdir(tmp);
          expect(splitPath(j('looks.asar', 'plain.txt'), true)).toBe(-1);
          expect(fs.readFileSync(j('looks.asar', 'plain.txt'), 'utf8')).toBe('plain');
        } finally {
          process.chdir(cwd);
        }
      });

      it('re-evaluates a *.asar path once something exists there', () => {
        const laterDir = j(tmp, 'later-dir.asar');
        const laterFile = j(tmp, 'later-file.asar');
        // Nothing there yet: not a directory, so provisionally an archive path.
        expect(splitPath(j(laterDir, 'x'), true)).toBe(laterDir.length);
        expect(splitPath(j(laterFile, 'a.txt'), true)).toBe(laterFile.length);
        expect(fs.existsSync(j(laterFile, 'a.txt'))).toBe(false);
        // The wrapped mkdir (win32) probes before creating; it must not poison later lookups.
        fs.mkdirSync(laterDir);
        fs.writeFileSync(j(laterDir, 'x'), 'plain file');
        expect(splitPath(j(laterDir, 'x'), true)).toBe(-1);
        expect(fs.readFileSync(j(laterDir, 'x'), 'utf8')).toBe('plain file');
        writeAsar(laterFile, tree);
        expect(splitPath(j(laterFile, 'a.txt'), true)).toBe(laterFile.length);
        expect(fs.readFileSync(j(laterFile, 'a.txt'), 'utf8')).toBe('alpha');
      });

      it('gives stable answers under repetition and across many distinct prefixes', () => {
        for (let i = 0; i < 1000; i++) {
          expect(splitPath(j(archive, 'dir', `f${i}`), true)).toBe(archive.length);
        }
        // Distinct ghost archives exercise the prefix memo without touching disk state.
        for (let i = 0; i < 5000; i++) {
          const ghost = j(tmp, 'plain', `g${i}.asar`);
          expect(splitPath(j(ghost, 'x'), true)).toBe(ghost.length);
        }
        expect(splitPath(j(dirNamedAsar, 'plain.txt'), true)).toBe(-1);
        expect(splitPath(j(archive, 'a.txt'), true)).toBe(archive.length);
      });

      if (process.platform === 'win32') {
        it('accepts either separator on Windows', () => {
          expect(splitPath(archive.replace(/\\/g, '/') + '/a.txt', false)).toBe(archive.length);
          expect(splitPath(archive + '/dir\\b.txt', false)).toBe(archive.length);
        });
      } else {
        it('treats a backslash as an ordinary file name character on POSIX', () => {
          expect(splitPath(archive + '\\a.txt', true)).toBe(-1);
          expect(splitPath(j(tmp, 'plain', 'app.asar\\x', 'y'), true)).toBe(-1);
        });
      }
    });

    describe('Archive lookups', () => {
      let a: NodeJS.AsarArchive;
      const kFile = 1;
      const kDir = 2;
      const kLink = 3;
      beforeAll(() => {
        a = new asarBinding.Archive(archive);
      });

      it('stats files, directories and links without following the final link', () => {
        expect(a.stat('')).toMatchObject({ type: kDir });
        expect(a.stat('a.txt')).toMatchObject({ type: kFile, size: 5, executable: false });
        expect(a.stat('run.sh')).toMatchObject({ type: kFile, executable: true });
        expect(a.stat('empty')).toMatchObject({ type: kFile, size: 0 });
        expect(a.stat('dir')).toMatchObject({ type: kDir });
        expect(a.stat(j('dir', 'sub', 'deeper'))).toMatchObject({ type: kDir });
        expect(a.stat(j('dir', 'sub', 'deeper', 'd.txt'))).toMatchObject({ type: kFile, size: 5 });
        expect(a.stat('file.lnk')).toMatchObject({ type: kLink });
        expect(a.stat('dir.lnk')).toMatchObject({ type: kLink });
        expect(a.stat('dangling.lnk')).toMatchObject({ type: kLink });
        expect(a.stat(j(unicodeDir, unicodeName))).toMatchObject({
          type: kFile,
          size: Buffer.byteLength('unicode-content')
        });
        expect(a.stat(j(unicodeDir, astralName))).toMatchObject({ type: kFile, size: 6 });
      });

      it('returns false for anything that does not resolve', () => {
        for (const p of [
          'missing',
          j('dir', 'missing'),
          j('a.txt', 'child'), // through a file
          j('empty', 'x'),
          j('dangling.lnk', 'x'), // through a dangling link
          j('loop1.lnk', 'x'), // through a link cycle
          j('dir', 'sub', 'c.txt', 'deeper'),
          'A.TXT', // entry names are case-sensitive
          j('dir', '.'), // no lexical normalization at this layer
          j('dir', '..', 'a.txt'),
          '.',
          '..'
        ]) {
          expect(a.stat(p), p).toBe(false);
          expect(a.getFileInfo(p), p).toBe(false);
          expect(a.readdir(p), p).toBe(false);
          expect(a.readdirWithTypes(p), p).toBe(false);
        }
        expect(a.realpath('missing')).toBe(false);
      });

      it('walks through directory links in the middle of a path', () => {
        expect(a.stat(j('dir.lnk', 'b.txt'))).toMatchObject({ type: kFile, size: 5 });
        expect(a.stat(j('dir.lnk', 'sub', 'c.txt'))).toMatchObject({ type: kFile });
        expect(a.stat(j('dir', 'self.lnk', 'self.lnk', 'b.txt'))).toMatchObject({ type: kFile });
        expect(a.stat(j('dir.lnk', 'up.lnk'))).toMatchObject({ type: kLink });
        expect(a.readdir(j('dir', 'self.lnk'))).toEqual(a.readdir('dir'));
      });

      it('tolerates leading, trailing and doubled separators', () => {
        expect(a.stat(sep + 'a.txt')).toMatchObject({ type: kFile, size: 5 });
        expect(a.stat('dir' + sep)).toMatchObject({ type: kDir });
        expect(a.stat('dir' + sep + sep + 'b.txt')).toMatchObject({ type: kFile });
        expect(a.readdir('dir' + sep)).toEqual(a.readdir('dir'));
        expect(a.stat(sep)).toMatchObject({ type: kDir });
      });

      it('getFileInfo follows links to files and reports offsets in file order', () => {
        const first = a.getFileInfo('a.txt');
        expect(first).toMatchObject({ size: 5, unpacked: false, executable: false });
        expect(a.getFileInfo('file.lnk')).toEqual(first);
        expect(a.getFileInfo('chain.lnk')).toEqual(first);
        expect(a.getFileInfo(j('dir', 'up.lnk'))).toEqual(first);
        expect(a.getFileInfo('dangling.lnk')).toBe(false);
        expect(a.getFileInfo('loop1.lnk')).toBe(false);
        expect(a.getFileInfo('run.sh')).toMatchObject({ executable: true });
        const second = a.getFileInfo('run.sh');
        expect(first && second && second.offset - first.offset).toBe(5);
        expect(first && (first as any).integrity).toBe(undefined);
      });

      it('realpath resolves only the final component when it is a link', () => {
        expect(a.realpath('a.txt')).toBe('a.txt');
        expect(a.realpath('file.lnk')).toBe('a.txt');
        expect(a.realpath('chain.lnk')).toBe('file.lnk');
        expect(a.realpath('dir.lnk')).toBe('dir');
        expect(a.realpath(j('dir.lnk', 'b.txt'))).toBe(j('dir.lnk', 'b.txt'));
        expect(a.realpath('dangling.lnk')).toBe('missing');
        expect(a.realpath('')).toBe('');
      });

      it('readdir lists entry names and readdirWithTypes agrees with stat', () => {
        const rootNames = a.readdir('');
        expect(rootNames).toBeInstanceOf(Array);
        expect(rootNames).toEqual(expect.arrayContaining(['a.txt', 'dir', 'file.lnk', unicodeDir, '.hidden']));
        expect(rootNames).toHaveLength(Object.keys(tree).length);
        expect(a.readdir('a.txt')).toBe(false);
        for (const dir of ['', 'dir', j('dir', 'sub'), 'dir.lnk', unicodeDir]) {
          const listing = a.readdirWithTypes(dir);
          expect(listing, dir).not.toBe(false);
          const [names, types] = listing as [string[], number[]];
          expect(names).toEqual(a.readdir(dir));
          expect(types).toHaveLength(names.length);
          names.forEach((name, i) => {
            const stats = a.stat(dir ? j(dir, name) : name);
            expect(stats && stats.type, j(dir, name)).toBe(types[i]);
          });
        }
        expect(a.readdirWithTypes(unicodeDir)).toEqual([[unicodeName, astralName].sort(), [kFile, kFile]]);
      });

      it('returns objects of a stable shape and fresh identity', () => {
        const s1 = a.stat('a.txt');
        const s2 = a.stat('a.txt');
        expect(s1).toEqual(s2);
        expect(s1).not.toBe(s2);
        expect(Object.keys(s1 as object)).toEqual(['size', 'offset', 'type', 'executable']);
        expect(Object.keys(a.getFileInfo('a.txt') as object)).toEqual([
          'size',
          'unpacked',
          'offset',
          'executable',
          'integrity'
        ]);
        (s1 as any).size = 123;
        expect(a.stat('a.txt')).toMatchObject({ size: 5 });
      });

      it('keeps answering correctly past the lookup memo limit', () => {
        for (let i = 0; i < 33 * 1024; i++) {
          if (a.stat(`missing-${i}`) !== false) throw new Error(`missing-${i} resolved`);
        }
        expect(a.stat('a.txt')).toMatchObject({ type: kFile, size: 5 });
        expect(a.stat(j('dir', 'sub', 'c.txt'))).toMatchObject({ type: kFile });
        expect(a.stat('missing-0')).toBe(false);
      });

      it('serves concurrent lookups from worker threads', async () => {
        const workerSource = `
          const { parentPort, workerData } = require('node:worker_threads');
          const fs = require('node:fs');
          const path = require('node:path');
          let ok = 0;
          for (let i = 0; i < 2000; i++) {
            const st = fs.statSync(path.join(workerData, 'dir', 'sub', 'c.txt'));
            if (st.isFile() && st.size === 7) ok++;
            if (fs.existsSync(path.join(workerData, 'nope-' + i))) ok = -1e9;
            if (fs.readdirSync(path.join(workerData, 'dir')).length === 4) ok++;
          }
          parentPort.postMessage(ok);
        `;
        const workers = Array.from({ length: 4 }, () => new Worker(workerSource, { eval: true, workerData: archive }));
        const results = workers.map((w) => once(w, 'message').then(([n]) => n));
        let mainOk = 0;
        for (let i = 0; i < 2000; i++) {
          if (fs.statSync(j(archive, 'a.txt')).size === 5) mainOk++;
        }
        expect(mainOk).toBe(2000);
        expect(await Promise.all(results)).toEqual([4000, 4000, 4000, 4000]);
        await Promise.all(workers.map((w) => w.terminate()));
      });
    });

    describe('archives with out-of-range size fields', () => {
      let oversizedHeader: string; // header size field larger than the file
      let badEntries: string; // entries whose size/offset do not fit the file

      beforeAll(() => {
        oversizedHeader = j(tmp, 'plain', 'oversized-header.asar');
        const bytes = Buffer.from(originalFs.readFileSync(archive));
        bytes.writeUInt32LE(0xffffffff, 4);
        originalFs.writeFileSync(oversizedHeader, bytes);

        badEntries = j(tmp, 'plain', 'bad-entries.asar');
        writeAsar(
          badEntries,
          { 'ok.txt': 'ok', 'negative.txt': 'neg', 'too-long.txt': 'long', 'far-away.txt': 'far' },
          (files) => {
            files['negative.txt'].size = -1;
            files['too-long.txt'].size = 1 << 30;
            files['far-away.txt'].offset = String(2 ** 40);
          }
        );
      });

      it('rejects an archive whose header size exceeds the file size', () => {
        expect(() => fs.readFileSync(j(oversizedHeader, 'a.txt'))).toThrow(/Invalid package/);
        expect(() => fs.statSync(j(oversizedHeader, 'a.txt'))).toThrow(/Invalid package/);
      });

      it('still serves well-formed entries next to malformed ones', () => {
        expect(fs.readFileSync(j(badEntries, 'ok.txt'), 'utf8')).toBe('ok');
      });

      for (const name of ['negative.txt', 'too-long.txt', 'far-away.txt']) {
        it(`treats ${name} as missing instead of reading past the archive`, () => {
          const p = j(badEntries, name);
          expect(() => fs.statSync(p)).toThrow(/ENOENT/);
          expect(() => fs.readFileSync(p)).toThrow(/ENOENT/);
          expect(() => fs.openSync(p, 'r')).toThrow(/ENOENT/);
          expect(() => fs.copyFileSync(p, j(tmp, 'copied-' + name))).toThrow(/ENOENT/);
          expect(fs.existsSync(p)).toBe(false);
        });
      }
    });

    describe('fs on archives in unusual locations', () => {
      it('works under a non-ASCII directory and with non-ASCII entry names', () => {
        expect(fs.readFileSync(j(unicodeArchive, 'a.txt'), 'utf8')).toBe('alpha');
        expect(fs.readFileSync(j(unicodeArchive, unicodeDir, unicodeName), 'utf8')).toBe('unicode-content');
        expect(fs.readFileSync(j(unicodeArchive, unicodeDir, astralName), 'utf8')).toBe('astral');
        expect(fs.statSync(j(unicodeArchive, unicodeDir)).isDirectory()).toBe(true);
        expect(fs.readdirSync(j(unicodeArchive, unicodeDir))).toHaveLength(2);
        expect(fs.readdirSync(j(unicodeArchive, unicodeDir))).toEqual(
          expect.arrayContaining([unicodeName, astralName])
        );
        expect(fs.realpathSync(j(unicodeArchive, 'file.lnk'))).toBe(j(unicodeArchive, 'a.txt'));
        expect(fs.existsSync(j(unicodeArchive, unicodeDir, 'nope'))).toBe(false);
      });

      // Node mode never calls setlocale(), so it always runs in the "C" locale.
      // A main process started with LC_ALL=C does too.
      // The archive lookup must not depend on the locale in either case.
      it('works under a non-ASCII directory in Node mode', async () => {
        const script = `process.stdout.write(require('fs').readFileSync(${JSON.stringify(j(unicodeArchive, 'a.txt'))}, 'utf8'))`;
        const { code, stdout, stderr } = await spawnAndWait(process.execPath, ['-e', script], {
          env: { ...process.env, ELECTRON_RUN_AS_NODE: '1' },
          timeout: 20000
        });
        expect(stdout, `exit code ${code}, stderr: ${stderr}`).toBe('alpha');
        expect(code).toBe(0);
      });

      ifit(process.platform === 'linux')(
        'starts an app from an archive under a non-ASCII directory with LC_ALL=C',
        async () => {
          const app = j(tmp, unicodeDir, 'main-app.asar');
          writeAsar(app, {
            'package.json': JSON.stringify({ main: 'main.js' }),
            'main.js':
              "const { app } = require('electron'); app.whenReady().then(() => { console.log('ready'); app.quit(); });"
          });
          const { code, stdout, stderr } = await spawnAndWait(process.execPath, [app], {
            env: { ...process.env, LC_ALL: 'C' },
            timeout: 20000
          });
          expect(stdout, `exit code ${code}, stderr: ${stderr}`).toContain('ready');
          expect(code).toBe(0);
        }
      );

      it('works when the archive extension is upper case', () => {
        expect(fs.readFileSync(j(upperArchive, 'dir', 'b.txt'), 'utf8')).toBe('bravo');
        expect(fs.statSync(upperArchive).isDirectory()).toBe(true);
      });

      it('works for an archive inside a real directory named *.asar', () => {
        expect(fs.readFileSync(j(archiveInDirNamedAsar, 'a.txt'), 'utf8')).toBe('alpha');
        expect(fs.readFileSync(j(dirNamedAsar, 'plain.txt'), 'utf8')).toBe('plain');
        expect(fs.readdirSync(dirNamedAsar)).toHaveLength(2);
        expect(fs.readdirSync(dirNamedAsar)).toEqual(expect.arrayContaining(['plain.txt', 'real.asar']));
      });

      it('accepts Buffer and file: URL paths', () => {
        expect(fs.readFileSync(Buffer.from(j(archive, 'a.txt')), 'utf8')).toBe('alpha');
        expect(fs.readFileSync(url.pathToFileURL(j(archive, 'dir', 'b.txt')), 'utf8')).toBe('bravo');
        expect(fs.existsSync(url.pathToFileURL(j(unicodeArchive, unicodeDir, astralName)) as any)).toBe(true);
      });

      it('accepts un-normalized paths into an archive', () => {
        expect(fs.readFileSync([archive, 'dir', '.', '..', 'dir', 'b.txt'].join(sep), 'utf8')).toBe('bravo');
        expect(fs.statSync(archive + sep + sep + 'dir' + sep + sep + 'b.txt' + sep).isFile()).toBe(true);
        expect(fs.existsSync([archive, '..', 'app.asar', 'dir'].join(sep))).toBe(true);
        expect(fs.existsSync([archive, 'dir', '..', '..', 'app.asar'].join(sep))).toBe(true);
        expect(fs.existsSync([archive, 'dir', '..', '..', 'nope.asar'].join(sep))).toBe(false);
        expect(fs.readdirSync([tmp, 'plain', '.', 'app.asar', 'dir', ''].join(sep))).toHaveLength(4);
        expect(fs.readdirSync([tmp, 'plain', '.', 'app.asar', 'dir', ''].join(sep))).toEqual(
          expect.arrayContaining(['b.txt', 'sub', 'up.lnk', 'self.lnk'])
        );
      });

      it('module resolution accepts un-normalized lookup paths', () => {
        const sloppyDir = archive + sep + '.' + sep + 'dir' + sep;
        expect(require.resolve('./b.txt', { paths: [sloppyDir] })).toBe(j(archive, 'dir', 'b.txt'));
        expect(require.resolve('./c.txt', { paths: [j(archive, 'dir', '..', 'dir', 'sub')] })).toBe(
          j(archive, 'dir', 'sub', 'c.txt')
        );
        expect(() => require.resolve('./nope.txt', { paths: [sloppyDir] })).toThrow(/Cannot find module/);
      });

      it('presents the archive root like a directory, with or without a trailing separator', () => {
        expect(fs.statSync(archive).isDirectory()).toBe(true);
        expect(fs.statSync(archive + sep).isDirectory()).toBe(true);
        expect(fs.readdirSync(archive + sep)).toEqual(fs.readdirSync(archive));
        const inParent = fs.readdirSync(j(tmp, 'plain'), { withFileTypes: true });
        expect(inParent.find((d) => d.name === 'app.asar')!.isFile()).toBe(true);
      });

      it('produces Stats consistent with the entry', () => {
        const st = fs.statSync(j(archive, 'run.sh'));
        expect(st.isFile()).toBe(true);
        expect(st.mode & 0o111).not.toBe(0);
        expect(fs.statSync(j(archive, 'a.txt')).mode & 0o111).toBe(0);
        expect(fs.lstatSync(j(archive, 'file.lnk')).isSymbolicLink()).toBe(true);
        expect(fs.statSync(j(archive, 'file.lnk')).isFile()).toBe(true);
        const big = fs.statSync(j(archive, 'a.txt'), { bigint: true });
        expect(big.size).toBe(5n);
        expect(typeof big.mtimeNs).toBe('bigint');
        // Two stats in a row must not alias each other (shared scratch buffer).
        const s1 = fs.statSync(j(archive, 'a.txt'));
        const s2 = fs.statSync(j(archive, 'dir', 'sub', 'c.txt'));
        const d = fs.statSync(j(archive, 'dir'));
        expect(s1.size).toBe(5);
        expect(s2.size).toBe(7);
        expect(s1.isFile() && s2.isFile()).toBe(true);
        expect(d.isDirectory()).toBe(true);
        expect(s1.isDirectory()).toBe(false);
      });

      it('realpath resolves the archive location once and reuses it', () => {
        const linkDir = j(tmp, 'via-link');
        importedFs.symlinkSync(j(tmp, 'plain'), linkDir, 'dir');
        const viaLink = j(linkDir, 'app.asar');
        expect(fs.realpathSync(j(viaLink, 'file.lnk'))).toBe(j(archive, 'a.txt'));
        // The native flavour may spell the directory differently (8.3 names
        // on Windows runners), so derive its expectation the same way.
        const nativeArchive = j(originalFs.realpathSync.native(j(tmp, 'plain')), 'app.asar');
        expect(fs.realpathSync.native(j(viaLink, 'dir', 'b.txt'))).toBe(j(nativeArchive, 'dir', 'b.txt'));
        expect(fs.realpathSync(j(viaLink, 'a.txt'), 'buffer')).toEqual(Buffer.from(j(archive, 'a.txt')));
        expect(fs.realpathSync(j(viaLink, 'a.txt'), { encoding: 'hex' })).toBe(
          Buffer.from(j(archive, 'a.txt')).toString('hex')
        );
        return new Promise<void>((resolve, reject) => {
          fs.realpath(j(viaLink, 'chain.lnk'), (err, resolved) => {
            if (err) return reject(err);
            try {
              expect(resolved).toBe(j(archive, 'file.lnk'));
              resolve();
            } catch (e) {
              reject(e);
            }
          });
        });
      });
    });
  });
});

async function expectToThrowErrorWithCode(_func: Function, _code: string) {
  /* dummy for typescript */
}

function promisify(_f: Function): any {
  /* dummy for typescript */
}

function tempPath(): string {
  /* dummy for typescript */
  return '';
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
        const dest = tempPath();
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
        const dest = tempPath();
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
        const dest = tempPath();
        await fs.promises.copyFile(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });

      itremote('copies a unpacked file', async () => {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const dest = tempPath();
        await fs.promises.copyFile(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });
    });

    describe('fs.copyFileSync', () => {
      itremote('copies a normal file', () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });

      itremote('copies a unpacked file', () => {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const dest = tempPath();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });
    });

    describe('fs.cpSync', () => {
      itremote('copies a normal file', () => {
        if (!fs.cpSync) return;
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
        fs.cpSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).toBe(true);
      });
    });

    describe('fs.cp', () => {
      itremote('copies a normal file', async () => {
        if (!fs.cp) return;
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
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
        const dest = tempPath();
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

    // The tests below cover the fs APIs that read packed archive entries
    // through file descriptors, streams, handles and copies (i.e. without
    // extracting them to a temporary file first), plus the directory and
    // symlink APIs.  Where a corresponding Node.js test exists
    // (test/parallel/test-fs-*), its expectations are mirrored here.

    describe('fs.openSync (fd semantics)', () => {
      itremote('returns a usable, closable file descriptor', () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        expect(fd).toBeTypeOf('number');
        expect(fd).toBeGreaterThan(2);
        fs.closeSync(fd);
        expect(() => fs.fstatSync(fd)).toThrow(/EBADF/);
        expect(() => fs.closeSync(fd)).toThrow(/EBADF/);
      });

      itremote('accepts default, string and numeric read-only flags', () => {
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
          expect(fs.readSync(fd, buffer, 0, 6, 0)).toBe(6);
          expect(String(buffer).trim()).toBe('file1');
          fs.closeSync(fd);
        }
      });

      itremote('accepts a mode argument', () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r', 0o644);
        fs.closeSync(fd);
      });

      itremote('refuses to open a packed file for writing', () => {
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
          expect(() => fs.openSync(p, flags as any), String(flags)).toThrow(/EACCES/);
        }
        // Nothing was written or corrupted.
        expect(fs.readFileSync(p).toString().trim()).toBe('file1');
      });

      itremote('throws EEXIST for O_CREAT | O_EXCL on an existing packed file', () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => fs.openSync(p, fs.constants.O_RDONLY | fs.constants.O_CREAT | fs.constants.O_EXCL)).toThrow(
          /EEXIST/
        );
      });

      itremote('throws EISDIR when opening a directory', () => {
        expect(() => fs.openSync(path.join(asarDir, 'a.asar', 'dir1'), 'r')).toThrow(/EISDIR/);
        expect(() => fs.openSync(path.join(asarDir, 'a.asar'), 'r')).toThrow(/EISDIR/);
        // ...including through a symbolic link to a directory.
        expect(() => fs.openSync(path.join(asarDir, 'a.asar', 'link2'), 'r')).toThrow(/EISDIR/);
        expect(() => fs.openSync(path.join(asarDir, 'a.asar', 'link2', 'link2'), 'r')).toThrow(/EISDIR/);
      });

      itremote('opens files with an empty size', () => {
        const fd = fs.openSync(path.join(asarDir, 'empty.asar', 'file1'), 'r');
        expect(fs.fstatSync(fd).size).toBe(0);
        expect(fs.readSync(fd, Buffer.alloc(8), 0, 8, 0)).toBe(0);
        expect(fs.readFileSync(fd).length).toBe(0);
        fs.closeSync(fd);
      });

      itremote('opens unpacked files as regular files', () => {
        const originalFs = require('node:original-fs');
        const fd = fs.openSync(path.join(asarDir, 'unpack.asar', 'a.txt'), 'r');
        const stats = fs.fstatSync(fd);
        const realStats = originalFs.statSync(path.join(asarDir, 'unpack.asar.unpacked', 'a.txt'));
        expect(stats.ino).toBe(realStats.ino);
        expect(stats.size).toBe(realStats.size);
        fs.closeSync(fd);
      });

      itremote('hands out a descriptor that is not usable outside of fs', () => {
        // The number identifies the entry to fs only; it must never expose the
        // archive's bytes to code that reads the raw descriptor, and that code
        // should fail loudly rather than get data from the wrong offset.
        const originalFs = require('node:original-fs');
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        const buffer = Buffer.alloc(16);
        expect(() => originalFs.readSync(fd, buffer, 0, 16, 0)).toThrow(/EBADF|EPERM|EACCES/);
        expect(() => originalFs.readSync(fd, buffer, 0, 16, null)).toThrow(/EBADF|EPERM|EACCES/);
        expect(originalFs.fstatSync(fd).isFile()).toBe(false);
        expect(originalFs.fstatSync(fd).size).not.toBe(fs.fstatSync(fd).size);
        expect(buffer.equals(Buffer.alloc(16))).toBe(true);
        // ...while fs itself still serves the entry.
        expect(fs.readSync(fd, buffer, 0, 6, 0)).toBe(6);
        expect(buffer.subarray(0, 6).toString()).toBe('file1\n');
        fs.closeSync(fd);
      });

      itremote('does not leak file descriptors', () => {
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
        expect(last).toBe(first);
      });

      itremote('gives every open its own file position', () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd1 = fs.openSync(p, 'r');
        const fd2 = fs.openSync(p, 'r');
        const b1 = Buffer.alloc(3);
        const b2 = Buffer.alloc(3);
        fs.readSync(fd1, b1, 0, 3, null);
        fs.readSync(fd2, b2, 0, 3, null);
        expect(b1.toString()).toBe('fil');
        expect(b2.toString()).toBe('fil');
        fs.readSync(fd1, b1, 0, 3, null);
        expect(b1.toString()).toBe('e1\n');
        fs.closeSync(fd1);
        fs.closeSync(fd2);
      });
    });

    describe('fs.open (callback forms)', () => {
      itremote('supports (path, cb), (path, flags, cb) and (path, flags, mode, cb)', async () => {
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
          expect(String(buffer).trim()).toBe('file1');
          await promisify(fs.close)(fd);
        }
      });

      itremote('reports EACCES for write flags asynchronously', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const err = await new Promise<any>((resolve) => fs.open(p, 'w', resolve));
        expect(err.code).toBe('EACCES');
      });

      itremote('reports EISDIR for directories asynchronously', async () => {
        const err = await new Promise<any>((resolve) => fs.open(path.join(asarDir, 'a.asar', 'dir1'), 'r', resolve));
        expect(err.code).toBe('EISDIR');
      });
    });

    describe('fs.readSync / fs.read on packed files', () => {
      // Mirrors test-fs-read.js.
      itremote('reads into Buffers and Uint8Arrays with an explicit position', async () => {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        const expected = Buffer.from('file1\n');
        for (const make of [() => Buffer.allocUnsafe(expected.length), () => new Uint8Array(expected.length)]) {
          const bufferSync = make();
          expect(fs.readSync(fd, bufferSync, 0, expected.length, 0)).toBe(expected.length);
          expect(Buffer.from(bufferSync).equals(expected)).toBe(true);
          const bufferAsync = make();
          const bytesRead = await new Promise<number>((resolve, reject) =>
            fs.read(fd, bufferAsync, 0, expected.length, 0, (e, n) => (e ? reject(e) : resolve(n)))
          );
          expect(bytesRead).toBe(expected.length);
          expect(Buffer.from(bufferAsync).equals(expected)).toBe(true);
        }
        fs.closeSync(fd);
      });

      itremote('returns 0 bytes when reading beyond the end of the file', async () => {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        expect(fs.readSync(fd, Buffer.alloc(1), 0, 1, 6)).toBe(0);
        expect(fs.readSync(fd, Buffer.alloc(1), 0, 1, 0xffffffff + 1)).toBe(0);
        const n = await new Promise<number>((resolve, reject) =>
          fs.read(fd, Buffer.alloc(1), 0, 1, 0xffffffff + 1, (e, n) => (e ? reject(e) : resolve(n)))
        );
        expect(n).toBe(0);
        fs.closeSync(fd);
      });

      itremote('clamps reads that run past the end of the entry', () => {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        const buffer = Buffer.alloc(100);
        expect(fs.readSync(fd, buffer, 0, 100, 4)).toBe(2);
        expect(buffer.subarray(0, 2).toString()).toBe('1\n');
        // Untouched bytes stay untouched.
        expect(buffer[2]).toBe(0);
        fs.closeSync(fd);
      });

      itremote('honours a null position by using and advancing the file position', () => {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        const chunk = Buffer.alloc(2);
        const parts: string[] = [];
        let n;
        while ((n = fs.readSync(fd, chunk, 0, 2, null)) > 0) parts.push(chunk.subarray(0, n).toString());
        expect(parts.join('')).toBe('file1\n');
        // An explicit position does not disturb the file position.
        const fd2 = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        const b = Buffer.alloc(3);
        fs.readSync(fd2, b, 0, 3, null);
        fs.readSync(fd2, b, 0, 3, 0);
        expect(b.toString()).toBe('fil');
        fs.readSync(fd2, b, 0, 3, null);
        expect(b.toString()).toBe('e1\n');
        fs.closeSync(fd);
        fs.closeSync(fd2);
      });

      itremote('accepts a bigint position', () => {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        const buffer = Buffer.alloc(2);
        expect(fs.readSync(fd, buffer, 0, 2, 4n as any)).toBe(2);
        expect(buffer.toString()).toBe('1\n');
        fs.closeSync(fd);
      });

      // Mirrors test-fs-read-optional-params.js and test-fs-readSync-optional-params.js.
      itremote('supports the options-object forms', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const expected = Buffer.from('file1\n');
        {
          const fd = fs.openSync(p, 'r');
          const buffer = Buffer.alloc(expected.length);
          expect(fs.readSync(fd, buffer, { offset: 0, length: expected.length, position: 0 })).toBe(expected.length);
          expect(buffer.equals(expected)).toBe(true);
          // The explicit position above did not move the file position.
          buffer.fill(0);
          expect(fs.readSync(fd, buffer, {})).toBe(expected.length);
          expect(buffer.equals(expected)).toBe(true);
          expect(fs.readSync(fd, buffer, {})).toBe(0);
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          const buffer = Buffer.alloc(expected.length);
          // cursor read via options without position
          expect(fs.readSync(fd, buffer, { offset: 0, length: expected.length })).toBe(expected.length);
          expect(buffer.equals(expected)).toBe(true);
          fs.closeSync(fd);
        }
        for (const options of [undefined, null, {}, { offset: 0, position: 0 }]) {
          const fd = fs.openSync(p, 'r');
          const { bytesRead, buffer } = await new Promise<{ bytesRead: number; buffer: Buffer }>((resolve, reject) => {
            const cb = (e: any, bytesRead: number, buffer: Buffer) => (e ? reject(e) : resolve({ bytesRead, buffer }));
            if (options === undefined) fs.read(fd, cb);
            else fs.read(fd, options as any, cb);
          });
          expect(bytesRead).toBe(expected.length);
          expect(buffer.subarray(0, bytesRead).equals(expected)).toBe(true);
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          const buffer = Buffer.alloc(expected.length);
          const bytesRead = await new Promise<number>((resolve, reject) =>
            fs.read(fd, buffer, { position: 0 }, (e, n) => (e ? reject(e) : resolve(n)))
          );
          expect(bytesRead).toBe(expected.length);
          expect(buffer.equals(expected)).toBe(true);
          fs.closeSync(fd);
        }
      });

      // Mirrors test-fs-read-zero-length.js and test-fs-read-empty-buffer.js.
      itremote('handles zero-length reads and rejects empty buffers like Node', async () => {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        expect(fs.readSync(fd, Buffer.alloc(4), 0, 0, 0)).toBe(0);
        const n = await new Promise<number>((resolve, reject) =>
          fs.read(fd, Buffer.alloc(4), 0, 0, 0, (e, n) => (e ? reject(e) : resolve(n)))
        );
        expect(n).toBe(0);
        expect(() => fs.readSync(fd, Buffer.alloc(0), 0, 1, 0)).toThrow(
          expect.objectContaining({ code: expect.stringMatching(/^(ERR_INVALID_ARG_VALUE|ERR_OUT_OF_RANGE)$/) })
        );
        fs.closeSync(fd);
      });

      itremote('validates arguments like Node', () => {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        expect(() => (fs.read as any)(fd, Buffer.alloc(1), 0, 1, 0)).toThrow(
          expect.objectContaining({ code: 'ERR_INVALID_ARG_TYPE' })
        );
        expect(() => fs.readSync(fd, Buffer.alloc(1), 0, 2, 0)).toThrow(
          expect.objectContaining({ code: 'ERR_OUT_OF_RANGE' })
        );
        expect(() => fs.readSync(fd, Buffer.alloc(1), 0, 1, -2)).toThrow(
          expect.objectContaining({ code: 'ERR_OUT_OF_RANGE' })
        );
        fs.closeSync(fd);
      });

      itremote('reads a larger packed file back byte-for-byte in odd sized chunks', () => {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const expected = fs.readFileSync(p);
        expect(expected.length).toBeGreaterThan(100000);
        const fd = fs.openSync(p, 'r');
        const out = Buffer.alloc(expected.length);
        let read = 0;
        while (read < expected.length) {
          const n = fs.readSync(fd, out, read, Math.min(4097, expected.length - read), null);
          if (n === 0) break;
          read += n;
        }
        fs.closeSync(fd);
        expect(read).toBe(expected.length);
        expect(out.equals(expected)).toBe(true);
      });
    });

    // Mirrors test-fs-readv-sync.js / test-fs-readv.js / test-fs-readv-promises.js.
    describe('fs.readvSync / fs.readv / FileHandle.readv on packed files', () => {
      itremote('reads into an array of buffers with and without a position', async () => {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const expected = fs.readFileSync(p);
        const allocate = () => [
          Buffer.alloc(Math.floor(expected.length / 2)),
          Buffer.alloc(Math.ceil(expected.length / 2))
        ];
        {
          const fd = fs.openSync(p, 'r');
          expect(fs.readvSync(fd, [Buffer.from('')], 0)).toBe(0);
          const buffers = allocate();
          expect(fs.readvSync(fd, buffers, 0)).toBe(expected.length);
          expect(Buffer.concat(buffers).equals(expected)).toBe(true);
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          expect(fs.readvSync(fd, [Buffer.from('')])).toBe(0);
          const buffers = allocate();
          expect(fs.readvSync(fd, buffers)).toBe(expected.length);
          expect(Buffer.concat(buffers).equals(expected)).toBe(true);
          expect(fs.readvSync(fd, allocate())).toBe(0);
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          const buffers = allocate();
          const bytesRead = await new Promise<number>((resolve, reject) =>
            fs.readv(fd, buffers, 0, (e, n) => (e ? reject(e) : resolve(n)))
          );
          expect(bytesRead).toBe(expected.length);
          expect(Buffer.concat(buffers).equals(expected)).toBe(true);
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          const buffers = allocate();
          const bytesRead = await new Promise<number>((resolve, reject) =>
            fs.readv(fd, buffers, (e, n) => (e ? reject(e) : resolve(n)))
          );
          expect(bytesRead).toBe(expected.length);
          expect(Buffer.concat(buffers).equals(expected)).toBe(true);
          fs.closeSync(fd);
        }
        {
          const handle = await fs.promises.open(p, 'r');
          const buffers = allocate();
          const { bytesRead } = await handle.readv(buffers, 0);
          expect(bytesRead).toBe(expected.length);
          expect(Buffer.concat(buffers).equals(expected)).toBe(true);
          await handle.close();
        }
      });

      itremote('skips zero-length buffers and clamps at the end of the entry like preadv', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1'); // 6 bytes: 'file1\n'
        {
          // Zero-length buffers do not stop the read, whether or not the tail is clamped.
          const fd = fs.openSync(p, 'r');
          const buffers = [Buffer.alloc(0), Buffer.alloc(3), Buffer.alloc(0), Buffer.alloc(3)];
          expect(fs.readvSync(fd, buffers, 0)).toBe(6);
          expect(Buffer.concat(buffers).toString()).toBe('file1\n');
          const clamped = [Buffer.alloc(0), Buffer.alloc(20)];
          expect(fs.readvSync(fd, clamped, 2)).toBe(4);
          expect(clamped[1].subarray(0, 4).toString()).toBe('le1\n');
          expect(clamped[1][4]).toBe(0);
          const clampedMore = [Buffer.alloc(2), Buffer.alloc(0), Buffer.alloc(2), Buffer.alloc(100)];
          expect(fs.readvSync(fd, clampedMore, 3)).toBe(3);
          expect(Buffer.concat(clampedMore).subarray(0, 3).toString()).toBe('e1\n');
          fs.closeSync(fd);
        }
        {
          // Same through the file position and the async / promise forms.
          const fd = fs.openSync(p, 'r');
          const first = [Buffer.alloc(0), Buffer.alloc(4)];
          expect(await promisify(fs.readv)(fd, first)).toBe(4);
          expect(first[1].toString()).toBe('file');
          const rest = [Buffer.alloc(0), Buffer.alloc(10)];
          expect(await promisify(fs.readv)(fd, rest)).toBe(2);
          expect(rest[1].subarray(0, 2).toString()).toBe('1\n');
          expect(await promisify(fs.readv)(fd, [Buffer.alloc(0), Buffer.alloc(10)])).toBe(0);
          fs.closeSync(fd);
          const handle = await fs.promises.open(p, 'r');
          const buffers = [Buffer.alloc(0), Buffer.alloc(3), Buffer.alloc(0), Buffer.alloc(30)];
          const { bytesRead } = await handle.readv(buffers, 1);
          expect(bytesRead).toBe(5);
          expect(Buffer.concat(buffers).subarray(0, 5).toString()).toBe('ile1\n');
          await handle.close();
        }
      });

      itremote('rejects invalid buffer arguments like Node', () => {
        const fd = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        for (const wrong of [false, 'test', {}, [{}], ['sdf'], null, undefined]) {
          expect(() => (fs.readvSync as any)(fd, wrong, null)).toThrow(
            expect.objectContaining({ code: 'ERR_INVALID_ARG_TYPE' })
          );
        }
        fs.closeSync(fd);
      });
    });

    describe('fs.fstat on packed files', () => {
      itremote('describes the entry', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        const stats = fs.fstatSync(fd);
        expect(stats.isFile()).toBe(true);
        expect(stats.isDirectory()).toBe(false);
        expect(stats.isSymbolicLink()).toBe(false);
        expect(stats.size).toBe(6);
        expect(stats.mtime).toBeInstanceOf(Date);
        const bigint = fs.fstatSync(fd, { bigint: true });
        expect(bigint.size).toBe(6n);
        expect(typeof bigint.mtimeMs).toBe('bigint');
        const async = await promisify(fs.fstat)(fd);
        expect(async.size).toBe(6);
        const asyncBig = await new Promise<any>((resolve, reject) =>
          fs.fstat(fd, { bigint: true }, (e, s) => (e ? reject(e) : resolve(s)))
        );
        expect(asyncBig.size).toBe(6n);
        // Stable across calls on the same descriptor.
        expect(fs.fstatSync(fd).ino).toBe(stats.ino);
        fs.closeSync(fd);
      });

      itremote('reports executable entries as executable', () => {
        const fd = fs.openSync(path.join(asarDir, 'echo.asar', 'echo'), 'r');
        const stats = fs.fstatSync(fd);
        expect(stats.mode & 0o111).not.toBe(0);
        fs.closeSync(fd);
        const fd2 = fs.openSync(path.join(asarDir, 'a.asar', 'file1'), 'r');
        expect(fs.fstatSync(fd2).mode & 0o111).toBe(0);
        fs.closeSync(fd2);
      });

      itremote('reports conventional permission bits for stat/lstat', () => {
        const a = path.join(asarDir, 'a.asar');
        expect(fs.statSync(path.join(a, 'file1')).mode & 0o777).toBe(0o644);
        expect(fs.statSync(path.join(asarDir, 'echo.asar', 'echo')).mode & 0o777).toBe(0o755);
        expect(fs.lstatSync(path.join(asarDir, 'echo.asar', 'echo')).mode & 0o777).toBe(0o755);
        expect(fs.statSync(path.join(a, 'dir1')).mode & 0o777).toBe(0o755);
        expect(fs.statSync(a).mode & 0o777).toBe(0o755);
        expect(fs.lstatSync(path.join(a, 'link1')).mode & 0o777).toBe(0o777);
      });
    });

    // Mirrors test-fs-readfile-fd.js.
    describe('fs.readFile / fs.readFileSync with a packed file descriptor', () => {
      itremote('reads from the current file position', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        {
          const fd = fs.openSync(p, 'r');
          expect(fs.readFileSync(fd).toString()).toBe('file1\n');
          expect(fs.readFileSync(fd).toString()).toBe('');
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          fs.readSync(fd, Buffer.alloc(2), 0, 2, null);
          expect(fs.readFileSync(fd, 'utf8')).toBe('le1\n');
          expect(fs.readFileSync(fd, 'utf8')).toBe('');
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          fs.readSync(fd, Buffer.alloc(3), 0, 3, null);
          expect((await promisify(fs.readFile)(fd)).toString()).toBe('e1\n');
          expect(await promisify(fs.readFile)(fd, 'utf8')).toBe('');
          fs.closeSync(fd);
        }
        {
          const fd = fs.openSync(p, 'r');
          expect(await promisify(fs.readFile)(fd, { encoding: 'utf8' })).toBe('file1\n');
          fs.closeSync(fd);
        }
      });

      itremote('reads an empty file through a descriptor', async () => {
        const fd = fs.openSync(path.join(asarDir, 'empty.asar', 'file1'), 'r');
        expect(fs.readFileSync(fd, 'utf8')).toBe('');
        expect((await promisify(fs.readFile)(fd)).length).toBe(0);
        fs.closeSync(fd);
      });
    });

    describe('fs.close / fs.closeSync of packed file descriptors', () => {
      itremote('closes and rejects further use', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        await promisify(fs.close)(fd);
        expect(() => fs.readSync(fd, Buffer.alloc(1), 0, 1, 0)).toThrow(/EBADF/);
        const err = await new Promise<any>((resolve) => fs.close(fd, resolve));
        expect(err.code).toBe('EBADF');
      });
    });

    describe('fchmod / fchown / futimes on packed file descriptors', () => {
      itremote('are refused so the archive itself cannot be modified through them', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        expect(() => fs.fchmodSync(fd, 0o777)).toThrow(/EACCES/);
        expect(() => fs.futimesSync(fd, new Date(), new Date())).toThrow(/EACCES/);
        if (process.platform !== 'win32') {
          expect(() => fs.fchownSync(fd, process.getuid!(), process.getgid!())).toThrow(/EACCES/);
        }
        const err = await new Promise<any>((resolve) => fs.fchmod(fd, 0o777, resolve));
        expect(err.code).toBe('EACCES');
        // Every mutation through the descriptor is refused.
        expect(() => fs.writeSync(fd, 'x')).toThrow(/EACCES/);
        expect(() => fs.writeSync(fd, Buffer.from('x'))).toThrow(/EACCES/);
        expect(() => fs.writevSync(fd, [Buffer.from('x')])).toThrow(/EACCES/);
        expect(() => fs.ftruncateSync(fd, 0)).toThrow(/EACCES/);
        const werr = await new Promise<any>((resolve) => fs.write(fd, 'x', resolve));
        expect(werr.code).toBe('EACCES');
        const terr = await new Promise<any>((resolve) => fs.ftruncate(fd, 0, resolve));
        expect(terr.code).toBe('EACCES');
        fs.closeSync(fd);
        // The archive is intact.
        expect(fs.readFileSync(p).toString().trim()).toBe('file1');
      });
    });

    // Mirrors test-fs-read-stream.js, test-fs-read-stream-pos.js,
    // test-fs-read-stream-fd.js and test-fs-read-stream-file-handle.js.
    describe('fs.createReadStream on packed files', () => {
      itremote('streams a whole file, emitting open/ready/end/close', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const events: string[] = [];
        const stream = fs.createReadStream(p);
        const content = await new Promise<Buffer>((resolve, reject) => {
          const chunks: Buffer[] = [];
          stream.on('open', (fd) => {
            events.push('open');
            expect(fd).toBeTypeOf('number');
          });
          stream.on('ready', () => events.push('ready'));
          stream.on('data', (chunk) => chunks.push(chunk as Buffer));
          stream.on('error', reject);
          stream.on('end', () => events.push('end'));
          stream.on('close', () => {
            events.push('close');
            resolve(Buffer.concat(chunks));
          });
        });
        expect(stream.bytesRead).toBe(6);
        expect(content.toString()).toBe('file1\n');
        expect(events).toEqual(['open', 'ready', 'end', 'close']);
      });

      itremote('honours encoding, highWaterMark, start and end', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const read = (options: any) =>
          new Promise<any[]>((resolve, reject) => {
            const chunks: any[] = [];
            fs.createReadStream(p, options)
              .on('data', (c) => chunks.push(c))
              .on('error', reject)
              .on('end', () => resolve(chunks));
          });
        expect((await read({ encoding: 'utf8' })).join('')).toBe('file1\n');
        const small = await read({ highWaterMark: 2 });
        expect(small.length).toBe(3);
        expect(Buffer.concat(small).toString()).toBe('file1\n');
        expect(Buffer.concat(await read({ start: 1, end: 3 })).toString()).toBe('ile');
        expect(Buffer.concat(await read({ start: 4 })).toString()).toBe('1\n');
        expect(Buffer.concat(await read({ end: 0 })).toString()).toBe('f');
        expect(Buffer.concat(await read({ start: 5, end: 100 })).toString()).toBe('\n');
        expect(Buffer.concat(await read({ start: 6 })).toString()).toBe('');
      });

      itremote('streams a larger file identically to readFileSync', async () => {
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
          expect(content.equals(expected)).toBe(true);
        }
      });

      itremote('streams linked files and files in linked directories', async () => {
        for (const file of ['link1', path.join('link2', 'file1'), path.join('link2', 'link2', 'file1')]) {
          const content = await new Promise<Buffer>((resolve, reject) => {
            const chunks: Buffer[] = [];
            fs.createReadStream(path.join(asarDir, 'a.asar', file))
              .on('data', (c) => chunks.push(c as Buffer))
              .on('error', reject)
              .on('end', () => resolve(Buffer.concat(chunks)));
          });
          expect(content.toString().trim()).toBe('file1');
        }
      });

      itremote('streams unpacked files', async () => {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const content = await new Promise<Buffer>((resolve, reject) => {
          const chunks: Buffer[] = [];
          fs.createReadStream(p)
            .on('data', (c) => chunks.push(c as Buffer))
            .on('error', reject)
            .on('end', () => resolve(Buffer.concat(chunks)));
        });
        expect(content.equals(fs.readFileSync(p))).toBe(true);
      });

      itremote('emits errors for missing files and directories', async () => {
        const errorFor = (p: string) =>
          new Promise<any>((resolve) => {
            fs.createReadStream(p)
              .on('error', resolve)
              .on('data', () => {});
          });
        expect((await errorFor(path.join(asarDir, 'a.asar', 'not-exist'))).code).toBe('ENOENT');
        expect((await errorFor(path.join(asarDir, 'a.asar', 'dir1'))).code).toBe('EISDIR');
      });

      itremote('can be created from an already open packed fd', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fd = fs.openSync(p, 'r');
        const content = await new Promise<Buffer>((resolve, reject) => {
          const chunks: Buffer[] = [];
          fs.createReadStream(null as any, { fd, autoClose: false })
            .on('data', (c) => chunks.push(c as Buffer))
            .on('error', reject)
            .on('end', () => resolve(Buffer.concat(chunks)));
        });
        expect(content.toString()).toBe('file1\n');
        // Not closed by the stream, and the position was consumed.
        expect(fs.readSync(fd, Buffer.alloc(1), 0, 1, null)).toBe(0);
        expect(fs.readSync(fd, Buffer.alloc(1), 0, 1, 0)).toBe(1);
        fs.closeSync(fd);
        // With autoClose the stream closes the descriptor.
        const fd2 = fs.openSync(p, 'r');
        await new Promise<void>((resolve, reject) => {
          fs.createReadStream(null as any, { fd: fd2 })
            .on('error', reject)
            .on('data', () => {})
            .on('close', resolve);
        });
        expect(() => fs.fstatSync(fd2)).toThrow(/EBADF/);
      });

      itremote('can be created from a FileHandle', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const handle = await fs.promises.open(p, 'r');
        const content = await new Promise<Buffer>((resolve, reject) => {
          const chunks: Buffer[] = [];
          fs.createReadStream(null as any, { fd: handle })
            .on('data', (c) => chunks.push(c as Buffer))
            .on('error', reject)
            .on('end', () => resolve(Buffer.concat(chunks)));
        });
        expect(content.toString()).toBe('file1\n');
      });

      itremote('supports many concurrent streams of the same entry', async () => {
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
        for (const content of await Promise.all(streams)) expect(content.equals(expected)).toBe(true);
      });

      itremote('works with pipe()', async () => {
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
        expect(fs.readFileSync(dest).equals(fs.readFileSync(p))).toBe(true);
        fs.unlinkSync(dest);
      });
    });

    describe('fs.createWriteStream on packed files', () => {
      itremote('fails with EACCES', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const err = await new Promise<any>((resolve) => {
          fs.createWriteStream(p).on('error', resolve).end('x');
        });
        expect(err.code).toBe('EACCES');
        expect(fs.readFileSync(p).toString().trim()).toBe('file1');
      });
    });

    describe('write APIs on packed files', () => {
      itremote('fail without touching the archive', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => fs.writeFileSync(p, 'x')).toThrow(/EACCES/);
        expect(() => fs.appendFileSync(p, 'x')).toThrow(/EACCES/);
        await expectToThrowErrorWithCode(() => fs.promises.writeFile(p, 'x'), 'EACCES');
        await expectToThrowErrorWithCode(() => fs.promises.appendFile(p, 'x'), 'EACCES');
        const err = await new Promise<any>((resolve) => fs.writeFile(p, 'x', resolve));
        expect(err.code).toBe('EACCES');
        expect(fs.readFileSync(p).toString().trim()).toBe('file1');
      });
    });

    // Mirrors test-fs-promises-file-handle-read.js, -readFile.js, -stat.js,
    // -readLines.mjs, -stream.js, -close.js, -dispose.js and -chmod.js.
    describe('fs.promises.open FileHandle on packed files', () => {
      itremote('exposes a numeric fd and supports all read forms', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const handle = await fs.promises.open(p, 'r');
        expect(handle.fd).toBeTypeOf('number');
        expect(handle.fd).toBeGreaterThan(2);

        let r = await handle.read(Buffer.alloc(3), 0, 3, 0);
        expect(r.bytesRead).toBe(3);
        expect(r.buffer.toString()).toBe('fil');

        r = await handle.read({ buffer: Buffer.alloc(3), offset: 0, length: 3, position: 3 });
        expect(r.bytesRead).toBe(3);
        expect(r.buffer.toString()).toBe('e1\n');

        r = await handle.read(Buffer.alloc(3), { offset: 0, length: 3, position: 1 });
        expect(r.buffer.toString()).toBe('ile');

        // Default arguments read from the current position into a fresh buffer.
        r = await handle.read();
        expect(r.bytesRead).toBe(6);
        expect(r.buffer.subarray(0, 6).toString()).toBe('file1\n');
        r = await handle.read();
        expect(r.bytesRead).toBe(0);

        // Null position advances; explicit position does not.
        const h2 = await fs.promises.open(p, 'r');
        expect((await h2.read(Buffer.alloc(2), 0, 2, null)).buffer.toString()).toBe('fi');
        expect((await h2.read(Buffer.alloc(2), 0, 2, 0)).buffer.toString()).toBe('fi');
        expect((await h2.read(Buffer.alloc(2), 0, 2, null)).buffer.toString()).toBe('le');
        await h2.close();

        await handle.close();
      });

      itremote('supports readFile, stat and readLines', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const handle = await fs.promises.open(p, 'r');
        const stats = await handle.stat();
        expect(stats.isFile()).toBe(true);
        expect(stats.size).toBe(6);
        expect((await handle.stat({ bigint: true })).size).toBe(6n);
        expect((await handle.readFile()).toString()).toBe('file1\n');
        // Like Node, readFile continues from the current position.
        expect(await handle.readFile('utf8')).toBe('');
        await handle.close();

        const h2 = await fs.promises.open(p, 'r');
        expect(await fs.promises.readFile(h2, 'utf8')).toBe('file1\n');
        await h2.close();

        const h3 = await fs.promises.open(path.join(asarDir, 'a.asar', 'ping.js'), 'r');
        const lines: string[] = [];
        for await (const line of h3.readLines()) lines.push(line);
        expect(lines.join('\n')).toBe(fs.readFileSync(path.join(asarDir, 'a.asar', 'ping.js'), 'utf8').trimEnd());
      });

      itremote('supports createReadStream and readableWebStream', async () => {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const expected = fs.readFileSync(p);
        const handle = await fs.promises.open(p, 'r');
        const chunks: Buffer[] = [];
        for await (const chunk of handle.createReadStream({ start: 10, end: 1009, autoClose: false })) {
          chunks.push(chunk);
        }
        expect(Buffer.concat(chunks).equals(expected.subarray(10, 1010))).toBe(true);
        let total = 0;
        for await (const chunk of handle.readableWebStream()) total += (chunk as Uint8Array).byteLength;
        expect(total).toBe(expected.length);
        await handle.close();
      });

      itremote('stops routing a descriptor number as soon as close() is requested', async () => {
        // FileHandle#close() runs close(2) on the threadpool; until then a real
        // file opened right after may be handed the same number. It must never
        // be served from the archive-backed reader.
        const real = tempPath();
        fs.writeFileSync(real, 'real-file-content');
        const p = path.join(asarDir, 'a.asar', 'file1');
        for (let i = 0; i < 200; i++) {
          const handle = await fs.promises.open(p, 'r');
          const closing = handle.close();
          const fd = fs.openSync(real, 'r');
          const buffer = Buffer.alloc(17);
          expect(fs.readSync(fd, buffer, 0, 17, 0)).toBe(17);
          expect(buffer.toString()).toBe('real-file-content');
          expect(fs.fstatSync(fd).size).toBe(17);
          fs.closeSync(fd);
          await closing;
        }
      });

      itremote('does not capture a descriptor number that was closed behind its back', async () => {
        // Something outside fs (a native FileHandle built on the number, a
        // handle moved to a worker, ...) can close a packed descriptor
        // without going through fs.close. Simulate that with original-fs and
        // check the next owner of the number is served natively.
        const originalFs = require('node:original-fs');
        const real = tempPath();
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
          expect(fs.readSync(fd, buffer, 0, 17, 0)).toBe(17);
          expect(buffer.toString()).toBe('real-file-content');
          expect(fs.fstatSync(fd).size).toBe(17);
          if (variant !== 'handle') fs.closeSync(fd);
          // A packed open that gets the same number again works too.
          const again = fs.openSync(p, 'r');
          expect(fs.readFileSync(again).toString()).toBe('file1\n');
          fs.closeSync(again);
        }
      });

      itremote('close semantics match Node', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const handle = await fs.promises.open(p, 'r');
        await handle.close();
        expect(handle.fd).toBe(-1);
        await expectToThrowErrorWithCode(() => handle.read(Buffer.alloc(1), 0, 1, 0), 'EBADF');
        // Closing twice resolves (Node returns the same promise / a resolved one).
        await handle.close();

        const disposable = await fs.promises.open(p, 'r');
        await disposable[Symbol.asyncDispose]();
        expect(disposable.fd).toBe(-1);
      });

      itremote('refuses metadata and write operations', async () => {
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
        expect(fs.readFileSync(p).toString().trim()).toBe('file1');
      });

      itremote('rejects write flags, missing files and directories', async () => {
        await expectToThrowErrorWithCode(() => fs.promises.open(path.join(asarDir, 'a.asar', 'file1'), 'w'), 'EACCES');
        await expectToThrowErrorWithCode(() => fs.promises.open(path.join(asarDir, 'a.asar', 'file1'), 'a+'), 'EACCES');
        await expectToThrowErrorWithCode(() => fs.promises.open(path.join(asarDir, 'a.asar', 'dir1'), 'r'), 'EISDIR');
        await expectToThrowErrorWithCode(() => fs.promises.open(path.join(asarDir, 'a.asar', 'nope'), 'r'), 'ENOENT');
      });

      itremote('opens unpacked files as regular handles', async () => {
        const originalFs = require('node:original-fs');
        const handle = await fs.promises.open(path.join(asarDir, 'unpack.asar', 'a.txt'), 'r');
        const stats = await handle.stat();
        expect(stats.ino).toBe(originalFs.statSync(path.join(asarDir, 'unpack.asar.unpacked', 'a.txt')).ino);
        await handle.close();
      });

      itremote('lets many handles coexist and be read concurrently', async () => {
        const p = path.join(asarDir, 'video.asar', 'video.mp4');
        const expected = fs.readFileSync(p);
        const handles = await Promise.all(Array.from({ length: 40 }, () => fs.promises.open(p, 'r')));
        const contents = await Promise.all(handles.map((h) => h.readFile()));
        for (const c of contents) expect(c.equals(expected)).toBe(true);
        await Promise.all(handles.map((h) => h.close()));
      });
    });

    // Mirrors test-fs-copyfile.js.
    describe('fs.copyFile flags and errors', () => {
      itremote('honours COPYFILE_EXCL and overwrites otherwise', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(dest).toString()).toBe('file1\n');
        expect(() => fs.copyFileSync(p, dest, fs.constants.COPYFILE_EXCL)).toThrow(/EEXIST/);
        await expectToThrowErrorWithCode(() => fs.promises.copyFile(p, dest, fs.constants.COPYFILE_EXCL), 'EEXIST');
        const err = await new Promise<any>((resolve) => fs.copyFile(p, dest, fs.constants.COPYFILE_EXCL, resolve));
        expect(err.code).toBe('EEXIST');
        // Overwrite a longer existing file: no trailing garbage.
        fs.writeFileSync(dest, 'a much longer file than the source');
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(dest).toString()).toBe('file1\n');
        // FICLONE is best-effort, FICLONE_FORCE cannot be honoured.
        fs.copyFileSync(p, dest, fs.constants.COPYFILE_FICLONE);
        expect(fs.readFileSync(dest).toString()).toBe('file1\n');
        expect(() => fs.copyFileSync(p, dest, fs.constants.COPYFILE_FICLONE_FORCE)).toThrow(/ENOTSUP/);
      });

      itremote('lets native fs decide about copy-on-write clones of unpacked files', () => {
        const originalFs = require('node:original-fs');
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
        const viaAsar = tempPath();
        const viaOriginal = tempPath();
        // Whatever the filesystem says about reflinks, the answer for an unpacked
        // entry must match the answer for the real file behind it.
        expect(outcome(() => fs.copyFileSync(unpacked, viaAsar, fs.constants.COPYFILE_FICLONE_FORCE))).toBe(
          outcome(() => originalFs.copyFileSync(real, viaOriginal, fs.constants.COPYFILE_FICLONE_FORCE))
        );
        if (fs.existsSync(viaAsar)) expect(fs.readFileSync(viaAsar).equals(fs.readFileSync(real))).toBe(true);
      });

      itremote('validates the mode argument like Node', () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => fs.copyFileSync(p, tempPath(), 8)).toThrow(expect.objectContaining({ code: 'ERR_OUT_OF_RANGE' }));
        expect(() => (fs.copyFileSync as any)(p, tempPath(), 'x')).toThrow(
          expect.objectContaining({ code: 'ERR_INVALID_ARG_TYPE' })
        );
      });

      itremote('reports ENOENT / EISDIR for bad sources', async () => {
        expect(() => fs.copyFileSync(path.join(asarDir, 'a.asar', 'nope'), tempPath())).toThrow(/ENOENT/);
        expect(() => fs.copyFileSync(path.join(asarDir, 'a.asar', 'dir1'), tempPath())).toThrow(/EISDIR/);
        await expectToThrowErrorWithCode(
          () => fs.promises.copyFile(path.join(asarDir, 'a.asar', 'nope'), tempPath()),
          'ENOENT'
        );
        const err = await new Promise<any>((resolve) =>
          fs.copyFile(path.join(asarDir, 'a.asar', 'dir1'), tempPath(), resolve)
        );
        expect(err.code).toBe('EISDIR');
      });

      itremote('copies linked files, larger files and executable bits', async () => {
        const link = path.join(asarDir, 'a.asar', 'link1');
        const d1 = tempPath();
        fs.copyFileSync(link, d1);
        expect(fs.readFileSync(d1).toString()).toBe('file1\n');

        const big = path.join(asarDir, 'video.asar', 'video.mp4');
        const d2 = tempPath();
        await fs.promises.copyFile(big, d2);
        expect(fs.readFileSync(d2).equals(fs.readFileSync(big))).toBe(true);
        const d3 = tempPath();
        await promisify(fs.copyFile)(big, d3);
        expect(fs.readFileSync(d3).equals(fs.readFileSync(big))).toBe(true);

        if (process.platform !== 'win32') {
          const d4 = tempPath();
          fs.copyFileSync(path.join(asarDir, 'echo.asar', 'echo'), d4);
          expect(fs.statSync(d4).mode & 0o111).not.toBe(0);
          const d5 = tempPath();
          fs.copyFileSync(path.join(asarDir, 'a.asar', 'file1'), d5);
          expect(fs.statSync(d5).mode & 0o111).toBe(0);
          // cp chmods the destination to the source's stat mode afterwards, so
          // stat must agree with what copyFile preserved.
          const d6 = tempPath();
          fs.cpSync(path.join(asarDir, 'echo.asar', 'echo'), d6);
          expect(fs.statSync(d6).mode & 0o111).not.toBe(0);
          const d7 = tempPath();
          await fs.promises.cp(path.join(asarDir, 'echo.asar', 'echo'), d7);
          expect(fs.statSync(d7).mode & 0o111).not.toBe(0);
          const d8 = tempPath();
          fs.cpSync(path.join(asarDir, 'echo.asar'), d8, { recursive: true, filter: () => true });
          expect(fs.statSync(path.join(d8, 'echo')).mode & 0o111).not.toBe(0);
        }
      });

      itremote('fails when the destination is inside an archive', async () => {
        const src = tempPath();
        fs.writeFileSync(src, 'x');
        expect(() => fs.copyFileSync(src, path.join(asarDir, 'a.asar', 'new-file'))).toThrow(/ENOTDIR|EACCES|ENOENT/);
        expect(() =>
          fs.copyFileSync(path.join(asarDir, 'a.asar', 'file1'), path.join(asarDir, 'a.asar', 'file2'))
        ).toThrow(/ENOTDIR|EACCES|ENOENT/);
      });
    });

    // Mirrors the test-fs-cp-* family (subset applicable to read-only sources).
    describe('fs.cp / fs.cpSync / fs.promises.cp from archives', () => {
      itremote('copies a directory tree recursively (sync, callback, promises)', async () => {
        const src = path.join(asarDir, 'a.asar');
        const expectedTop = ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js'];
        for (const variant of ['sync', 'callback', 'promises']) {
          const dest = tempPath();
          if (variant === 'sync') fs.cpSync(src, dest, { recursive: true });
          else if (variant === 'callback') await promisify(fs.cp)(src, dest, { recursive: true });
          else await fs.promises.cp(src, dest, { recursive: true });
          expect(fs.readdirSync(dest).sort(), variant).toEqual(expectedTop);
          for (const dir of ['dir1', 'dir2', 'dir3']) {
            expect(fs.statSync(path.join(dest, dir)).isDirectory(), variant).toBe(true);
            for (const file of ['file1', 'file2', 'file3']) {
              expect(
                fs.readFileSync(path.join(dest, dir, file)).equals(fs.readFileSync(path.join(src, dir, file))),
                `${variant} ${dir}/${file}`
              ).toBe(true);
            }
          }
          expect(fs.readFileSync(path.join(dest, 'ping.js')).equals(fs.readFileSync(path.join(src, 'ping.js')))).toBe(
            true
          );
          // Like Node's cp of a real tree, symlinks are preserved and (without
          // verbatimSymlinks) made absolute, i.e. they point back into the archive.
          expect(fs.lstatSync(path.join(dest, 'link1')).isSymbolicLink(), variant).toBe(true);
          expect(fs.readlinkSync(path.join(dest, 'link1')), variant).toBe(path.join(src, 'file1'));
          expect(fs.lstatSync(path.join(dest, 'link2')).isSymbolicLink(), variant).toBe(true);
          expect(fs.readlinkSync(path.join(dest, 'link2')), variant).toBe(path.join(src, 'dir1'));
          expect(fs.lstatSync(path.join(dest, 'dir1', 'link2')).isSymbolicLink(), variant).toBe(true);
          expect(fs.readlinkSync(path.join(dest, 'dir1', 'link2')), variant).toBe(path.join(src, 'dir1'));
        }
      });

      itremote('copies symlinks verbatim when asked, producing a self-contained tree', async () => {
        const src = path.join(asarDir, 'a.asar');
        for (const variant of ['sync', 'promises']) {
          const dest = tempPath();
          if (variant === 'sync') fs.cpSync(src, dest, { recursive: true, verbatimSymlinks: true });
          else await fs.promises.cp(src, dest, { recursive: true, verbatimSymlinks: true });
          expect(fs.readlinkSync(path.join(dest, 'link1')), variant).toBe('file1');
          expect(fs.readlinkSync(path.join(dest, 'link2')), variant).toBe('dir1');
          expect(fs.readlinkSync(path.join(dest, 'dir1', 'link1')), variant).toBe(path.join('..', 'file1'));
          expect(fs.readlinkSync(path.join(dest, 'dir1', 'link2')), variant).toBe('.');
          expect(fs.readFileSync(path.join(dest, 'dir1', 'link1')).toString(), variant).toBe('file1\n');
          // ...which resolve on the real filesystem.
          expect(fs.readFileSync(path.join(dest, 'link1')).toString(), variant).toBe('file1\n');
          expect(fs.readdirSync(path.join(dest, 'link2')).sort(), variant).toEqual(
            fs.readdirSync(path.join(src, 'dir1')).sort()
          );
          expect(fs.readFileSync(path.join(dest, 'link2', 'link2', 'file1')).toString(), variant).toBe('file1\n');
        }
      });

      itremote('requires recursive for directories', async () => {
        const src = path.join(asarDir, 'a.asar');
        // Node reports this as the ERR_FS_EISDIR system error (with EISDIR as its info code).
        expect(() => fs.cpSync(src, tempPath())).toThrow(expect.objectContaining({ code: 'ERR_FS_EISDIR' }));
        await expectToThrowErrorWithCode(() => fs.promises.cp(src, tempPath()), 'ERR_FS_EISDIR');
        const err = await new Promise<any>((resolve) => fs.cp(src, tempPath(), resolve));
        expect(err.code).toBe('ERR_FS_EISDIR');
        expect(err.info.code).toBe('EISDIR');
      });

      itremote('honours errorOnExist and force', async () => {
        const src = path.join(asarDir, 'a.asar', 'file1');
        const dest = tempPath();
        fs.writeFileSync(dest, 'existing');
        // force defaults to true
        fs.cpSync(src, dest);
        expect(fs.readFileSync(dest).toString()).toBe('file1\n');
        fs.writeFileSync(dest, 'existing');
        fs.cpSync(src, dest, { force: false });
        expect(fs.readFileSync(dest).toString()).toBe('existing');
        expect(() => fs.cpSync(src, dest, { force: false, errorOnExist: true })).toThrow(
          expect.objectContaining({ code: 'ERR_FS_CP_EEXIST' })
        );
        await expectToThrowErrorWithCode(
          () => fs.promises.cp(src, dest, { force: false, errorOnExist: true }),
          'ERR_FS_CP_EEXIST'
        );
        await fs.promises.cp(src, dest, { force: false });
        expect(fs.readFileSync(dest).toString()).toBe('existing');
        await fs.promises.cp(src, dest);
        expect(fs.readFileSync(dest).toString()).toBe('file1\n');

        // Directory variants
        const dir = tempPath();
        fs.mkdirSync(path.join(dir, 'dir1'), { recursive: true });
        fs.writeFileSync(path.join(dir, 'dir1', 'file1'), 'existing');
        fs.cpSync(path.join(asarDir, 'a.asar'), dir, { recursive: true, force: false });
        expect(fs.readFileSync(path.join(dir, 'dir1', 'file1')).toString()).toBe('existing');
        expect(fs.readFileSync(path.join(dir, 'dir1', 'file2')).toString()).toBe('file2\n');
        expect(() =>
          fs.cpSync(path.join(asarDir, 'a.asar'), dir, { recursive: true, force: false, errorOnExist: true })
        ).toThrow(/EEXIST/);
        fs.cpSync(path.join(asarDir, 'a.asar'), dir, { recursive: true });
        expect(fs.readFileSync(path.join(dir, 'dir1', 'file1')).toString()).toBe('file1\n');
      });

      itremote('applies filter functions', async () => {
        const src = path.join(asarDir, 'a.asar');
        const dest = tempPath();
        fs.cpSync(src, dest, { recursive: true, filter: (p: string) => !p.endsWith('file2') });
        expect(fs.existsSync(path.join(dest, 'file1'))).toBe(true);
        expect(fs.existsSync(path.join(dest, 'file2'))).toBe(false);
        expect(fs.existsSync(path.join(dest, 'dir1', 'file2'))).toBe(false);
        const dest2 = tempPath();
        await fs.promises.cp(src, dest2, {
          recursive: true,
          filter: async (p: string) => !path.basename(p).startsWith('dir')
        });
        expect(fs.readdirSync(dest2).sort()).toEqual(['file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
        // Symlink to a filtered-out directory still gets created as a link.
        expect(fs.lstatSync(path.join(dest2, 'link2')).isSymbolicLink()).toBe(true);
      });

      itremote('dereferences symlinks when asked', async () => {
        const src = path.join(asarDir, 'a.asar');
        // Note: a.asar's directory links form a cycle (dir1/link2 -> dir1), so a
        // dereferencing copy of the whole tree cannot terminate (Node behaves
        // the same on a real tree); dereference the acyclic parts instead.
        for (const variant of ['sync', 'promises']) {
          const single = tempPath();
          if (variant === 'sync') fs.cpSync(path.join(src, 'link1'), single, { dereference: true });
          else await fs.promises.cp(path.join(src, 'link1'), single, { dereference: true });
          expect(fs.lstatSync(single).isFile(), variant).toBe(true);
          expect(fs.readFileSync(single).toString(), variant).toBe('file1\n');

          const dir = tempPath();
          if (variant === 'sync') fs.cpSync(path.join(src, 'dir2'), dir, { recursive: true, dereference: true });
          else await fs.promises.cp(path.join(src, 'dir2'), dir, { recursive: true, dereference: true });
          expect(fs.readdirSync(dir).sort(), variant).toEqual(['file1', 'file2', 'file3']);
        }
      });

      itremote('errors on non-existent sources and file-to-directory mismatches', async () => {
        expect(() => fs.cpSync(path.join(asarDir, 'a.asar', 'nope'), tempPath())).toThrow(/ENOENT/);
        await expectToThrowErrorWithCode(
          () => fs.promises.cp(path.join(asarDir, 'a.asar', 'nope'), tempPath()),
          'ENOENT'
        );
        const dir = tempPath();
        fs.mkdirSync(dir);
        expect(() => fs.cpSync(path.join(asarDir, 'a.asar', 'file1'), dir)).toThrow(
          expect.objectContaining({ code: 'ERR_FS_CP_NON_DIR_TO_DIR' })
        );
        await expectToThrowErrorWithCode(
          () => fs.promises.cp(path.join(asarDir, 'a.asar', 'file1'), dir),
          'ERR_FS_CP_NON_DIR_TO_DIR'
        );
        const file = tempPath();
        fs.writeFileSync(file, 'x');
        expect(() => fs.cpSync(path.join(asarDir, 'a.asar'), file, { recursive: true })).toThrow(
          expect.objectContaining({ code: 'ERR_FS_CP_DIR_TO_NON_DIR' })
        );
        await expectToThrowErrorWithCode(
          () => fs.promises.cp(path.join(asarDir, 'a.asar'), file, { recursive: true }),
          'ERR_FS_CP_DIR_TO_NON_DIR'
        );
      });

      itremote('copies unpacked files and mixed archives', async () => {
        const dest = tempPath();
        fs.cpSync(path.join(asarDir, 'unpack.asar'), dest, { recursive: true });
        expect(
          fs.readFileSync(path.join(dest, 'a.txt')).equals(fs.readFileSync(path.join(asarDir, 'unpack.asar', 'a.txt')))
        ).toBe(true);
        expect(
          fs
            .readFileSync(path.join(dest, 'atom.png'))
            .equals(fs.readFileSync(path.join(asarDir, 'unpack.asar', 'atom.png')))
        ).toBe(true);
      });
    });

    // Mirrors test-fs-opendir.js.
    describe('fs.opendir / fs.opendirSync / fs.promises.opendir on archives', () => {
      itremote('lists entries with correct types (sync)', () => {
        const dir = fs.opendirSync(path.join(asarDir, 'a.asar'));
        expect(dir.path).toBe(path.join(asarDir, 'a.asar'));
        const entries: Record<string, string> = {};
        let dirent;
        while ((dirent = dir.readSync()) !== null) {
          expect(dirent).toBeInstanceOf(fs.Dirent);
          expect(dirent.parentPath).toBe(path.join(asarDir, 'a.asar'));
          entries[dirent.name] = dirent.isDirectory()
            ? 'dir'
            : dirent.isSymbolicLink()
              ? 'link'
              : dirent.isFile()
                ? 'file'
                : '?';
        }
        dir.closeSync();
        expect(entries).toEqual({
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
        expect(() => dir.readSync()).toThrow(expect.objectContaining({ code: 'ERR_DIR_CLOSED' }));
        expect(() => dir.closeSync()).toThrow(expect.objectContaining({ code: 'ERR_DIR_CLOSED' }));
      });

      itremote('lists entries via callbacks, promises and async iteration', async () => {
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
        expect(names1.sort()).toEqual(expected);

        const dir2 = await fs.promises.opendir(p);
        const names2: string[] = [];
        for await (const dirent of dir2) names2.push(dirent.name);
        expect(names2.sort()).toEqual(expected);
        await expectToThrowErrorWithCode(() => dir2.close(), 'ERR_DIR_CLOSED');

        const dir3 = await fs.promises.opendir(p, { bufferSize: 2 });
        const names3: string[] = [];
        let dirent;
        while ((dirent = await dir3.read()) !== null) names3.push(dirent.name);
        await dir3.close();
        expect(names3.sort()).toEqual(expected);

        // Breaking out of iteration closes the handle.
        const dir4 = await fs.promises.opendir(p);
        for await (const _ of dir4) {
          expect(_).toBeInstanceOf(fs.Dirent);
          break;
        }
        await expectToThrowErrorWithCode(() => dir4.read(), 'ERR_DIR_CLOSED');
      });

      itremote('supports recursive iteration', async () => {
        const p = path.join(asarDir, 'a.asar');
        const dir = await fs.promises.opendir(p, { recursive: true });
        const found: string[] = [];
        for await (const dirent of dir) found.push(path.relative(p, path.join(dirent.parentPath, dirent.name)));
        expect(found).toContain(path.join('dir1', 'file1'));
        expect(found).toContain(path.join('dir3', 'file3'));
        expect(found).toContain('link2');
        // Symlinked directories are not followed (like Node).
        expect(found).not.toContain(path.join('link2', 'file1'));
        const sync = fs.opendirSync(p, { recursive: true });
        const foundSync: string[] = [];
        let dirent;
        while ((dirent = sync.readSync()) !== null) {
          foundSync.push(path.relative(p, path.join(dirent.parentPath, dirent.name)));
        }
        sync.closeSync();
        expect(foundSync.sort()).toEqual(found.sort());
      });

      itremote('lists sub directories, linked directories and empty archives', async () => {
        expect(
          Array.from({ length: 5 }, () => fs.opendirSync(path.join(asarDir, 'a.asar', 'dir1')).readSync()!.name).sort()
        ).toEqual(['file1', 'file1', 'file1', 'file1', 'file1']);
        const linked = fs.opendirSync(path.join(asarDir, 'a.asar', 'link2'));
        const names: string[] = [];
        let dirent;
        while ((dirent = linked.readSync()) !== null) names.push(dirent.name);
        linked.closeSync();
        expect(names.sort()).toEqual(fs.readdirSync(path.join(asarDir, 'a.asar', 'dir1')).sort());
        const empty = fs.opendirSync(path.join(asarDir, 'empty.asar'));
        expect(empty.readSync()!.name).toBe('file1');
        expect(empty.readSync()).toBe(null);
        empty.closeSync();
      });

      itremote('supports the buffer and other name encodings', () => {
        const dir = fs.opendirSync(path.join(asarDir, 'a.asar'), { encoding: 'buffer' as any });
        const dirent = dir.readSync()!;
        expect(Buffer.isBuffer(dirent.name)).toBe(true);
        dir.closeSync();
        const hex = fs.opendirSync(path.join(asarDir, 'a.asar'), { encoding: 'hex' as any });
        const names: string[] = [];
        let d;
        while ((d = hex.readSync()) !== null) names.push(d.name as any);
        hex.closeSync();
        expect(names.sort()).toEqual(
          fs
            .readdirSync(path.join(asarDir, 'a.asar'))
            .map((n) => Buffer.from(n).toString('hex'))
            .sort()
        );
      });

      itremote('reports ENOTDIR and ENOENT', async () => {
        expect(() => fs.opendirSync(path.join(asarDir, 'a.asar', 'file1'))).toThrow(/ENOTDIR/);
        expect(() => fs.opendirSync(path.join(asarDir, 'a.asar', 'nope'))).toThrow(/ENOENT/);
        await expectToThrowErrorWithCode(() => fs.promises.opendir(path.join(asarDir, 'a.asar', 'file1')), 'ENOTDIR');
        const err = await new Promise<any>((resolve) => fs.opendir(path.join(asarDir, 'a.asar', 'nope'), resolve));
        expect(err.code).toBe('ENOENT');
        expect(() => fs.opendirSync(path.join(asarDir, 'a.asar'), { bufferSize: 0 })).toThrow(
          expect.objectContaining({ code: 'ERR_OUT_OF_RANGE' })
        );
      });
    });

    describe('fs.readlink / fs.readlinkSync / fs.promises.readlink on archives', () => {
      itremote('returns link targets relative to the link, like a real filesystem', async () => {
        const a = path.join(asarDir, 'a.asar');
        // In this fixture every link points at a root entry (dir1/link1 -> /file1,
        // dir1/link2 -> /dir1), and asar stores targets relative to the archive
        // root; readlink reports them relative to the link's directory.
        expect(fs.readlinkSync(path.join(a, 'link1'))).toBe('file1');
        expect(fs.readlinkSync(path.join(a, 'link2'))).toBe('dir1');
        expect(fs.readlinkSync(path.join(a, 'dir1', 'link1'))).toBe(path.join('..', 'file1'));
        expect(fs.readlinkSync(path.join(a, 'dir1', 'link2'))).toBe('.');
        expect(fs.readlinkSync(path.join(a, 'link2', 'link1'))).toBe(path.join('..', 'file1'));
        expect(await fs.promises.readlink(path.join(a, 'link1'))).toBe('file1');
        expect(await promisify(fs.readlink)(path.join(a, 'link1'))).toBe('file1');
        const asBuffer = fs.readlinkSync(path.join(a, 'link1'), 'buffer');
        expect(Buffer.isBuffer(asBuffer)).toBe(true);
        expect(asBuffer.toString()).toBe('file1');
        expect(fs.readlinkSync(path.join(a, 'link1'), { encoding: 'utf8' })).toBe('file1');
        expect(fs.readlinkSync(path.join(a, 'link1'), 'hex')).toBe(Buffer.from('file1').toString('hex'));
        expect(fs.readlinkSync(path.join(a, 'link1'), { encoding: 'base64' })).toBe(
          Buffer.from('file1').toString('base64')
        );
        // Resolving the target against the link's directory gives a readable path.
        const resolve = (link: string) => path.resolve(path.dirname(link), fs.readlinkSync(link));
        expect(fs.readFileSync(resolve(path.join(a, 'link1'))).toString()).toBe('file1\n');
        expect(fs.readdirSync(resolve(path.join(a, 'link2'))).sort()).toEqual(
          fs.readdirSync(path.join(a, 'dir1')).sort()
        );
        expect(fs.readFileSync(resolve(path.join(a, 'link2', 'link1'))).toString()).toBe('file1\n');
      });

      itremote('reports EINVAL for non-links and ENOENT for missing paths', async () => {
        const a = path.join(asarDir, 'a.asar');
        expect(() => fs.readlinkSync(path.join(a, 'file1'))).toThrow(/EINVAL/);
        expect(() => fs.readlinkSync(path.join(a, 'dir1'))).toThrow(/EINVAL/);
        expect(() => fs.readlinkSync(path.join(a, 'nope'))).toThrow(/ENOENT/);
        await expectToThrowErrorWithCode(() => fs.promises.readlink(path.join(a, 'file1')), 'EINVAL');
        const err = await new Promise<any>((resolve) => fs.readlink(path.join(a, 'nope'), resolve));
        expect(err.code).toBe('ENOENT');
      });
    });

    describe('fs.stat follows symbolic links inside archives', () => {
      itremote('reports the link target for stat and the link itself for lstat', async () => {
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
            expect(stats.isSymbolicLink(), link).toBe(false);
            expect(stats.isFile(), link).toBe(expected === 'file');
            expect(stats.isDirectory(), link).toBe(expected === 'dir');
            if (expected === 'file') expect(Number(stats.size), link).toBe(6);
          }
          expect(fs.lstatSync(p).isSymbolicLink(), link).toBe(true);
          expect((await fs.promises.lstat(p)).isSymbolicLink(), link).toBe(true);
        }
        expect(fs.statSync(path.join(a, 'link1'), { throwIfNoEntry: false })!.isFile()).toBe(true);
        expect(fs.statSync(path.join(a, 'nope'), { throwIfNoEntry: false })).toBe(null as any);
        expect(() => fs.statSync(path.join(a, 'nope'))).toThrow(/ENOENT/);
        await expectToThrowErrorWithCode(() => fs.promises.stat(path.join(a, 'nope')), 'ENOENT');
      });

      itremote('resolves paths through symlinked directories for module loading', () => {
        // require() relies on internalModuleStat, which must follow links to
        // recognise directories.
        expect(fs.readdirSync(path.join(asarDir, 'a.asar', 'link2'))).toContain('file1');
        expect(fs.readFileSync(path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1')).toString()).toBe('file1\n');
        expect(fs.existsSync(path.join(asarDir, 'a.asar', 'link2', 'link1'))).toBe(true);
      });
    });

    describe('fs.stat / fs.lstat with bigint on archives', () => {
      itremote('returns BigIntStats when requested', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const s = fs.statSync(p, { bigint: true });
        expect(typeof s.size).toBe('bigint');
        expect(s.size).toBe(6n);
        expect(typeof s.mtimeMs).toBe('bigint');
        expect(typeof s.mtimeNs).toBe('bigint');
        expect(s.isFile()).toBe(true);
        expect(fs.lstatSync(path.join(asarDir, 'a.asar', 'link1'), { bigint: true }).isSymbolicLink()).toBe(true);
        expect(fs.lstatSync(path.join(asarDir, 'a.asar', 'dir1'), { bigint: true }).isDirectory()).toBe(true);
        expect((await fs.promises.stat(p, { bigint: true })).size).toBe(6n);
        expect((await fs.promises.lstat(p, { bigint: true })).size).toBe(6n);
        const cb = await new Promise<any>((resolve, reject) =>
          fs.stat(p, { bigint: true }, (e, s) => (e ? reject(e) : resolve(s)))
        );
        expect(cb.size).toBe(6n);
        // Plain stats are still numbers.
        expect(typeof fs.statSync(p).size).toBe('number');
        expect(fs.statSync(p).mtime).toBeInstanceOf(Date);
        expect(fs.statSync(p, { bigint: false }).size).toBe(6);
      });
    });

    describe('fs.exists on an invalid archive', () => {
      itremote('reports false rather than an error object', async () => {
        const p = path.join(asarDir, 'not-an-archive.asar', 'file');
        expect(fs.existsSync(p)).toBe(false);
        const exists = await new Promise((resolve) => fs.exists(p, resolve));
        expect(exists).toBe(false);
      });
    });

    describe('fs.readFileSync options handling', () => {
      itremote('returns a Buffer for empty files when options is an object without an encoding', () => {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        expect(Buffer.isBuffer(fs.readFileSync(p, { flag: 'r' }))).toBe(true);
        expect(Buffer.isBuffer(fs.readFileSync(p, { encoding: null }))).toBe(true);
        expect(fs.readFileSync(p, { encoding: 'utf8' })).toBe('');
      });
    });

    describe('original-fs is not affected by the archive-aware fs overrides', () => {
      itremote('treats archives as plain files through every fd, stream, copy and dir API', async () => {
        const originalFs = require('node:original-fs');
        const archive = path.join(asarDir, 'a.asar');
        const rawSize = originalFs.statSync(archive).size;
        expect(originalFs.statSync(archive).isFile()).toBe(true);
        expect(rawSize).toBeGreaterThan(1000);
        const raw = originalFs.readFileSync(archive);
        expect(raw.length).toBe(rawSize);
        expect(raw.subarray(0, 4).readUInt32LE(0)).toBe(4); // asar pickle header

        // fd-based access reads the archive itself.
        const fd = originalFs.openSync(archive, 'r');
        expect(originalFs.fstatSync(fd).size).toBe(rawSize);
        const head = Buffer.alloc(16);
        expect(originalFs.readSync(fd, head, 0, 16, 0)).toBe(16);
        expect(head.equals(raw.subarray(0, 16))).toBe(true);
        expect(originalFs.readFileSync(fd).length).toBe(rawSize);
        expect(originalFs.readvSync(fd, [Buffer.alloc(8)], 0)).toBe(8);
        originalFs.closeSync(fd);

        const handle = await originalFs.promises.open(archive, 'r');
        expect((await handle.stat()).size).toBe(rawSize);
        expect((await handle.readFile()).length).toBe(rawSize);
        await handle.close();

        const streamed = await new Promise<Buffer>((resolve, reject) => {
          const chunks: Buffer[] = [];
          originalFs
            .createReadStream(archive)
            .on('data', (c: Buffer) => chunks.push(c))
            .on('error', reject)
            .on('end', () => resolve(Buffer.concat(chunks)));
        });
        expect(streamed.equals(raw)).toBe(true);

        // Paths "inside" the archive are just non-existent for original-fs.
        expect(() => originalFs.openSync(path.join(archive, 'file1'), 'r')).toThrow(/ENOTDIR|ENOENT/);
        expect(() => originalFs.readlinkSync(path.join(archive, 'link1'))).toThrow(/ENOTDIR|ENOENT/);
        expect(() => originalFs.opendirSync(path.join(archive, 'dir1'))).toThrow(/ENOTDIR|ENOENT/);
        expect(() => originalFs.opendirSync(archive)).toThrow(/ENOTDIR/);
        await expectToThrowErrorWithCode(
          () => originalFs.promises.open(path.join(archive, 'file1'), 'r'),
          process.platform === 'win32' ? 'ENOENT' : 'ENOTDIR'
        );

        // Copies copy the archive file itself.
        const d1 = tempPath();
        originalFs.copyFileSync(archive, d1);
        expect(originalFs.readFileSync(d1).equals(raw)).toBe(true);
        const d2 = tempPath();
        await originalFs.promises.copyFile(archive, d2);
        expect(originalFs.readFileSync(d2).equals(raw)).toBe(true);
        const d3 = tempPath();
        originalFs.cpSync(archive, d3);
        expect(originalFs.readFileSync(d3).equals(raw)).toBe(true);
        const d4 = tempPath();
        await originalFs.promises.cp(archive, d4);
        expect(originalFs.readFileSync(d4).equals(raw)).toBe(true);

        // And a directory listing of the fixtures dir shows the archive as a file.
        const dir = originalFs.opendirSync(asarDir);
        let dirent;
        let seen = false;
        while ((dirent = dir.readSync()) !== null) {
          if (dirent.name === 'a.asar') {
            seen = true;
            expect(dirent.isFile()).toBe(true);
          }
        }
        dir.closeSync();
        expect(seen).toBe(true);

        // Writing through original-fs to a path that merely mentions .asar works.
        const scratch = path.join(
          fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'asar-original-fs-')),
          'not-really.asar'
        );
        originalFs.writeFileSync(scratch, 'hello');
        expect(originalFs.readFileSync(scratch, 'utf8')).toBe('hello');
        const wfd = originalFs.openSync(scratch, 'w');
        originalFs.writeSync(wfd, 'bye');
        originalFs.closeSync(wfd);
        expect(originalFs.readFileSync(scratch, 'utf8')).toBe('bye');
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

      itremote('throws error when calling recursively inside asar archive', () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.mkdirSync(p, { recursive: true });
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
        const originalFs = require('node:original-fs');
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
        const originalFs = require('node:original-fs');
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
      const originalFs = require('node:original-fs');
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
      const originalFs = require('node:original-fs');
      originalFs.createReadStream(path.join(asarDir, 'a.asar'));
    });

    itremote('can recursively delete a directory with an asar file in itremote using rmdirSync', () => {
      const deleteDir = path.join(asarDir, 'deleteme');
      fs.mkdirSync(deleteDir);

      const originalFs = require('node:original-fs');
      originalFs.rmdirSync(deleteDir, { recursive: true });

      expect(fs.existsSync(deleteDir)).toBe(false);
    });

    itremote('can recursively delete a directory with an asar file in itremote using promises.rmdir', async () => {
      const deleteDir = path.join(asarDir, 'deleteme');
      fs.mkdirSync(deleteDir);

      const originalFs = require('node:original-fs');
      await originalFs.promises.rmdir(deleteDir, { recursive: true });

      expect(fs.existsSync(deleteDir)).toBe(false);
    });

    itremote('has the same APIs as fs', () => {
      expect(Object.keys(require('node:fs'))).toEqual(Object.keys(require('node:original-fs')));
      expect(Object.keys(require('node:fs').promises)).toEqual(Object.keys(require('node:original-fs').promises));
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
