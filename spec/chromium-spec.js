const assert = require('assert');
const http = require('http');
const path = require('path');
const ws = require('ws');
const remote = require('electron').remote;

const BrowserWindow = remote.require('electron').BrowserWindow;
const session = remote.require('electron').session;

describe('chromium feature', function() {
  var fixtures, listener;
  fixtures = path.resolve(__dirname, 'fixtures');
  listener = null;
  afterEach(function() {
    if (listener != null) {
      window.removeEventListener('message', listener);
    }
    return listener = null;
  });
  xdescribe('heap snapshot', function() {
    return it('does not crash', function() {
      return process.atomBinding('v8_util').takeHeapSnapshot();
    });
  });
  describe('sending request of http protocol urls', function() {
    return it('does not crash', function(done) {
      var server;
      this.timeout(5000);
      server = http.createServer(function(req, res) {
        res.end();
        server.close();
        return done();
      });
      return server.listen(0, '127.0.0.1', function() {
        var port;
        port = server.address().port;
        return $.get("http://127.0.0.1:" + port);
      });
    });
  });
  describe('document.hidden', function() {
    var url, w;
    url = "file://" + fixtures + "/pages/document-hidden.html";
    w = null;
    afterEach(function() {
      return w != null ? w.destroy() : void 0;
    });
    it('is set correctly when window is not shown', function(done) {
      w = new BrowserWindow({
        show: false
      });
      w.webContents.on('ipc-message', function(event, args) {
        assert.deepEqual(args, ['hidden', true]);
        return done();
      });
      return w.loadURL(url);
    });
    return it('is set correctly when window is inactive', function(done) {
      w = new BrowserWindow({
        show: false
      });
      w.webContents.on('ipc-message', function(event, args) {
        assert.deepEqual(args, ['hidden', false]);
        return done();
      });
      w.showInactive();
      return w.loadURL(url);
    });
  });
  xdescribe('navigator.webkitGetUserMedia', function() {
    return it('calls its callbacks', function(done) {
      this.timeout(5000);
      return navigator.webkitGetUserMedia({
        audio: true,
        video: false
      }, function() {
        return done();
      }, function() {
        return done();
      });
    });
  });
  describe('navigator.language', function() {
    return it('should not be empty', function() {
      return assert.notEqual(navigator.language, '');
    });
  });
  describe('navigator.serviceWorker', function() {
    var url, w;
    url = "file://" + fixtures + "/pages/service-worker/index.html";
    w = null;
    afterEach(function() {
      return w != null ? w.destroy() : void 0;
    });
    return it('should register for file scheme', function(done) {
      w = new BrowserWindow({
        show: false
      });
      w.webContents.on('ipc-message', function(event, args) {
        if (args[0] === 'reload') {
          return w.webContents.reload();
        } else if (args[0] === 'error') {
          return done('unexpected error : ' + args[1]);
        } else if (args[0] === 'response') {
          assert.equal(args[1], 'Hello from serviceWorker!');
          return session.defaultSession.clearStorageData({
            storages: ['serviceworkers']
          }, function() {
            return done();
          });
        }
      });
      return w.loadURL(url);
    });
  });
  describe('window.open', function() {
    this.timeout(20000);
    it('returns a BrowserWindowProxy object', function() {
      var b;
      b = window.open('about:blank', '', 'show=no');
      assert.equal(b.closed, false);
      assert.equal(b.constructor.name, 'BrowserWindowProxy');
      return b.close();
    });
    it('accepts "node-integration" as feature', function(done) {
      var b;
      listener = function(event) {
        assert.equal(event.data, 'undefined');
        b.close();
        return done();
      };
      window.addEventListener('message', listener);
      return b = window.open("file://" + fixtures + "/pages/window-opener-node.html", '', 'nodeIntegration=no,show=no');
    });
    it('inherit options of parent window', function(done) {
      var b;
      listener = function(event) {
        var height, ref1, width;
        ref1 = remote.getCurrentWindow().getSize(), width = ref1[0], height = ref1[1];
        assert.equal(event.data, "size: " + width + " " + height);
        b.close();
        return done();
      };
      window.addEventListener('message', listener);
      return b = window.open("file://" + fixtures + "/pages/window-open-size.html", '', 'show=no');
    });
    return it('does not override child options', function(done) {
      var b, size;
      size = {
        width: 350,
        height: 450
      };
      listener = function(event) {
        assert.equal(event.data, "size: " + size.width + " " + size.height);
        b.close();
        return done();
      };
      window.addEventListener('message', listener);
      return b = window.open("file://" + fixtures + "/pages/window-open-size.html", '', "show=no,width=" + size.width + ",height=" + size.height);
    });
  });
  describe('window.opener', function() {
    var url, w;
    this.timeout(10000);
    url = "file://" + fixtures + "/pages/window-opener.html";
    w = null;
    afterEach(function() {
      return w != null ? w.destroy() : void 0;
    });
    it('is null for main window', function(done) {
      w = new BrowserWindow({
        show: false
      });
      w.webContents.on('ipc-message', function(event, args) {
        assert.deepEqual(args, ['opener', null]);
        return done();
      });
      return w.loadURL(url);
    });
    return it('is not null for window opened by window.open', function(done) {
      var b;
      listener = function(event) {
        assert.equal(event.data, 'object');
        b.close();
        return done();
      };
      window.addEventListener('message', listener);
      return b = window.open(url, '', 'show=no');
    });
  });
  describe('window.postMessage', function() {
    return it('sets the source and origin correctly', function(done) {
      var b, sourceId;
      sourceId = remote.getCurrentWindow().id;
      listener = function(event) {
        var message;
        window.removeEventListener('message', listener);
        b.close();
        message = JSON.parse(event.data);
        assert.equal(message.data, 'testing');
        assert.equal(message.origin, 'file://');
        assert.equal(message.sourceEqualsOpener, true);
        assert.equal(message.sourceId, sourceId);
        assert.equal(event.origin, 'file://');
        return done();
      };
      window.addEventListener('message', listener);
      b = window.open("file://" + fixtures + "/pages/window-open-postMessage.html", '', 'show=no');
      return BrowserWindow.fromId(b.guestId).webContents.once('did-finish-load', function() {
        return b.postMessage('testing', '*');
      });
    });
  });
  describe('window.opener.postMessage', function() {
    return it('sets source and origin correctly', function(done) {
      var b;
      listener = function(event) {
        window.removeEventListener('message', listener);
        b.close();
        assert.equal(event.source, b);
        assert.equal(event.origin, 'file://');
        return done();
      };
      window.addEventListener('message', listener);
      return b = window.open("file://" + fixtures + "/pages/window-opener-postMessage.html", '', 'show=no');
    });
  });
  describe('creating a Uint8Array under browser side', function() {
    return it('does not crash', function() {
      var RUint8Array;
      RUint8Array = remote.getGlobal('Uint8Array');
      return new RUint8Array;
    });
  });
  describe('webgl', function() {
    return it('can be get as context in canvas', function() {
      var webgl;
      if (process.platform === 'linux') {
        return;
      }
      webgl = document.createElement('canvas').getContext('webgl');
      return assert.notEqual(webgl, null);
    });
  });
  describe('web workers', function() {
    it('Worker can work', function(done) {
      var message, worker;
      worker = new Worker('../fixtures/workers/worker.js');
      message = 'ping';
      worker.onmessage = function(event) {
        assert.equal(event.data, message);
        worker.terminate();
        return done();
      };
      return worker.postMessage(message);
    });
    return it('SharedWorker can work', function(done) {
      var message, worker;
      worker = new SharedWorker('../fixtures/workers/shared_worker.js');
      message = 'ping';
      worker.port.onmessage = function(event) {
        assert.equal(event.data, message);
        return done();
      };
      return worker.port.postMessage(message);
    });
  });
  describe('iframe', function() {
    var iframe;
    iframe = null;
    beforeEach(function() {
      return iframe = document.createElement('iframe');
    });
    afterEach(function() {
      return document.body.removeChild(iframe);
    });
    return it('does not have node integration', function(done) {
      iframe.src = "file://" + fixtures + "/pages/set-global.html";
      document.body.appendChild(iframe);
      return iframe.onload = function() {
        assert.equal(iframe.contentWindow.test, 'undefined undefined undefined');
        return done();
      };
    });
  });
  describe('storage', function() {
    return it('requesting persitent quota works', function(done) {
      return navigator.webkitPersistentStorage.requestQuota(1024 * 1024, function(grantedBytes) {
        assert.equal(grantedBytes, 1048576);
        return done();
      });
    });
  });
  describe('websockets', function() {
    var WebSocketServer, server, wss;
    wss = null;
    server = null;
    WebSocketServer = ws.Server;
    afterEach(function() {
      wss.close();
      return server.close();
    });
    return it('has user agent', function(done) {
      server = http.createServer();
      return server.listen(0, '127.0.0.1', function() {
        var port = server.address().port;
        wss = new WebSocketServer({
          server: server
        });
        wss.on('error', done);
        wss.on('connection', function(ws) {
          if (ws.upgradeReq.headers['user-agent']) {
            return done();
          } else {
            return done('user agent is empty');
          }
        });
        new WebSocket("ws://127.0.0.1:" + port);
      });
    });
  });
  return describe('Promise', function() {
    it('resolves correctly in Node.js calls', function(done) {
      document.registerElement('x-element', {
        prototype: Object.create(HTMLElement.prototype, {
          createdCallback: {
            value: function() {}
          }
        })
      });
      return setImmediate(function() {
        var called;
        called = false;
        Promise.resolve().then(function() {
          return done(called ? void 0 : new Error('wrong sequence'));
        });
        document.createElement('x-element');
        return called = true;
      });
    });
    return it('resolves correctly in Electron calls', function(done) {
      document.registerElement('y-element', {
        prototype: Object.create(HTMLElement.prototype, {
          createdCallback: {
            value: function() {}
          }
        })
      });
      return remote.getGlobal('setImmediate')(function() {
        var called;
        called = false;
        Promise.resolve().then(function() {
          return done(called ? void 0 : new Error('wrong sequence'));
        });
        document.createElement('y-element');
        return called = true;
      });
    });
  });
});
