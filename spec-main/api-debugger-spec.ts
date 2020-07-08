import { expect } from 'chai';
import * as http from 'http';
import * as path from 'path';
import { AddressInfo } from 'net';
import { BrowserWindow } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import { emittedOnce, emittedUntil } from './events-helpers';

describe('debugger module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures');
  let w: BrowserWindow;

  beforeEach(() => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400
    });
  });

  afterEach(closeAllWindows);

  describe('debugger.attach', () => {
    it('succeeds when devtools is already open', async () => {
      await w.webContents.loadURL('about:blank');
      w.webContents.openDevTools();
      w.webContents.debugger.attach();
      expect(w.webContents.debugger.isAttached()).to.be.true();
    });

    it('fails when protocol version is not supported', done => {
      try {
        w.webContents.debugger.attach('2.0');
      } catch (err) {
        expect(w.webContents.debugger.isAttached()).to.be.false();
        done();
      }
    });

    it('attaches when no protocol version is specified', async () => {
      w.webContents.debugger.attach();
      expect(w.webContents.debugger.isAttached()).to.be.true();
    });
  });

  describe('debugger.detach', () => {
    it('fires detach event', async () => {
      const detach = emittedOnce(w.webContents.debugger, 'detach');
      w.webContents.debugger.attach();
      w.webContents.debugger.detach();
      const [, reason] = await detach;
      expect(reason).to.equal('target closed');
      expect(w.webContents.debugger.isAttached()).to.be.false();
    });

    it('doesn\'t disconnect an active devtools session', async () => {
      w.webContents.loadURL('about:blank');
      const detach = emittedOnce(w.webContents.debugger, 'detach');
      w.webContents.debugger.attach();
      w.webContents.openDevTools();
      w.webContents.once('devtools-opened', () => {
        w.webContents.debugger.detach();
      });
      await detach;
      expect(w.webContents.debugger.isAttached()).to.be.false();
      expect((w as any).devToolsWebContents.isDestroyed()).to.be.false();
    });
  });

  describe('debugger.sendCommand', () => {
    let server: http.Server;

    afterEach(() => {
      if (server != null) {
        server.close();
        server = null as any;
      }
    });

    it('returns response', async () => {
      w.webContents.loadURL('about:blank');
      w.webContents.debugger.attach();

      const params = { expression: '4+2' };
      const res = await w.webContents.debugger.sendCommand('Runtime.evaluate', params);

      expect(res.wasThrown).to.be.undefined();
      expect(res.result.value).to.equal(6);

      w.webContents.debugger.detach();
    });

    it('returns response when devtools is opened', async () => {
      w.webContents.loadURL('about:blank');
      w.webContents.debugger.attach();

      const opened = emittedOnce(w.webContents, 'devtools-opened');
      w.webContents.openDevTools();
      await opened;

      const params = { expression: '4+2' };
      const res = await w.webContents.debugger.sendCommand('Runtime.evaluate', params);

      expect(res.wasThrown).to.be.undefined();
      expect(res.result.value).to.equal(6);

      w.webContents.debugger.detach();
    });

    it('fires message event', async () => {
      const url = process.platform !== 'win32'
        ? `file://${path.join(fixtures, 'pages', 'a.html')}`
        : `file:///${path.join(fixtures, 'pages', 'a.html').replace(/\\/g, '/')}`;
      w.webContents.loadURL(url);
      w.webContents.debugger.attach();
      const message = emittedUntil(w.webContents.debugger, 'message',
        (event: Electron.Event, method: string) => method === 'Console.messageAdded');
      w.webContents.debugger.sendCommand('Console.enable');
      const [,, params] = await message;
      w.webContents.debugger.detach();
      expect((params as any).message.level).to.equal('log');
      expect((params as any).message.url).to.equal(url);
      expect((params as any).message.text).to.equal('a');
    });

    it('returns error message when command fails', async () => {
      w.webContents.loadURL('about:blank');
      w.webContents.debugger.attach();

      const promise = w.webContents.debugger.sendCommand('Test');
      await expect(promise).to.be.eventually.rejectedWith(Error, "'Test' wasn't found");

      w.webContents.debugger.detach();
    });

    // TODO(deepak1556): Fix and enable with upgrade
    it.skip('handles valid unicode characters in message', (done) => {
      try {
        w.webContents.debugger.attach();
      } catch (err) {
        done(`unexpected error : ${err}`);
      }

      let requestId : number;
      w.webContents.debugger.on('message', (event, method, params) => {
        if (method === 'Network.responseReceived' &&
            params.response.url.startsWith('http://127.0.0.1')) {
          requestId = params.requestId;
        } else if (method === 'Network.loadingFinished' &&
                   params.requestId === requestId) {
          w.webContents.debugger.sendCommand('Network.getResponseBody', {
            requestId: params.requestId
          }).then(data => {
            expect(data.body).to.equal('\u0024');
            done();
          }).catch(result => done(result));
        }
      });

      server = http.createServer((req, res) => {
        res.setHeader('Content-Type', 'text/plain; charset=utf-8');
        res.end('\u0024');
      });

      server.listen(0, '127.0.0.1', () => {
        w.webContents.debugger.sendCommand('Network.enable');
        w.loadURL(`http://127.0.0.1:${(server.address() as AddressInfo).port}`);
      });
    });

    it('does not crash for invalid unicode characters in message', (done) => {
      try {
        w.webContents.debugger.attach();
      } catch (err) {
        done(`unexpected error : ${err}`);
      }

      w.webContents.debugger.on('message', (event, method) => {
        // loadingFinished indicates that page has been loaded and it did not
        // crash because of invalid UTF-8 data
        if (method === 'Network.loadingFinished') {
          done();
        }
      });

      server = http.createServer((req, res) => {
        res.setHeader('Content-Type', 'text/plain; charset=utf-8');
        res.end('\uFFFF');
      });

      server.listen(0, '127.0.0.1', () => {
        w.webContents.debugger.sendCommand('Network.enable');
        w.loadURL(`http://127.0.0.1:${(server.address() as AddressInfo).port}`);
      });
    });

    it('uses empty sessionId by default', async () => {
      w.webContents.loadURL('about:blank');
      w.webContents.debugger.attach();
      const onMessage = emittedOnce(w.webContents.debugger, 'message');
      await w.webContents.debugger.sendCommand('Target.setDiscoverTargets', { discover: true });
      const [, method, params, sessionId] = await onMessage;
      expect(method).to.equal('Target.targetCreated');
      expect(params.targetInfo.targetId).to.not.be.empty();
      expect(sessionId).to.be.empty();
      w.webContents.debugger.detach();
    });

    it('creates unique session id for each target', (done) => {
      w.webContents.loadFile(path.join(__dirname, 'fixtures', 'sub-frames', 'debug-frames.html'));
      w.webContents.debugger.attach();
      let session: String;

      w.webContents.debugger.on('message', (event, ...args) => {
        const [method, params, sessionId] = args;
        if (method === 'Target.targetCreated') {
          w.webContents.debugger.sendCommand('Target.attachToTarget', { targetId: params.targetInfo.targetId, flatten: true }).then(result => {
            session = result.sessionId;
            w.webContents.debugger.sendCommand('Debugger.enable', {}, result.sessionId);
          });
        }
        if (method === 'Debugger.scriptParsed') {
          expect(sessionId).to.equal(session);
          w.webContents.debugger.detach();
          done();
        }
      });
      w.webContents.debugger.sendCommand('Target.setDiscoverTargets', { discover: true });
    });
  });
});
