const { expect } = require('chai');
const { remote } = require('electron');
const path = require('path');
const http = require('http');

const { emittedNTimes, emittedOnce } = require('./events-helpers');
const { closeWindow } = require('./window-helpers');

const { app, BrowserWindow, ipcMain } = remote;

describe('renderer nodeIntegrationInSubFrames', () => {
  const generateTests = (description, webPreferences) => {
    describe(description, () => {
      const fixtureSuffix = webPreferences.webviewTag ? '-webview' : '';
      let w;

      beforeEach(async () => {
        await closeWindow(w);
        w = new BrowserWindow({
          show: false,
          width: 400,
          height: 400,
          webPreferences
        });
      });

      afterEach(() => {
        return closeWindow(w).then(() => {
          w = null;
        });
      });

      it('should load preload scripts in top level iframes', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [event1, event2] = await detailsPromise;
        expect(event1[0].frameId).to.not.equal(event2[0].frameId);
        expect(event1[0].frameId).to.equal(event1[2]);
        expect(event2[0].frameId).to.equal(event2[2]);
      });

      it('should load preload scripts in nested iframes', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-with-frame-container${fixtureSuffix}.html`));
        const [event1, event2, event3] = await detailsPromise;
        expect(event1[0].frameId).to.not.equal(event2[0].frameId);
        expect(event1[0].frameId).to.not.equal(event3[0].frameId);
        expect(event2[0].frameId).to.not.equal(event3[0].frameId);
        expect(event1[0].frameId).to.equal(event1[2]);
        expect(event2[0].frameId).to.equal(event2[2]);
        expect(event3[0].frameId).to.equal(event3[2]);
      });

      it('should correctly reply to the main frame with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [event1] = await detailsPromise;
        const pongPromise = emittedOnce(ipcMain, 'preload-pong');
        event1[0].reply('preload-ping');
        const details = await pongPromise;
        expect(details[1]).to.equal(event1[0].frameId);
      });

      it('should correctly reply to the sub-frames with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [, event2] = await detailsPromise;
        const pongPromise = emittedOnce(ipcMain, 'preload-pong');
        event2[0].reply('preload-ping');
        const details = await pongPromise;
        expect(details[1]).to.equal(event2[0].frameId);
      });

      it('should correctly reply to the nested sub-frames with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-with-frame-container${fixtureSuffix}.html`));
        const [, , event3] = await detailsPromise;
        const pongPromise = emittedOnce(ipcMain, 'preload-pong');
        event3[0].reply('preload-ping');
        const details = await pongPromise;
        expect(details[1]).to.equal(event3[0].frameId);
      });

      it('should not expose globals in main world', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const details = await detailsPromise;
        const senders = details.map(event => event[0].sender);
        await new Promise(async resolve => {
          let resultCount = 0;
          senders.forEach(async sender => {
            const result = await sender.webContents.executeJavaScript('window.isolatedGlobal');
            if (webPreferences.contextIsolation) {
              expect(result).to.be.null();
            } else {
              expect(result).to.equal(true);
            }
            resultCount++;
            if (resultCount === senders.length) resolve();
          });
        });
      });
    });
  };

  const generateConfigs = (webPreferences, ...permutations) => {
    const configs = [{ webPreferences, names: [] }];
    for (let i = 0; i < permutations.length; i++) {
      const length = configs.length;
      for (let j = 0; j < length; j++) {
        const newConfig = Object.assign({}, configs[j]);
        newConfig.webPreferences = Object.assign({},
          newConfig.webPreferences, permutations[i].webPreferences);
        newConfig.names = newConfig.names.slice(0);
        newConfig.names.push(permutations[i].name);
        configs.push(newConfig);
      }
    }

    return configs.map(config => {
      if (config.names.length > 0) {
        config.title = `with ${config.names.join(', ')} on`;
      } else {
        config.title = `without anything special turned on`;
      }
      delete config.names;

      return config;
    });
  };

  generateConfigs(
    {
      preload: path.resolve(__dirname, 'fixtures/sub-frames/preload.js'),
      nodeIntegrationInSubFrames: true
    },
    {
      name: 'sandbox',
      webPreferences: { sandbox: true }
    },
    {
      name: 'context isolation',
      webPreferences: { contextIsolation: true }
    },
    {
      name: 'webview',
      webPreferences: { webviewTag: true, preload: false }
    }
  ).forEach(config => {
    generateTests(config.title, config.webPreferences);
  });

  describe('internal <iframe> inside of <webview>', () => {
    let w;

    beforeEach(async () => {
      await closeWindow(w);
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          preload: path.resolve(__dirname, 'fixtures/sub-frames/webview-iframe-preload.js'),
          nodeIntegrationInSubFrames: true,
          webviewTag: true
        }
      });
    });

    afterEach(() => {
      return closeWindow(w).then(() => {
        w = null;
      });
    });

    it('should not load preload scripts', async () => {
      const promisePass = emittedOnce(ipcMain, 'webview-loaded');
      const promiseFail = emittedOnce(ipcMain, 'preload-in-frame').then(() => {
        throw new Error('preload loaded in internal frame');
      });
      await w.loadURL('about:blank');
      return Promise.race([promisePass, promiseFail]);
    });
  });
});

describe('cross-site frame sandboxing', () => {
  let server = null;

  beforeEach(function () {
    if (process.platform === 'linux') {
      this.skip();
    }
  });

  before(function (done) {
    server = http.createServer((req, res) => {
      res.end(`<iframe name="frame" src="${server.cross_site_url}" />`);
    });
    server.listen(0, '127.0.0.1', () => {
      server.url = `http://127.0.0.1:${server.address().port}/`;
      server.cross_site_url = `http://localhost:${server.address().port}/`;
      done();
    });
  });

  after(() => {
    server.close();
    server = null;
  });

  let w;

  afterEach(() => {
    return closeWindow(w).then(() => {
      w = null;
    });
  });

  const generateSpecs = (description, webPreferences) => {
    describe(description, () => {
      it('iframe process is sandboxed if possible', async () => {
        w = new BrowserWindow({
          show: false,
          webPreferences
        });

        await w.loadURL(server.url);

        const pidMain = w.webContents.getOSProcessId();
        const pidFrame = w.webContents._getOSProcessIdForFrame('frame', server.cross_site_url);

        const metrics = app.getAppMetrics();
        const isProcessSandboxed = function (pid) {
          const entry = metrics.filter(metric => metric.pid === pid)[0];
          return entry && entry.sandboxed;
        };

        const sandboxMain = !!(webPreferences.sandbox || process.mas);
        const sandboxFrame = sandboxMain || !webPreferences.nodeIntegrationInSubFrames;

        expect(isProcessSandboxed(pidMain)).to.equal(sandboxMain);
        expect(isProcessSandboxed(pidFrame)).to.equal(sandboxFrame);
      });
    });
  };

  generateSpecs('nodeIntegrationInSubFrames = false, sandbox = false', {
    nodeIntegrationInSubFrames: false,
    sandbox: false
  });

  generateSpecs('nodeIntegrationInSubFrames = false, sandbox = true', {
    nodeIntegrationInSubFrames: false,
    sandbox: true
  });

  generateSpecs('nodeIntegrationInSubFrames = true, sandbox = false', {
    nodeIntegrationInSubFrames: true,
    sandbox: false
  });

  generateSpecs('nodeIntegrationInSubFrames = true, sandbox = true', {
    nodeIntegrationInSubFrames: true,
    sandbox: true
  });
});
