import { BrowserWindow, session, desktopCapturer } from 'electron/main';

import { afterAll, afterEach, beforeAll, describe, expect, it } from 'vitest';

import * as http from 'node:http';

import { captureWithTabSourceId } from './lib/media-helpers.ts';
import { ifit, listen, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

describe('setDisplayMediaRequestHandler', () => {
  afterEach(closeAllWindows);
  // These tests are done on an http server because navigator.userAgentData
  // requires a secure context.
  let server: http.Server;
  let serverUrl: string;
  beforeAll(async () => {
    server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html');
      res.end('');
    });
    serverUrl = (await listen(server)).url;
  });
  afterAll(() => {
    server.close();
  });

  ifit(process.platform !== 'darwin')('works when calling getDisplayMedia', async (ctx) => {
    if ((await desktopCapturer.getSources({ types: ['screen'] })).length === 0) {
      return ctx.skip();
    }
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
    const { ok, message } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        video: true,
        audio: false,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(mediaRequest.videoRequested).toBe(true);
    expect(mediaRequest.audioRequested).toBe(false);
    expect(ok, message).toBe(true);
  });

  // Process-level loopback audio capture (restrictOwnAudio / loopbackWithoutChrome)
  // is not supported on Linux audio backends (PulseAudio / PipeWire).
  ifit(process.platform !== 'linux')(
    'honors the restrictOwnAudio constraint when granted loopback audio',
    async (ctx) => {
      if ((await desktopCapturer.getSources({ types: ['screen'] })).length === 0) {
        return ctx.skip();
      }
      const ses = session.fromPartition('' + Math.random());
      let requestHandlerCalled = false;
      ses.setDisplayMediaRequestHandler((request, callback) => {
        requestHandlerCalled = true;
        desktopCapturer.getSources({ types: ['screen'] }).then((sources) => {
          callback({ video: sources[0], audio: 'loopback' });
        });
      });
      const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
      await w.loadURL(serverUrl);
      const { ok, message, audioTrackCount, audioDeviceId, restrictOwnAudioSetting } =
        await w.webContents.executeJavaScript(
          `
        navigator.mediaDevices.getDisplayMedia({
          video: true,
          audio: { restrictOwnAudio: true },
        }).then(stream => {
          const audioTracks = stream.getAudioTracks();
          const audioSettings = audioTracks[0]?.getSettings();
          return {
            ok: stream instanceof MediaStream,
            message: null,
            audioTrackCount: audioTracks.length,
            audioDeviceId: audioSettings?.deviceId ?? null,
            restrictOwnAudioSetting: audioSettings?.restrictOwnAudio ?? null
          };
        }, e => ({ ok: false, message: e.message, audioTrackCount: 0, audioDeviceId: null, restrictOwnAudioSetting: null }))
      `,
          true
        );
      expect(requestHandlerCalled).toBe(true);
      expect(ok, message).toBe(true);
      expect(audioTrackCount).toBe(1);
      expect(audioDeviceId).toBe('loopbackWithoutChrome');
      expect(restrictOwnAudioSetting).toBe(true);
    }
  );

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
    const { ok, message } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(ok).toBe(false);
    expect(message).toBe('Could not start video source');
  });

  it('successfully returns a capture handle', async () => {
    let w: BrowserWindow | null = null;
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    let mediaRequest: any = null;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      mediaRequest = request;
      callback({ video: w?.webContents.mainFrame });
    });

    w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);

    const { ok, handleID, captureHandle, message } = await w.webContents.executeJavaScript(
      `
      const handleID = crypto.randomUUID();
      navigator.mediaDevices.setCaptureHandleConfig({
        handle: handleID,
        exposeOrigin: true,
        permittedOrigins: ["*"],
      });

      navigator.mediaDevices.getDisplayMedia({
        video: true,
        audio: false
      }).then(stream => {
        const [videoTrack] = stream.getVideoTracks();
        const captureHandle = videoTrack.getCaptureHandle();
        return { ok: true, handleID, captureHandle, message: null }
      }, e => ({ ok: false, message: e.message }))
    `,
      true
    );

    expect(requestHandlerCalled).toBe(true);
    expect(mediaRequest.videoRequested).toBe(true);
    expect(mediaRequest.audioRequested).toBe(false);
    expect(ok).toBe(true);
    expect(captureHandle.handle).toBeTypeOf('string');
    expect(handleID).toBe(captureHandle.handle);
    expect(message).toBeNull();
  });

  const addIframe = async (w: BrowserWindow) => {
    await w.webContents.executeJavaScript(`new Promise((resolve) => {
      const f = document.createElement('iframe');
      f.allow = 'display-capture';
      f.src = ${JSON.stringify(serverUrl)};
      f.onload = resolve;
      document.body.appendChild(f);
    })`);
    return w.webContents.mainFrame.frames[0];
  };

  it('does not crash when the requesting frame is gone before the callback', async () => {
    const ses = session.fromPartition('' + Math.random());
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    let pending: { request: any; callback: (streams: any) => void } | undefined;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      pending = { request, callback };
    });
    const iframe = await addIframe(w);
    iframe.executeJavaScript('navigator.mediaDevices.getDisplayMedia({ video: true }).catch(() => {}); true', true);
    await waitUntil(() => !!pending);
    expect(pending!.request.frame).toBe(iframe);
    await w.webContents.executeJavaScript("document.querySelector('iframe').remove()");
    await waitUntil(() => iframe.isDestroyed());
    pending!.callback({ video: pending!.request.frame });
    expect(await w.webContents.executeJavaScript('1 + 1')).toBe(2);
  });

  it('throws instead of crashing when the granted frame has been destroyed', async () => {
    const ses = session.fromPartition('' + Math.random());
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const iframe = await addIframe(w);
    await w.webContents.executeJavaScript("document.querySelector('iframe').remove()");
    await waitUntil(() => iframe.isDestroyed());
    let error: unknown;
    ses.setDisplayMediaRequestHandler((_request, callback) => {
      try {
        callback({ video: iframe });
      } catch (e) {
        error = e;
      }
    });
    const { ok } = await w.webContents.executeJavaScript(
      'navigator.mediaDevices.getDisplayMedia({ video: true }).then(() => ({ ok: true }), (e) => ({ ok: false, message: e.message }))',
      true
    );
    expect(ok).toBe(false);
    expect(error).toHaveProperty('message', expect.stringMatching(/destroyed/));
  });

  it('returns a capture handle when another webContents is granted by id', async () => {
    // Capture handles are withheld across in-memory (incognito-like) sessions
    // unless a tab captures itself, so use a persistent partition here.
    const ses = session.fromPartition('persist:display-media-' + Math.random());
    const captured = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await captured.loadURL(serverUrl);
    await captured.webContents.executeJavaScript(`
      navigator.mediaDevices.setCaptureHandleConfig({
        handle: 'captured-tab', exposeOrigin: true, permittedOrigins: ['*']
      }); true`);
    const capturer = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await capturer.loadURL(serverUrl);
    ses.setDisplayMediaRequestHandler((_request, callback) => {
      const { processId, routingId } = captured.webContents.mainFrame;
      callback({ video: { id: `web-contents-media-stream://${processId}:${routingId}`, name: 'captured' } });
    });
    const { ok, captureHandle, message } = await capturer.webContents.executeJavaScript(
      `navigator.mediaDevices.getDisplayMedia({ video: true }).then((stream) => {
        const [track] = stream.getVideoTracks();
        return { ok: true, captureHandle: track.getCaptureHandle(), message: null };
      }, (e) => ({ ok: false, message: e.message }))`,
      true
    );
    expect(message).toBeNull();
    expect(ok).toBe(true);
    expect(captureHandle.handle).toBe('captured-tab');
    expect(captureHandle.origin).toBe(new URL(serverUrl).origin);
  });

  it('does not crash when providing only audio for a video request', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    let callbackError: any;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      try {
        callback({
          audio: 'loopback'
        });
      } catch (e) {
        callbackError = e;
      }
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        video: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(ok).toBe(false);
    expect(callbackError?.message).toBe('Video was requested, but no video stream was provided');
  });

  it('does not crash when providing only an audio stream for an audio+video request', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    let callbackError: any;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      try {
        callback({
          audio: 'loopback'
        });
      } catch (e) {
        callbackError = e;
      }
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(ok).toBe(false);
    expect(callbackError?.message).toBe('Video was requested, but no video stream was provided');
  });

  it('does not crash when providing a non-loopback audio stream', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      callback({
        video: w.webContents.mainFrame,
        audio: 'default' as any
      });
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(ok).toBe(true);
  });

  it('does not crash when providing no streams', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    let callbackError: any;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      try {
        callback({});
      } catch (e) {
        callbackError = e;
      }
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(ok).toBe(false);
    expect(callbackError.message).toBe('Video was requested, but no video stream was provided');
  });

  it('does not crash when using a bogus web-contents-media-stream:// ID', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      callback({
        video: { id: 'web-contents-media-stream://9999:9999', name: 'whatever' }
      });
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(ok).toBe(false);
    expect(message).toBe('Could not start video source');
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
    expect(ok, message).toBe(true);
  });

  it('works when calling getDisplayMedia with preferCurrentTab', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      callback({ video: w.webContents.mainFrame });
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        preferCurrentTab: true,
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(ok, message).toBe(true);
  });

  it('returns a MediaStream with BrowserCaptureMediaStreamTrack when the current tab is selected', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    ses.setDisplayMediaRequestHandler((request, callback) => {
      requestHandlerCalled = true;
      callback({ video: w.webContents.mainFrame });
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        preferCurrentTab: true,
        video: true,
        audio: false,
      }).then(stream => {
        const [videoTrack] = stream.getVideoTracks();
        return { ok: videoTrack instanceof BrowserCaptureMediaStreamTrack, message: null };
      }, e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(ok, message).toBe(true);
  });

  ifit(process.platform !== 'darwin')('can supply a screen response to preferCurrentTab', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    ses.setDisplayMediaRequestHandler(async (request, callback) => {
      requestHandlerCalled = true;
      const sources = await desktopCapturer.getSources({ types: ['screen'] });
      callback({ video: sources[0] });
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        preferCurrentTab: true,
        video: true,
        audio: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(ok, message).toBe(true);
  });

  it('can supply a frame response', async () => {
    const ses = session.fromPartition('' + Math.random());
    let requestHandlerCalled = false;
    ses.setDisplayMediaRequestHandler(async (request, callback) => {
      requestHandlerCalled = true;
      callback({ video: w.webContents.mainFrame });
    });
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        video: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(requestHandlerCalled).toBe(true);
    expect(ok, message).toBe(true);
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
    expect(ok, message).toBe(true);
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
    expect(ok, message).toBe(true);
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
    expect(ok, message).toBe(true);
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
    expect(ok, message).toBe(true);
  });

  it('throws an error when calling legacy getUserMedia with invalid chromeMediaSourceId', async () => {
    const w = new BrowserWindow({ show: false });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => navigator.getUserMedia({
         video: {
          mandatory: {
            chromeMediaSource: 'desktop',
            chromeMediaSourceId: undefined,
          },
        },
      }, x => resolve({ok: x instanceof MediaStream}), e => resolve({ ok: false, message: e.message })))
    `);
    expect(ok).toBe(false);
    expect(message).toBe('Invalid state');
  });

  it('works when mediaDevices.getUserMedia uses a tab source id from webContents.getMediaSourceId', async () => {
    const sourceWindow = new BrowserWindow({ show: false });
    const requestingWindow = new BrowserWindow({ show: false });
    await Promise.all([sourceWindow.loadURL(serverUrl), requestingWindow.loadURL(serverUrl)]);

    const sourceId = sourceWindow.webContents.getMediaSourceId(requestingWindow.webContents);
    const { ok, message, origin, videoTrackCount } = await captureWithTabSourceId(requestingWindow, sourceId);

    expect(ok, message).toBe(true);
    expect(origin).toBe(new URL(serverUrl).origin);
    expect(videoTrackCount).toBe(1);
  });

  it('does not capture a webContents through the desktop source with a raw web-contents id', async () => {
    const sourceWindow = new BrowserWindow({ show: false });
    const requestingWindow = new BrowserWindow({ show: false });
    await Promise.all([sourceWindow.loadURL(serverUrl), requestingWindow.loadURL(serverUrl)]);
    const { processId, routingId } = sourceWindow.webContents.mainFrame;
    const rawId = `web-contents-media-stream://${processId}:${routingId}`;
    const { ok, message } = await requestingWindow.webContents.executeJavaScript(`
      navigator.mediaDevices.getUserMedia({
        video: { mandatory: { chromeMediaSource: 'desktop', chromeMediaSourceId: ${JSON.stringify(rawId)} } }
      }).then((stream) => { stream.getTracks().forEach((t) => t.stop()); return { ok: true }; },
              (e) => ({ ok: false, message: e.message }))
    `);
    expect(ok).toBe(false);
    expect(message).toBe('Invalid state');
  });

  it('rejects a tab source id when used from a different requesting webContents', async () => {
    const sourceWindow = new BrowserWindow({ show: false });
    const registeredRequesterWindow = new BrowserWindow({ show: false });
    const otherRequesterWindow = new BrowserWindow({ show: false });
    await Promise.all([
      sourceWindow.loadURL(serverUrl),
      registeredRequesterWindow.loadURL(serverUrl),
      otherRequesterWindow.loadURL(serverUrl)
    ]);

    const sourceId = sourceWindow.webContents.getMediaSourceId(registeredRequesterWindow.webContents);
    const { ok, message, origin } = await captureWithTabSourceId(otherRequesterWindow, sourceId);

    expect(ok).toBe(false);
    expect(message).toMatch(/Invalid state|Error starting tab capture/);
    expect(origin).toBe(new URL(serverUrl).origin);
  });

  it('can remove a displayMediaRequestHandler', async () => {
    const ses = session.fromPartition('' + Math.random());

    ses.setDisplayMediaRequestHandler(() => {
      throw new Error('bad');
    });
    ses.setDisplayMediaRequestHandler(null);
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    await w.loadURL(serverUrl);
    const { ok, message } = await w.webContents.executeJavaScript(
      `
      navigator.mediaDevices.getDisplayMedia({
        video: true,
      }).then(x => ({ok: x instanceof MediaStream}), e => ({ok: false, message: e.message}))
    `,
      true
    );
    expect(ok).toBe(false);
    expect(message).toBe('Not supported');
  });
});
