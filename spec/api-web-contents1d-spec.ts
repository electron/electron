import { BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import * as http from 'node:http';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { listen, ifit } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('webContents1d module', () => {
  describe('loadURL() promise API', () => {
    let w: BrowserWindow;
    let s: http.Server;

    afterEach(() => {
      if (s) {
        s.close();
        s = null as unknown as http.Server;
      }
    });

    beforeEach(async () => {
      w = new BrowserWindow({ show: false });
    });
    afterEach(closeAllWindows);

    it('resolves when done loading', async () => {
      await expect(w.loadURL('about:blank')).to.eventually.be.fulfilled();
    });

    it('resolves when done loading a file URL', async () => {
      await expect(w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'))).to.eventually.be.fulfilled();
    });

    it('resolves when navigating within the page', async () => {
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
      await setTimeout();
      await expect(w.loadURL(w.getURL() + '#foo')).to.eventually.be.fulfilled();
    });

    it('resolves after browser initiated navigation', async () => {
      let finishedLoading = false;
      w.webContents.on('did-finish-load', function () {
        finishedLoading = true;
      });

      await w.loadFile(path.join(fixturesPath, 'pages', 'navigate_in_page_and_wait.html'));
      expect(finishedLoading).to.be.true();
    });

    it('rejects when failing to load a file URL', async () => {
      await expect(w.loadURL('file:non-existent')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_FILE_NOT_FOUND');
    });

    // FIXME: Temporarily disable on WOA until
    // https://github.com/electron/electron/issues/20008 is resolved
    ifit(!(process.platform === 'win32' && process.arch === 'arm64'))('rejects when loading fails due to DNS not resolved', async () => {
      await expect(w.loadURL('https://err.name.not.resolved')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_NAME_NOT_RESOLVED');
    });

    it('rejects when navigation is cancelled due to a bad scheme', async () => {
      await expect(w.loadURL('bad-scheme://foo')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_FAILED');
    });

    it('does not crash when loading a new URL with emulation settings set', async () => {
      const setEmulation = async () => {
        if (w.webContents) {
          w.webContents.debugger.attach('1.3');

          const deviceMetrics = {
            width: 700,
            height: 600,
            deviceScaleFactor: 2,
            mobile: true,
            dontSetVisibleSize: true
          };
          await w.webContents.debugger.sendCommand(
            'Emulation.setDeviceMetricsOverride',
            deviceMetrics
          );
        }
      };

      try {
        await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
        await setEmulation();
        await w.loadURL('data:text/html,<h1>HELLO</h1>');
        await setEmulation();
      } catch (e) {
        expect((e as Error).message).to.match(/Debugger is already attached to the target/);
      }
    });

    it('fails if loadURL is called inside a non-reentrant critical section', (done) => {
      w.webContents.once('did-fail-load', (_event, _errorCode, _errorDescription, validatedURL) => {
        expect(validatedURL).to.contain('blank.html');
        done();
      });

      w.webContents.once('did-start-loading', () => {
        w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      });

      w.loadURL('data:text/html,<h1>HELLO</h1>');
    });

    it('sets appropriate error information on rejection', async () => {
      let err: any;
      try {
        await w.loadURL('file:non-existent');
      } catch (e) {
        err = e;
      }
      expect(err).not.to.be.null();
      expect(err.code).to.eql('ERR_FILE_NOT_FOUND');
      expect(err.errno).to.eql(-6);
      expect(err.url).to.eql(process.platform === 'win32' ? 'file://non-existent/' : 'file:///non-existent');
    });

    it('rejects if the load is aborted', async () => {
      s = http.createServer(() => { /* never complete the request */ });
      const { port } = await listen(s);
      const p = expect(w.loadURL(`http://127.0.0.1:${port}`)).to.eventually.be.rejectedWith(Error, /ERR_ABORTED/);
      // load a different file before the first load completes, causing the
      // first load to be aborted.
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
      await p;
    });

    it("doesn't reject when a subframe fails to load", async () => {
      let resp = null as unknown as http.ServerResponse;
      s = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.write('<iframe src="http://err.name.not.resolved"></iframe>');
        resp = res;
        // don't end the response yet
      });
      const { port } = await listen(s);
      const p = new Promise<void>(resolve => {
        w.webContents.on('did-fail-load', (event, errorCode, errorDescription, validatedURL, isMainFrame) => {
          if (!isMainFrame) {
            resolve();
          }
        });
      });
      const main = w.loadURL(`http://127.0.0.1:${port}`);
      await p;
      resp.end();
      await main;
    });

    it("doesn't resolve when a subframe loads", async () => {
      let resp = null as unknown as http.ServerResponse;
      s = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.write('<iframe src="about:blank"></iframe>');
        resp = res;
        // don't end the response yet
      });
      const { port } = await listen(s);
      const p = new Promise<void>(resolve => {
        w.webContents.on('did-frame-finish-load', (event, isMainFrame) => {
          if (!isMainFrame) {
            resolve();
          }
        });
      });
      const main = w.loadURL(`http://127.0.0.1:${port}`);
      await p;
      resp.destroy(); // cause the main request to fail
      await expect(main).to.eventually.be.rejected()
        .and.have.property('errno', -355); // ERR_INCOMPLETE_CHUNKED_ENCODING
    });

    it('subsequent load failures reject each time', async () => {
      await expect(w.loadURL('file:non-existent')).to.eventually.be.rejected();
      await expect(w.loadURL('file:non-existent')).to.eventually.be.rejected();
    });

    it('invalid URL load rejects', async () => {
      await expect(w.loadURL('invalidURL')).to.eventually.be.rejected();
    });
  });

  describe('navigationHistory', () => {
    let w: BrowserWindow;
    const urlPage1 = 'data:text/html,<html><head><script>document.title = "Page 1";</script></head><body></body></html>';
    const urlPage2 = 'data:text/html,<html><head><script>document.title = "Page 2";</script></head><body></body></html>';
    const urlPage3 = 'data:text/html,<html><head><script>document.title = "Page 3";</script></head><body></body></html>';

    beforeEach(async () => {
      w = new BrowserWindow({ show: false });
    });
    afterEach(closeAllWindows);
    describe('navigationHistory.removeEntryAtIndex(index) API', () => {
      it('should remove a navigation entry given a valid index', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);
        const initialLength = w.webContents.navigationHistory.length();
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(1); // Attempt to remove the second entry
        const newLength = w.webContents.navigationHistory.length();
        expect(wasRemoved).to.be.true();
        expect(newLength).to.equal(initialLength - 1);
      });

      it('should not remove the current active navigation entry', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        const activeIndex = w.webContents.navigationHistory.getActiveIndex();
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(activeIndex);
        expect(wasRemoved).to.be.false();
      });

      it('should return false given an invalid index larger than history length', async () => {
        await w.loadURL(urlPage1);
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(5); // Index larger than history length
        expect(wasRemoved).to.be.false();
      });

      it('should return false given an invalid negative index', async () => {
        await w.loadURL(urlPage1);
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(-1); // Negative index
        expect(wasRemoved).to.be.false();
      });
    });

    describe('navigationHistory.canGoBack and navigationHistory.goBack API', () => {
      it('should not be able to go back if history is empty', async () => {
        expect(w.webContents.navigationHistory.canGoBack()).to.be.false();
      });

      it('should be able to go back if history is not empty', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(1);
        expect(w.webContents.navigationHistory.canGoBack()).to.be.true();
        w.webContents.navigationHistory.goBack();
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(0);
      });
    });

    describe('navigationHistory.canGoForward and navigationHistory.goForward API', () => {
      it('should not be able to go forward if history is empty', async () => {
        expect(w.webContents.navigationHistory.canGoForward()).to.be.false();
      });

      it('should not be able to go forward if current index is same as history length', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        expect(w.webContents.navigationHistory.canGoForward()).to.be.false();
      });

      it('should be able to go forward if history is not empty and active index is less than history length', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        w.webContents.navigationHistory.goBack();
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(0);
        expect(w.webContents.navigationHistory.canGoForward()).to.be.true();
        w.webContents.navigationHistory.goForward();
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(1);
      });
    });

    describe('navigationHistory.canGoToOffset(index) and navigationHistory.goToOffset(index) API', () => {
      it('should not be able to go to invalid offset', async () => {
        expect(w.webContents.navigationHistory.canGoToOffset(-1)).to.be.false();
        expect(w.webContents.navigationHistory.canGoToOffset(10)).to.be.false();
      });

      it('should be able to go to valid negative offset', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);
        expect(w.webContents.navigationHistory.canGoToOffset(-2)).to.be.true();
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(2);
        w.webContents.navigationHistory.goToOffset(-2);
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(0);
      });

      it('should be able to go to valid positive offset', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        w.webContents.navigationHistory.goBack();
        expect(w.webContents.navigationHistory.canGoToOffset(1)).to.be.true();
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(1);
        w.webContents.navigationHistory.goToOffset(1);
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(2);
      });
    });

    describe('navigationHistory.clear API', () => {
      it('should be able clear history', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        expect(w.webContents.navigationHistory.length()).to.equal(3);
        w.webContents.navigationHistory.clear();
        expect(w.webContents.navigationHistory.length()).to.equal(1);
      });
    });

    describe('navigationHistory.getEntryAtIndex(index) API ', () => {
      it('should fetch default navigation entry when no urls are loaded', async () => {
        const result = w.webContents.navigationHistory.getEntryAtIndex(0);
        expect(result).to.deep.equal({ url: '', title: '' });
      });
      it('should fetch navigation entry given a valid index', async () => {
        await w.loadURL(urlPage1);
        const result = w.webContents.navigationHistory.getEntryAtIndex(0);
        expect(result).to.deep.equal({ url: urlPage1, title: 'Page 1' });
      });
      it('should return null given an invalid index larger than history length', async () => {
        await w.loadURL(urlPage1);
        const result = w.webContents.navigationHistory.getEntryAtIndex(5);
        expect(result).to.be.null();
      });
      it('should return null given an invalid negative index', async () => {
        await w.loadURL(urlPage1);
        const result = w.webContents.navigationHistory.getEntryAtIndex(-1);
        expect(result).to.be.null();
      });
    });

    describe('navigationHistory.getActiveIndex() API', () => {
      it('should return valid active index after a single page visit', async () => {
        await w.loadURL(urlPage1);
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(0);
      });

      it('should return valid active index after a multiple page visits', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(2);
      });

      it('should return valid active index given no page visits', async () => {
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(0);
      });
    });

    describe('navigationHistory.length() API', () => {
      it('should return valid history length after a single page visit', async () => {
        await w.loadURL(urlPage1);
        expect(w.webContents.navigationHistory.length()).to.equal(1);
      });

      it('should return valid history length after a multiple page visits', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        expect(w.webContents.navigationHistory.length()).to.equal(3);
      });

      it('should return valid history length given no page visits', async () => {
        // Note: Even if no navigation has committed, the history list will always start with an initial navigation entry
        // Ref: https://source.chromium.org/chromium/chromium/src/+/main:ceontent/public/browser/navigation_controller.h;l=381
        expect(w.webContents.navigationHistory.length()).to.equal(1);
      });
    });

    describe('navigationHistory.getAllEntries() API', () => {
      it('should return all navigation entries as an array of NavigationEntry objects', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);
        const entries = w.webContents.navigationHistory.getAllEntries();
        expect(entries.length).to.equal(3);
        expect(entries[0]).to.deep.equal({ url: urlPage1, title: 'Page 1' });
        expect(entries[1]).to.deep.equal({ url: urlPage2, title: 'Page 2' });
        expect(entries[2]).to.deep.equal({ url: urlPage3, title: 'Page 3' });
      });

      it('should return an empty array when there is no navigation history', async () => {
        const entries = w.webContents.navigationHistory.getAllEntries();
        expect(entries.length).to.equal(0);
      });
    });
  });
});
