var assert, http, path, url;

assert = require('assert');

path = require('path');

http = require('http');

url = require('url');

describe('<webview> tag', function() {
  var fixtures, webview;
  this.timeout(10000);
  fixtures = path.join(__dirname, 'fixtures');
  webview = null;
  beforeEach(function() {
    return webview = new WebView;
  });
  afterEach(function() {
    if (document.body.contains(webview)) {
      return document.body.removeChild(webview);
    }
  });
  describe('src attribute', function() {
    it('specifies the page to load', function(done) {
      webview.addEventListener('console-message', function(e) {
        assert.equal(e.message, 'a');
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/a.html";
      return document.body.appendChild(webview);
    });
    return it('navigates to new page when changed', function(done) {
      var listener = function() {
        webview.src = "file://" + fixtures + "/pages/b.html";
        webview.addEventListener('console-message', function(e) {
          assert.equal(e.message, 'b');
          return done();
        });
        return webview.removeEventListener('did-finish-load', listener);
      };
      webview.addEventListener('did-finish-load', listener);
      webview.src = "file://" + fixtures + "/pages/a.html";
      return document.body.appendChild(webview);
    });
  });
  describe('nodeintegration attribute', function() {
    it('inserts no node symbols when not set', function(done) {
      webview.addEventListener('console-message', function(e) {
        assert.equal(e.message, 'undefined undefined undefined undefined');
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/c.html";
      return document.body.appendChild(webview);
    });
    it('inserts node symbols when set', function(done) {
      webview.addEventListener('console-message', function(e) {
        assert.equal(e.message, 'function object object');
        return done();
      });
      webview.setAttribute('nodeintegration', 'on');
      webview.src = "file://" + fixtures + "/pages/d.html";
      return document.body.appendChild(webview);
    });
    it('loads node symbols after POST navigation when set', function(done) {
      webview.addEventListener('console-message', function(e) {
        assert.equal(e.message, 'function object object');
        return done();
      });
      webview.setAttribute('nodeintegration', 'on');
      webview.src = "file://" + fixtures + "/pages/post.html";
      return document.body.appendChild(webview);
    });
    if (process.platform !== 'win32' || process.execPath.toLowerCase().indexOf('\\out\\d\\') === -1) {
      return it('loads native modules when navigation happens', function(done) {
        var listener = function() {
          webview.removeEventListener('did-finish-load', listener);
          var listener2 = function(e) {
            assert.equal(e.message, 'function');
            return done();
          };
          webview.addEventListener('console-message', listener2);
          return webview.reload();
        };
        webview.addEventListener('did-finish-load', listener);
        webview.setAttribute('nodeintegration', 'on');
        webview.src = "file://" + fixtures + "/pages/native-module.html";
        return document.body.appendChild(webview);
      });
    }
  });
  describe('preload attribute', function() {
    it('loads the script before other scripts in window', function(done) {
      var listener;
      listener = function(e) {
        assert.equal(e.message, 'function object object');
        webview.removeEventListener('console-message', listener);
        return done();
      };
      webview.addEventListener('console-message', listener);
      webview.setAttribute('preload', fixtures + "/module/preload.js");
      webview.src = "file://" + fixtures + "/pages/e.html";
      return document.body.appendChild(webview);
    });
    it('preload script can still use "process" in required modules when nodeintegration is off', function(done) {
      webview.addEventListener('console-message', function(e) {
        assert.equal(e.message, 'object undefined object');
        return done();
      });
      webview.setAttribute('preload', fixtures + "/module/preload-node-off.js");
      webview.src = "file://" + fixtures + "/api/blank.html";
      return document.body.appendChild(webview);
    });
    return it('receives ipc message in preload script', function(done) {
      var listener, listener2, message;
      message = 'boom!';
      listener = function(e) {
        assert.equal(e.channel, 'pong');
        assert.deepEqual(e.args, [message]);
        webview.removeEventListener('ipc-message', listener);
        return done();
      };
      listener2 = function() {
        webview.send('ping', message);
        return webview.removeEventListener('did-finish-load', listener2);
      };
      webview.addEventListener('ipc-message', listener);
      webview.addEventListener('did-finish-load', listener2);
      webview.setAttribute('preload', fixtures + "/module/preload-ipc.js");
      webview.src = "file://" + fixtures + "/pages/e.html";
      return document.body.appendChild(webview);
    });
  });
  describe('httpreferrer attribute', function() {
    return it('sets the referrer url', function(done) {
      var listener, referrer;
      referrer = 'http://github.com/';
      listener = function(e) {
        assert.equal(e.message, referrer);
        webview.removeEventListener('console-message', listener);
        return done();
      };
      webview.addEventListener('console-message', listener);
      webview.setAttribute('httpreferrer', referrer);
      webview.src = "file://" + fixtures + "/pages/referrer.html";
      return document.body.appendChild(webview);
    });
  });
  describe('useragent attribute', function() {
    return it('sets the user agent', function(done) {
      var listener, referrer;
      referrer = 'Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko';
      listener = function(e) {
        assert.equal(e.message, referrer);
        webview.removeEventListener('console-message', listener);
        return done();
      };
      webview.addEventListener('console-message', listener);
      webview.setAttribute('useragent', referrer);
      webview.src = "file://" + fixtures + "/pages/useragent.html";
      return document.body.appendChild(webview);
    });
  });
  describe('disablewebsecurity attribute', function() {
    it('does not disable web security when not set', function(done) {
      var encoded, listener, src;
      src = "<script src='file://" + __dirname + "/static/jquery-2.0.3.min.js'></script> <script>console.log('ok');</script>";
      encoded = btoa(unescape(encodeURIComponent(src)));
      listener = function(e) {
        assert(/Not allowed to load local resource/.test(e.message));
        webview.removeEventListener('console-message', listener);
        return done();
      };
      webview.addEventListener('console-message', listener);
      webview.src = "data:text/html;base64," + encoded;
      return document.body.appendChild(webview);
    });
    return it('disables web security when set', function(done) {
      var encoded, listener, src;
      src = "<script src='file://" + __dirname + "/static/jquery-2.0.3.min.js'></script> <script>console.log('ok');</script>";
      encoded = btoa(unescape(encodeURIComponent(src)));
      listener = function(e) {
        assert.equal(e.message, 'ok');
        webview.removeEventListener('console-message', listener);
        return done();
      };
      webview.addEventListener('console-message', listener);
      webview.setAttribute('disablewebsecurity', '');
      webview.src = "data:text/html;base64," + encoded;
      return document.body.appendChild(webview);
    });
  });
  describe('partition attribute', function() {
    it('inserts no node symbols when not set', function(done) {
      webview.addEventListener('console-message', function(e) {
        assert.equal(e.message, 'undefined undefined undefined undefined');
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/c.html";
      webview.partition = 'test1';
      return document.body.appendChild(webview);
    });
    it('inserts node symbols when set', function(done) {
      webview.addEventListener('console-message', function(e) {
        assert.equal(e.message, 'function object object');
        return done();
      });
      webview.setAttribute('nodeintegration', 'on');
      webview.src = "file://" + fixtures + "/pages/d.html";
      webview.partition = 'test2';
      return document.body.appendChild(webview);
    });
    it('isolates storage for different id', function(done) {
      var listener;
      listener = function(e) {
        assert.equal(e.message, " 0");
        webview.removeEventListener('console-message', listener);
        return done();
      };
      window.localStorage.setItem('test', 'one');
      webview.addEventListener('console-message', listener);
      webview.src = "file://" + fixtures + "/pages/partition/one.html";
      webview.partition = 'test3';
      return document.body.appendChild(webview);
    });
    return it('uses current session storage when no id is provided', function(done) {
      var listener;
      listener = function(e) {
        assert.equal(e.message, "one 1");
        webview.removeEventListener('console-message', listener);
        return done();
      };
      window.localStorage.setItem('test', 'one');
      webview.addEventListener('console-message', listener);
      webview.src = "file://" + fixtures + "/pages/partition/one.html";
      return document.body.appendChild(webview);
    });
  });
  describe('allowpopups attribute', function() {
    it('can not open new window when not set', function(done) {
      var listener;
      listener = function(e) {
        assert.equal(e.message, 'null');
        webview.removeEventListener('console-message', listener);
        return done();
      };
      webview.addEventListener('console-message', listener);
      webview.src = "file://" + fixtures + "/pages/window-open-hide.html";
      return document.body.appendChild(webview);
    });
    return it('can open new window when set', function(done) {
      var listener;
      listener = function(e) {
        assert.equal(e.message, 'window');
        webview.removeEventListener('console-message', listener);
        return done();
      };
      webview.addEventListener('console-message', listener);
      webview.setAttribute('allowpopups', 'on');
      webview.src = "file://" + fixtures + "/pages/window-open-hide.html";
      return document.body.appendChild(webview);
    });
  });
  describe('new-window event', function() {
    it('emits when window.open is called', function(done) {
      webview.addEventListener('new-window', function(e) {
        assert.equal(e.url, 'http://host/');
        assert.equal(e.frameName, 'host');
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/window-open.html";
      return document.body.appendChild(webview);
    });
    return it('emits when link with target is called', function(done) {
      webview.addEventListener('new-window', function(e) {
        assert.equal(e.url, 'http://host/');
        assert.equal(e.frameName, 'target');
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/target-name.html";
      return document.body.appendChild(webview);
    });
  });
  describe('ipc-message event', function() {
    return it('emits when guest sends a ipc message to browser', function(done) {
      webview.addEventListener('ipc-message', function(e) {
        assert.equal(e.channel, 'channel');
        assert.deepEqual(e.args, ['arg1', 'arg2']);
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/ipc-message.html";
      webview.setAttribute('nodeintegration', 'on');
      return document.body.appendChild(webview);
    });
  });
  describe('page-title-set event', function() {
    return it('emits when title is set', function(done) {
      webview.addEventListener('page-title-set', function(e) {
        assert.equal(e.title, 'test');
        assert(e.explicitSet);
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/a.html";
      return document.body.appendChild(webview);
    });
  });
  describe('page-favicon-updated event', function() {
    return it('emits when favicon urls are received', function(done) {
      webview.addEventListener('page-favicon-updated', function(e) {
        var pageUrl;
        assert.equal(e.favicons.length, 2);
        pageUrl = process.platform === 'win32' ? 'file:///C:/favicon.png' : 'file:///favicon.png';
        assert.equal(e.favicons[0], pageUrl);
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/a.html";
      return document.body.appendChild(webview);
    });
  });
  describe('will-navigate event', function() {
    return it('emits when a url that leads to oustide of the page is clicked', function(done) {
      webview.addEventListener('will-navigate', function(e) {
        assert.equal(e.url, "http://host/");
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/webview-will-navigate.html";
      return document.body.appendChild(webview);
    });
  });
  describe('did-navigate event', function() {
    var p, pageUrl;
    p = path.join(fixtures, 'pages', 'webview-will-navigate.html');
    p = p.replace(/\\/g, '/');
    pageUrl = url.format({
      protocol: 'file',
      slashes: true,
      pathname: p
    });
    return it('emits when a url that leads to outside of the page is clicked', function(done) {
      webview.addEventListener('did-navigate', function(e) {
        assert.equal(e.url, pageUrl);
        return done();
      });
      webview.src = pageUrl;
      return document.body.appendChild(webview);
    });
  });
  describe('did-navigate-in-page event', function() {
    it('emits when an anchor link is clicked', function(done) {
      var p, pageUrl;
      p = path.join(fixtures, 'pages', 'webview-did-navigate-in-page.html');
      p = p.replace(/\\/g, '/');
      pageUrl = url.format({
        protocol: 'file',
        slashes: true,
        pathname: p
      });
      webview.addEventListener('did-navigate-in-page', function(e) {
        assert.equal(e.url, pageUrl + "#test_content");
        return done();
      });
      webview.src = pageUrl;
      return document.body.appendChild(webview);
    });
    it('emits when window.history.replaceState is called', function(done) {
      webview.addEventListener('did-navigate-in-page', function(e) {
        assert.equal(e.url, "http://host/");
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/webview-did-navigate-in-page-with-history.html";
      return document.body.appendChild(webview);
    });
    return it('emits when window.location.hash is changed', function(done) {
      var p, pageUrl;
      p = path.join(fixtures, 'pages', 'webview-did-navigate-in-page-with-hash.html');
      p = p.replace(/\\/g, '/');
      pageUrl = url.format({
        protocol: 'file',
        slashes: true,
        pathname: p
      });
      webview.addEventListener('did-navigate-in-page', function(e) {
        assert.equal(e.url, pageUrl + "#test");
        return done();
      });
      webview.src = pageUrl;
      return document.body.appendChild(webview);
    });
  });
  describe('close event', function() {
    return it('should fire when interior page calls window.close', function(done) {
      webview.addEventListener('close', function() {
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/close.html";
      return document.body.appendChild(webview);
    });
  });
  describe('devtools-opened event', function() {
    return it('should fire when webview.openDevTools() is called', function(done) {
      var listener;
      listener = function() {
        webview.removeEventListener('devtools-opened', listener);
        webview.closeDevTools();
        return done();
      };
      webview.addEventListener('devtools-opened', listener);
      webview.addEventListener('dom-ready', function() {
        return webview.openDevTools();
      });
      webview.src = "file://" + fixtures + "/pages/base-page.html";
      return document.body.appendChild(webview);
    });
  });
  describe('devtools-closed event', function() {
    return it('should fire when webview.closeDevTools() is called', function(done) {
      var listener, listener2;
      listener2 = function() {
        webview.removeEventListener('devtools-closed', listener2);
        return done();
      };
      listener = function() {
        webview.removeEventListener('devtools-opened', listener);
        return webview.closeDevTools();
      };
      webview.addEventListener('devtools-opened', listener);
      webview.addEventListener('devtools-closed', listener2);
      webview.addEventListener('dom-ready', function() {
        return webview.openDevTools();
      });
      webview.src = "file://" + fixtures + "/pages/base-page.html";
      return document.body.appendChild(webview);
    });
  });
  describe('devtools-focused event', function() {
    return it('should fire when webview.openDevTools() is called', function(done) {
      var listener;
      listener = function() {
        webview.removeEventListener('devtools-focused', listener);
        webview.closeDevTools();
        return done();
      };
      webview.addEventListener('devtools-focused', listener);
      webview.addEventListener('dom-ready', function() {
        return webview.openDevTools();
      });
      webview.src = "file://" + fixtures + "/pages/base-page.html";
      return document.body.appendChild(webview);
    });
  });
  describe('<webview>.reload()', function() {
    return it('should emit beforeunload handler', function(done) {
      var listener, listener2;
      listener = function(e) {
        assert.equal(e.channel, 'onbeforeunload');
        webview.removeEventListener('ipc-message', listener);
        return done();
      };
      listener2 = function() {
        webview.reload();
        return webview.removeEventListener('did-finish-load', listener2);
      };
      webview.addEventListener('ipc-message', listener);
      webview.addEventListener('did-finish-load', listener2);
      webview.setAttribute('nodeintegration', 'on');
      webview.src = "file://" + fixtures + "/pages/beforeunload-false.html";
      return document.body.appendChild(webview);
    });
  });
  describe('<webview>.clearHistory()', function() {
    return it('should clear the navigation history', function(done) {
      var listener;
      listener = function(e) {
        assert.equal(e.channel, 'history');
        assert.equal(e.args[0], 2);
        assert(webview.canGoBack());
        webview.clearHistory();
        assert(!webview.canGoBack());
        webview.removeEventListener('ipc-message', listener);
        return done();
      };
      webview.addEventListener('ipc-message', listener);
      webview.setAttribute('nodeintegration', 'on');
      webview.src = "file://" + fixtures + "/pages/history.html";
      return document.body.appendChild(webview);
    });
  });
  describe('basic auth', function() {
    var auth;
    auth = require('basic-auth');
    return it('should authenticate with correct credentials', function(done) {
      var message, server;
      message = 'Authenticated';
      server = http.createServer(function(req, res) {
        var credentials;
        credentials = auth(req);
        if (credentials.name === 'test' && credentials.pass === 'test') {
          res.end(message);
        } else {
          res.end('failed');
        }
        return server.close();
      });
      return server.listen(0, '127.0.0.1', function() {
        var port;
        port = server.address().port;
        webview.addEventListener('ipc-message', function(e) {
          assert.equal(e.channel, message);
          return done();
        });
        webview.src = "file://" + fixtures + "/pages/basic-auth.html?port=" + port;
        webview.setAttribute('nodeintegration', 'on');
        return document.body.appendChild(webview);
      });
    });
  });
  describe('dom-ready event', function() {
    return it('emits when document is loaded', function(done) {
      var server;
      server = http.createServer(function() {});
      return server.listen(0, '127.0.0.1', function() {
        var port;
        port = server.address().port;
        webview.addEventListener('dom-ready', function() {
          return done();
        });
        webview.src = "file://" + fixtures + "/pages/dom-ready.html?port=" + port;
        return document.body.appendChild(webview);
      });
    });
  });
  describe('executeJavaScript', function() {
    if (process.env.TRAVIS !== 'true') {
      return;
    }
    return it('should support user gesture', function(done) {
      var listener, listener2;
      listener = function() {
        webview.removeEventListener('enter-html-full-screen', listener);
        return done();
      };
      listener2 = function() {
        var jsScript;
        jsScript = 'document.getElementsByTagName("video")[0].webkitRequestFullScreen()';
        webview.executeJavaScript(jsScript, true);
        return webview.removeEventListener('did-finish-load', listener2);
      };
      webview.addEventListener('enter-html-full-screen', listener);
      webview.addEventListener('did-finish-load', listener2);
      webview.src = "file://" + fixtures + "/pages/fullscreen.html";
      return document.body.appendChild(webview);
    });
  });
  describe('sendInputEvent', function() {
    it('can send keyboard event', function(done) {
      webview.addEventListener('ipc-message', function(e) {
        assert.equal(e.channel, 'keyup');
        assert.deepEqual(e.args, [67, true, false]);
        return done();
      });
      webview.addEventListener('dom-ready', function() {
        return webview.sendInputEvent({
          type: 'keyup',
          keyCode: 'c',
          modifiers: ['shift']
        });
      });
      webview.src = "file://" + fixtures + "/pages/onkeyup.html";
      webview.setAttribute('nodeintegration', 'on');
      return document.body.appendChild(webview);
    });
    return it('can send mouse event', function(done) {
      webview.addEventListener('ipc-message', function(e) {
        assert.equal(e.channel, 'mouseup');
        assert.deepEqual(e.args, [10, 20, false, true]);
        return done();
      });
      webview.addEventListener('dom-ready', function() {
        return webview.sendInputEvent({
          type: 'mouseup',
          modifiers: ['ctrl'],
          x: 10,
          y: 20
        });
      });
      webview.src = "file://" + fixtures + "/pages/onmouseup.html";
      webview.setAttribute('nodeintegration', 'on');
      return document.body.appendChild(webview);
    });
  });
  describe('media-started-playing media-paused events', function() {
    return it('emits when audio starts and stops playing', function(done) {
      var audioPlayed;
      audioPlayed = false;
      webview.addEventListener('media-started-playing', function() {
        return audioPlayed = true;
      });
      webview.addEventListener('media-paused', function() {
        assert(audioPlayed);
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/audio.html";
      return document.body.appendChild(webview);
    });
  });
  describe('found-in-page event', function() {
    return it('emits when a request is made', function(done) {
      var listener, listener2, requestId;
      requestId = null;
      listener = function(e) {
        assert.equal(e.result.requestId, requestId);
        if (e.result.finalUpdate) {
          assert.equal(e.result.matches, 3);
          webview.stopFindInPage("clearSelection");
          return done();
        }
      };
      listener2 = function() {
        return requestId = webview.findInPage("virtual");
      };
      webview.addEventListener('found-in-page', listener);
      webview.addEventListener('did-finish-load', listener2);
      webview.src = "file://" + fixtures + "/pages/content.html";
      return document.body.appendChild(webview);
    });
  });
  return xdescribe('did-change-theme-color event', function() {
    return it('emits when theme color changes', function(done) {
      webview.addEventListener('did-change-theme-color', function() {
        return done();
      });
      webview.src = "file://" + fixtures + "/pages/theme-color.html";
      return document.body.appendChild(webview);
    });
  });
});
