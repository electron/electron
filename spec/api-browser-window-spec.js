'use strict';

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
  var fixtures = path.resolve(__dirname, 'fixtures');
  var w = null;

  beforeEach(function() {
    if (w != null) {
      w.destroy();
    }
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400
    });
  });

  afterEach(function() {
    if (w != null) {
      w.destroy();
    }
    w = null;
  });

  describe('BrowserWindow.close()', function() {
    it('should emit unload handler', function(done) {
      w.webContents.on('did-finish-load', function() {
        w.close();
      });
      w.on('closed', function() {
        var content, test;
        test = path.join(fixtures, 'api', 'unload');
        content = fs.readFileSync(test);
        fs.unlinkSync(test);
        assert.equal(String(content), 'unload');
        done();
      });
      w.loadURL('file://' + path.join(fixtures, 'api', 'unload.html'));
    });

    it('should emit beforeunload handler', function(done) {
      w.on('onbeforeunload', function() {
        done();
      });
      w.webContents.on('did-finish-load', function() {
        w.close();
      });
      w.loadURL('file://' + path.join(fixtures, 'api', 'beforeunload-false.html'));
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
        done();
      });
      w.loadURL('file://' + path.join(fixtures, 'api', 'close.html'));
    });

    it('should emit beforeunload handler', function(done) {
      w.on('onbeforeunload', function() {
        done();
      });
      w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-false.html'));
    });
  });

  describe('BrowserWindow.destroy()', function() {
    it('prevents users to access methods of webContents', function() {
      var webContents;
      webContents = w.webContents;
      w.destroy();
      assert.throws((function() {
        webContents.getId();
      }), /Object has been destroyed/);
    });
  });

  describe('BrowserWindow.loadURL(url)', function() {
    it('should emit did-start-loading event', function(done) {
      w.webContents.on('did-start-loading', function() {
        done();
      });
      w.loadURL('about:blank');
    });

    it('should emit did-fail-load event', function(done) {
      w.webContents.on('did-fail-load', function() {
        done();
      });
      w.loadURL('file://a.txt');
    });
  });

  describe('BrowserWindow.show()', function() {
    it('should focus on window', function() {
      if (isCI) {
        return;
      }

      w.show();
      assert(w.isFocused());
    });
  });

  describe('BrowserWindow.showInactive()', function() {
    it('should not focus on window', function() {
      w.showInactive();
      assert(!w.isFocused());
    });
  });

  describe('BrowserWindow.focus()', function() {
    it('does not make the window become visible', function() {
      assert.equal(w.isVisible(), false);
      w.focus();
      assert.equal(w.isVisible(), false);
    });
  });

  describe('BrowserWindow.capturePage(rect, callback)', function() {
    it('calls the callback with a Buffer', function(done) {
      w.capturePage({
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }, function(image) {
        assert.equal(image.isEmpty(), true);
        done();
      });
    });
  });

  describe('BrowserWindow.setSize(width, height)', function() {
    it('sets the window size', function(done) {
      var size = [300, 400];
      w.once('resize', function() {
        var newSize = w.getSize();
        assert.equal(newSize[0], size[0]);
        assert.equal(newSize[1], size[1]);
        done();
      });
      w.setSize(size[0], size[1]);
    });
  });

  describe('BrowserWindow.setPosition(x, y)', function() {
    it('sets the window position', function(done) {
      var pos = [10, 10];
      w.once('move', function() {
        var newPos;
        newPos = w.getPosition();
        assert.equal(newPos[0], pos[0]);
        assert.equal(newPos[1], pos[1]);
        done();
      });
      w.setPosition(pos[0], pos[1]);
    });
  });

  describe('BrowserWindow.setContentSize(width, height)', function() {
    it('sets the content size', function() {
      var after, size;
      size = [400, 400];
      w.setContentSize(size[0], size[1]);
      after = w.getContentSize();
      assert.equal(after[0], size[0]);
      assert.equal(after[1], size[1]);
    });

    it('works for framless window', function() {
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
      assert.equal(after[1], size[1]);
    });
  });

  describe('BrowserWindow.fromId(id)', function() {
    it('returns the window with id', function() {
      assert.equal(w.id, BrowserWindow.fromId(w.id).id);
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
      assert.equal(contentSize[1], 400);
    });

    it('make window created with window size when not used', function() {
      var size = w.getSize();
      assert.equal(size[0], 400);
      assert.equal(size[1], 400);
    });

    it('works for framless window', function() {
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
      assert.equal(size[1], 400);
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
      assert.equal(contentSize[1], 400);
    });

    it('creates browser window with hidden inset title bar', function() {
      var contentSize;
      w.destroy();
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hidden-inset'
      });
      contentSize = w.getContentSize();
      assert.equal(contentSize[1], 400);
    });
  });

  describe('"enableLargerThanScreen" option', function() {
    if (process.platform === 'linux') {
      return;
    }

    beforeEach(function() {
      w.destroy();
      w = new BrowserWindow({
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
      assert.equal(after[1], -10);
    });

    it('can set the window larger than screen', function() {
      var after, size;
      size = screen.getPrimaryDisplay().size;
      size.width += 100;
      size.height += 100;
      w.setSize(size.width, size.height);
      after = w.getSize();
      assert.equal(after[0], size.width);
      assert.equal(after[1], size.height);
    });
  });

  describe('"web-preferences" option', function() {
    afterEach(function() {
      ipcMain.removeAllListeners('answer');
    });

    describe('"preload" option', function() {
      it('loads the script before other scripts in window', function(done) {
        var preload;
        preload = path.join(fixtures, 'module', 'set-global.js');
        ipcMain.once('answer', function(event, test) {
          assert.equal(test, 'preload');
          done();
        });
        w.destroy();
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: preload
          }
        });
        w.loadURL('file://' + path.join(fixtures, 'api', 'preload.html'));
      });
    });

    describe('"node-integration" option', function() {
      it('disables node integration when specified to false', function(done) {
        var preload;
        preload = path.join(fixtures, 'module', 'send-later.js');
        ipcMain.once('answer', function(event, test) {
          assert.equal(test, 'undefined');
          done();
        });
        w.destroy();
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: preload,
            nodeIntegration: false
          }
        });
        w.loadURL('file://' + path.join(fixtures, 'api', 'blank.html'));
      });
    });
  });

  describe('beforeunload handler', function() {
    it('returning true would not prevent close', function(done) {
      w.on('closed', function() {
        done();
      });
      w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-true.html'));
    });

    it('returning non-empty string would not prevent close', function(done) {
      w.on('closed', function() {
        done();
      });
      w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-string.html'));
    });

    it('returning false would prevent close', function(done) {
      w.on('onbeforeunload', function() {
        done();
      });
      w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-false.html'));
    });

    it('returning empty string would prevent close', function(done) {
      w.on('onbeforeunload', function() {
        done();
      });
      w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-empty-string.html'));
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
        done();
      });
      w.loadURL("file://" + fixtures + "/pages/window-open.html");
    });

    it('emits when link with target is called', function(done) {
      this.timeout(10000);
      w.webContents.once('new-window', function(e, url, frameName) {
        e.preventDefault();
        assert.equal(url, 'http://host/');
        assert.equal(frameName, 'target');
        done();
      });
      w.loadURL("file://" + fixtures + "/pages/target-name.html");
    });
  });

  describe('maximize event', function() {
    if (isCI) {
      return;
    }

    it('emits when window is maximized', function(done) {
      this.timeout(10000);
      w.once('maximize', function() {
        done();
      });
      w.show();
      w.maximize();
    });
  });

  describe('unmaximize event', function() {
    if (isCI) {
      return;
    }

    it('emits when window is unmaximized', function(done) {
      this.timeout(10000);
      w.once('unmaximize', function() {
        done();
      });
      w.show();
      w.maximize();
      w.unmaximize();
    });
  });

  describe('minimize event', function() {
    if (isCI) {
      return;
    }

    it('emits when window is minimized', function(done) {
      this.timeout(10000);
      w.once('minimize', function() {
        done();
      });
      w.show();
      w.minimize();
    });
  });

  xdescribe('beginFrameSubscription method', function() {
    it('subscribes frame updates', function(done) {
      w.loadURL("file://" + fixtures + "/api/blank.html");
      w.webContents.beginFrameSubscription(function(data) {
        assert.notEqual(data.length, 0);
        w.webContents.endFrameSubscription();
        done();
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
    it('should create a window with default size (800x600)', function() {
      var size;
      w.destroy();
      w = new BrowserWindow();
      size = w.getSize();
      assert.equal(size[0], 800);
      assert.equal(size[1], 600);
    });
  });

  describe('window states', function() {
    // Not implemented on Linux.
    if (process.platform == 'linux')
      return;

    describe('movable state', function() {
      it('can be changed with movable option', function() {
        w.destroy();
        w = new BrowserWindow({show: false, movable: false});
        assert.equal(w.isMovable(), false);
      });

      it('can be changed with setMovable method', function() {
        assert.equal(w.isMovable(), true);
        w.setMovable(false);
        assert.equal(w.isMovable(), false);
        w.setMovable(true);
        assert.equal(w.isMovable(), true);
      });
    });

    describe('minimizable state', function() {
      it('can be changed with minimizable option', function() {
        w.destroy();
        w = new BrowserWindow({show: false, minimizable: false});
        assert.equal(w.isMinimizable(), false);
      });

      it('can be changed with setMinimizable method', function() {
        assert.equal(w.isMinimizable(), true);
        w.setMinimizable(false);
        assert.equal(w.isMinimizable(), false);
        w.setMinimizable(true);
        assert.equal(w.isMinimizable(), true);
      });
    });

    describe('maximizable state', function() {
      it('can be changed with maximizable option', function() {
        w.destroy();
        w = new BrowserWindow({show: false, maximizable: false});
        assert.equal(w.isMaximizable(), false);
      });

      it('can be changed with setMaximizable method', function() {
        assert.equal(w.isMaximizable(), true);
        w.setMaximizable(false);
        assert.equal(w.isMaximizable(), false);
        w.setMaximizable(true);
        assert.equal(w.isMaximizable(), true);
      });

      it('is not affected when changing other states', function() {
        w.setMaximizable(false);
        assert.equal(w.isMaximizable(), false);
        w.setMinimizable(false);
        assert.equal(w.isMaximizable(), false);
        w.setClosable(false);
        assert.equal(w.isMaximizable(), false);
      });
    });

    describe('fullscreenable state', function() {
      // Only implemented on OS X.
      if (process.platform != 'darwin')
        return;

      it('can be changed with fullscreenable option', function() {
        w.destroy();
        w = new BrowserWindow({show: false, fullscreenable: false});
        assert.equal(w.isFullScreenable(), false);
      });

      it('can be changed with setFullScreenable method', function() {
        assert.equal(w.isFullScreenable(), true);
        w.setFullScreenable(false);
        assert.equal(w.isFullScreenable(), false);
        w.setFullScreenable(true);
        assert.equal(w.isFullScreenable(), true);
      });
    });

    describe('closable state', function() {
      it('can be changed with closable option', function() {
        w.destroy();
        w = new BrowserWindow({show: false, closable: false});
        assert.equal(w.isClosable(), false);
      });

      it('can be changed with setClosable method', function() {
        assert.equal(w.isClosable(), true);
        w.setClosable(false);
        assert.equal(w.isClosable(), false);
        w.setClosable(true);
        assert.equal(w.isClosable(), true);
      });
    });

    describe('resizable state', function() {
      it('can be changed with resizable option', function() {
        w.destroy();
        w = new BrowserWindow({show: false, resizable: false});
        assert.equal(w.isResizable(), false);
      });

      it('can be changed with setResizable method', function() {
        assert.equal(w.isResizable(), true);
        w.setResizable(false);
        assert.equal(w.isResizable(), false);
        w.setResizable(true);
        assert.equal(w.isResizable(), true);
      });
    });

    describe('hasShadow state', function() {
      // On Window there is no shadow by default and it can not be changed
      // dynamically.
      it('can be changed with hasShadow option', function() {
        w.destroy();
        let hasShadow = process.platform == 'darwin' ? false : true;
        w = new BrowserWindow({show: false, hasShadow: hasShadow});
        assert.equal(w.hasShadow(), hasShadow);
      });

      it('can be changed with setHasShadow method', function() {
        if (process.platform != 'darwin')
          return;

        assert.equal(w.hasShadow(), true);
        w.setHasShadow(false);
        assert.equal(w.hasShadow(), false);
        w.setHasShadow(true);
        assert.equal(w.hasShadow(), true);
      });
    });
  });

  describe('window.webContents.send(channel, args...)', function() {
    it('throws an error when the channel is missing', function() {
      assert.throws(function() {
        w.webContents.send();
      }, 'channel must be specified');

      assert.throws(function() {
        w.webContents.send(null);
      }, 'channel must be specified');
    });
  });
});
