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
        // eslint-disable-next-line n/no-deprecated-api
        const exists = await new Promise((resolve) => fs.exists(p, resolve));
        expect(exists).toBe(true);
      });

      itremote('handles a non-existent file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        // eslint-disable-next-line n/no-deprecated-api
        const exists = await new Promise((resolve) => fs.exists(p, resolve));
        expect(exists).toBe(false);
      });

      itremote('promisified version handles an existing file', async () => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        // eslint-disable-next-line n/no-deprecated-api
        const exists = await require('node:util').promisify(fs.exists)(p);
        expect(exists).toBe(true);
      });

      itremote('promisified version handles a non-existent file', async () => {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        // eslint-disable-next-line n/no-deprecated-api
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
