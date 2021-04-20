import { BrowserWindow, app } from 'electron/main';
import { shell } from 'electron/common';
import { closeAllWindows } from './window-helpers';
import { emittedOnce } from './events-helpers';
import * as http from 'http';
import * as fs from 'fs-extra';
import * as path from 'path';
import { AddressInfo } from 'net';
import { expect } from 'chai';
import { ifit } from './spec-helpers';
import { execSync } from 'child_process';

describe('shell module', () => {
  describe('shell.openExternal()', () => {
    let envVars: Record<string, string | undefined> = {};

    beforeEach(function () {
      envVars = {
        display: process.env.DISPLAY,
        de: process.env.DE,
        browser: process.env.BROWSER
      };
    });

    afterEach(async () => {
      // reset env vars to prevent side effects
      if (process.platform === 'linux') {
        process.env.DE = envVars.de;
        process.env.BROWSER = envVars.browser;
        process.env.DISPLAY = envVars.display;
      }
    });
    afterEach(closeAllWindows);

    it('opens an external link', async () => {
      let url = 'http://127.0.0.1';
      let requestReceived;
      if (process.platform === 'linux') {
        process.env.BROWSER = '/bin/true';
        process.env.DE = 'generic';
        process.env.DISPLAY = '';
        requestReceived = Promise.resolve();
      } else if (process.platform === 'darwin') {
        // On the Mac CI machines, Safari tries to ask for a password to the
        // code signing keychain we set up to test code signing (see
        // https://github.com/electron/electron/pull/19969#issuecomment-526278890),
        // so use a blur event as a crude proxy.
        const w = new BrowserWindow({ show: true });
        requestReceived = emittedOnce(w, 'blur');
      } else {
        const server = http.createServer((req, res) => {
          res.end();
        });
        await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
        requestReceived = new Promise(resolve => server.on('connection', () => resolve()));
        url = `http://127.0.0.1:${(server.address() as AddressInfo).port}`;
      }

      await Promise.all([
        shell.openExternal(url),
        requestReceived
      ]);
    });
  });

  describe('shell.moveItemToTrash()', () => {
    it('moves an item to the trash', async () => {
      const dir = await fs.mkdtemp(path.resolve(app.getPath('temp'), 'electron-shell-spec-'));
      const filename = path.join(dir, 'temp-to-be-deleted');
      await fs.writeFile(filename, 'dummy-contents');
      const result = shell.moveItemToTrash(filename);
      expect(result).to.be.true();
      expect(fs.existsSync(filename)).to.be.false();
    });

    it('returns false when called with a nonexistent path', () => {
      const filename = path.join(app.getPath('temp'), 'does-not-exist');
      const result = shell.moveItemToTrash(filename);
      expect(result).to.be.false();
    });

    ifit(process.platform === 'darwin')('returns false when file has immutable flag', async () => {
      const dir = await fs.mkdtemp(path.resolve(app.getPath('temp'), 'electron-shell-spec-'));
      const tempPath = path.join(dir, 'locked-file');
      await fs.writeFile(tempPath, 'delete me if you can');

      // https://ss64.com/osx/chflags.html
      execSync(`chflags uchg ${tempPath}`);
      expect(shell.moveItemToTrash(tempPath)).to.be.false();
      expect(await fs.pathExists(tempPath)).to.be.true();

      execSync(`chflags nouchg ${tempPath}`);
      expect(shell.moveItemToTrash(tempPath)).to.be.true();
      expect(await fs.pathExists(tempPath)).to.be.false();
    });

    ifit(process.platform === 'win32')('returns false when path is in use', async () => {
      const tempPath = await fs.mkdtemp(path.resolve(app.getPath('temp'), 'electron-shell-spec-'));
      const cwd = process.cwd();
      try {
        // A process working directory is automatically locked on Windows.
        // This is a workaround to avoid pulling in fs-extras flock method.
        process.chdir(tempPath);

        expect(shell.moveItemToTrash(tempPath)).to.be.false();
        expect(await fs.pathExists(tempPath)).to.be.true();
      } finally {
        process.chdir(cwd);
      }

      expect(shell.moveItemToTrash(tempPath)).to.be.true();
      expect(await fs.pathExists(tempPath)).to.be.false();
    });
  });

  describe('shell.trashItem()', () => {
    afterEach(closeAllWindows);

    it('moves an item to the trash', async () => {
      const dir = await fs.mkdtemp(path.resolve(app.getPath('temp'), 'electron-shell-spec-'));
      const filename = path.join(dir, 'temp-to-be-deleted');
      await fs.writeFile(filename, 'dummy-contents');
      await shell.trashItem(filename);
      expect(fs.existsSync(filename)).to.be.false();
    });

    it('throws when called with a nonexistent path', async () => {
      const filename = path.join(app.getPath('temp'), 'does-not-exist');
      await expect(shell.trashItem(filename)).to.eventually.be.rejected();
    });

    it('works in the renderer process', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      w.loadURL('about:blank');
      await expect(w.webContents.executeJavaScript('require(\'electron\').shell.trashItem(\'does-not-exist\')')).to.be.rejectedWith(/does-not-exist|Failed to move item/);
    });
  });
});
