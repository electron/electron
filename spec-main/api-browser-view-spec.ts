import { expect } from 'chai';
import * as path from 'path';
import { emittedOnce } from './events-helpers';
import { BrowserView, BrowserWindow, screen, webContents } from 'electron/main';
import { closeWindow } from './window-helpers';
import { defer, ifit, startRemoteControlApp } from './spec-helpers';
import { areColorsSimilar, captureScreen, getPixelColor } from './screen-helpers';

describe('BrowserView module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures');

  let w: BrowserWindow;
  let view: BrowserView;

  beforeEach(() => {
    expect(webContents.getAllWebContents()).to.have.length(0);
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        backgroundThrottling: false
      }
    });
  });

  afterEach(async () => {
    const p = emittedOnce(w.webContents, 'destroyed');
    await closeWindow(w);
    w = null as any;
    await p;

    if (view) {
      const p = emittedOnce(view.webContents, 'destroyed');
      (view.webContents as any).destroy();
      view = null as any;
      await p;
    }

    expect(webContents.getAllWebContents()).to.have.length(0);
  });

  it('can be created with an existing webContents', async () => {
    const wc = (webContents as any).create({ sandbox: true });
    await wc.loadURL('about:blank');

    view = new BrowserView({ webContents: wc } as any);

    expect(view.webContents.getURL()).to.equal('about:blank');
  });

  describe('BrowserView.setBackgroundColor()', () => {
    it('does not throw for valid args', () => {
      view = new BrowserView();
      view.setBackgroundColor('#000');
    });

    it('throws for invalid args', () => {
      view = new BrowserView();
      expect(() => {
        view.setBackgroundColor(null as any);
      }).to.throw(/conversion failure/);
    });

    ifit(process.platform !== 'linux')('sets the background color to transparent if none is set', async () => {
      const display = screen.getPrimaryDisplay();
      const WINDOW_BACKGROUND_COLOR = '#55ccbb';

      w.show();
      w.setBounds(display.bounds);
      w.setBackgroundColor(WINDOW_BACKGROUND_COLOR);
      await w.loadURL('about:blank');

      view = new BrowserView();
      view.setBounds(display.bounds);
      w.setBrowserView(view);
      await view.webContents.loadURL('data:text/html,hello there');

      const screenCapture = await captureScreen();
      const centerColor = getPixelColor(screenCapture, {
        x: display.size.width / 2,
        y: display.size.height / 2
      });

      expect(areColorsSimilar(centerColor, WINDOW_BACKGROUND_COLOR)).to.be.true();
    });

    ifit(process.platform !== 'linux')('successfully applies the background color', async () => {
      const WINDOW_BACKGROUND_COLOR = '#55ccbb';
      const VIEW_BACKGROUND_COLOR = '#ff00ff';
      const display = screen.getPrimaryDisplay();

      w.show();
      w.setBounds(display.bounds);
      w.setBackgroundColor(WINDOW_BACKGROUND_COLOR);
      await w.loadURL('about:blank');

      view = new BrowserView();
      view.setBounds(display.bounds);
      w.setBrowserView(view);
      w.setBackgroundColor(VIEW_BACKGROUND_COLOR);
      await view.webContents.loadURL('data:text/html,hello there');

      const screenCapture = await captureScreen();
      const centerColor = getPixelColor(screenCapture, {
        x: display.size.width / 2,
        y: display.size.height / 2
      });

      expect(areColorsSimilar(centerColor, VIEW_BACKGROUND_COLOR)).to.be.true();
    });
  });

  describe('BrowserView.setAutoResize()', () => {
    it('does not throw for valid args', () => {
      view = new BrowserView();
      view.setAutoResize({});
      view.setAutoResize({ width: true, height: false });
    });

    it('throws for invalid args', () => {
      view = new BrowserView();
      expect(() => {
        view.setAutoResize(null as any);
      }).to.throw(/conversion failure/);
    });
  });

  describe('BrowserView.setBounds()', () => {
    it('does not throw for valid args', () => {
      view = new BrowserView();
      view.setBounds({ x: 0, y: 0, width: 1, height: 1 });
    });

    it('throws for invalid args', () => {
      view = new BrowserView();
      expect(() => {
        view.setBounds(null as any);
      }).to.throw(/conversion failure/);
      expect(() => {
        view.setBounds({} as any);
      }).to.throw(/conversion failure/);
    });
  });

  describe('BrowserView.getBounds()', () => {
    it('returns the current bounds', () => {
      view = new BrowserView();
      const bounds = { x: 10, y: 20, width: 30, height: 40 };
      view.setBounds(bounds);
      expect(view.getBounds()).to.deep.equal(bounds);
    });
  });

  describe('BrowserWindow.setBrowserView()', () => {
    it('does not throw for valid args', () => {
      view = new BrowserView();
      w.setBrowserView(view);
    });

    it('does not throw if called multiple times with same view', () => {
      view = new BrowserView();
      w.setBrowserView(view);
      w.setBrowserView(view);
      w.setBrowserView(view);
    });
  });

  describe('BrowserWindow.getBrowserView()', () => {
    it('returns the set view', () => {
      view = new BrowserView();
      w.setBrowserView(view);

      const view2 = w.getBrowserView();
      expect(view2!.webContents.id).to.equal(view.webContents.id);
    });

    it('returns null if none is set', () => {
      const view = w.getBrowserView();
      expect(view).to.be.null('view');
    });
  });

  describe('BrowserWindow.addBrowserView()', () => {
    it('does not throw for valid args', () => {
      const view1 = new BrowserView();
      defer(() => (view1.webContents as any).destroy());
      w.addBrowserView(view1);
      defer(() => w.removeBrowserView(view1));
      const view2 = new BrowserView();
      defer(() => (view2.webContents as any).destroy());
      w.addBrowserView(view2);
      defer(() => w.removeBrowserView(view2));
    });

    it('does not throw if called multiple times with same view', () => {
      view = new BrowserView();
      w.addBrowserView(view);
      w.addBrowserView(view);
      w.addBrowserView(view);
    });

    it('does not crash if the BrowserView webContents are destroyed prior to window addition', () => {
      expect(() => {
        const view1 = new BrowserView();
        (view1.webContents as any).destroy();
        w.addBrowserView(view1);
      }).to.not.throw();
    });

    it('does not crash if the webContents is destroyed after a URL is loaded', () => {
      view = new BrowserView();
      expect(async () => {
        view.setBounds({ x: 0, y: 0, width: 400, height: 300 });
        await view.webContents.loadURL('data:text/html,hello there');
        view.webContents.destroy();
      }).to.not.throw();
    });

    it('can handle BrowserView reparenting', async () => {
      view = new BrowserView();

      w.addBrowserView(view);
      view.webContents.loadURL('about:blank');
      await emittedOnce(view.webContents, 'did-finish-load');

      const w2 = new BrowserWindow({ show: false });
      w2.addBrowserView(view);

      w.close();

      view.webContents.loadURL(`file://${fixtures}/pages/blank.html`);
      await emittedOnce(view.webContents, 'did-finish-load');

      // Clean up - the afterEach hook assumes the webContents on w is still alive.
      w = new BrowserWindow({ show: false });
      w2.close();
      w2.destroy();
    });
  });

  describe('BrowserWindow.removeBrowserView()', () => {
    it('does not throw if called multiple times with same view', () => {
      expect(() => {
        view = new BrowserView();
        w.addBrowserView(view);
        w.removeBrowserView(view);
        w.removeBrowserView(view);
      }).to.not.throw();
    });
  });

  describe('BrowserWindow.getBrowserViews()', () => {
    it('returns same views as was added', () => {
      const view1 = new BrowserView();
      defer(() => (view1.webContents as any).destroy());
      w.addBrowserView(view1);
      defer(() => w.removeBrowserView(view1));
      const view2 = new BrowserView();
      defer(() => (view2.webContents as any).destroy());
      w.addBrowserView(view2);
      defer(() => w.removeBrowserView(view2));

      const views = w.getBrowserViews();
      expect(views).to.have.lengthOf(2);
      expect(views[0].webContents.id).to.equal(view1.webContents.id);
      expect(views[1].webContents.id).to.equal(view2.webContents.id);
    });
  });

  describe('BrowserWindow.setTopBrowserView()', () => {
    it('should throw an error when a BrowserView is not attached to the window', () => {
      view = new BrowserView();
      expect(() => {
        w.setTopBrowserView(view);
      }).to.throw(/is not attached/);
    });

    it('should throw an error when a BrowserView is attached to some other window', () => {
      view = new BrowserView();

      const win2 = new BrowserWindow();

      w.addBrowserView(view);
      view.setBounds({ x: 0, y: 0, width: 100, height: 100 });
      win2.addBrowserView(view);

      expect(() => {
        w.setTopBrowserView(view);
      }).to.throw(/is not attached/);

      win2.close();
      win2.destroy();
    });
  });

  describe('BrowserView.webContents.getOwnerBrowserWindow()', () => {
    it('points to owning window', () => {
      view = new BrowserView();
      expect(view.webContents.getOwnerBrowserWindow()).to.be.null('owner browser window');

      w.setBrowserView(view);
      expect(view.webContents.getOwnerBrowserWindow()).to.equal(w);

      w.setBrowserView(null);
      expect(view.webContents.getOwnerBrowserWindow()).to.be.null('owner browser window');
    });
  });

  describe('shutdown behavior', () => {
    it('does not crash on exit', async () => {
      const rc = await startRemoteControlApp();
      await rc.remotely(() => {
        const { BrowserView, app } = require('electron');
        new BrowserView({})  // eslint-disable-line
        setTimeout(() => {
          app.quit();
        });
      });
      const [code] = await emittedOnce(rc.process, 'exit');
      expect(code).to.equal(0);
    });

    it('does not crash on exit if added to a browser window', async () => {
      const rc = await startRemoteControlApp();
      await rc.remotely(() => {
        const { app, BrowserView, BrowserWindow } = require('electron');
        const bv = new BrowserView();
        bv.webContents.loadURL('about:blank');
        const bw = new BrowserWindow({ show: false });
        bw.addBrowserView(bv);
        setTimeout(() => {
          app.quit();
        });
      });
      const [code] = await emittedOnce(rc.process, 'exit');
      expect(code).to.equal(0);
    });
  });

  describe('window.open()', () => {
    it('works in BrowserView', (done) => {
      view = new BrowserView();
      w.setBrowserView(view);
      view.webContents.setWindowOpenHandler(({ url, frameName }) => {
        expect(url).to.equal('http://host/');
        expect(frameName).to.equal('host');
        done();
        return { action: 'deny' };
      });
      view.webContents.loadFile(path.join(fixtures, 'pages', 'window-open.html'));
    });
  });

  describe('BrowserView.capturePage(rect)', () => {
    it('returns a Promise with a Buffer', async () => {
      view = new BrowserView({
        webPreferences: {
          backgroundThrottling: false
        }
      });
      w.addBrowserView(view);
      view.setBounds({
        ...w.getBounds(),
        x: 0,
        y: 0
      });
      const image = await view.webContents.capturePage({
        x: 0,
        y: 0,
        width: 100,
        height: 100
      });

      expect(image.isEmpty()).to.equal(true);
    });

    xit('resolves after the window is hidden and capturer count is non-zero', async () => {
      view = new BrowserView({
        webPreferences: {
          backgroundThrottling: false
        }
      });
      w.setBrowserView(view);
      view.setBounds({
        ...w.getBounds(),
        x: 0,
        y: 0
      });
      await view.webContents.loadFile(path.join(fixtures, 'pages', 'a.html'));

      view.webContents.incrementCapturerCount();
      const image = await view.webContents.capturePage();
      expect(image.isEmpty()).to.equal(false);
    });

    it('should increase the capturer count', () => {
      view = new BrowserView({
        webPreferences: {
          backgroundThrottling: false
        }
      });
      w.setBrowserView(view);
      view.setBounds({
        ...w.getBounds(),
        x: 0,
        y: 0
      });

      view.webContents.incrementCapturerCount();
      expect(view.webContents.isBeingCaptured()).to.be.true();
      view.webContents.decrementCapturerCount();
      expect(view.webContents.isBeingCaptured()).to.be.false();
    });
  });
});
