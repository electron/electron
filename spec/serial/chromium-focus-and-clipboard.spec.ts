import { clipboard } from 'electron/common';
import { BrowserWindow, ipcMain, session, webContents } from 'electron/main';

import { expect } from 'chai';
import { afterAll, afterEach, beforeAll, beforeEach, describe, it } from 'vitest';

import { once } from 'node:events';
import * as path from 'node:path';
import * as url from 'node:url';

import { withDone } from '../lib/spec-helpers';
import { closeAllWindows } from '../lib/window-helpers';

const fixturesPath = path.resolve(__dirname, '..', 'fixtures');

describe('chromium features (serial)', () => {
  describe('File System API,', () => {
    let w: BrowserWindow | null = null;

    afterEach(closeAllWindows);
    afterEach(() => {
      ipcMain.removeAllListeners('did-create-file-handle');
      ipcMain.removeAllListeners('did-create-directory-handle');
      session.defaultSession.setPermissionCheckHandler(null);
      session.defaultSession.setPermissionRequestHandler(null);
      closeAllWindows();
    });

    it('allows access by default to reading an OPFS file', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'file-system-spec',
          contextIsolation: false
        }
      });

      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const result = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          const root = await navigator.storage.getDirectory();
          const fileHandle = await root.getFileHandle('test', { create: true });
          const { name, size } = await fileHandle.getFile();
          resolve({ name, size });
        }
      )`,
        true
      );
      expect(result).to.deep.equal({ name: 'test', size: 0 });
    });

    it('fileHandle.queryPermission by default has permission to read and write to OPFS files', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'file-system-spec',
          contextIsolation: false
        }
      });

      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const status = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          const root = await navigator.storage.getDirectory();
          const fileHandle = await root.getFileHandle('test', { create: true });
          const status = await fileHandle.queryPermission({ mode: 'readwrite' });
          resolve(status);
        }
      )`,
        true
      );
      expect(status).to.equal('granted');
    });

    it('fileHandle.requestPermission automatically grants permission to read and write to OPFS files', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'file-system-spec',
          contextIsolation: false
        }
      });

      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const status = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          const root = await navigator.storage.getDirectory();
          const fileHandle = await root.getFileHandle('test', { create: true });
          const status = await fileHandle.requestPermission({ mode: 'readwrite' });
          resolve(status);
        }
      )`,
        true
      );
      expect(status).to.equal('granted');
    });

    it(
      'concurrent getFileHandle calls on the same file do not stall',
      withDone((done) => {
        const writablePath = path.join(fixturesPath, 'file-system', 'test-perms.html');
        const testDir = path.join(fixturesPath, 'file-system');
        const testFile = path.join(testDir, 'test.txt');

        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            sandbox: false
          }
        });

        w.webContents.session.setPermissionRequestHandler((wc, permission, callback, details) => {
          if (permission === 'fileSystem') {
            const { href } = url.pathToFileURL(writablePath);
            expect(details).to.deep.equal({
              fileAccessType: 'readable',
              isDirectory: false,
              isMainFrame: true,
              filePath: testFile,
              requestingUrl: href
            });
            callback(true);
          } else {
            callback(false);
          }
        });

        ipcMain.once('did-create-directory-handle', async () => {
          const result = await w.webContents.executeJavaScript(
            `
          new Promise(async (resolve, reject) => {
            try {
              const handles = await Promise.all([
                handle.getFileHandle('test.txt'),
                handle.getFileHandle('test.txt')
              ]);
              resolve(handles.length === 2);
            } catch (err) {
              reject(err.message);
            }
          })
        `,
            true
          );
          expect(result).to.be.true();
          done();
        });

        w.loadFile(writablePath);

        w.webContents.once('did-finish-load', () => {
          // @ts-expect-error Undocumented testing method.
          clipboard._writeFilesForTesting([testDir]);
          w.webContents.focus();
          w.webContents.paste();
        });
      })
    );

    it(
      'allows permission when trying to create a writable file handle',
      withDone((done) => {
        const writablePath = path.join(fixturesPath, 'file-system', 'test-perms.html');
        const testFile = path.join(fixturesPath, 'file-system', 'test.txt');

        const w = new BrowserWindow({
          webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            sandbox: false
          }
        });

        w.webContents.session.setPermissionRequestHandler((wc, permission, callback, details) => {
          if (permission === 'fileSystem') {
            const { href } = url.pathToFileURL(writablePath);
            expect(details).to.deep.equal({
              fileAccessType: 'writable',
              isDirectory: false,
              isMainFrame: true,
              filePath: testFile,
              requestingUrl: href
            });

            callback(true);
            return;
          }
          callback(false);
        });

        ipcMain.once('did-create-file-handle', async () => {
          const result = await w.webContents.executeJavaScript(
            `
          new Promise(async (resolve, reject) => {
            try {
              const writable = await handle.createWritable();
              resolve(true);
            } catch {
              resolve(false);
            }
          })
        `,
            true
          );
          expect(result).to.be.true();
          done();
        });

        w.loadFile(writablePath);

        w.webContents.once('did-finish-load', () => {
          // @ts-expect-error Undocumented testing method.
          clipboard._writeFilesForTesting([testFile]);
          w.webContents.focus();
          w.webContents.paste();
        });
      })
    );

    it(
      'denies permission when trying to create a writable file handle',
      withDone((done) => {
        const writablePath = path.join(fixturesPath, 'file-system', 'test-perms.html');
        const testFile = path.join(fixturesPath, 'file-system', 'test.txt');

        const w = new BrowserWindow({
          webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            sandbox: false
          }
        });

        w.webContents.session.setPermissionRequestHandler((wc, permission, callback, details) => {
          if (permission === 'fileSystem') {
            const { href } = url.pathToFileURL(writablePath);
            expect(details).to.deep.equal({
              fileAccessType: 'writable',
              isDirectory: false,
              isMainFrame: true,
              filePath: testFile,
              requestingUrl: href
            });

            callback(false);
            return;
          }
          callback(false);
        });

        ipcMain.once('did-create-file-handle', async () => {
          const result = await w.webContents.executeJavaScript(
            `
          new Promise(async (resolve, reject) => {
            try {
              const writable = await handle.createWritable();
              resolve(true);
            } catch {
              resolve(false);
            }
          })
        `,
            true
          );
          expect(result).to.be.false();
          done();
        });

        w.loadFile(writablePath);

        w.webContents.once('did-finish-load', () => {
          // @ts-expect-error Undocumented testing method.
          clipboard._writeFilesForTesting([testFile]);
          w.webContents.focus();
          w.webContents.paste();
        });
      })
    );

    it(
      'calls twice when trying to query a read/write file handle permissions',
      withDone((done) => {
        const writablePath = path.join(fixturesPath, 'file-system', 'test-perms.html');
        const testFile = path.join(fixturesPath, 'file-system', 'test.txt');

        const w = new BrowserWindow({
          webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            sandbox: false
          }
        });

        let calls = 0;
        w.webContents.session.setPermissionCheckHandler((wc, permission, origin, details) => {
          if (permission === 'fileSystem') {
            const { fileAccessType, isDirectory, filePath } = details;
            expect(['writable', 'readable']).to.contain(fileAccessType);
            expect(isDirectory).to.be.false();
            expect(filePath).to.equal(testFile);
            calls++;
            return true;
          }

          return false;
        });

        ipcMain.once('did-create-file-handle', async () => {
          const permission = await w.webContents.executeJavaScript(
            `
          new Promise(async (resolve, reject) => {
            try {
              const permission = await handle.queryPermission({ mode: 'readwrite' });
              resolve(permission);
            } catch {
              resolve('denied');
            }
          })
        `,
            true
          );
          expect(permission).to.equal('granted');
          expect(calls).to.equal(2);
          done();
        });

        w.loadFile(writablePath);

        w.webContents.once('did-finish-load', () => {
          // @ts-expect-error Undocumented testing method.
          clipboard._writeFilesForTesting([testFile]);
          w.webContents.focus();
          w.webContents.paste();
        });
      })
    );

    it(
      'correctly denies permissions after creating a readable directory handle',
      withDone((done) => {
        const permPath = path.join(fixturesPath, 'file-system', 'test-perms.html');
        const testDir = path.join(fixturesPath, 'file-system');

        const w = new BrowserWindow({
          webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            sandbox: false
          }
        });

        w.webContents.session.setPermissionCheckHandler((wc, permission, origin, details) => {
          if (permission === 'fileSystem') {
            const { fileAccessType, isDirectory, filePath } = details;
            expect(fileAccessType).to.equal('readable');
            expect(isDirectory).to.be.true();
            expect(filePath).to.equal(testDir);
            return false;
          }
          return false;
        });

        ipcMain.once('did-create-directory-handle', async () => {
          const permission = await w.webContents.executeJavaScript(
            `
          new Promise(async (resolve, reject) => {
            try {
              const permission = await handle.queryPermission({ mode: 'read' });
              resolve(permission);
            } catch {
              resolve('denied');
            }
          })
        `,
            true
          );
          expect(permission).to.equal('denied');
          done();
        });

        w.loadFile(permPath);

        w.webContents.once('did-finish-load', () => {
          // @ts-expect-error Undocumented testing method.
          clipboard._writeFilesForTesting([testDir]);
          w.webContents.focus();
          w.webContents.paste();
        });
      })
    );

    it(
      'correctly allows permissions after creating a readable directory handle',
      withDone((done) => {
        const permPath = path.join(fixturesPath, 'file-system', 'test-perms.html');
        const testDir = path.join(fixturesPath, 'file-system');

        const w = new BrowserWindow({
          webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            sandbox: false
          }
        });

        w.webContents.session.setPermissionCheckHandler((wc, permission, origin, details) => {
          if (permission === 'fileSystem') {
            const { fileAccessType, isDirectory, filePath } = details;
            expect(fileAccessType).to.equal('readable');
            expect(isDirectory).to.be.true();
            expect(filePath).to.equal(testDir);
            return true;
          }
          return false;
        });

        ipcMain.once('did-create-directory-handle', async () => {
          const permission = await w.webContents.executeJavaScript(
            `
          new Promise(async (resolve, reject) => {
            try {
              const permission = await handle.queryPermission({ mode: 'read' });
              resolve(permission);
            } catch {
              resolve('denied');
            }
          })
        `,
            true
          );
          expect(permission).to.equal('granted');
          done();
        });

        w.loadFile(permPath);

        w.webContents.once('did-finish-load', () => {
          // @ts-expect-error Undocumented testing method.
          clipboard._writeFilesForTesting([testDir]);
          w.webContents.focus();
          w.webContents.paste();
        });
      })
    );

    it(
      'allows in-session persistence of granted file permissions',
      withDone((done) => {
        const writablePath = path.join(fixturesPath, 'file-system', 'test-perms.html');
        const testFile = path.join(fixturesPath, 'file-system', 'persist.txt');

        const w = new BrowserWindow({
          webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            sandbox: false
          }
        });

        w.webContents.session.setPermissionRequestHandler((_wc, _permission, callback) => {
          callback(true);
        });

        w.webContents.session.setPermissionCheckHandler((_wc, permission, _origin, details) => {
          if (permission === 'fileSystem') {
            const { fileAccessType, isDirectory, filePath } = details;
            expect(fileAccessType).to.deep.equal('readable');
            expect(isDirectory).to.be.false();
            expect(filePath).to.equal(testFile);
            return true;
          }
          return false;
        });

        let reload = true;
        ipcMain.on('did-create-file-handle', async () => {
          if (reload) {
            w.webContents.reload();
            reload = false;
          } else {
            const permission = await w.webContents.executeJavaScript(
              `
            new Promise(async (resolve, reject) => {
              try {
                const permission = await handle.queryPermission({ mode: 'read' });
                resolve(permission);
              } catch {
                resolve('denied');
              }
            })
          `,
              true
            );
            expect(permission).to.equal('granted');
            done();
          }
        });

        w.loadFile(writablePath);

        w.webContents.on('did-finish-load', () => {
          // @ts-expect-error Undocumented testing method.
          clipboard._writeFilesForTesting([testFile]);
          w.webContents.focus();
          w.webContents.paste();
        });
      })
    );
  });

  describe('document.hasFocus', () => {
    afterEach(closeAllWindows);

    it('has correct value when multiple windows are opened', async () => {
      const w1 = new BrowserWindow({ show: true });
      const w2 = new BrowserWindow({ show: true });
      const w3 = new BrowserWindow({ show: false });
      await w1.loadFile(path.join(__dirname, '..', 'fixtures', 'blank.html'));
      await w2.loadFile(path.join(__dirname, '..', 'fixtures', 'blank.html'));
      await w3.loadFile(path.join(__dirname, '..', 'fixtures', 'blank.html'));
      expect(webContents.getFocusedWebContents()?.id).to.equal(w2.webContents.id);
      let focus = false;
      focus = await w1.webContents.executeJavaScript('document.hasFocus()');
      expect(focus).to.be.false();
      focus = await w2.webContents.executeJavaScript('document.hasFocus()');
      expect(focus).to.be.true();
      focus = await w3.webContents.executeJavaScript('document.hasFocus()');
      expect(focus).to.be.false();
    });
  });

  describe('navigator.clipboard.read', () => {
    let w: BrowserWindow;
    beforeAll(async () => {
      w = new BrowserWindow();
      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    });
    afterAll(closeAllWindows);

    const readClipboard = async () => {
      if (!w.webContents.isFocused()) {
        const focus = once(w.webContents, 'focus');
        w.webContents.focus();
        await focus;
      }
      return w.webContents.executeJavaScript(
        `
      navigator.clipboard.read().then(clipboard => clipboard.toString()).catch(err => err.message);
    `,
        true
      );
    };

    afterEach(() => {
      session.defaultSession.setPermissionRequestHandler(null);
    });

    it('returns clipboard contents when a PermissionRequestHandler is not defined', async () => {
      const clipboard = await readClipboard();
      expect(clipboard).to.not.contain('Read permission denied.');
    });

    it('returns an error when permission denied', async () => {
      session.defaultSession.setPermissionRequestHandler((wc, permission, callback) => {
        callback(permission !== 'clipboard-read');
      });
      const clipboard = await readClipboard();
      expect(clipboard).to.contain('Read permission denied.');
    });

    it('returns clipboard contents when permission is granted', async () => {
      session.defaultSession.setPermissionRequestHandler((wc, permission, callback) => {
        callback(permission === 'clipboard-read');
      });
      const clipboard = await readClipboard();
      expect(clipboard).to.not.contain('Read permission denied.');
    });
  });

  describe('navigator.clipboard.write', () => {
    let w: BrowserWindow;
    beforeAll(async () => {
      w = new BrowserWindow();
      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    });
    afterAll(closeAllWindows);

    const writeClipboard = async () => {
      if (!w.webContents.isFocused()) {
        const focus = once(w.webContents, 'focus');
        w.webContents.focus();
        await focus;
      }
      return w.webContents.executeJavaScript(
        `
      navigator.clipboard.writeText('Hello World!').catch(err => err.message);
    `,
        true
      );
    };

    afterEach(() => {
      session.defaultSession.setPermissionRequestHandler(null);
    });

    it('returns clipboard contents when a PermissionRequestHandler is not defined', async () => {
      const clipboard = await writeClipboard();
      expect(clipboard).to.be.undefined();
    });

    it('returns an error when permission denied', async () => {
      session.defaultSession.setPermissionRequestHandler((wc, permission, callback) => {
        if (permission === 'clipboard-sanitized-write') {
          callback(false);
        } else {
          callback(true);
        }
      });
      const clipboard = await writeClipboard();
      expect(clipboard).to.contain('Write permission denied.');
    });

    it('returns clipboard contents when permission is granted', async () => {
      session.defaultSession.setPermissionRequestHandler((wc, permission, callback) => {
        if (permission === 'clipboard-sanitized-write') {
          callback(true);
        } else {
          callback(false);
        }
      });
      const clipboard = await writeClipboard();
      expect(clipboard).to.be.undefined();
    });
  });
});

describe('paste execCommand', () => {
  const readClipboard = async (w: BrowserWindow) => {
    if (!w.webContents.isFocused()) {
      const focus = once(w.webContents, 'focus');
      w.webContents.focus();
      await focus;
    }

    return w.webContents.executeJavaScript(
      `
      new Promise((resolve) => {
        const timeout = setTimeout(() => {
          resolve('');
        }, 2000);
        document.addEventListener('paste', (event) => {
          clearTimeout(timeout);
          event.preventDefault();
          let paste = event.clipboardData.getData("text");
          resolve(paste);
        });
        document.execCommand('paste');
      });
    `,
      true
    );
  };

  let ses: Electron.Session;
  beforeEach(() => {
    ses = session.fromPartition(`paste-execCommand-${Math.random()}`);
  });

  afterEach(() => {
    ses.setPermissionCheckHandler(null);
    closeAllWindows();
  });

  it('is disabled by default', async () => {
    const w: BrowserWindow = new BrowserWindow({});
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    const text = 'Sync Clipboard Disabled by default';
    clipboard.write({
      text
    });
    const paste = await readClipboard(w);
    expect(paste).to.be.empty();
    expect(clipboard.readText()).to.equal(text);
  });

  it('does not execute with default permissions', async () => {
    const w: BrowserWindow = new BrowserWindow({
      webPreferences: {
        enableDeprecatedPaste: true,
        session: ses
      }
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    const text = 'Sync Clipboard Disabled by default permissions';
    clipboard.write({
      text
    });
    const paste = await readClipboard(w);
    expect(paste).to.be.empty();
    expect(clipboard.readText()).to.equal(text);
  });

  it('does not execute with permission denied', async () => {
    const w: BrowserWindow = new BrowserWindow({
      webPreferences: {
        enableDeprecatedPaste: true,
        session: ses
      }
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    ses.setPermissionCheckHandler((webContents, permission) => {
      if (permission === 'deprecated-sync-clipboard-read') {
        return false;
      }
      return true;
    });
    const text = 'Sync Clipboard Disabled by permission denied';
    clipboard.write({
      text
    });
    const paste = await readClipboard(w);
    expect(paste).to.be.empty();
    expect(clipboard.readText()).to.equal(text);
  });

  it('can trigger paste event when permission is granted', async () => {
    const w: BrowserWindow = new BrowserWindow({
      webPreferences: {
        enableDeprecatedPaste: true,
        session: ses
      }
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    ses.setPermissionCheckHandler((webContents, permission) => {
      if (permission === 'deprecated-sync-clipboard-read') {
        return true;
      }
      return false;
    });
    const text = 'Sync Clipboard Test';
    clipboard.write({
      text
    });
    const paste = await readClipboard(w);
    expect(paste).to.equal(text);
  });

  it('can trigger paste event when permission is granted for child windows', async () => {
    const w: BrowserWindow = new BrowserWindow({
      webPreferences: {
        session: ses
      }
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    w.webContents.setWindowOpenHandler((details) => {
      if (details.url === 'about:blank') {
        return {
          action: 'allow',
          overrideBrowserWindowOptions: {
            webPreferences: {
              enableDeprecatedPaste: true,
              session: ses
            }
          }
        };
      } else {
        return {
          action: 'deny'
        };
      }
    });
    ses.setPermissionCheckHandler((webContents, permission, requestingOrigin, details) => {
      if (
        requestingOrigin === `${webContents?.opener?.origin}/` &&
        details.requestingUrl === 'about:blank' &&
        permission === 'deprecated-sync-clipboard-read'
      ) {
        return true;
      }
      return false;
    });
    const childPromise = once(w.webContents, 'did-create-window') as Promise<
      [BrowserWindow, Electron.DidCreateWindowDetails]
    >;
    w.webContents.executeJavaScript('window.open("about:blank")', true);
    const [childWindow] = await childPromise;
    expect(childWindow.webContents.opener).to.equal(w.webContents.mainFrame);
    const text = 'Sync Clipboard Test for Child Window';
    clipboard.write({
      text
    });
    const paste = await readClipboard(childWindow);
    expect(paste).to.equal(text);
  });
});
