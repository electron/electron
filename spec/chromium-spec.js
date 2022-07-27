const { expect } = require('chai');
const fs = require('fs');
const http = require('http');
const path = require('path');
const url = require('url');
const ChildProcess = require('child_process');
const { ipcRenderer } = require('electron');
const { emittedOnce, waitForEvent } = require('./events-helpers');
const { ifit, ifdescribe, delay } = require('./spec-helpers');
const features = process._linkedBinding('electron_common_features');

/* Most of the APIs here don't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('chromium feature', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');

  describe('window.open', () => {
    it('inherit options of parent window', async () => {
      const message = waitForEvent(window, 'message');
      const b = window.open(`file://${fixtures}/pages/window-open-size.html`, '', 'show=no');
      const event = await message;
      b.close();
      const width = outerWidth;
      const height = outerHeight;
      expect(event.data).to.equal(`size: ${width} ${height}`);
    });

    // FIXME(zcbenz): This test is making the spec runner hang on exit on Windows.
    ifit(process.platform !== 'win32')('disables node integration when it is disabled on the parent window', async () => {
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

    it('requesting persistent quota works', async () => {
      const grantedBytes = await new Promise(resolve => {
        navigator.webkitPersistentStorage.requestQuota(1024 * 1024, resolve);
      });
      expect(grantedBytes).to.equal(1048576);
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
      it('throws an exception when the argument cannot be converted to a string', () => {
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
      // Create a dummy utterance so that speech synthesis state
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
