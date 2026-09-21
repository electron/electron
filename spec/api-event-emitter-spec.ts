import { app, BaseWindow, BrowserWindow, ipcMain, session } from 'electron/main';

import { expect } from 'chai';

import { EventEmitter, once } from 'node:events';
import * as path from 'node:path';

import { closeAllWindows } from './lib/window-helpers';

// Native code does not call emit() for an event nobody listens to. Whether it
// did is not something JavaScript can see, which is the point: these pin down
// everything around it that JavaScript can see, for both native emitter bases.
describe('native event emission', () => {
  const fixturesPath = path.resolve(__dirname, 'fixtures');

  afterEach(closeAllWindows);

  const loadWindow = async () => {
    const w = new BrowserWindow({ show: false });
    await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
    return w;
  };

  const sendMouseMove = (w: BrowserWindow) => w.webContents.sendInputEvent({ type: 'mouseMove', x: 10, y: 10 });

  // Runs |fn| with the process 'uncaughtException' listeners swapped for one
  // that collects, and returns what it collected. |fn| is handed a promise for
  // the first exception so it can wait for one without polling.
  const collectUncaughtExceptions = async (fn: (firstException: Promise<Error>) => Promise<void>) => {
    const listeners = process.listeners('uncaughtException');
    const uncaught: Error[] = [];
    let onFirstException: (error: Error) => void;
    const firstException = new Promise<Error>((resolve) => {
      onFirstException = resolve;
    });
    process.removeAllListeners('uncaughtException');
    process.on('uncaughtException', (error) => {
      uncaught.push(error);
      onFirstException(error);
    });
    try {
      await fn(firstException);
    } finally {
      process.removeAllListeners('uncaughtException');
      for (const listener of listeners) process.on('uncaughtException', listener);
    }
    return uncaught;
  };

  const createFailingURLLoader = () => {
    const { createURLLoader } = process._linkedBinding('electron_common_net');
    // Port 1 is refused as unsafe before anything touches the network.
    return createURLLoader({
      method: 'GET',
      url: 'http://127.0.0.1:1/',
      extraHeaders: {},
      useSessionCookies: false,
      credentials: 'omit',
      referrer: '',
      origin: '',
      mode: '',
      destination: '',
      hasUserActivation: false,
      bypassCustomProtocolHandlers: false,
      session: session.defaultSession
    } as any);
  };

  describe('Emit()', () => {
    it('reaches a listener added after the event went unobserved', async () => {
      const w = await loadWindow();
      sendMouseMove(w);
      const inputEvent = once(w.webContents, 'input-event') as Promise<[any, Electron.InputEvent]>;
      sendMouseMove(w);
      const [, input] = await inputEvent;
      expect(input.type).to.equal('mouseMove');
    });

    it('stops reaching a listener once it is removed, and resumes when re-added', async () => {
      const w = await loadWindow();
      let received = 0;
      const listener = () => {
        received++;
      };
      w.webContents.on('input-event', listener);
      sendMouseMove(w);
      expect(received).to.equal(1);

      w.webContents.off('input-event', listener);
      sendMouseMove(w);
      expect(received).to.equal(1);

      w.webContents.on('input-event', listener);
      sendMouseMove(w);
      expect(received).to.equal(2);
    });

    it('reports preventDefault() from a listener added after the event went unobserved', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadFile(path.join(fixturesPath, 'pages', 'mouse-events.html'));
      const buttons: number[] = [];
      const twoMouseDowns = new Promise<void>((resolve) => {
        const onMouseDown = (event: Electron.IpcMainEvent, button: number) => {
          buttons.push(button);
          if (buttons.length === 2) {
            ipcMain.off('mousedown', onMouseDown);
            resolve();
          }
        };
        ipcMain.on('mousedown', onMouseDown);
      });

      // Nobody can prevent this one, so the page sees it.
      w.webContents.sendInputEvent({ type: 'mouseDown', button: 'left', x: 100, y: 100 });

      w.webContents.on('before-mouse-event', (event, mouse) => {
        if (mouse.button === 'left') event.preventDefault();
      });
      w.webContents.sendInputEvent({ type: 'mouseDown', button: 'left', x: 100, y: 100 });
      w.webContents.sendInputEvent({ type: 'mouseDown', button: 'right', x: 100, y: 100 });

      await twoMouseDowns;
      expect(buttons).to.deep.equal([0, 2]);
    });

    it("throws an unhandled 'error'", async () => {
      let errorListeners = -1;
      const uncaught = await collectUncaughtExceptions(async (firstException) => {
        const loader = createFailingURLLoader();
        errorListeners = loader.listenerCount('error');
        await firstException;
      });
      expect(errorListeners).to.equal(0);
      expect(uncaught.map((error: any) => error.code)).to.deep.equal(['ERR_UNHANDLED_ERROR']);
    });

    it("delivers a handled 'error'", async () => {
      const [, error] = await once(createFailingURLLoader(), 'error');
      expect(error).to.equal('net::ERR_UNSAFE_PORT');
    });
  });

  describe('EmitWithoutEvent()', () => {
    it('reaches a listener added after the event went unobserved', async () => {
      session.fromPartition(`event-emitter-spec-unobserved-${Date.now()}`);

      const sessionCreated = once(app, 'session-created');
      const created = session.fromPartition(`event-emitter-spec-observed-${Date.now()}`);
      const [emitted] = await sessionCreated;
      expect(emitted).to.equal(created);
    });
  });

  describe('the legacy emitter base', () => {
    it('reaches a listener added after the event went unobserved', async () => {
      const w = new BaseWindow({ show: false });
      w.show();
      const hidden = once(w, 'hide');
      w.hide();
      await hidden;
      const shown = once(w, 'show');
      w.show();
      await shown;
    });

    it('still reaches an emit() replaced on the instance', async () => {
      const w = new BaseWindow({ show: false });
      const seen: (string | symbol)[] = [];
      const shown = new Promise<void>((resolve) => {
        const emit = w.emit;
        w.emit = function (this: BaseWindow, eventName: string | symbol, ...args: any[]) {
          seen.push(eventName);
          if (eventName === 'show') resolve();
          return emit.call(this, eventName, ...args);
        } as any;
      });
      expect(w.listenerCount('show')).to.equal(0);
      w.show();
      await shown;
      expect(seen).to.include('show');
    });
  });

  describe('a replaced emit()', () => {
    it('on the instance still sees events without listeners', async () => {
      const w = await loadWindow();
      const seen: (string | symbol)[] = [];
      const emit = w.webContents.emit;
      w.webContents.emit = function (this: Electron.WebContents, eventName: any, ...args: any[]) {
        seen.push(eventName);
        return emit.call(this, eventName, ...args);
      } as any;
      sendMouseMove(w);
      expect(seen).to.include('input-event');
    });

    it('on EventEmitter.prototype still sees events without listeners', async () => {
      const w = await loadWindow();
      const seen: (string | symbol)[] = [];
      const emit = EventEmitter.prototype.emit;
      EventEmitter.prototype.emit = function (this: EventEmitter, eventName: string | symbol, ...args: any[]) {
        if (this === w.webContents) seen.push(eventName);
        return emit.call(this, eventName, ...args);
      };
      try {
        sendMouseMove(w);
      } finally {
        EventEmitter.prototype.emit = emit;
      }
      expect(seen).to.include('input-event');
    });

    it('stops seeing them once the original is back', async () => {
      const w = await loadWindow();
      const emit = w.webContents.emit;
      w.webContents.emit = function (this: Electron.WebContents, eventName: any, ...args: any[]) {
        return emit.call(this, eventName, ...args);
      } as any;
      delete (w.webContents as any).emit;

      const inputEvent = once(w.webContents, 'input-event');
      sendMouseMove(w);
      await inputEvent;
    });
  });

  describe('an emitter only JavaScript can answer for', () => {
    it('does not have its emit getter called to decide whether to emit', async () => {
      const w = await loadWindow();
      const emit = w.webContents.emit;
      const received: string[] = [];
      w.webContents.on('before-mouse-event', () => {
        received.push('before-mouse-event');
      });
      w.webContents.on('input-event', () => {
        received.push('input-event');
      });

      // A mouse move emits 'before-mouse-event' and then 'input-event'. The
      // getter fails the first time it is asked for emit(), so the first of
      // those is lost and its error surfaces, exactly once.
      let lookups = 0;
      Object.defineProperty(w.webContents, 'emit', {
        configurable: true,
        get: () => {
          if (++lookups === 1) throw new Error('emit getter failed');
          return emit;
        }
      });

      const uncaught = await collectUncaughtExceptions(async (firstException) => {
        // Emission is synchronous, so both events have been handled by the
        // time this returns; the error is all that is left to wait for.
        sendMouseMove(w);
        await firstException;
      });
      delete (w.webContents as any).emit;

      expect(received).to.deep.equal(['input-event']);
      expect(uncaught.map((error) => error.message)).to.deep.equal(['emit getter failed']);
    });

    it('still emits when `_events` is an accessor', async () => {
      const w = await loadWindow();
      let received = 0;
      w.webContents.on('input-event', () => {
        received++;
      });
      const events = (w.webContents as any)._events;
      Object.defineProperty(w.webContents, '_events', { configurable: true, get: () => events });
      sendMouseMove(w);
      expect(received).to.equal(1);
    });
  });
});
