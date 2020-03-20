const chai = require('chai');
const dirtyChai = require('dirty-chai');
const fs = require('fs');
const http = require('http');
const path = require('path');
const ws = require('ws');
const url = require('url');
const ChildProcess = require('child_process');
const { ipcRenderer, remote } = require('electron');
const { emittedOnce } = require('./events-helpers');
const { closeWindow, waitForWebContentsToLoad } = require('./window-helpers');
const { resolveGetters } = require('./expect-helpers');
const { app, BrowserWindow, ipcMain, protocol, session, webContents } = remote;
const isCI = remote.getGlobal('isCi');
const features = process.electronBinding('features');

const { expect } = chai;
chai.use(dirtyChai);

/* Most of the APIs here don't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('chromium feature', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');
  let listener = null;
  let w = null;

  afterEach(() => {
    if (listener != null) {
      window.removeEventListener('message', listener);
    }
    listener = null;
  });

  afterEach(async () => {
    await closeWindow(w);
    w = null;
  });

  describe('command line switches', () => {
    describe('--lang switch', () => {
      const currentLocale = app.getLocale();
      const testLocale = (locale, result, done) => {
        const appPath = path.join(__dirname, 'fixtures', 'api', 'locale-check');
        const electronPath = remote.getGlobal('process').execPath;
        let output = '';
        const appProcess = ChildProcess.spawn(electronPath, [appPath, `--lang=${locale}`]);

        appProcess.stdout.on('data', (data) => { output += data; });
        appProcess.stdout.on('end', () => {
          output = output.replace(/(\r\n|\n|\r)/gm, '');
          expect(output).to.equal(result);
          done();
        });
      };

      it('should set the locale', (done) => testLocale('fr', 'fr', done));
      it('should not set an invalid locale', (done) => testLocale('asdfkl', currentLocale, done));
    });

    describe('--remote-debugging-port switch', () => {
      it('should display the discovery page', (done) => {
        const electronPath = remote.getGlobal('process').execPath;
        let output = '';
        const appProcess = ChildProcess.spawn(electronPath, [`--remote-debugging-port=`]);

        appProcess.stderr.on('data', (data) => {
          output += data;
          const m = /DevTools listening on ws:\/\/127.0.0.1:(\d+)\//.exec(output);
          if (m) {
            appProcess.stderr.removeAllListeners('data');
            const port = m[1];
            http.get(`http://127.0.0.1:${port}`, (res) => {
              res.destroy();
              appProcess.kill();
              expect(res.statusCode).to.eql(200);
              expect(parseInt(res.headers['content-length'])).to.be.greaterThan(0);
              done();
            });
          }
        });
      });
    });
  });

  describe('heap snapshot', () => {
    it('does not crash', function () {
      process.electronBinding('v8_util').takeHeapSnapshot();
    });
  });

  describe('accessing key names also used as Node.js module names', () => {
    it('does not crash', (done) => {
      w = new BrowserWindow({ show: false });
      w.webContents.once('did-finish-load', () => { done(); });
      w.webContents.once('crashed', () => done(new Error('WebContents crashed.')));
      w.loadFile(path.join(fixtures, 'pages', 'external-string.html'));
    });
  });

  describe('loading jquery', () => {
    it('does not crash', (done) => {
      w = new BrowserWindow({ show: false });
      w.webContents.once('did-finish-load', () => { done(); });
      w.webContents.once('crashed', () => done(new Error('WebContents crashed.')));
      w.loadFile(path.join(fixtures, 'pages', 'jquery.html'));
    });
  });

  describe('navigator.webkitGetUserMedia', () => {
    it('calls its callbacks', (done) => {
      navigator.webkitGetUserMedia({
        audio: true,
        video: false
      }, () => done(),
      () => done());
    });
  });

  describe('navigator.mediaDevices', () => {
    if (isCI) return;

    afterEach(() => {
      remote.getGlobal('permissionChecks').allow();
    });

    it('can return labels of enumerated devices', (done) => {
      navigator.mediaDevices.enumerateDevices().then((devices) => {
        const labels = devices.map((device) => device.label);
        const labelFound = labels.some((label) => !!label);
        if (labelFound) {
          done();
        } else {
          done(new Error(`No device labels found: ${JSON.stringify(labels)}`));
        }
      }).catch(done);
    });

    it('does not return labels of enumerated devices when permission denied', (done) => {
      remote.getGlobal('permissionChecks').reject();
      navigator.mediaDevices.enumerateDevices().then((devices) => {
        const labels = devices.map((device) => device.label);
        const labelFound = labels.some((label) => !!label);
        if (labelFound) {
          done(new Error(`Device labels were found: ${JSON.stringify(labels)}`));
        } else {
          done();
        }
      }).catch(done);
    });

    it('can return new device id when cookie storage is cleared', (done) => {
      const options = {
        origin: null,
        storages: ['cookies']
      };
      const deviceIds = [];
      const ses = session.fromPartition('persist:media-device-id');
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          session: ses
        }
      });
      w.webContents.on('ipc-message', (event, channel, deviceId) => {
        if (channel === 'deviceIds') deviceIds.push(deviceId);
        if (deviceIds.length === 2) {
          expect(deviceIds[0]).to.not.deep.equal(deviceIds[1]);
          closeWindow(w).then(() => {
            w = null;
            done();
          }).catch((error) => done(error));
        } else {
          ses.clearStorageData(options).then(() => {
            w.webContents.reload();
          });
        }
      });
      w.loadFile(path.join(fixtures, 'pages', 'media-id-reset.html'));
    });
  });

  describe('navigator.language', () => {
    it('should not be empty', () => {
      expect(navigator.language).to.not.equal('');
    });
  });

  describe('navigator.languages', (done) => {
    it('should return the system locale only', () => {
      const appLocale = app.getLocale();
      expect(navigator.languages).to.deep.equal([appLocale]);
    });
  });

  describe('navigator.serviceWorker', () => {
    it('should register for file scheme', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'sw-file-scheme-spec'
        }
      });
      w.webContents.on('ipc-message', (event, channel, message) => {
        if (channel === 'reload') {
          w.webContents.reload();
        } else if (channel === 'error') {
          done(message);
        } else if (channel === 'response') {
          expect(message).to.equal('Hello from serviceWorker!');
          session.fromPartition('sw-file-scheme-spec').clearStorageData({
            storages: ['serviceworkers']
          }).then(() => done());
        }
      });
      w.webContents.on('crashed', () => done(new Error('WebContents crashed.')));
      w.loadFile(path.join(fixtures, 'pages', 'service-worker', 'index.html'));
    });

    it('should register for intercepted file scheme', (done) => {
      const customSession = session.fromPartition('intercept-file');
      customSession.protocol.interceptBufferProtocol('file', (request, callback) => {
        let file = url.parse(request.url).pathname;
        if (file[0] === '/' && process.platform === 'win32') file = file.slice(1);

        const content = fs.readFileSync(path.normalize(file));
        const ext = path.extname(file);
        let type = 'text/html';

        if (ext === '.js') type = 'application/javascript';
        callback({ data: content, mimeType: type });
      }, (error) => {
        if (error) done(error);
      });

      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          session: customSession
        }
      });
      w.webContents.on('ipc-message', (event, channel, message) => {
        if (channel === 'reload') {
          w.webContents.reload();
        } else if (channel === 'error') {
          done(`unexpected error : ${message}`);
        } else if (channel === 'response') {
          expect(message).to.equal('Hello from serviceWorker!');
          customSession.clearStorageData({
            storages: ['serviceworkers']
          }).then(() => {
            customSession.protocol.uninterceptProtocol('file', error => done(error));
          });
        }
      });
      w.webContents.on('crashed', () => done(new Error('WebContents crashed.')));
      w.loadFile(path.join(fixtures, 'pages', 'service-worker', 'index.html'));
    });
  });

  describe('navigator.geolocation', () => {
    before(function () {
      if (!features.isFakeLocationProviderEnabled()) {
        return this.skip();
      }
    });

    it('returns position when permission is granted', (done) => {
      navigator.geolocation.getCurrentPosition((position) => {
        expect(position).to.have.a.property('coords');
        expect(position).to.have.a.property('timestamp');
        done();
      }, (error) => {
        done(error);
      });
    });

    it('returns error when permission is denied', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'geolocation-spec'
        }
      });
      w.webContents.on('ipc-message', (event, channel) => {
        if (channel === 'success') {
          done();
        } else {
          done('unexpected response from geolocation api');
        }
      });
      w.webContents.session.setPermissionRequestHandler((wc, permission, callback) => {
        if (permission === 'geolocation') {
          callback(false);
        } else {
          callback(true);
        }
      });
      w.loadFile(path.join(fixtures, 'pages', 'geolocation', 'index.html'));
    });
  });

  describe('window.open', () => {
    it('returns a BrowserWindowProxy object', () => {
      const b = window.open('about:blank', '', 'show=no');
      expect(b.closed).to.be.false();
      expect(b.constructor.name).to.equal('BrowserWindowProxy');
      b.close();
    });

    it('accepts "nodeIntegration" as feature', (done) => {
      let b = null;
      listener = (event) => {
        expect(event.data.isProcessGlobalUndefined).to.be.true();
        b.close();
        done();
      };
      window.addEventListener('message', listener);
      b = window.open(`file://${fixtures}/pages/window-opener-node.html`, '', 'nodeIntegration=no,show=no');
    });

    it('inherit options of parent window', (done) => {
      let b = null;
      listener = (event) => {
        const ref1 = remote.getCurrentWindow().getSize();
        const width = ref1[0];
        const height = ref1[1];
        expect(event.data).to.equal(`size: ${width} ${height}`);
        b.close();
        done();
      };
      window.addEventListener('message', listener);
      b = window.open(`file://${fixtures}/pages/window-open-size.html`, '', 'show=no');
    });

    for (const show of [true, false]) {
      it(`inherits parent visibility over parent {show=${show}} option`, (done) => {
        const w = new BrowserWindow({ show });

        // toggle visibility
        if (show) {
          w.hide();
        } else {
          w.show();
        }

        w.webContents.once('new-window', (e, url, frameName, disposition, options) => {
          expect(options.show).to.equal(w.isVisible());
          w.close();
          done();
        });
        w.loadFile(path.join(fixtures, 'pages', 'window-open.html'));
      });
    }

    it('disables node integration when it is disabled on the parent window', (done) => {
      let b = null;
      listener = (event) => {
        expect(event.data.isProcessGlobalUndefined).to.be.true();
        b.close();
        done();
      };
      window.addEventListener('message', listener);

      const windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-opener-no-node-integration.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-node.html`
        },
        slashes: true
      });
      b = window.open(windowUrl, '', 'nodeIntegration=no,show=no');
    });

    // TODO(codebytere): re-enable this test
    xit('disables node integration when it is disabled on the parent window for chrome devtools URLs', (done) => {
      let b = null;
      app.once('web-contents-created', (event, contents) => {
        contents.once('did-finish-load', () => {
          contents.executeJavaScript('typeof process').then((typeofProcessGlobal) => {
            expect(typeofProcessGlobal).to.equal('undefined');
            b.close();
            done();
          }).catch(done);
        });
      });
      b = window.open('devtools://devtools/bundled/inspector.html', '', 'nodeIntegration=no,show=no');
    });

    it('disables JavaScript when it is disabled on the parent window', (done) => {
      let b = null;
      app.once('web-contents-created', (event, contents) => {
        contents.once('did-finish-load', () => {
          app.once('browser-window-created', (event, window) => {
            const preferences = window.webContents.getLastWebPreferences();
            expect(preferences.javascript).to.be.false();
            window.destroy();
            b.close();
            done();
          });
          // Click link on page
          contents.sendInputEvent({ type: 'mouseDown', clickCount: 1, x: 1, y: 1 });
          contents.sendInputEvent({ type: 'mouseUp', clickCount: 1, x: 1, y: 1 });
        });
      });

      const windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-no-javascript.html`,
        protocol: 'file',
        slashes: true
      });
      b = window.open(windowUrl, '', 'javascript=no,show=no');
    });

    it('disables the <webview> tag when it is disabled on the parent window', (done) => {
      let b = null;
      listener = (event) => {
        expect(event.data.isWebViewGlobalUndefined).to.be.true();
        b.close();
        done();
      };
      window.addEventListener('message', listener);

      const windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-opener-no-webview-tag.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-webview.html`
        },
        slashes: true
      });
      b = window.open(windowUrl, '', 'webviewTag=no,nodeIntegration=yes,show=no');
    });

    it('does not override child options', (done) => {
      let b = null;
      const size = {
        width: 350,
        height: 450
      };
      listener = (event) => {
        expect(event.data).to.equal(`size: ${size.width} ${size.height}`);
        b.close();
        done();
      };
      window.addEventListener('message', listener);
      b = window.open(`file://${fixtures}/pages/window-open-size.html`, '', 'show=no,width=' + size.width + ',height=' + size.height);
    });

    it('handles cycles when merging the parent options into the child options', (done) => {
      w = BrowserWindow.fromId(ipcRenderer.sendSync('create-window-with-options-cycle'));
      w.loadFile(path.join(fixtures, 'pages', 'window-open.html'));
      w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
        expect(options.show).to.be.false();
        expect(...resolveGetters(options.foo)).to.deep.equal({
          bar: undefined,
          baz: {
            hello: {
              world: true
            }
          },
          baz2: {
            hello: {
              world: true
            }
          }
        });
        done();
      });
    });

    it('defines a window.location getter', (done) => {
      let b = null;
      let targetURL;
      if (process.platform === 'win32') {
        targetURL = `file:///${fixtures.replace(/\\/g, '/')}/pages/base-page.html`;
      } else {
        targetURL = `file://${fixtures}/pages/base-page.html`;
      }
      app.once('browser-window-created', (event, window) => {
        window.webContents.once('did-finish-load', () => {
          expect(b.location.href).to.equal(targetURL);
          b.close();
          done();
        });
      });
      b = window.open(targetURL);
    });

    it('defines a window.location setter', (done) => {
      let b = null;
      app.once('browser-window-created', (event, { webContents }) => {
        webContents.once('did-finish-load', () => {
          // When it loads, redirect
          b.location = `file://${fixtures}/pages/base-page.html`;
          webContents.once('did-finish-load', () => {
            // After our second redirect, cleanup and callback
            b.close();
            done();
          });
        });
      });
      b = window.open('about:blank');
    });

    it('defines a window.location.href setter', (done) => {
      let b = null;
      app.once('browser-window-created', (event, { webContents }) => {
        webContents.once('did-finish-load', () => {
          // When it loads, redirect
          b.location.href = `file://${fixtures}/pages/base-page.html`;
          webContents.once('did-finish-load', () => {
            // After our second redirect, cleanup and callback
            b.close();
            done();
          });
        });
      });
      b = window.open('about:blank');
    });

    it('open a blank page when no URL is specified', async () => {
      const browserWindowCreated = emittedOnce(app, 'browser-window-created');
      const w = window.open();
      try {
        const [, { webContents }] = await browserWindowCreated;
        await waitForWebContentsToLoad(webContents);
        expect(w.location.href).to.equal('about:blank');
      } finally {
        w.close();
      }
    });

    it('open a blank page when an empty URL is specified', async () => {
      const browserWindowCreated = emittedOnce(app, 'browser-window-created');
      const w = window.open('');
      try {
        const [, { webContents }] = await browserWindowCreated;
        await waitForWebContentsToLoad(webContents);
        expect(w.location.href).to.equal('about:blank');
      } finally {
        w.close();
      }
    });

    it('throws an exception when the arguments cannot be converted to strings', () => {
      expect(() => {
        window.open('', { toString: null });
      }).to.throw('Cannot convert object to primitive value');

      expect(() => {
        window.open('', '', { toString: 3 });
      }).to.throw('Cannot convert object to primitive value');
    });

    it('sets the window title to the specified frameName', (done) => {
      let b = null;
      app.once('browser-window-created', (event, createdWindow) => {
        expect(createdWindow.getTitle()).to.equal('hello');
        b.close();
        done();
      });
      b = window.open('', 'hello');
    });

    it('does not throw an exception when the frameName is a built-in object property', (done) => {
      let b = null;
      app.once('browser-window-created', (event, createdWindow) => {
        expect(createdWindow.getTitle()).to.equal('__proto__');
        b.close();
        done();
      });
      b = window.open('', '__proto__');
    });

    it('does not throw an exception when the features include webPreferences', () => {
      let b = null;
      expect(() => {
        b = window.open('', '', 'webPreferences=');
      }).to.not.throw();
      b.close();
    });
  });

  describe('window.opener', () => {
    const url = `file://${fixtures}/pages/window-opener.html`;
    it('is null for main window', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      });
      const promise = emittedOnce(w.webContents, 'ipc-message');
      w.loadFile(path.join(fixtures, 'pages', 'window-opener.html'));
      const [, channel, opener] = await promise;
      expect(channel).to.equal('opener');
      expect(opener).to.equal(null);
    });

    it('is not null for window opened by window.open', (done) => {
      let b = null;
      listener = (event) => {
        expect(event.data).to.equal('object');
        b.close();
        done();
      };
      window.addEventListener('message', listener);
      b = window.open(url, '', 'show=no');
    });
  });

  describe('window.opener access from BrowserWindow', () => {
    const scheme = 'other';
    const url = `${scheme}://${fixtures}/pages/window-opener-location.html`;
    let w = null;

    before((done) => {
      protocol.registerFileProtocol(scheme, (request, callback) => {
        callback(`${fixtures}/pages/window-opener-location.html`);
      }, (error) => done(error));
    });

    after(() => {
      protocol.unregisterProtocol(scheme);
    });

    afterEach(() => {
      w.close();
    });

    it('fails when origin of current window does not match opener', (done) => {
      listener = (event) => {
        expect(event.data).to.equal(null);
        done();
      };
      window.addEventListener('message', listener);
      w = window.open(url, '', 'show=no,nodeIntegration=no');
    });

    it('works when origin matches', (done) => {
      listener = (event) => {
        expect(event.data).to.equal(location.href);
        done();
      };
      window.addEventListener('message', listener);
      w = window.open(`file://${fixtures}/pages/window-opener-location.html`, '', 'show=no,nodeIntegration=no');
    });

    it('works when origin does not match opener but has node integration', (done) => {
      listener = (event) => {
        expect(event.data).to.equal(location.href);
        done();
      };
      window.addEventListener('message', listener);
      w = window.open(url, '', 'show=no,nodeIntegration=yes');
    });
  });

  describe('window.opener access from <webview>', () => {
    const scheme = 'other';
    const srcPath = `${fixtures}/pages/webview-opener-postMessage.html`;
    const pageURL = `file://${fixtures}/pages/window-opener-location.html`;
    let webview = null;

    before((done) => {
      protocol.registerFileProtocol(scheme, (request, callback) => {
        callback(srcPath);
      }, (error) => done(error));
    });

    after(() => {
      protocol.unregisterProtocol(scheme);
    });

    afterEach(() => {
      if (webview != null) webview.remove();
    });

    it('fails when origin of webview src URL does not match opener', (done) => {
      webview = new WebView();
      webview.addEventListener('console-message', (e) => {
        expect(e.message).to.equal('null');
        done();
      });
      webview.setAttribute('allowpopups', 'on');
      webview.src = url.format({
        pathname: srcPath,
        protocol: scheme,
        query: {
          p: pageURL
        },
        slashes: true
      });
      document.body.appendChild(webview);
    });

    it('works when origin matches', (done) => {
      webview = new WebView();
      webview.addEventListener('console-message', (e) => {
        expect(e.message).to.equal(webview.src);
        done();
      });
      webview.setAttribute('allowpopups', 'on');
      webview.src = url.format({
        pathname: srcPath,
        protocol: 'file',
        query: {
          p: pageURL
        },
        slashes: true
      });
      document.body.appendChild(webview);
    });

    it('works when origin does not match opener but has node integration', (done) => {
      webview = new WebView();
      webview.addEventListener('console-message', (e) => {
        webview.remove();
        expect(e.message).to.equal(webview.src);
        done();
      });
      webview.setAttribute('allowpopups', 'on');
      webview.setAttribute('nodeintegration', 'on');
      webview.src = url.format({
        pathname: srcPath,
        protocol: scheme,
        query: {
          p: pageURL
        },
        slashes: true
      });
      document.body.appendChild(webview);
    });
  });

  describe('window.postMessage', () => {
    it('throws an exception when the targetOrigin cannot be converted to a string', () => {
      const b = window.open('');
      expect(() => {
        b.postMessage('test', { toString: null });
      }).to.throw('Cannot convert object to primitive value');
      b.close();
    });
  });

  describe('window.opener.postMessage', () => {
    it('sets source and origin correctly', (done) => {
      let b = null;
      listener = (event) => {
        window.removeEventListener('message', listener);
        b.close();
        expect(event.source).to.equal(b);
        expect(event.origin).to.equal('file://');
        done();
      };
      window.addEventListener('message', listener);
      b = window.open(`file://${fixtures}/pages/window-opener-postMessage.html`, '', 'show=no');
    });

    it('supports windows opened from a <webview>', (done) => {
      const webview = new WebView();
      webview.addEventListener('console-message', (e) => {
        webview.remove();
        expect(e.message).to.equal('message');
        done();
      });
      webview.allowpopups = true;
      webview.src = url.format({
        pathname: `${fixtures}/pages/webview-opener-postMessage.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-postMessage.html`
        },
        slashes: true
      });
      document.body.appendChild(webview);
    });

    describe('targetOrigin argument', () => {
      let serverURL;
      let server;

      beforeEach((done) => {
        server = http.createServer((req, res) => {
          res.writeHead(200);
          const filePath = path.join(fixtures, 'pages', 'window-opener-targetOrigin.html');
          res.end(fs.readFileSync(filePath, 'utf8'));
        });
        server.listen(0, '127.0.0.1', () => {
          serverURL = `http://127.0.0.1:${server.address().port}`;
          done();
        });
      });

      afterEach(() => {
        server.close();
      });

      it('delivers messages that match the origin', (done) => {
        let b = null;
        listener = (event) => {
          window.removeEventListener('message', listener);
          b.close();
          expect(event.data).to.equal('deliver');
          done();
        };
        window.addEventListener('message', listener);
        b = window.open(serverURL, '', 'show=no');
      });
    });
  });

  describe('creating a Uint8Array under browser side', () => {
    it('does not crash', () => {
      const RUint8Array = remote.getGlobal('Uint8Array');
      const arr = new RUint8Array();
    });
  });

  describe('webgl', () => {
    before(function () {
      if (isCI && process.platform === 'win32') {
        this.skip();
      }
    });

    it('can be get as context in canvas', () => {
      if (process.platform === 'linux') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return;
      }

      const webgl = document.createElement('canvas').getContext('webgl');
      expect(webgl).to.not.be.null();
    });
  });

  describe('web workers', () => {
    it('Worker can work', (done) => {
      const worker = new Worker('../fixtures/workers/worker.js');
      const message = 'ping';
      worker.onmessage = (event) => {
        expect(event.data).to.equal(message);
        worker.terminate();
        done();
      };
      worker.postMessage(message);
    });

    it('Worker has no node integration by default', (done) => {
      const worker = new Worker('../fixtures/workers/worker_node.js');
      worker.onmessage = (event) => {
        expect(event.data).to.equal('undefined undefined undefined undefined');
        worker.terminate();
        done();
      };
    });

    it('Worker has node integration with nodeIntegrationInWorker', (done) => {
      const webview = new WebView();
      webview.addEventListener('ipc-message', (e) => {
        expect(e.channel).to.equal('object function object function');
        webview.remove();
        done();
      });
      webview.src = `file://${fixtures}/pages/worker.html`;
      webview.setAttribute('webpreferences', 'nodeIntegration, nodeIntegrationInWorker');
      document.body.appendChild(webview);
    });

    // FIXME: disabled during chromium update due to crash in content::WorkerScriptFetchInitiator::CreateScriptLoaderOnIO
    xdescribe('SharedWorker', () => {
      it('can work', (done) => {
        const worker = new SharedWorker('../fixtures/workers/shared_worker.js');
        const message = 'ping';
        worker.port.onmessage = (event) => {
          expect(event.data).to.equal(message);
          done();
        };
        worker.port.postMessage(message);
      });

      it('has no node integration by default', (done) => {
        const worker = new SharedWorker('../fixtures/workers/shared_worker_node.js');
        worker.port.onmessage = (event) => {
          expect(event.data).to.equal('undefined undefined undefined undefined');
          done();
        };
      });

      it('has node integration with nodeIntegrationInWorker', (done) => {
        const webview = new WebView();
        webview.addEventListener('console-message', (e) => {
          console.log(e);
        });
        webview.addEventListener('ipc-message', (e) => {
          expect(e.channel).to.equal('object function object function');
          webview.remove();
          done();
        });
        webview.src = `file://${fixtures}/pages/shared_worker.html`;
        webview.setAttribute('webpreferences', 'nodeIntegration, nodeIntegrationInWorker');
        document.body.appendChild(webview);
      });
    });
  });

  describe('iframe', () => {
    let iframe = null;

    beforeEach(() => {
      iframe = document.createElement('iframe');
    });

    afterEach(() => {
      document.body.removeChild(iframe);
    });

    it('does not have node integration', (done) => {
      iframe.src = `file://${fixtures}/pages/set-global.html`;
      document.body.appendChild(iframe);
      iframe.onload = () => {
        expect(iframe.contentWindow.test).to.equal('undefined undefined undefined');
        done();
      };
    });
  });

  describe('storage', () => {
    describe('DOM storage quota override', () => {
      ['localStorage', 'sessionStorage'].forEach((storageName) => {
        it(`allows saving at least 50MiB in ${storageName}`, () => {
          const storage = window[storageName];
          const testKeyName = '_electronDOMStorageQuotaOverrideTest';
          // 25 * 2^20 UTF-16 characters will require 50MiB
          const arraySize = 25 * Math.pow(2, 20);
          storage[testKeyName] = new Array(arraySize).fill('X').join('');
          expect(storage[testKeyName]).to.have.lengthOf(arraySize);
          delete storage[testKeyName];
        });
      });
    });

    it('requesting persitent quota works', (done) => {
      navigator.webkitPersistentStorage.requestQuota(1024 * 1024, (grantedBytes) => {
        expect(grantedBytes).to.equal(1048576);
        done();
      });
    });

    describe('custom non standard schemes', () => {
      const protocolName = 'storage';
      let contents = null;
      before((done) => {
        const handler = (request, callback) => {
          const parsedUrl = url.parse(request.url);
          let filename;
          switch (parsedUrl.pathname) {
            case '/localStorage' : filename = 'local_storage.html'; break;
            case '/sessionStorage' : filename = 'session_storage.html'; break;
            case '/WebSQL' : filename = 'web_sql.html'; break;
            case '/indexedDB' : filename = 'indexed_db.html'; break;
            case '/cookie' : filename = 'cookie.html'; break;
            default : filename = '';
          }
          callback({ path: `${fixtures}/pages/storage/${filename}` });
        };
        protocol.registerFileProtocol(protocolName, handler, (error) => done(error));
      });

      after((done) => {
        protocol.unregisterProtocol(protocolName, () => done());
      });

      beforeEach(() => {
        contents = webContents.create({
          nodeIntegration: true
        });
      });

      afterEach(() => {
        contents.destroy();
        contents = null;
      });

      it('cannot access localStorage', (done) => {
        ipcMain.once('local-storage-response', (event, error) => {
          expect(error).to.equal(`Failed to read the 'localStorage' property from 'Window': Access is denied for this document.`);
          done();
        });
        contents.loadURL(protocolName + '://host/localStorage');
      });

      it('cannot access sessionStorage', (done) => {
        ipcMain.once('session-storage-response', (event, error) => {
          expect(error).to.equal(`Failed to read the 'sessionStorage' property from 'Window': Access is denied for this document.`);
          done();
        });
        contents.loadURL(`${protocolName}://host/sessionStorage`);
      });

      it('cannot access WebSQL database', (done) => {
        ipcMain.once('web-sql-response', (event, error) => {
          expect(error).to.equal(`Failed to execute 'openDatabase' on 'Window': Access to the WebDatabase API is denied in this context.`);
          done();
        });
        contents.loadURL(`${protocolName}://host/WebSQL`);
      });

      it('cannot access indexedDB', (done) => {
        ipcMain.once('indexed-db-response', (event, error) => {
          expect(error).to.equal(`Failed to execute 'open' on 'IDBFactory': access to the Indexed Database API is denied in this context.`);
          done();
        });
        contents.loadURL(`${protocolName}://host/indexedDB`);
      });

      it('cannot access cookie', (done) => {
        ipcMain.once('cookie-response', (event, error) => {
          expect(error).to.equal(`Failed to set the 'cookie' property on 'Document': Access is denied for this document.`);
          done();
        });
        contents.loadURL(`${protocolName}://host/cookie`);
      });
    });

    describe('can be accessed', () => {
      let server = null;
      before((done) => {
        server = http.createServer((req, res) => {
          const respond = () => {
            if (req.url === '/redirect-cross-site') {
              res.setHeader('Location', `${server.cross_site_url}/redirected`);
              res.statusCode = 302;
              res.end();
            } else if (req.url === '/redirected') {
              res.end('<html><script>window.localStorage</script></html>');
            } else {
              res.end();
            }
          };
          setTimeout(respond, 0);
        });
        server.listen(0, '127.0.0.1', () => {
          server.url = `http://127.0.0.1:${server.address().port}`;
          server.cross_site_url = `http://localhost:${server.address().port}`;
          done();
        });
      });

      after(() => {
        server.close();
        server = null;
      });

      const testLocalStorageAfterXSiteRedirect = (testTitle, extraPreferences = {}) => {
        it(testTitle, (done) => {
          w = new BrowserWindow({
            show: false,
            ...extraPreferences
          });
          let redirected = false;
          w.webContents.on('crashed', () => {
            expect.fail('renderer crashed / was killed');
          });
          w.webContents.on('did-redirect-navigation', (event, url) => {
            expect(url).to.equal(`${server.cross_site_url}/redirected`);
            redirected = true;
          });
          w.webContents.on('did-finish-load', () => {
            expect(redirected).to.be.true('didnt redirect');
            done();
          });
          w.loadURL(`${server.url}/redirect-cross-site`);
        });
      };

      testLocalStorageAfterXSiteRedirect('after a cross-site redirect');
      testLocalStorageAfterXSiteRedirect('after a cross-site redirect in sandbox mode', { sandbox: true });
    });
  });

  describe('websockets', () => {
    let wss = null;
    let server = null;
    const WebSocketServer = ws.Server;

    afterEach(() => {
      wss.close();
      server.close();
    });

    it('has user agent', (done) => {
      server = http.createServer();
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port;
        wss = new WebSocketServer({ server: server });
        wss.on('error', done);
        wss.on('connection', (ws, upgradeReq) => {
          if (upgradeReq.headers['user-agent']) {
            done();
          } else {
            done('user agent is empty');
          }
        });
        const socket = new WebSocket(`ws://127.0.0.1:${port}`);
      });
    });
  });

  describe('Promise', () => {
    it('resolves correctly in Node.js calls', (done) => {
      document.registerElement('x-element', {
        prototype: Object.create(HTMLElement.prototype, {
          createdCallback: {
            value: () => {}
          }
        })
      });
      setImmediate(() => {
        let called = false;
        Promise.resolve().then(() => {
          done(called ? void 0 : new Error('wrong sequence'));
        });
        document.createElement('x-element');
        called = true;
      });
    });

    it('resolves correctly in Electron calls', (done) => {
      document.registerElement('y-element', {
        prototype: Object.create(HTMLElement.prototype, {
          createdCallback: {
            value: () => {}
          }
        })
      });
      remote.getGlobal('setImmediate')(() => {
        let called = false;
        Promise.resolve().then(() => {
          done(called ? void 0 : new Error('wrong sequence'));
        });
        document.createElement('y-element');
        called = true;
      });
    });
  });

  describe('fetch', () => {
    it('does not crash', (done) => {
      const server = http.createServer((req, res) => {
        res.end('test');
        server.close();
      });
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port;
        fetch(`http://127.0.0.1:${port}`).then((res) => res.body.getReader())
          .then((reader) => {
            reader.read().then((r) => {
              reader.cancel();
              done();
            });
          }).catch((e) => done(e));
      });
    });
  });

  describe('PDF Viewer', () => {
    before(function () {
      if (!features.isPDFViewerEnabled()) {
        return this.skip();
      }
    });

    beforeEach(() => {
      this.pdfSource = url.format({
        pathname: path.join(fixtures, 'assets', 'cat.pdf').replace(/\\/g, '/'),
        protocol: 'file',
        slashes: true
      });

      this.pdfSourceWithParams = url.format({
        pathname: path.join(fixtures, 'assets', 'cat.pdf').replace(/\\/g, '/'),
        query: {
          a: 1,
          b: 2
        },
        protocol: 'file',
        slashes: true
      });

      this.createBrowserWindow = ({ plugins, preload }) => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: path.join(fixtures, 'module', preload),
            plugins: plugins
          }
        });
      };

      this.testPDFIsLoadedInSubFrame = (page, preloadFile, done) => {
        const pagePath = url.format({
          pathname: path.join(fixtures, 'pages', page).replace(/\\/g, '/'),
          protocol: 'file',
          slashes: true
        });

        this.createBrowserWindow({ plugins: true, preload: preloadFile });
        ipcMain.once('pdf-loaded', (event, state) => {
          expect(state).to.equal('success');
          done();
        });
        w.webContents.on('page-title-updated', () => {
          const parsedURL = url.parse(w.webContents.getURL(), true);
          expect(parsedURL.protocol).to.equal('chrome:');
          expect(parsedURL.hostname).to.equal('pdf-viewer');
          expect(parsedURL.query.src).to.equal(pagePath);
          expect(w.webContents.getTitle()).to.equal('cat.pdf');
        });
        w.loadFile(path.join(fixtures, 'pages', page));
      };
    });

    it('opens when loading a pdf resource as top level navigation', (done) => {
      this.createBrowserWindow({ plugins: true, preload: 'preload-pdf-loaded.js' });
      ipcMain.once('pdf-loaded', (event, state) => {
        expect(state).to.equal('success');
        done();
      });
      w.webContents.on('page-title-updated', () => {
        const parsedURL = url.parse(w.webContents.getURL(), true);
        expect(parsedURL.protocol).to.equal('chrome:');
        expect(parsedURL.hostname).to.equal('pdf-viewer');
        expect(parsedURL.query.src).to.equal(this.pdfSource);
        expect(w.webContents.getTitle()).to.equal('cat.pdf');
      });
      w.webContents.loadURL(this.pdfSource);
    });

    it('opens a pdf link given params, the query string should be escaped', (done) => {
      this.createBrowserWindow({ plugins: true, preload: 'preload-pdf-loaded.js' });
      ipcMain.once('pdf-loaded', (event, state) => {
        expect(state).to.equal('success');
        done();
      });
      w.webContents.on('page-title-updated', () => {
        const parsedURL = url.parse(w.webContents.getURL(), true);
        expect(parsedURL.protocol).to.equal('chrome:');
        expect(parsedURL.hostname).to.equal('pdf-viewer');
        expect(parsedURL.query.src).to.equal(this.pdfSourceWithParams);
        expect(parsedURL.query.b).to.be.undefined();
        expect(parsedURL.search.endsWith('%3Fa%3D1%26b%3D2')).to.be.true();
        expect(w.webContents.getTitle()).to.equal('cat.pdf');
      });
      w.webContents.loadURL(this.pdfSourceWithParams);
    });

    it('should download a pdf when plugins are disabled', (done) => {
      this.createBrowserWindow({ plugins: false, preload: 'preload-pdf-loaded.js' });
      // NOTE(nornagon): this test has been skipped for ages, so there's no way
      // to refactor it confidently. The 'set-download-option' ipc was removed
      // around May 2019, so if you're working on the pdf viewer and arrive at
      // this test and want to know what 'set-download-option' did, look here:
      // https://github.com/electron/electron/blob/d87b3ead760ae2d20f2401a8dac4ce548f8cd5f5/spec/static/main.js#L164
      ipcRenderer.sendSync('set-download-option', false, false);
      ipcRenderer.once('download-done', (event, state, url, mimeType, receivedBytes, totalBytes, disposition, filename) => {
        expect(state).to.equal('completed');
        expect(filename).to.equal('cat.pdf');
        expect(mimeType).to.equal('application/pdf');
        fs.unlinkSync(path.join(fixtures, 'mock.pdf'));
        done();
      });
      w.webContents.loadURL(this.pdfSource);
    });

    it('should not open when pdf is requested as sub resource', (done) => {
      fetch(this.pdfSource).then((res) => {
        expect(res.status).to.equal(200);
        expect(document.title).to.not.equal('cat.pdf');
        done();
      }).catch((e) => done(e));
    });

    it('opens when loading a pdf resource in a iframe', (done) => {
      this.testPDFIsLoadedInSubFrame('pdf-in-iframe.html', 'preload-pdf-loaded-in-subframe.js', done);
    });

    it('opens when loading a pdf resource in a nested iframe', (done) => {
      this.testPDFIsLoadedInSubFrame('pdf-in-nested-iframe.html', 'preload-pdf-loaded-in-nested-subframe.js', done);
    });
  });

  describe('window.alert(message, title)', () => {
    it('throws an exception when the arguments cannot be converted to strings', () => {
      expect(() => {
        window.alert({ toString: null });
      }).to.throw('Cannot convert object to primitive value');
    });
  });

  describe('window.confirm(message, title)', () => {
    it('throws an exception when the arguments cannot be converted to strings', () => {
      expect(() => {
        window.confirm({ toString: null }, 'title');
      }).to.throw('Cannot convert object to primitive value');
    });
  });

  describe('window.history', () => {
    describe('window.history.go(offset)', () => {
      it('throws an exception when the argumnet cannot be converted to a string', () => {
        expect(() => {
          window.history.go({ toString: null });
        }).to.throw('Cannot convert object to primitive value');
      });
    });

    describe('window.history.pushState', () => {
      it('should push state after calling history.pushState() from the same url', (done) => {
        w = new BrowserWindow({ show: false });
        w.webContents.once('did-finish-load', async () => {
          // History should have current page by now.
          expect(w.webContents.length()).to.equal(1);

          w.webContents.executeJavaScript('window.history.pushState({}, "")').then(() => {
            // Initial page + pushed state
            expect(w.webContents.length()).to.equal(2);
            done();
          });
        });
        w.loadURL('about:blank');
      });
    });
  });

  describe('SpeechSynthesis', () => {
    before(function () {
      if (isCI || !features.isTtsEnabled()) {
        this.skip();
      }
    });

    it('should emit lifecycle events', async () => {
      const sentence = `long sentence which will take at least a few seconds to
          utter so that it's possible to pause and resume before the end`;
      const utter = new SpeechSynthesisUtterance(sentence);
      // Create a dummy utterence so that speech synthesis state
      // is initialized for later calls.
      speechSynthesis.speak(new SpeechSynthesisUtterance());
      speechSynthesis.cancel();
      speechSynthesis.speak(utter);
      // paused state after speak()
      expect(speechSynthesis.paused).to.be.false();
      await new Promise((resolve) => { utter.onstart = resolve; });
      // paused state after start event
      expect(speechSynthesis.paused).to.be.false();

      speechSynthesis.pause();
      // paused state changes async, right before the pause event
      expect(speechSynthesis.paused).to.be.false();
      await new Promise((resolve) => { utter.onpause = resolve; });
      expect(speechSynthesis.paused).to.be.true();

      speechSynthesis.resume();
      await new Promise((resolve) => { utter.onresume = resolve; });
      // paused state after resume event
      expect(speechSynthesis.paused).to.be.false();

      await new Promise((resolve) => { utter.onend = resolve; });
    });
  });
});

describe('console functions', () => {
  it('should exist', () => {
    expect(console.log, 'log').to.be.a('function');
    expect(console.error, 'error').to.be.a('function');
    expect(console.warn, 'warn').to.be.a('function');
    expect(console.info, 'info').to.be.a('function');
    expect(console.debug, 'debug').to.be.a('function');
    expect(console.trace, 'trace').to.be.a('function');
    expect(console.time, 'time').to.be.a('function');
    expect(console.timeEnd, 'timeEnd').to.be.a('function');
  });
});

describe('font fallback', () => {
  async function getRenderedFonts (html) {
    const w = new BrowserWindow({ show: false });
    try {
      await w.loadURL(`data:text/html,${html}`);
      w.webContents.debugger.attach();
      const sendCommand = (...args) => w.webContents.debugger.sendCommand(...args);
      const { nodeId } = (await sendCommand('DOM.getDocument')).root.children[0];
      await sendCommand('CSS.enable');
      const { fonts } = await sendCommand('CSS.getPlatformFontsForNode', { nodeId });
      return fonts;
    } finally {
      w.close();
    }
  }

  it('should use Helvetica for sans-serif on Mac, and Arial on Windows and Linux', async () => {
    const html = `<body style="font-family: sans-serif">test</body>`;
    const fonts = await getRenderedFonts(html);
    expect(fonts).to.be.an('array');
    expect(fonts).to.have.length(1);
    expect(fonts[0].familyName).to.equal({
      'win32': 'Arial',
      'darwin': 'Helvetica',
      'linux': 'DejaVu Sans' // I think this depends on the distro? We don't specify a default.
    }[process.platform]);
  });

  it('should fall back to Japanese font for sans-serif Japanese script', async function () {
    if (process.platform === 'linux') {
      return this.skip();
    }
    const html = `
    <html lang="ja-JP">
      <head>
        <meta charset="utf-8" />
      </head>
      <body style="font-family: sans-serif">test 智史</body>
    </html>
    `;
    const fonts = await getRenderedFonts(html);
    expect(fonts).to.be.an('array');
    expect(fonts).to.have.length(1);
    expect(fonts[0].familyName).to.be.oneOf({
      'win32': ['Meiryo', 'Yu Gothic'],
      'darwin': ['Hiragino Kaku Gothic ProN']
    }[process.platform]);
  });
});
