import { expect } from 'chai';
import * as path from 'path';
import { emittedOnce } from './events-helpers';
import { BrowserView, BrowserWindow, webContents } from 'electron/main';
import { closeWindow } from './window-helpers';
import { defer, startRemoteControlApp } from './spec-helpers';

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
  });

  describe('BrowserWindow.removeBrowserView()', () => {
    it('does not throw if called multiple times with same view', () => {
      view = new BrowserView();
      w.addBrowserView(view);
      w.removeBrowserView(view);
      w.removeBrowserView(view);
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
    it('works in BrowserView', async () => {
      view = new BrowserView();
      w.setBrowserView(view);
      const newWindow = emittedOnce(view.webContents, 'new-window');
      view.webContents.once('new-window', event => event.preventDefault());
      view.webContents.loadFile(path.join(fixtures, 'pages', 'window-open.html'));
      const [, url, frameName,,, additionalFeatures] = await newWindow;
      expect(url).to.equal('http://host/');
      expect(frameName).to.equal('host');
      expect(additionalFeatures[0]).to.equal('this-is-not-a-standard-feature');
    });
  });
});
