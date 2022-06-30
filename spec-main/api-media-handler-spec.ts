import { expect } from 'chai';
import { BrowserWindow, session, desktopCapturer } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import * as http from 'http';
import { ifdescribe } from './spec-helpers';

const features = process._linkedBinding('electron_common_features');

ifdescribe(features.isDesktopCapturerEnabled())('setMediaRequestHandler', () => {
  afterEach(closeAllWindows);
  // These tests are done on an http server because navigator.userAgentData
  // requires a secure context.
  let server: http.Server;
  let serverUrl: string;
  before(async () => {
    server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html');
      res.end('');
    });
    await new Promise<void>(resolve => server.listen(0, '127.0.0.1', resolve));
    serverUrl = `http://localhost:${(server.address() as any).port}`;
  });
  after(() => {
    server.close();
  });

  it('works when calling getDisplayMedia', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    ses.setMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      if (request.videoType === 'displayVideoCapture') {
        desktopCapturer.getSources({ types: ['screen'] }).then((sources) => {
          // Grant access to the first screen found.
          const { id, name } = sources[0];
          callback([{
            videoDevice: { id, name, type: request.videoType }
            // TODO: 'loopback' and 'loopbackWithMute' are currently only supported on Windows.
            // audioDevice: { id: 'loopback', name: 'System Audio', type: request.audioType }
          }], 'ok');
        });
      }
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(`
      navigator.mediaDevices.getDisplayMedia({
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `);
    expect(requestHandlerCalled).to.be.true();
    expect(ok).to.be.true(message);
  });

  it('works when calling getUserMedia', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    ses.setMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      const devices = (process._linkedBinding('electron_browser_desktop_capturer') as any).getVideoCaptureDevices();
      callback([
        { videoDevice: devices[0] }
      ], 'ok');
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(`
      navigator.mediaDevices.getUserMedia({
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `);
    expect(requestHandlerCalled).to.be.true();
    expect(ok).to.be.true(message);
  });

  it('works when calling getUserMedia without a media request handler', async () => {
    const w = new BrowserWindow({ show: false });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(`
      navigator.mediaDevices.getUserMedia({
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `);
    expect(ok).to.be.true(message);
  });
});
