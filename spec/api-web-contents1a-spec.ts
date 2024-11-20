import { BrowserWindow, ipcMain, webContents, BrowserView, WebContents } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { waitUntil } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('webContents1a module', () => {
  describe('getAllWebContents() API', () => {
    afterEach(closeAllWindows);
    it('returns an array of web contents', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: { webviewTag: true }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'webview-zoom-factor.html'));

      await once(w.webContents, 'did-attach-webview') as [any, WebContents];

      w.webContents.openDevTools();

      await once(w.webContents, 'devtools-opened');

      const all = webContents.getAllWebContents().sort((a, b) => {
        return a.id - b.id;
      });

      expect(all).to.have.length(3);
      expect(all[0].getType()).to.equal('window');
      expect(all[all.length - 2].getType()).to.equal('webview');
      expect(all[all.length - 1].getType()).to.equal('remote');
    });
  });

  describe('webContents properties', () => {
    afterEach(closeAllWindows);

    it('has expected additional enumerable properties', () => {
      const w = new BrowserWindow({ show: false });
      const properties = Object.getOwnPropertyNames(w.webContents);
      expect(properties).to.include('ipc');
      expect(properties).to.include('navigationHistory');
    });
  });

  describe('fromId()', () => {
    it('returns undefined for an unknown id', () => {
      expect(webContents.fromId(12345)).to.be.undefined();
    });
  });

  describe('fromFrame()', () => {
    it('returns WebContents for mainFrame', () => {
      const contents = (webContents as typeof ElectronInternal.WebContents).create();
      expect(webContents.fromFrame(contents.mainFrame)).to.equal(contents);
    });
    it('returns undefined for disposed frame', async () => {
      const contents = (webContents as typeof ElectronInternal.WebContents).create();
      const { mainFrame } = contents;
      contents.destroy();
      await waitUntil(() => typeof webContents.fromFrame(mainFrame) === 'undefined');
    });
    it('throws when passing invalid argument', async () => {
      let errored = false;
      try {
        webContents.fromFrame({} as any);
      } catch {
        errored = true;
      }
      expect(errored).to.be.true();
    });
  });

  describe('fromDevToolsTargetId()', () => {
    it('returns WebContents for attached DevTools target', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      try {
        await w.webContents.debugger.attach('1.3');
        const { targetInfo } = await w.webContents.debugger.sendCommand('Target.getTargetInfo');
        expect(webContents.fromDevToolsTargetId(targetInfo.targetId)).to.equal(w.webContents);
      } finally {
        await w.webContents.debugger.detach();
      }
    });

    it('returns undefined for an unknown id', () => {
      expect(webContents.fromDevToolsTargetId('nope')).to.be.undefined();
    });
  });

  describe('will-prevent-unload event', function () {
    afterEach(closeAllWindows);
    it('does not emit if beforeunload returns undefined in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.once('will-prevent-unload', () => {
        expect.fail('should not have fired');
      });
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-undefined.html'));
      const wait = once(w, 'closed');
      w.close();
      await wait;
    });

    it('does not emit if beforeunload returns undefined in a BrowserView', async () => {
      const w = new BrowserWindow({ show: false });
      const view = new BrowserView();
      w.setBrowserView(view);
      view.setBounds(w.getBounds());

      view.webContents.once('will-prevent-unload', () => {
        expect.fail('should not have fired');
      });

      await view.webContents.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-undefined.html'));
      const wait = once(w, 'closed');
      w.close();
      await wait;
    });

    it('emits if beforeunload returns false in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.close();
      await once(w.webContents, 'will-prevent-unload');
    });

    it('emits if beforeunload returns false in a BrowserView', async () => {
      const w = new BrowserWindow({ show: false });
      const view = new BrowserView();
      w.setBrowserView(view);
      view.setBounds(w.getBounds());

      await view.webContents.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.close();
      await once(view.webContents, 'will-prevent-unload');
    });

    it('supports calling preventDefault on will-prevent-unload events in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.once('will-prevent-unload', event => event.preventDefault());
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      const wait = once(w, 'closed');
      w.close();
      await wait;
    });
  });

  describe('webContents.send(channel, args...)', () => {
    afterEach(closeAllWindows);
    it('throws an error when the channel is missing', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        (w.webContents.send as any)();
      }).to.throw('Missing required channel argument');

      expect(() => {
        w.webContents.send(null as any);
      }).to.throw('Missing required channel argument');
    });

    it('does not block node async APIs when sent before document is ready', (done) => {
      // Please reference https://github.com/electron/electron/issues/19368 if
      // this test fails.
      ipcMain.once('async-node-api-done', () => {
        done();
      });
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          sandbox: false,
          contextIsolation: false
        }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'send-after-node.html'));
      setTimeout(50).then(() => {
        w.webContents.send('test');
      });
    });
  });
});
