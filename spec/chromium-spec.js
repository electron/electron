const { expect } = require('chai');
const fs = require('fs');
const http = require('http');
const path = require('path');
const ws = require('ws');
const url = require('url');
const ChildProcess = require('child_process');
const { ipcRenderer } = require('electron');
const { emittedOnce, waitForEvent } = require('./events-helpers');
const { resolveGetters } = require('./expect-helpers');
const { ifdescribe, delay } = require('./spec-helpers');
const features = process._linkedBinding('electron_common_features');

/* Most of the APIs here don't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('chromium feature', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');

  describe('Badging API', () => {
    it('does not crash', () => {
      expect(() => {
        navigator.setAppBadge(42);
      }).to.not.throw();
      expect(() => {
        // setAppBadge with no argument should show dot
        navigator.setAppBadge();
      }).to.not.throw();
      expect(() => {
        navigator.clearAppBadge();
      }).to.not.throw();
    });
  });

  describe('heap snapshot', () => {
    it('does not crash', function () {
      process._linkedBinding('electron_common_v8_util').takeHeapSnapshot();
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

  describe('navigator.language', () => {
    it('should not be empty', () => {
      expect(navigator.language).to.not.equal('');
    });
  });

  ifdescribe(features.isFakeLocationProviderEnabled())('navigator.geolocation', () => {
    it('returns position when permission is granted', async () => {
      const position = await new Promise((resolve, reject) => navigator.geolocation.getCurrentPosition(resolve, reject));
      expect(position).to.have.a.property('coords');
      expect(position).to.have.a.property('timestamp');
    });
  });

  describe('window.open', () => {
    it('accepts "nodeIntegration" as feature', async () => {
      const message = waitForEvent(window, 'message');
      const b = window.open(`file://${fixtures}/pages/window-opener-node.html`, '', 'nodeIntegration=no,show=no');
      const event = await message;
      b.close();
      expect(event.data.isProcessGlobalUndefined).to.be.true();
    });

    it('inherit options of parent window', async () => {
      const message = waitForEvent(window, 'message');
      const b = window.open(`file://${fixtures}/pages/window-open-size.html`, '', 'show=no');
      const event = await message;
      b.close();
      const width = outerWidth;
      const height = outerHeight;
      expect(event.data).to.equal(`size: ${width} ${height}`);
    });

    it('disables node integration when it is disabled on the parent window', async () => {
      const windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-opener-no-node-integration.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-node.html`
        },
        slashes: true
      });
      const message = waitForEvent(window, 'message');
      const b = window.open(windowUrl, '', 'nodeIntegration=no,contextIsolation=no,show=no');
      const event = await message;
      b.close();
      expect(event.data.isProcessGlobalUndefined).to.be.true();
    });

    it('disables the <webview> tag when it is disabled on the parent window', async () => {
      const windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-opener-no-webview-tag.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-webview.html`
        },
        slashes: true
      });
      const message = waitForEvent(window, 'message');
      const b = window.open(windowUrl, '', 'webviewTag=no,contextIsolation=no,nodeIntegration=yes,show=no');
      const event = await message;
      b.close();
      expect(event.data.isWebViewGlobalUndefined).to.be.true();
    });

    it('does not override child options', async () => {
      const size = {
        width: 350,
        height: 450
      };
      const message = waitForEvent(window, 'message');
      const b = window.open(`file://${fixtures}/pages/window-open-size.html`, '', 'show=no,width=' + size.width + ',height=' + size.height);
      const event = await message;
      b.close();
      expect(event.data).to.equal(`size: ${size.width} ${size.height}`);
    });

    it('throws an exception when the arguments cannot be converted to strings', () => {
      expect(() => {
        window.open('', { toString: null });
      }).to.throw('Cannot convert object to primitive value');

      expect(() => {
        window.open('', '', { toString: 3 });
      }).to.throw('Cannot convert object to primitive value');
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
    it('is not null for window opened by window.open', async () => {
      const message = waitForEvent(window, 'message');
      const b = window.open(`file://${fixtures}/pages/window-opener.html`, '', 'show=no');
      const event = await message;
      b.close();
      expect(event.data).to.equal('object');
    });
  });

  describe('window.opener.postMessage', () => {
    it('sets source and origin correctly', async () => {
      const message = waitForEvent(window, 'message');
      const b = window.open(`file://${fixtures}/pages/window-opener-postMessage.html`, '', 'show=no');
      const event = await message;
      try {
        expect(event.source).to.deep.equal(b);
        expect(event.origin).to.equal('file://');
      } finally {
        b.close();
      }
    });

    it('supports windows opened from a <webview>', async () => {
      const webview = new WebView();
      const consoleMessage = waitForEvent(webview, 'console-message');
      webview.allowpopups = true;
      webview.setAttribute('webpreferences', 'contextIsolation=no');
      webview.src = url.format({
        pathname: `${fixtures}/pages/webview-opener-postMessage.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-postMessage.html`
        },
        slashes: true
      });
      document.body.appendChild(webview);
      const event = await consoleMessage;
      webview.remove();
      expect(event.message).to.equal('message');
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

      it('delivers messages that match the origin', async () => {
        const message = waitForEvent(window, 'message');
        const b = window.open(serverURL, '', 'show=no,contextIsolation=no,nodeIntegration=yes');
        const event = await message;
        b.close();
        expect(event.data).to.equal('deliver');
      });
    });
  });

  describe('webgl', () => {
    before(function () {
      if (process.platform === 'win32') {
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
    it('Worker can work', async () => {
      const worker = new Worker('../fixtures/workers/worker.js');
      const message = 'ping';
      const eventPromise = new Promise((resolve) => { worker.onmessage = resolve; });
      worker.postMessage(message);
      const event = await eventPromise;
      worker.terminate();
      expect(event.data).to.equal(message);
    });

    it('Worker has no node integration by default', async () => {
      const worker = new Worker('../fixtures/workers/worker_node.js');
      const event = await new Promise((resolve) => { worker.onmessage = resolve; });
      worker.terminate();
      expect(event.data).to.equal('undefined undefined undefined undefined');
    });

    it('Worker has node integration with nodeIntegrationInWorker', async () => {
      const webview = new WebView();
      const eventPromise = waitForEvent(webview, 'ipc-message');
      webview.src = `file://${fixtures}/pages/worker.html`;
      webview.setAttribute('webpreferences', 'nodeIntegration, nodeIntegrationInWorker, contextIsolation=no');
      document.body.appendChild(webview);
      const event = await eventPromise;
      webview.remove();
      expect(event.channel).to.equal('object function object function');
    });

    describe('SharedWorker', () => {
      it('can work', async () => {
        const worker = new SharedWorker('../fixtures/workers/shared_worker.js');
        const message = 'ping';
        const eventPromise = new Promise((resolve) => { worker.port.onmessage = resolve; });
        worker.port.postMessage(message);
        const event = await eventPromise;
        expect(event.data).to.equal(message);
      });

      it('has no node integration by default', async () => {
        const worker = new SharedWorker('../fixtures/workers/shared_worker_node.js');
        const event = await new Promise((resolve) => { worker.port.onmessage = resolve; });
        expect(event.data).to.equal('undefined undefined undefined undefined');
      });

      // FIXME: disabled during chromium update due to crash in content::WorkerScriptFetchInitiator::CreateScriptLoaderOnIO
      xit('has node integration with nodeIntegrationInWorker', async () => {
        const webview = new WebView();
        webview.addEventListener('console-message', (e) => {
          console.log(e);
        });
        const eventPromise = waitForEvent(webview, 'ipc-message');
        webview.src = `file://${fixtures}/pages/shared_worker.html`;
        webview.setAttribute('webpreferences', 'nodeIntegration, nodeIntegrationInWorker');
        document.body.appendChild(webview);
        const event = await eventPromise;
        webview.remove();
        expect(event.channel).to.equal('object function object function');
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

    it('does not have node integration', async () => {
      iframe.src = `file://${fixtures}/pages/set-global.html`;
      document.body.appendChild(iframe);
      await waitForEvent(iframe, 'load');
      expect(iframe.contentWindow.test).to.equal('undefined undefined undefined');
    });
  });

  describe('storage', () => {
    describe('DOM storage quota increase', () => {
      ['localStorage', 'sessionStorage'].forEach((storageName) => {
        const storage = window[storageName];
        it(`allows saving at least 40MiB in ${storageName}`, async () => {
          // Although JavaScript strings use UTF-16, the underlying
          // storage provider may encode strings differently, muddling the
          // translation between character and byte counts. However,
          // a string of 40 * 2^20 characters will require at least 40MiB
          // and presumably no more than 80MiB, a size guaranteed to
          // to exceed the original 10MiB quota yet stay within the
          // new 100MiB quota.
          // Note that both the key name and value affect the total size.
          const testKeyName = '_electronDOMStorageQuotaIncreasedTest';
          const length = 40 * Math.pow(2, 20) - testKeyName.length;
          storage.setItem(testKeyName, 'X'.repeat(length));
          // Wait at least one turn of the event loop to help avoid false positives
          // Although not entirely necessary, the previous version of this test case
          // failed to detect a real problem (perhaps related to DOM storage data caching)
          // wherein calling `getItem` immediately after `setItem` would appear to work
          // but then later (e.g. next tick) it would not.
          await delay(1);
          try {
            expect(storage.getItem(testKeyName)).to.have.lengthOf(length);
          } finally {
            storage.removeItem(testKeyName);
          }
        });
        it(`throws when attempting to use more than 128MiB in ${storageName}`, () => {
          expect(() => {
            const testKeyName = '_electronDOMStorageQuotaStillEnforcedTest';
            const length = 128 * Math.pow(2, 20) - testKeyName.length;
            try {
              storage.setItem(testKeyName, 'X'.repeat(length));
            } finally {
              storage.removeItem(testKeyName);
            }
          }).to.throw();
        });
      });
    });

    it('requesting persitent quota works', async () => {
      const grantedBytes = await new Promise(resolve => {
        navigator.webkitPersistentStorage.requestQuota(1024 * 1024, resolve);
      });
      expect(grantedBytes).to.equal(1048576);
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
      class XElement extends HTMLElement {}
      customElements.define('x-element', XElement);
      setImmediate(() => {
        let called = false;
        Promise.resolve().then(() => {
          done(called ? undefined : new Error('wrong sequence'));
        });
        document.createElement('x-element');
        called = true;
      });
    });

    it('resolves correctly in Electron calls', (done) => {
      class YElement extends HTMLElement {}
      customElements.define('y-element', YElement);
      ipcRenderer.invoke('ping').then(() => {
        let called = false;
        Promise.resolve().then(() => {
          done(called ? undefined : new Error('wrong sequence'));
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
  });

  // TODO(nornagon): this is broken on CI, it triggers:
  // [FATAL:speech_synthesis.mojom-shared.h(237)] The outgoing message will
  // trigger VALIDATION_ERROR_UNEXPECTED_NULL_POINTER at the receiving side
  // (null text in SpeechSynthesisUtterance struct).
  describe.skip('SpeechSynthesis', () => {
    before(function () {
      if (!features.isTtsEnabled()) {
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
