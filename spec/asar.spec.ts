import { BrowserWindow, ipcMain } from 'electron/main';

import { expect } from 'chai';

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
      expect(downloading, 'the viewer never started a download').to.be.an('array');
      const item = downloading![1] as Electron.DownloadItem;
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

  describe('internals', function () {
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
    const writeAsar = (file: string, tree: Record<string, any>) => {
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
      const json = Buffer.from(JSON.stringify({ files: build(tree) }));
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

    before(function () {
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

    after(function () {
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

    describe('splitPath (native archive prefix detection)', function () {
      it('finds the archive component and returns its length in JS string units', function () {
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
          expect(splitPath(input, true), input).to.equal(expected);
          expect(splitPath(input, false), input).to.equal(expected);
        }
      });

      it('returns -1 for paths that are not inside an archive', function () {
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
          expect(splitPath(input, true), JSON.stringify(input)).to.equal(-1);
        }
      });

      it('leaves paths with control characters to the real filesystem, like Archive::New', function () {
        if (process.platform !== 'win32') {
          const odd = j(tmp, 'tab\there');
          originalFs.mkdirSync(odd);
          originalFs.copyFileSync(archive, j(odd, 'app.asar'));
          expect(splitPath(j(odd, 'app.asar', 'a.txt'), true)).to.equal(-1);
          expect(fs.statSync(j(odd, 'app.asar')).isFile()).to.equal(true);
          expect(fs.readFileSync(j(odd, 'app.asar')).length).to.be.greaterThan(8);
        }
        expect(splitPath(j(archive, 'a\u0000b'), true)).to.equal(-1);
        expect(() => fs.readFileSync(j(archive, 'a\u0000b'))).to.throw(
          /must be .* without null bytes|ERR_INVALID_ARG_VALUE/
        );
      });

      it('ignores non-string arguments', function () {
        for (const input of [undefined, null, 42, {}, Buffer.from(archive), [archive]]) {
          expect(splitPath(input as any, true)).to.equal(-1);
        }
      });

      it('picks the deepest *.asar component that is not a directory on disk', function () {
        // An entry named *.asar inside an archive cannot be a directory on
        // disk, so it becomes the archive path (and later fails to open):
        // nested archives are not supported, matching GetAsarArchivePath().
        const nested = j(archive, 'inner.asar', 'x');
        expect(splitPath(nested, true)).to.equal(j(archive, 'inner.asar').length);
        // A *.asar that does not exist at all is still "not a directory".
        const ghost = j(tmp, 'plain', 'ghost.asar', 'x');
        expect(splitPath(ghost, true)).to.equal(j(tmp, 'plain', 'ghost.asar').length);
        // A plain file named *.asar is accepted here; opening it is what fails.
        expect(splitPath(j(bogusArchive, 'x'), true)).to.equal(bogusArchive.length);
        expect(() => fs.readFileSync(j(bogusArchive, 'x'))).to.throw(/Invalid package/);
      });

      it('matches the extension case-insensitively, like base::FilePath', function () {
        const dir = j(tmp, 'plain');
        for (const name of ['x.ASAR', 'x.Asar', 'x.aSaR', '.asar', 'x.y.asar', '..asar']) {
          expect(splitPath(j(dir, name, 'f'), true), name).to.equal(j(dir, name).length);
        }
        for (const name of ['x.asar.', 'x.asar ', 'x.asa', 'x.tasar', 'asar.x']) {
          expect(splitPath(j(dir, name, 'f'), true), name).to.equal(-1);
        }
      });

      it('reports paths that need lexical normalization instead of guessing', function () {
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
          expect(splitPath(input, true), input).to.equal(-2);
          expect(splitPath(input, false), input).to.be.greaterThan(0);
        }
        // No archive component at all: never -2, whatever the shape.
        expect(splitPath(j(tmp, '.', 'plain', '..', 'x'), true)).to.equal(-1);
        // Dots inside names are not dot components.
        expect(splitPath(j(archive, '.hidden'), true)).to.equal(archive.length);
        expect(splitPath(j(archive, 'a.txt.bak'), true)).to.equal(archive.length);
        expect(splitPath(j(archive, '...'), true)).to.equal(archive.length);
      });

      it('handles relative paths against the current directory', function () {
        const cwd = process.cwd();
        try {
          process.chdir(j(tmp, 'plain'));
          expect(splitPath(j('app.asar', 'a.txt'), true)).to.equal('app.asar'.length);
          expect(fs.readFileSync(j('app.asar', 'a.txt'), 'utf8')).to.equal('alpha');
          process.chdir(tmp);
          expect(splitPath(j('looks.asar', 'plain.txt'), true)).to.equal(-1);
          expect(fs.readFileSync(j('looks.asar', 'plain.txt'), 'utf8')).to.equal('plain');
        } finally {
          process.chdir(cwd);
        }
      });

      it('re-evaluates a *.asar path once something exists there', function () {
        const laterDir = j(tmp, 'later-dir.asar');
        const laterFile = j(tmp, 'later-file.asar');
        // Nothing there yet: not a directory, so provisionally an archive path.
        expect(splitPath(j(laterDir, 'x'), true)).to.equal(laterDir.length);
        expect(splitPath(j(laterFile, 'a.txt'), true)).to.equal(laterFile.length);
        expect(fs.existsSync(j(laterFile, 'a.txt'))).to.equal(false);
        // The wrapped mkdir (win32) probes before creating; it must not poison later lookups.
        fs.mkdirSync(laterDir);
        fs.writeFileSync(j(laterDir, 'x'), 'plain file');
        expect(splitPath(j(laterDir, 'x'), true)).to.equal(-1);
        expect(fs.readFileSync(j(laterDir, 'x'), 'utf8')).to.equal('plain file');
        writeAsar(laterFile, tree);
        expect(splitPath(j(laterFile, 'a.txt'), true)).to.equal(laterFile.length);
        expect(fs.readFileSync(j(laterFile, 'a.txt'), 'utf8')).to.equal('alpha');
      });

      it('gives stable answers under repetition and across many distinct prefixes', function () {
        for (let i = 0; i < 1000; i++) {
          expect(splitPath(j(archive, 'dir', `f${i}`), true)).to.equal(archive.length);
        }
        // Distinct ghost archives exercise the prefix memo without touching disk state.
        for (let i = 0; i < 5000; i++) {
          const ghost = j(tmp, 'plain', `g${i}.asar`);
          expect(splitPath(j(ghost, 'x'), true)).to.equal(ghost.length);
        }
        expect(splitPath(j(dirNamedAsar, 'plain.txt'), true)).to.equal(-1);
        expect(splitPath(j(archive, 'a.txt'), true)).to.equal(archive.length);
      });

      if (process.platform === 'win32') {
        it('accepts either separator on Windows', function () {
          expect(splitPath(archive.replace(/\\/g, '/') + '/a.txt', false)).to.equal(archive.length);
          expect(splitPath(archive + '/dir\\b.txt', false)).to.equal(archive.length);
        });
      } else {
        it('treats a backslash as an ordinary file name character on POSIX', function () {
          expect(splitPath(archive + '\\a.txt', true)).to.equal(-1);
          expect(splitPath(j(tmp, 'plain', 'app.asar\\x', 'y'), true)).to.equal(-1);
        });
      }
    });

    describe('Archive lookups', function () {
      let a: NodeJS.AsarArchive;
      const kFile = 1;
      const kDir = 2;
      const kLink = 3;
      before(function () {
        a = new asarBinding.Archive(archive);
      });

      it('stats files, directories and links without following the final link', function () {
        expect(a.stat('')).to.include({ type: kDir });
        expect(a.stat('a.txt')).to.include({ type: kFile, size: 5, executable: false });
        expect(a.stat('run.sh')).to.include({ type: kFile, executable: true });
        expect(a.stat('empty')).to.include({ type: kFile, size: 0 });
        expect(a.stat('dir')).to.include({ type: kDir });
        expect(a.stat(j('dir', 'sub', 'deeper'))).to.include({ type: kDir });
        expect(a.stat(j('dir', 'sub', 'deeper', 'd.txt'))).to.include({ type: kFile, size: 5 });
        expect(a.stat('file.lnk')).to.include({ type: kLink });
        expect(a.stat('dir.lnk')).to.include({ type: kLink });
        expect(a.stat('dangling.lnk')).to.include({ type: kLink });
        expect(a.stat(j(unicodeDir, unicodeName))).to.include({
          type: kFile,
          size: Buffer.byteLength('unicode-content')
        });
        expect(a.stat(j(unicodeDir, astralName))).to.include({ type: kFile, size: 6 });
      });

      it('returns false for anything that does not resolve', function () {
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
          expect(a.stat(p), p).to.equal(false);
          expect(a.getFileInfo(p), p).to.equal(false);
          expect(a.readdir(p), p).to.equal(false);
          expect(a.readdirWithTypes(p), p).to.equal(false);
        }
        expect(a.realpath('missing')).to.equal(false);
      });

      it('walks through directory links in the middle of a path', function () {
        expect(a.stat(j('dir.lnk', 'b.txt'))).to.include({ type: kFile, size: 5 });
        expect(a.stat(j('dir.lnk', 'sub', 'c.txt'))).to.include({ type: kFile });
        expect(a.stat(j('dir', 'self.lnk', 'self.lnk', 'b.txt'))).to.include({ type: kFile });
        expect(a.stat(j('dir.lnk', 'up.lnk'))).to.include({ type: kLink });
        expect(a.readdir(j('dir', 'self.lnk'))).to.deep.equal(a.readdir('dir'));
      });

      it('tolerates leading, trailing and doubled separators', function () {
        expect(a.stat(sep + 'a.txt')).to.include({ type: kFile, size: 5 });
        expect(a.stat('dir' + sep)).to.include({ type: kDir });
        expect(a.stat('dir' + sep + sep + 'b.txt')).to.include({ type: kFile });
        expect(a.readdir('dir' + sep)).to.deep.equal(a.readdir('dir'));
        expect(a.stat(sep)).to.include({ type: kDir });
      });

      it('getFileInfo follows links to files and reports offsets in file order', function () {
        const first = a.getFileInfo('a.txt');
        expect(first).to.include({ size: 5, unpacked: false, executable: false });
        expect(a.getFileInfo('file.lnk')).to.deep.equal(first);
        expect(a.getFileInfo('chain.lnk')).to.deep.equal(first);
        expect(a.getFileInfo(j('dir', 'up.lnk'))).to.deep.equal(first);
        expect(a.getFileInfo('dangling.lnk')).to.equal(false);
        expect(a.getFileInfo('loop1.lnk')).to.equal(false);
        expect(a.getFileInfo('run.sh')).to.include({ executable: true });
        const second = a.getFileInfo('run.sh');
        expect(first && second && second.offset - first.offset).to.equal(5);
        expect(first && (first as any).integrity).to.equal(undefined);
      });

      it('realpath resolves only the final component when it is a link', function () {
        expect(a.realpath('a.txt')).to.equal('a.txt');
        expect(a.realpath('file.lnk')).to.equal('a.txt');
        expect(a.realpath('chain.lnk')).to.equal('file.lnk');
        expect(a.realpath('dir.lnk')).to.equal('dir');
        expect(a.realpath(j('dir.lnk', 'b.txt'))).to.equal(j('dir.lnk', 'b.txt'));
        expect(a.realpath('dangling.lnk')).to.equal('missing');
        expect(a.realpath('')).to.equal('');
      });

      it('readdir lists entry names and readdirWithTypes agrees with stat', function () {
        const rootNames = a.readdir('');
        expect(rootNames).to.be.an('array').that.includes.members(['a.txt', 'dir', 'file.lnk', unicodeDir, '.hidden']);
        expect(rootNames).to.have.lengthOf(Object.keys(tree).length);
        expect(a.readdir('a.txt')).to.equal(false);
        for (const dir of ['', 'dir', j('dir', 'sub'), 'dir.lnk', unicodeDir]) {
          const listing = a.readdirWithTypes(dir);
          expect(listing, dir).to.not.equal(false);
          const [names, types] = listing as [string[], number[]];
          expect(names).to.deep.equal(a.readdir(dir));
          expect(types).to.have.lengthOf(names.length);
          names.forEach((name, i) => {
            const stats = a.stat(dir ? j(dir, name) : name);
            expect(stats && stats.type, j(dir, name)).to.equal(types[i]);
          });
        }
        expect(a.readdirWithTypes(unicodeDir)).to.deep.equal([[unicodeName, astralName].sort(), [kFile, kFile]]);
      });

      it('returns objects of a stable shape and fresh identity', function () {
        const s1 = a.stat('a.txt');
        const s2 = a.stat('a.txt');
        expect(s1).to.deep.equal(s2);
        expect(s1).to.not.equal(s2);
        expect(Object.keys(s1 as object)).to.deep.equal(['size', 'offset', 'type', 'executable']);
        expect(Object.keys(a.getFileInfo('a.txt') as object)).to.deep.equal([
          'size',
          'unpacked',
          'offset',
          'executable',
          'integrity'
        ]);
        (s1 as any).size = 123;
        expect(a.stat('a.txt')).to.include({ size: 5 });
      });

      it('keeps answering correctly past the lookup memo limit', function () {
        for (let i = 0; i < 33 * 1024; i++) {
          if (a.stat(`missing-${i}`) !== false) throw new Error(`missing-${i} resolved`);
        }
        expect(a.stat('a.txt')).to.include({ type: kFile, size: 5 });
        expect(a.stat(j('dir', 'sub', 'c.txt'))).to.include({ type: kFile });
        expect(a.stat('missing-0')).to.equal(false);
      });

      it('serves concurrent lookups from worker threads', async function () {
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
        expect(mainOk).to.equal(2000);
        expect(await Promise.all(results)).to.deep.equal([4000, 4000, 4000, 4000]);
        await Promise.all(workers.map((w) => w.terminate()));
      });
    });

    describe('fs on archives in unusual locations', function () {
      it('works under a non-ASCII directory and with non-ASCII entry names', function () {
        expect(fs.readFileSync(j(unicodeArchive, 'a.txt'), 'utf8')).to.equal('alpha');
        expect(fs.readFileSync(j(unicodeArchive, unicodeDir, unicodeName), 'utf8')).to.equal('unicode-content');
        expect(fs.readFileSync(j(unicodeArchive, unicodeDir, astralName), 'utf8')).to.equal('astral');
        expect(fs.statSync(j(unicodeArchive, unicodeDir)).isDirectory()).to.equal(true);
        expect(fs.readdirSync(j(unicodeArchive, unicodeDir))).to.have.members([unicodeName, astralName]);
        expect(fs.realpathSync(j(unicodeArchive, 'file.lnk'))).to.equal(j(unicodeArchive, 'a.txt'));
        expect(fs.existsSync(j(unicodeArchive, unicodeDir, 'nope'))).to.equal(false);
      });

      it('works when the archive extension is upper case', function () {
        expect(fs.readFileSync(j(upperArchive, 'dir', 'b.txt'), 'utf8')).to.equal('bravo');
        expect(fs.statSync(upperArchive).isDirectory()).to.equal(true);
      });

      it('works for an archive inside a real directory named *.asar', function () {
        expect(fs.readFileSync(j(archiveInDirNamedAsar, 'a.txt'), 'utf8')).to.equal('alpha');
        expect(fs.readFileSync(j(dirNamedAsar, 'plain.txt'), 'utf8')).to.equal('plain');
        expect(fs.readdirSync(dirNamedAsar)).to.have.members(['plain.txt', 'real.asar']);
      });

      it('accepts Buffer and file: URL paths', function () {
        expect(fs.readFileSync(Buffer.from(j(archive, 'a.txt')), 'utf8')).to.equal('alpha');
        expect(fs.readFileSync(url.pathToFileURL(j(archive, 'dir', 'b.txt')), 'utf8')).to.equal('bravo');
        expect(fs.existsSync(url.pathToFileURL(j(unicodeArchive, unicodeDir, astralName)) as any)).to.equal(true);
      });

      it('accepts un-normalized paths into an archive', function () {
        expect(fs.readFileSync([archive, 'dir', '.', '..', 'dir', 'b.txt'].join(sep), 'utf8')).to.equal('bravo');
        expect(fs.statSync(archive + sep + sep + 'dir' + sep + sep + 'b.txt' + sep).isFile()).to.equal(true);
        expect(fs.existsSync([archive, '..', 'app.asar', 'dir'].join(sep))).to.equal(true);
        expect(fs.existsSync([archive, 'dir', '..', '..', 'app.asar'].join(sep))).to.equal(true);
        expect(fs.existsSync([archive, 'dir', '..', '..', 'nope.asar'].join(sep))).to.equal(false);
        expect(fs.readdirSync([tmp, 'plain', '.', 'app.asar', 'dir', ''].join(sep))).to.have.members([
          'b.txt',
          'sub',
          'up.lnk',
          'self.lnk'
        ]);
      });

      it('module resolution accepts un-normalized lookup paths', function () {
        const sloppyDir = archive + sep + '.' + sep + 'dir' + sep;
        expect(require.resolve('./b.txt', { paths: [sloppyDir] })).to.equal(j(archive, 'dir', 'b.txt'));
        expect(require.resolve('./c.txt', { paths: [j(archive, 'dir', '..', 'dir', 'sub')] })).to.equal(
          j(archive, 'dir', 'sub', 'c.txt')
        );
        expect(() => require.resolve('./nope.txt', { paths: [sloppyDir] })).to.throw(/Cannot find module/);
      });

      it('presents the archive root like a directory, with or without a trailing separator', function () {
        expect(fs.statSync(archive).isDirectory()).to.equal(true);
        expect(fs.statSync(archive + sep).isDirectory()).to.equal(true);
        expect(fs.readdirSync(archive + sep)).to.deep.equal(fs.readdirSync(archive));
        const inParent = fs.readdirSync(j(tmp, 'plain'), { withFileTypes: true });
        expect(inParent.find((d) => d.name === 'app.asar')!.isFile()).to.equal(true);
      });

      it('produces Stats consistent with the entry', function () {
        const st = fs.statSync(j(archive, 'run.sh'));
        expect(st.isFile()).to.equal(true);
        expect(st.mode & 0o111).to.not.equal(0);
        expect(fs.statSync(j(archive, 'a.txt')).mode & 0o111).to.equal(0);
        expect(fs.lstatSync(j(archive, 'file.lnk')).isSymbolicLink()).to.equal(true);
        const big = fs.statSync(j(archive, 'a.txt'), { bigint: true });
        expect(big.size).to.equal(5n);
        expect(typeof big.mtimeNs).to.equal('bigint');
        // Two stats in a row must not alias each other (shared scratch buffer).
        const s1 = fs.statSync(j(archive, 'a.txt'));
        const s2 = fs.statSync(j(archive, 'dir', 'sub', 'c.txt'));
        const d = fs.statSync(j(archive, 'dir'));
        expect(s1.size).to.equal(5);
        expect(s2.size).to.equal(7);
        expect(s1.isFile() && s2.isFile()).to.equal(true);
        expect(d.isDirectory()).to.equal(true);
        expect(s1.isDirectory()).to.equal(false);
      });

      it('realpath resolves the archive location once and reuses it', function () {
        const linkDir = j(tmp, 'via-link');
        importedFs.symlinkSync(j(tmp, 'plain'), linkDir, 'dir');
        const viaLink = j(linkDir, 'app.asar');
        expect(fs.realpathSync(j(viaLink, 'file.lnk'))).to.equal(j(archive, 'a.txt'));
        // The native flavour may spell the directory differently (8.3 names
        // on Windows runners), so derive its expectation the same way.
        const nativeArchive = j(originalFs.realpathSync.native(j(tmp, 'plain')), 'app.asar');
        expect(fs.realpathSync.native(j(viaLink, 'dir', 'b.txt'))).to.equal(j(nativeArchive, 'dir', 'b.txt'));
        expect(fs.realpathSync(j(viaLink, 'a.txt'), 'buffer')).to.deep.equal(Buffer.from(j(archive, 'a.txt')));
        expect(fs.realpathSync(j(viaLink, 'a.txt'), { encoding: 'hex' })).to.equal(
          Buffer.from(j(archive, 'a.txt')).toString('hex')
        );
        return new Promise<void>((resolve, reject) => {
          fs.realpath(j(viaLink, 'chain.lnk'), (err, resolved) => {
            if (err) return reject(err);
            try {
              expect(resolved).to.equal(j(archive, 'file.lnk'));
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

describe('asar package', function () {
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

        const chai = require('chai')
        chai.expect(error).to.have.property('code').which.equals(code);
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
        const temp = require('temp').track();
        const dest = temp.path();
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
        const temp = require('temp').track();
        const dest = temp.path();
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
        const temp = require('temp').track();
        const dest = temp.path();
        await fs.promises.copyFile(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });

      itremote('copies a unpacked file', async function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const temp = require('temp').track();
        const dest = temp.path();
        await fs.promises.copyFile(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.copyFileSync', function () {
      itremote('copies a normal file', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        const dest = temp.path();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });

      itremote('copies a unpacked file', function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const temp = require('temp').track();
        const dest = temp.path();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.cpSync', function () {
      itremote('copies a normal file', function () {
        if (!fs.cpSync) return;
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        const dest = temp.path();
        fs.cpSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.cp', function () {
      itremote('copies a normal file', async function () {
        if (!fs.cp) return;
        const p = path.join(asarDir, 'a.asar', 'file1');
        const temp = require('temp').track();
        const dest = temp.path();
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
        const temp = require('temp').track();
        const dest = temp.path();
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
            const code = await new Promise((resolve, reject) => {
              process.once('close', resolve);
              process.once('error', reject);
            });
            expect(code).to.equal(0);
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
        const originalFs = require('original-fs');
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
        const originalFs = require('original-fs');
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
      const originalFs = require('original-fs');
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
      const originalFs = require('original-fs');
      originalFs.createReadStream(path.join(asarDir, 'a.asar'));
    });

    itremote('can recursively delete a directory with an asar file in itremote using rmdirSync', () => {
      const deleteDir = path.join(asarDir, 'deleteme');
      fs.mkdirSync(deleteDir);

      const originalFs = require('original-fs');
      originalFs.rmdirSync(deleteDir, { recursive: true });

      expect(fs.existsSync(deleteDir)).to.be.false();
    });

    itremote('can recursively delete a directory with an asar file in itremote using promises.rmdir', async () => {
      const deleteDir = path.join(asarDir, 'deleteme');
      fs.mkdirSync(deleteDir);

      const originalFs = require('original-fs');
      await originalFs.promises.rmdir(deleteDir, { recursive: true });

      expect(fs.existsSync(deleteDir)).to.be.false();
    });

    itremote('has the same APIs as fs', function () {
      expect(Object.keys(require('node:fs'))).to.deep.equal(Object.keys(require('original-fs')));
      expect(Object.keys(require('node:fs').promises)).to.deep.equal(Object.keys(require('original-fs').promises));
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

  describe('mkdirp module', function () {
    itremote('throws error when calling inside asar archive', function () {
      const mkdirp = require('mkdirp');

      const p = path.join(asarDir, 'a.asar', 'not-exist');
      expect(() => {
        mkdirp.sync(p);
      }).to.throw(/ENOTDIR/);
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
