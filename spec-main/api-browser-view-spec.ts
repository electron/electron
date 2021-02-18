import { expect } from 'chai';
import * as ChildProcess from 'child_process';
import * as path from 'path';
import { emittedOnce } from './events-helpers';
import { BrowserView, BrowserWindow, webContents } from 'electron/main';
import { closeWindow } from './window-helpers';

describe('BrowserView module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures');

  let w: BrowserWindow;
  let view: BrowserView;

  beforeEach(() => {
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
    await closeWindow(w);

    if (view) {
      view.destroy();
    }
  });

  describe('BrowserView.destroy()', () => {
    it('does not throw', () => {
      view = new BrowserView();
      view.destroy();
    });
  });

  describe('BrowserView.isDestroyed()', () => {
    it('returns correct value', () => {
      view = new BrowserView();
      expect(view.isDestroyed()).to.be.false('view is destroyed');
      view.destroy();
      expect(view.isDestroyed()).to.be.true('view is destroyed');
    });
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
      expect(view.id).to.not.be.null('view id');

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
      w.addBrowserView(view1);
      const view2 = new BrowserView();
      w.addBrowserView(view2);
      view1.destroy();
      view2.destroy();
    });

    it('does not throw if called multiple times with same view', () => {
      view = new BrowserView();
      w.addBrowserView(view);
      w.addBrowserView(view);
      w.addBrowserView(view);
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
      view = new BrowserView();
      w.addBrowserView(view);
      w.removeBrowserView(view);
      w.removeBrowserView(view);
    });
  });

  describe('BrowserWindow.getBrowserViews()', () => {
    it('returns same views as was added', () => {
      const view1 = new BrowserView();
      w.addBrowserView(view1);
      const view2 = new BrowserView();
      w.addBrowserView(view2);

      expect(view1.id).to.be.not.null('view id');
      const views = w.getBrowserViews();
      expect(views).to.have.lengthOf(2);
      expect(views[0].webContents.id).to.equal(view1.webContents.id);
      expect(views[1].webContents.id).to.equal(view2.webContents.id);

      view1.destroy();
      view2.destroy();
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

  describe('BrowserView.fromId()', () => {
    it('returns the view with given id', () => {
      view = new BrowserView();
      w.setBrowserView(view);
      expect(view.id).to.not.be.null('view id');

      const view2 = BrowserView.fromId(view.id);
      expect(view2.webContents.id).to.equal(view.webContents.id);
    });
  });

  describe('BrowserView.fromWebContents()', () => {
    it('returns the view with given id', () => {
      view = new BrowserView();
      w.setBrowserView(view);
      expect(view.id).to.not.be.null('view id');

      const view2 = BrowserView.fromWebContents(view.webContents);
      expect(view2!.webContents.id).to.equal(view.webContents.id);
    });
  });

  describe('BrowserView.getAllViews()', () => {
    it('returns all views', () => {
      view = new BrowserView();
      w.setBrowserView(view);
      expect(view.id).to.not.be.null('view id');

      const views = BrowserView.getAllViews();
      expect(views).to.be.an('array').that.has.lengthOf(1);
      expect(views[0].webContents.id).to.equal(view.webContents.id);
    });
  });

  describe('new BrowserView()', () => {
    it('does not crash on exit', async () => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'leak-exit-browserview.js');
      const electronPath = process.execPath;
      const appProcess = ChildProcess.spawn(electronPath, [appPath]);
      const [code] = await emittedOnce(appProcess, 'exit');
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
