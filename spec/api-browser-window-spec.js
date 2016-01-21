const assert = require('assert');
const fs = require('fs');
const path = require('path');
const os = require('os');

const remote = require('electron').remote;
const screen = require('electron').screen;

const ipcMain = remote.require('electron').ipcMain;
const BrowserWindow = remote.require('electron').BrowserWindow;

const isCI = remote.getGlobal('isCi');

describe('browser-window module', function() {
  var fixtures, w;
  fixtures = path.resolve(__dirname, 'fixtures');
  w = null;
  beforeEach(function() {
    if (w != null) {
      w.destroy();
    }
    return w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400
    });
  });
  afterEach(function() {
    if (w != null) {
      w.destroy();
    }
    return w = null;
  });
  describe('BrowserWindow.close()', function() {
    it('should emit unload handler', function(done) {
      w.webContents.on('did-finish-load', function() {
        return w.close();
      });
      w.on('closed', function() {
        var content, test;
        test = path.join(fixtures, 'api', 'unload');
        content = fs.readFileSync(test);
        fs.unlinkSync(test);
        assert.equal(String(content), 'unload');
        return done();
      });
      return w.loadURL('file://' + path.join(fixtures, 'api', 'unload.html'));
    });
    return it('should emit beforeunload handler', function(done) {
      w.on('onbeforeunload', function() {
        return done();
      });
      w.webContents.on('did-finish-load', function() {
        return w.close();
      });
      return w.loadURL('file://' + path.join(fixtures, 'api', 'beforeunload-false.html'));
    });
  });
  describe('window.close()', function() {
    it('should emit unload handler', function(done) {
      w.on('closed', function() {
        var content, test;
        test = path.join(fixtures, 'api', 'close');
        content = fs.readFileSync(test);
        fs.unlinkSync(test);
        assert.equal(String(content), 'close');
        return done();
      });
      return w.loadURL('file://' + path.join(fixtures, 'api', 'close.html'));
    });
    return it('should emit beforeunload handler', function(done) {
      w.on('onbeforeunload', function() {
        return done();
      });
      return w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-false.html'));
    });
  });
  describe('BrowserWindow.destroy()', function() {
    return it('prevents users to access methods of webContents', function() {
      var webContents;
      webContents = w.webContents;
      w.destroy();
      return assert.throws((function() {
        return webContents.getId();
      }), /Object has been destroyed/);
    });
  });
  describe('BrowserWindow.loadURL(url)', function() {
    it('should emit did-start-loading event', function(done) {
      w.webContents.on('did-start-loading', function() {
        return done();
      });
      return w.loadURL('about:blank');
    });
    return it('should emit did-fail-load event', function(done) {
      w.webContents.on('did-fail-load', function() {
        return done();
      });
      return w.loadURL('file://a.txt');
    });
  });
  describe('BrowserWindow.show()', function() {
    return it('should focus on window', function() {
      if (isCI) {
        return;
      }
      w.show();
      return assert(w.isFocused());
    });
  });
  describe('BrowserWindow.showInactive()', function() {
    return it('should not focus on window', function() {
      w.showInactive();
      return assert(!w.isFocused());
    });
  });
  describe('BrowserWindow.focus()', function() {
    return it('does not make the window become visible', function() {
      assert.equal(w.isVisible(), false);
      w.focus();
      return assert.equal(w.isVisible(), false);
    });
  });
  describe('BrowserWindow.capturePage(rect, callback)', function() {
    return it('calls the callback with a Buffer', function(done) {
      return w.capturePage({
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }, function(image) {
        assert.equal(image.isEmpty(), true);
        return done();
      });
    });
  });
  describe('BrowserWindow.setSize(width, height)', function() {
    return it('sets the window size', function(done) {
      var size;
      size = [300, 400];
      w.once('resize', function() {
        var newSize;
        newSize = w.getSize();
        assert.equal(newSize[0], size[0]);
        assert.equal(newSize[1], size[1]);
        return done();
      });
      return w.setSize(size[0], size[1]);
    });
  });
  describe('BrowserWindow.setPosition(x, y)', function() {
    return it('sets the window position', function(done) {
      var pos;
      pos = [10, 10];
      w.once('move', function() {
        var newPos;
        newPos = w.getPosition();
        assert.equal(newPos[0], pos[0]);
        assert.equal(newPos[1], pos[1]);
        return done();
      });
      return w.setPosition(pos[0], pos[1]);
    });
  });
  describe('BrowserWindow.setContentSize(width, height)', function() {
    it('sets the content size', function() {
      var after, size;
      size = [400, 400];
      w.setContentSize(size[0], size[1]);
      after = w.getContentSize();
      assert.equal(after[0], size[0]);
      return assert.equal(after[1], size[1]);
    });
    return it('works for framless window', function() {
      var after, size;
      w.destroy();
      w = new BrowserWindow({
        show: false,
        frame: false,
        width: 400,
        height: 400
      });
      size = [400, 400];
      w.setContentSize(size[0], size[1]);
      after = w.getContentSize();
      assert.equal(after[0], size[0]);
      return assert.equal(after[1], size[1]);
    });
  });
  describe('BrowserWindow.fromId(id)', function() {
    return it('returns the window with id', function() {
      return assert.equal(w.id, BrowserWindow.fromId(w.id).id);
    });
  });
  describe('BrowserWindow.setResizable(resizable)', function() {
    return it('does not change window size for frameless window', function() {
      var s;
      w.destroy();
      w = new BrowserWindow({
        show: true,
        frame: false
      });
      s = w.getSize();
      w.setResizable(!w.isResizable());
      return assert.deepEqual(s, w.getSize());
    });
  });
  describe('"useContentSize" option', function() {
    it('make window created with content size when used', function() {
      var contentSize;
      w.destroy();
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        useContentSize: true
      });
      contentSize = w.getContentSize();
      assert.equal(contentSize[0], 400);
      return assert.equal(contentSize[1], 400);
    });
    it('make window created with window size when not used', function() {
      var size;
      size = w.getSize();
      assert.equal(size[0], 400);
      return assert.equal(size[1], 400);
    });
    return it('works for framless window', function() {
      var contentSize, size;
      w.destroy();
      w = new BrowserWindow({
        show: false,
        frame: false,
        width: 400,
        height: 400,
        useContentSize: true
      });
      contentSize = w.getContentSize();
      assert.equal(contentSize[0], 400);
      assert.equal(contentSize[1], 400);
      size = w.getSize();
      assert.equal(size[0], 400);
      return assert.equal(size[1], 400);
    });
  });
  describe('"title-bar-style" option', function() {
    if (process.platform !== 'darwin') {
      return;
    }
    if (parseInt(os.release().split('.')[0]) < 14) {
      return;
    }
    it('creates browser window with hidden title bar', function() {
      var contentSize;
      w.destroy();
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hidden'
      });
      contentSize = w.getContentSize();
      return assert.equal(contentSize[1], 400);
    });
    return it('creates browser window with hidden inset title bar', function() {
      var contentSize;
      w.destroy();
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hidden-inset'
      });
      contentSize = w.getContentSize();
      return assert.equal(contentSize[1], 400);
    });
  });
  describe('"enableLargerThanScreen" option', function() {
    if (process.platform === 'linux') {
      return;
    }
    beforeEach(function() {
      w.destroy();
      return w = new BrowserWindow({
        show: true,
        width: 400,
        height: 400,
        enableLargerThanScreen: true
      });
    });
    it('can move the window out of screen', function() {
      var after;
      w.setPosition(-10, -10);
      after = w.getPosition();
      assert.equal(after[0], -10);
      return assert.equal(after[1], -10);
    });
    return it('can set the window larger than screen', function() {
      var after, size;
      size = screen.getPrimaryDisplay().size;
      size.width += 100;
      size.height += 100;
      w.setSize(size.width, size.height);
      after = w.getSize();
      assert.equal(after[0], size.width);
      return assert.equal(after[1], size.height);
    });
  });
  describe('"web-preferences" option', function() {
    afterEach(function() {
      return ipcMain.removeAllListeners('answer');
    });
    describe('"preload" option', function() {
      return it('loads the script before other scripts in window', function(done) {
        var preload;
        preload = path.join(fixtures, 'module', 'set-global.js');
        ipcMain.once('answer', function(event, test) {
          assert.equal(test, 'preload');
          return done();
        });
        w.destroy();
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: preload
          }
        });
        return w.loadURL('file://' + path.join(fixtures, 'api', 'preload.html'));
      });
    });
    return describe('"node-integration" option', function() {
      return it('disables node integration when specified to false', function(done) {
        var preload;
        preload = path.join(fixtures, 'module', 'send-later.js');
        ipcMain.once('answer', function(event, test) {
          assert.equal(test, 'undefined');
          return done();
        });
        w.destroy();
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: preload,
            nodeIntegration: false
          }
        });
        return w.loadURL('file://' + path.join(fixtures, 'api', 'blank.html'));
      });
    });
  });
  describe('beforeunload handler', function() {
    it('returning true would not prevent close', function(done) {
      w.on('closed', function() {
        return done();
      });
      return w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-true.html'));
    });
    it('returning non-empty string would not prevent close', function(done) {
      w.on('closed', function() {
        return done();
      });
      return w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-string.html'));
    });
    it('returning false would prevent close', function(done) {
      w.on('onbeforeunload', function() {
        return done();
      });
      return w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-false.html'));
    });
    return it('returning empty string would prevent close', function(done) {
      w.on('onbeforeunload', function() {
        return done();
      });
      return w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-empty-string.html'));
    });
  });
  describe('new-window event', function() {
    if (isCI && process.platform === 'darwin') {
      return;
    }
    it('emits when window.open is called', function(done) {
      w.webContents.once('new-window', function(e, url, frameName) {
        e.preventDefault();
        assert.equal(url, 'http://host/');
        assert.equal(frameName, 'host');
        return done();
      });
      return w.loadURL("file://" + fixtures + "/pages/window-open.html");
    });
    return it('emits when link with target is called', function(done) {
      this.timeout(10000);
      w.webContents.once('new-window', function(e, url, frameName) {
        e.preventDefault();
        assert.equal(url, 'http://host/');
        assert.equal(frameName, 'target');
        return done();
      });
      return w.loadURL("file://" + fixtures + "/pages/target-name.html");
    });
  });
  describe('maximize event', function() {
    if (isCI) {
      return;
    }
    return it('emits when window is maximized', function(done) {
      this.timeout(10000);
      w.once('maximize', function() {
        return done();
      });
      w.show();
      return w.maximize();
    });
  });
  describe('unmaximize event', function() {
    if (isCI) {
      return;
    }
    return it('emits when window is unmaximized', function(done) {
      this.timeout(10000);
      w.once('unmaximize', function() {
        return done();
      });
      w.show();
      w.maximize();
      return w.unmaximize();
    });
  });
  describe('minimize event', function() {
    if (isCI) {
      return;
    }
    return it('emits when window is minimized', function(done) {
      this.timeout(10000);
      w.once('minimize', function() {
        return done();
      });
      w.show();
      return w.minimize();
    });
  });
  xdescribe('beginFrameSubscription method', function() {
    return it('subscribes frame updates', function(done) {
      w.loadURL("file://" + fixtures + "/api/blank.html");
      return w.webContents.beginFrameSubscription(function(data) {
        assert.notEqual(data.length, 0);
        w.webContents.endFrameSubscription();
        return done();
      });
    });
  });

  describe('savePage method', function() {
    const savePageDir = path.join(fixtures, 'save_page');
    const savePageHtmlPath = path.join(savePageDir, 'save_page.html');
    const savePageJsPath = path.join(savePageDir, 'save_page_files', 'test.js');
    const savePageCssPath = path.join(savePageDir, 'save_page_files', 'test.css');

    after(function() {
      try {
        fs.unlinkSync(savePageCssPath);
        fs.unlinkSync(savePageJsPath);
        fs.unlinkSync(savePageHtmlPath);
        fs.rmdirSync(path.join(savePageDir, 'save_page_files'));
        fs.rmdirSync(savePageDir);
      } catch (e) {
        // Ignore error
      }
    });

    it('should save page to disk', function(done) {
      w.webContents.on('did-finish-load', function() {
        w.webContents.savePage(savePageHtmlPath, 'HTMLComplete', function(error) {
          assert.equal(error, null);
          assert(fs.existsSync(savePageHtmlPath));
          assert(fs.existsSync(savePageJsPath));
          assert(fs.existsSync(savePageCssPath));
          done();
        });
      });
      w.loadURL("file://" + fixtures + "/pages/save_page/index.html");
    });
  });

  describe('BrowserWindow options argument is optional', function() {
    return it('should create a window with default size (800x600)', function() {
      var size;
      w.destroy();
      w = new BrowserWindow();
      size = w.getSize();
      assert.equal(size[0], 800);
      return assert.equal(size[1], 600);
    });
  });
});
