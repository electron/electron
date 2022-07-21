import { expect } from 'chai';
import { BrowserWindow, session, desktopCapturer } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import * as http from 'http';
import { ifdescribe } from './spec-helpers';

const features = process._linkedBinding('electron_common_features');

ifdescribe(features.isDesktopCapturerEnabled())('setDisplayMediaRequestHandler', () => {
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
    let mediaRequest: any = null;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      mediaRequest = request;
      desktopCapturer.getSources({ types: ['screen'] }).then((sources) => {
        // Grant access to the first screen found.
        const { id, name } = sources[0];
        callback({
          video: { id, name }
          // TODO: 'loopback' and 'loopbackWithMute' are currently only supported on Windows.
          // audio: { id: 'loopback', name: 'System Audio' }
        });
      });
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(`
      navigator.mediaDevices.getDisplayMedia({
        video: true,
        audio: false,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `);
    expect(requestHandlerCalled).to.be.true();
    expect(mediaRequest.videoRequested).to.be.true();
    expect(mediaRequest.audioRequested).to.be.false();
    expect(ok).to.be.true(message);
  });

  it('does not crash when using a bogus ID', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      callback({
        video: { id: 'bogus', name: 'whatever' }
      });
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
    expect(ok).to.be.false();
    expect(message).to.equal('Could not start video source');
  });

  it('is not called when calling getUserMedia', async () => {
    const ses = session.fromPartition('' + Math.random());
    ses.setDisplayMediaRequestHandler(() => {
      throw new Error('bad');
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(`
      navigator.mediaDevices.getUserMedia({
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `);
    expect(ok).to.be.true(message);
  });

  it('works when calling getDisplayMedia with preferCurrentTab', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      callback(
        {
          video: {
            id: `web-contents-media-stream://${w.webContents.mainFrame.processId}:${w.webContents.mainFrame.routingId}`,
            name: 'self'
          }
        }
      );
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(`
      navigator.mediaDevices.getDisplayMedia({
        preferCurrentTab: true,
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `);
    expect(requestHandlerCalled).to.be.true();
    expect(ok).to.be.true(message);
  });

  it('is not called when calling legacy getUserMedia', async () => {
    const ses = session.fromPartition('' + Math.random());
    ses.setDisplayMediaRequestHandler(() => {
      throw new Error('bad');
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => navigator.getUserMedia({
        video: true,
        audio: true,
      }, x => resolve({ok: x instanceof MediaStream}), e => reject({ok: false, message: e.message})))
    `);
    expect(ok).to.be.true(message);
  });

  it('is not called when calling legacy getUserMedia with desktop capture constraint', async () => {
    const ses = session.fromPartition('' + Math.random());
    ses.setDisplayMediaRequestHandler(() => {
      throw new Error('bad');
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => navigator.getUserMedia({
        video: {
          mandatory: {
            chromeMediaSource: 'desktop'
          }
        },
      }, x => resolve({ok: x instanceof MediaStream}), e => reject({ok: false, message: e.message})))
    `);
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

  it('works when calling legacy getUserMedia without a media request handler', async () => {
    const w = new BrowserWindow({ show: false });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => navigator.getUserMedia({
        video: true,
        audio: true,
      }, x => resolve({ok: x instanceof MediaStream}), e => reject({ok: false, message: e.message})))
    `);
    expect(ok).to.be.true(message);
  });
});
