import { BrowserWindow, apiBridgeMain, ipcMain, session, WebFrameMain } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { listen } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

// Records whether the API was there before any other page script ran.
const page = "<script>window.availableAtStart = typeof navigator.electron?.test === 'object'</script><p>apiBridge</p>";
// Records every electronapichange event, from before any other page script.
const listenPage =
  "<script>window.changes = []; addEventListener('electronapichange', (e) => changes.push([e.detail.name, e.detail.change]))</script>";
const preload = path.join(__dirname, 'fixtures', 'api', 'api-bridge', 'preload.js');

describe('apiBridge module', () => {
  let server: http.Server;
  let otherServer: http.Server;
  let url: string;
  let origin: string;
  let otherUrl: string;
  let otherOrigin: string;

  const handler = (req: http.IncomingMessage, res: http.ServerResponse) => {
    res.setHeader('Content-Type', 'text/html');
    if (req.url === '/iframe') res.end('<iframe src="/child"></iframe>');
    else if (req.url === '/listen') res.end(listenPage);
    else res.end(page);
  };

  before(async () => {
    server = http.createServer(handler);
    otherServer = http.createServer(handler);
    url = (await listen(server)).url;
    otherUrl = (await listen(otherServer)).url;
    origin = new URL(url).origin;
    otherOrigin = new URL(otherUrl).origin;
  });

  after(() => {
    server.close();
    otherServer.close();
  });

  afterEach(closeAllWindows);

  const createWindow = (webPreferences: Electron.WebPreferences = {}) =>
    new BrowserWindow({ show: false, webPreferences });

  // Passes |api| as 'test' before the first navigation, then loads the page.
  async function loadWithApi(api: Record<string, any>, webPreferences?: Electron.WebPreferences) {
    const w = createWindow(webPreferences);
    w.webContents.mainFrame.apiBridge.pass('test', api, { origin });
    await w.loadURL(url);
    return w;
  }

  // An API method that stays unfinished until the test calls |finish|, and
  // gives the test its caller.signal once the page has called it.
  function unfinishedMethod() {
    let onCall!: (signal: AbortSignal) => void;
    const called = new Promise<AbortSignal>((resolve) => {
      onCall = resolve;
    });
    let finish: (value: string) => void = () => {};
    const method = apiBridgeMain.withCaller((caller: Electron.ApiBridgeCaller) => {
      onCall(caller.signal);
      return new Promise<string>((resolve) => {
        finish = resolve;
      });
    });
    return { method, called, finish: (value: string) => finish(value) };
  }

  describe('frame.apiBridge.pass()', () => {
    it('requires an origin before the frame has one', () => {
      const w = createWindow();
      expect(() => w.webContents.mainFrame.apiBridge.pass('test', {})).to.throw(/origin/);
    });

    it('defaults to the origin of the current document', async () => {
      const w = createWindow();
      await w.loadURL(url);
      w.webContents.mainFrame.apiBridge.pass('test', { ping: () => 'pong' });
      // Passed after load, so it arrives asynchronously.
      await waitInPage(w.webContents, "typeof navigator.electron?.test === 'object'");
      expect(await w.webContents.executeJavaScript('navigator.electron.test.ping()')).to.equal('pong');
    });

    it('rejects origins with a path', () => {
      const w = createWindow();
      expect(() => w.webContents.mainFrame.apiBridge.pass('test', {}, { origin: `${origin}/path` })).to.throw(/origin/);
    });

    it('rejects file:// origins', async () => {
      const w = createWindow();
      expect(() => w.webContents.mainFrame.apiBridge.pass('test', {}, { origin: 'file://' })).to.throw(/file:\/\//);
      await w.loadFile(preload);
      expect(() => w.webContents.mainFrame.apiBridge.pass('test', {})).to.throw(/file:\/\//);
    });

    it('accepts a list of origins', async () => {
      const w = createWindow();
      w.webContents.mainFrame.apiBridge.pass('test', { ping: () => 'pong' }, { origin: [origin, otherOrigin] });
      await w.loadURL(otherUrl);
      expect(await w.webContents.executeJavaScript('navigator.electron.test.ping()')).to.equal('pong');
    });

    it('rejects names that are not identifiers', () => {
      const w = createWindow();
      expect(() => w.webContents.mainFrame.apiBridge.pass('not-an-identifier', {}, { origin })).to.throw(/name/);
    });

    it('rejects members that are not functions, events or stores', () => {
      const w = createWindow();
      expect(() => w.webContents.mainFrame.apiBridge.pass('test', { value: 1 }, { origin })).to.throw(
        /API member 'value'/
      );
    });

    it('only exposes own enumerable properties', async () => {
      const proto = { inherited: () => 'no' };
      const api = Object.create(proto, { hidden: { value: () => 'no', enumerable: false } });
      api.own = () => 'yes';
      const w = await loadWithApi(api);
      expect(await w.webContents.executeJavaScript('Object.keys(navigator.electron.test)')).to.deep.equal(['own']);
    });

    it('can pass the same API to several frames', async () => {
      const count = apiBridgeMain.store(0);
      const api = { bump: () => count.set(count.get() + 1), count };
      const windows = [createWindow(), createWindow()];
      for (const w of windows) w.webContents.mainFrame.apiBridge.pass('test', api, { origin });
      await Promise.all(windows.map((w) => w.loadURL(url)));

      await windows[0].webContents.executeJavaScript('navigator.electron.test.bump()');
      await waitInPage(windows[1].webContents, 'navigator.electron.test.count.get() === 1');
      await windows[1].webContents.executeJavaScript('navigator.electron.test.bump()');
      await waitInPage(windows[0].webContents, 'navigator.electron.test.count.get() === 2');
      expect(count.get()).to.equal(2);
    });
  });

  describe('session.apiBridge.pass()', () => {
    let partition = 0;
    // A fresh session for each test, so its APIs don't reach other tests.
    const newSession = () => session.fromPartition(`api-bridge-spec-${partition++}`);

    it('requires an origin', () => {
      const ses = newSession();
      expect(() => ses.apiBridge.pass('test', {}, {} as any)).to.throw(/origin/);
    });

    it('rejects file:// origins', () => {
      const ses = newSession();
      expect(() => ses.apiBridge.pass('test', {}, { origin: 'file:///' })).to.throw(/file:\/\//);
    });

    it('gives the API to every window of the session', async () => {
      const ses = newSession();
      ses.apiBridge.pass('test', { ping: () => 'pong' }, { origin });
      const windows = [createWindow({ session: ses }), createWindow({ session: ses })];
      await Promise.all(windows.map((w) => w.loadURL(url)));
      for (const w of windows) {
        expect(await w.webContents.executeJavaScript('availableAtStart')).to.be.true();
        expect(await w.webContents.executeJavaScript('navigator.electron.test.ping()')).to.equal('pong');
      }
    });

    it('reaches documents that are already open', async () => {
      const ses = newSession();
      const w = createWindow({ session: ses });
      await w.loadURL(url);
      ses.apiBridge.pass('test', { ping: () => 'pong' }, { origin });
      await waitInPage(w.webContents, "typeof navigator.electron?.test === 'object'");
      expect(await w.webContents.executeJavaScript('navigator.electron.test.ping()')).to.equal('pong');
    });

    it('only reaches the listed origins', async () => {
      const ses = newSession();
      ses.apiBridge.pass('test', { ping: () => 'pong' }, { origin });
      const w = createWindow({ session: ses });
      await w.loadURL(otherUrl);
      expect(await w.webContents.executeJavaScript("'electron' in navigator")).to.be.false();
    });

    it('does not reach other sessions', async () => {
      newSession().apiBridge.pass('test', { ping: () => 'pong' }, { origin });
      const w = createWindow({ session: newSession() });
      await w.loadURL(url);
      expect(await w.webContents.executeJavaScript("'electron' in navigator")).to.be.false();
    });

    it('only reaches main frames unless frames is all', async () => {
      const mainOnly = newSession();
      mainOnly.apiBridge.pass('test', { ping: () => 'pong' }, { origin });
      const w1 = createWindow({ session: mainOnly });
      await w1.loadURL(`${url}/iframe`);
      expect(await w1.webContents.executeJavaScript("'electron' in navigator")).to.be.true();
      const child1 = await waitForChildFrame(w1.webContents.mainFrame);
      expect(await child1.executeJavaScript("'electron' in navigator")).to.be.false();

      const allFrames = newSession();
      allFrames.apiBridge.pass('test', { ping: () => 'pong' }, { origin, frames: 'all' });
      const w2 = createWindow({ session: allFrames });
      await w2.loadURL(`${url}/iframe`);
      const child2 = await waitForChildFrame(w2.webContents.mainFrame);
      expect(await child2.executeJavaScript('navigator.electron.test.ping()')).to.equal('pong');
    });

    it('only reaches windows a page opens with popups: true', async () => {
      for (const popups of [false, true]) {
        const ses = newSession();
        ses.apiBridge.pass('test', { ping: () => 'pong' }, { origin, popups });
        const w = createWindow({ session: ses });
        await w.loadURL(url);
        const popupUrl = `${url}/popup`;
        const opened = nextWindowLoaded(w.webContents, popupUrl);
        await w.webContents.executeJavaScript(`window.open(${JSON.stringify(popupUrl)}); null`);
        const child = await opened;
        expect(await child.webContents.executeJavaScript("'electron' in navigator")).to.equal(popups);
      }
    });

    it('only reaches <webview> guests with guests: true', async () => {
      for (const guests of [false, true]) {
        const name = `api-bridge-spec-guests-${guests}`;
        const ses = session.fromPartition(name);
        ses.apiBridge.pass('test', { ping: () => 'pong' }, { origin, guests });
        const w = createWindow({ session: ses, webviewTag: true });
        await w.loadURL(otherUrl);
        const attached = once(w.webContents, 'did-attach-webview') as Promise<[Electron.Event, Electron.WebContents]>;
        await w.webContents.executeJavaScript(`new Promise((resolve) => {
          const view = document.createElement('webview');
          view.setAttribute('partition', ${JSON.stringify(name)});
          view.src = ${JSON.stringify(`${url}/guest`)};
          view.addEventListener('did-finish-load', () => resolve(null), { once: true });
          document.body.appendChild(view);
        })`);
        const [, guest] = await attached;
        expect(await guest.executeJavaScript("'electron' in navigator")).to.equal(guests);
        ses.apiBridge.revoke('test');
      }
    });

    it('lets an API passed to the frame take precedence', async () => {
      const ses = newSession();
      ses.apiBridge.pass('test', { who: apiBridgeMain.sync(() => 'session') }, { origin });
      const w = createWindow({ session: ses });
      w.webContents.mainFrame.apiBridge.pass('test', { who: apiBridgeMain.sync(() => 'frame') }, { origin });
      await w.loadURL(url);
      expect(await w.webContents.executeJavaScript('navigator.electron.test.who()')).to.equal('frame');

      // Revoking the frame's API brings the session's back.
      w.webContents.mainFrame.apiBridge.revoke('test');
      await waitInPage(w.webContents, "navigator.electron?.test?.who() === 'session'");
    });

    it('revokes the API from every document', async () => {
      const ses = newSession();
      ses.apiBridge.pass('test', { ping: () => 'pong' }, { origin });
      const windows = [createWindow({ session: ses }), createWindow({ session: ses })];
      await Promise.all(windows.map((w) => w.loadURL(url)));
      expect(ses.apiBridge.revoke('test')).to.be.true();
      expect(ses.apiBridge.revoke('test')).to.be.false();
      for (const w of windows) await waitInPage(w.webContents, "!('electron' in navigator)");

      // Later documents don't get it either.
      await windows[0].loadURL(url);
      expect(await windows[0].webContents.executeJavaScript("'electron' in navigator")).to.be.false();
    });
  });

  describe('passToIsolatedWorld()', () => {
    it('gives the API to the preload script and not to the page', async () => {
      const w = createWindow({ preload });
      w.webContents.mainFrame.apiBridge.passToIsolatedWorld('test', { ping: () => 'pong' }, { origin });
      const names = once(ipcMain, 'api-bridge-preload');
      const pinged = once(ipcMain, 'api-bridge-preload-ping');
      await w.loadURL(url);
      expect((await names)[1]).to.deep.equal(['test']);
      expect((await pinged)[1]).to.equal('pong');
      expect(await w.webContents.executeJavaScript("'electron' in navigator")).to.be.false();
    });

    it('keeps the names of the two worlds apart', async () => {
      const w = createWindow({ preload });
      const frame = w.webContents.mainFrame;
      frame.apiBridge.pass('test', { ping: () => 'page' }, { origin });
      frame.apiBridge.passToIsolatedWorld('test', { ping: () => 'preload' }, { origin });
      const pinged = once(ipcMain, 'api-bridge-preload-ping');
      await w.loadURL(url);
      expect((await pinged)[1]).to.equal('preload');
      expect(await w.webContents.executeJavaScript('navigator.electron.test.ping()')).to.equal('page');
      expect(frame.apiBridge.revokeFromIsolatedWorld('test')).to.be.true();
      expect(frame.apiBridge.revokeFromIsolatedWorld('test')).to.be.false();
      expect(await w.webContents.executeJavaScript('navigator.electron.test.ping()')).to.equal('page');
    });

    it('is not delivered without contextIsolation', async () => {
      const w = createWindow({ preload, contextIsolation: false });
      w.webContents.mainFrame.apiBridge.passToIsolatedWorld('test', { ping: () => 'pong' }, { origin });
      const names = once(ipcMain, 'api-bridge-preload');
      await w.loadURL(url);
      expect((await names)[1]).to.equal(null);
    });

    it('works for a session', async () => {
      const ses = session.fromPartition('api-bridge-spec-isolated');
      ses.apiBridge.passToIsolatedWorld('test', { ping: () => 'pong' }, { origin });
      const w = createWindow({ session: ses, preload });
      const pinged = once(ipcMain, 'api-bridge-preload-ping');
      await w.loadURL(url);
      expect((await pinged)[1]).to.equal('pong');
      expect(await w.webContents.executeJavaScript("'electron' in navigator")).to.be.false();
      expect(ses.apiBridge.revokeFromIsolatedWorld('test')).to.be.true();
    });
  });

  describe('apiBridgeMain.withCaller()', () => {
    it('tells a shared method which frame called it', async () => {
      const ses = session.fromPartition('api-bridge-spec-caller');
      ses.apiBridge.pass(
        'test',
        { whoAmI: apiBridgeMain.withCaller((caller: Electron.ApiBridgeCaller) => caller.frame.frameToken) },
        {
          origin
        }
      );
      const windows = [createWindow({ session: ses }), createWindow({ session: ses })];
      await Promise.all(windows.map((w) => w.loadURL(url)));
      for (const w of windows) {
        expect(await w.webContents.executeJavaScript('navigator.electron.test.whoAmI()')).to.equal(
          w.webContents.mainFrame.frameToken
        );
      }
      ses.apiBridge.revoke('test');
    });

    it('tells the method the origin of the caller', async () => {
      const w = await loadWithApi({
        where: apiBridgeMain.withCaller((caller: Electron.ApiBridgeCaller) => caller.origin)
      });
      expect(await w.webContents.executeJavaScript('navigator.electron.test.where()')).to.equal(origin);
    });

    it('aborts caller.signal when the calling document goes away', async () => {
      const { method, called, finish } = unfinishedMethod();
      const w = await loadWithApi({ wait: method });
      w.webContents.executeJavaScript('navigator.electron.test.wait(); null');
      const signal = await called;
      expect(signal.aborted).to.be.false();
      await w.loadURL(otherUrl);
      if (!signal.aborted) await once(signal, 'abort');
      expect(signal.aborted).to.be.true();
      finish('late');
    });

    it('passes the page arguments after the caller', async () => {
      const w = await loadWithApi({
        add: apiBridgeMain.withCaller((caller: Electron.ApiBridgeCaller, a: number, b: number) => {
          expect(caller.frame).to.equal(w.webContents.mainFrame);
          return a + b;
        })
      });
      expect(await w.webContents.executeJavaScript('navigator.electron.test.add(1, 2)')).to.equal(3);
    });

    it('combines with apiBridgeMain.sync()', async () => {
      const w = await loadWithApi({
        url: apiBridgeMain.sync(apiBridgeMain.withCaller((caller: Electron.ApiBridgeCaller) => caller.frame.url))
      });
      expect(await w.webContents.executeJavaScript('navigator.electron.test.url()')).to.equal(w.webContents.getURL());
    });
  });

  describe('methods', () => {
    it('resolves with the return value', async () => {
      const w = await loadWithApi({ add: (a: number, b: number) => a + b });
      expect(await w.webContents.executeJavaScript('navigator.electron.test.add(1, 2)')).to.equal(3);
    });

    it('awaits async implementations', async () => {
      const w = await loadWithApi({
        double: async (x: number) => {
          await setTimeout(10);
          return x * 2;
        }
      });
      expect(await w.webContents.executeJavaScript('navigator.electron.test.double(21)')).to.equal(42);
    });

    it('calls implementations with the API as this', async () => {
      const w = await loadWithApi({
        name: () => 'api',
        self() {
          return this.name();
        }
      });
      expect(await w.webContents.executeJavaScript('navigator.electron.test.self()')).to.equal('api');
    });

    it('rejects with the name and message of a thrown error', async () => {
      class NotFoundError extends Error {
        constructor(message: string) {
          super(message);
          this.name = 'NotFoundError';
        }
      }
      const w = await loadWithApi({
        fail: () => {
          throw new TypeError('bad type');
        },
        failAsync: async () => {
          throw new NotFoundError('missing');
        },
        failSync: apiBridgeMain.sync(() => {
          throw new RangeError('out of range');
        })
      });
      const errors = await w.webContents.executeJavaScript(`(async () => {
        const { test } = navigator.electron;
        const describe = (e) => [e instanceof Error, e.constructor.name, e.name, e.message];
        const sync = (() => { try { test.failSync() } catch (e) { return describe(e) } })();
        return [await test.fail().catch(describe), await test.failAsync().catch(describe), sync];
      })()`);
      expect(errors).to.deep.equal([
        [true, 'TypeError', 'TypeError', 'bad type'],
        [true, 'Error', 'NotFoundError', 'missing'],
        [true, 'RangeError', 'RangeError', 'out of range']
      ]);
    });

    it('copies arguments and results with structured clone', async () => {
      let received: any;
      const w = await loadWithApi({
        echo: (value: any) => {
          received = value;
          return value;
        }
      });
      const result = await w.webContents.executeJavaScript(`navigator.electron.test
        .echo({ map: new Map([[1, 'one']]), date: new Date(0), bytes: new Uint8Array([1, 2]) })
        .then((v) => [v.map instanceof Map, v.map.get(1), v.date.getTime(), Array.from(v.bytes)])`);
      expect(result).to.deep.equal([true, 'one', 0, [1, 2]]);
      expect(received.map).to.be.an.instanceOf(Map);
      expect(received.date).to.be.an.instanceOf(Date);
    });

    it('rejects when an argument cannot be cloned', async () => {
      const w = await loadWithApi({ echo: (v: any) => v });
      const message = await w.webContents.executeJavaScript(
        "navigator.electron.test.echo(Symbol('nope')).catch((e) => e.message)"
      );
      expect(message).to.match(/could not be cloned/);
    });

    it('calls sync methods synchronously', async () => {
      const w = await loadWithApi({
        value: apiBridgeMain.sync(() => 42),
        fail: apiBridgeMain.sync(() => {
          throw new Error('sync boom');
        })
      });
      expect(await w.webContents.executeJavaScript('navigator.electron.test.value()')).to.equal(42);
      expect(
        await w.webContents.executeJavaScript(
          '(() => { try { navigator.electron.test.fail() } catch (e) { return e.message } })()'
        )
      ).to.equal('sync boom');
    });

    it('waits for async implementations of sync methods', async () => {
      const w = await loadWithApi({
        value: apiBridgeMain.sync(async () => {
          await setTimeout(10);
          return 'late';
        })
      });
      expect(await w.webContents.executeJavaScript('navigator.electron.test.value()')).to.equal('late');
    });
  });

  describe('events', () => {
    it('calls listeners with the emitted arguments', async () => {
      const changed = apiBridgeMain.event();
      const w = await loadWithApi({ ready: () => changed.emit('hello', { n: 42 }), changed });
      const args = await w.webContents.executeJavaScript(`new Promise((resolve) => {
        const { test } = navigator.electron;
        test.changed.on((...args) => resolve(args));
        test.ready();
      })`);
      expect(args).to.deep.equal(['hello', { n: 42 }]);
    });

    it('stops calling a listener after it unsubscribes', async () => {
      const changed = apiBridgeMain.event();
      const w = await loadWithApi({ emit: (n: number) => changed.emit(n), changed });
      const seen = await w.webContents.executeJavaScript(`(async () => {
        const { test } = navigator.electron;
        const seen = [];
        const off = test.changed.on((n) => seen.push(n));
        test.changed.on((n) => { if (n === 1) off(); });
        await test.emit(1);
        await test.emit(2);
        await test.emit(3);
        await new Promise((resolve) => setTimeout(resolve, 50));
        return seen;
      })()`);
      expect(seen).to.deep.equal([1]);
    });

    it('reaches every window of a session', async () => {
      const ses = session.fromPartition('api-bridge-spec-events');
      const changed = apiBridgeMain.event();
      ses.apiBridge.pass('test', { changed }, { origin });
      const windows = [createWindow({ session: ses }), createWindow({ session: ses })];
      await Promise.all(windows.map((w) => w.loadURL(url)));
      const received = windows.map((w) =>
        w.webContents.executeJavaScript('new Promise((resolve) => navigator.electron.test.changed.on(resolve))')
      );
      // Give both pages a moment to subscribe.
      await Promise.all(windows.map((w) => w.webContents.executeJavaScript('null')));
      changed.emit('hi');
      expect(await Promise.all(received)).to.deep.equal(['hi', 'hi']);
      ses.apiBridge.revoke('test');
    });

    it('throws when emitting a value that cannot be cloned', () => {
      const changed = apiBridgeMain.event();
      expect(() => changed.emit(() => {})).to.throw();
    });
  });

  describe('stores', () => {
    it('makes the latest value readable without a call', async () => {
      const count = apiBridgeMain.store(1);
      count.set(5);
      const w = await loadWithApi({ count });
      expect(await w.webContents.executeJavaScript('navigator.electron.test.count.get()')).to.equal(5);
      expect(count.get()).to.equal(5);
    });

    it('sends updates to subscribers and to get()', async () => {
      const count = apiBridgeMain.store(1);
      const w = await loadWithApi({ bump: () => count.set(count.get() + 1), count });
      const result = await w.webContents.executeJavaScript(`new Promise((resolve) => {
        const { test } = navigator.electron;
        test.count.subscribe((value) => resolve([value, test.count.get()]));
        test.bump();
      })`);
      expect(result).to.deep.equal([2, 2]);
    });

    it('returns the same object from get() until the value changes', async () => {
      const w = await loadWithApi({ config: apiBridgeMain.store({ theme: 'dark' }) });
      expect(
        await w.webContents.executeJavaScript(
          'navigator.electron.test.config.get() === navigator.electron.test.config.get()'
        )
      ).to.be.true();
    });
  });

  describe('delivery', () => {
    it('puts the API on navigator.electron before page scripts run', async () => {
      const w = await loadWithApi({ ping: () => 'pong' });
      expect(await w.webContents.executeJavaScript('availableAtStart')).to.be.true();
    });

    it('puts the API on navigator.electron before the preload runs', async () => {
      const w = createWindow({ preload, contextIsolation: false });
      w.webContents.mainFrame.apiBridge.pass('test', { ping: () => 'pong' }, { origin });
      const names = once(ipcMain, 'api-bridge-preload');
      await w.loadURL(url);
      expect((await names)[1]).to.deep.equal(['test']);
    });

    it('adds nothing to window', async () => {
      const w = await loadWithApi({ ping: () => 'pong' });
      expect(await w.webContents.executeJavaScript("[typeof window.test, 'electron' in window]")).to.deep.equal([
        'undefined',
        false
      ]);
    });

    it('makes navigator.electron hidden and each API read-only', async () => {
      const w = await loadWithApi({ ping: () => 'pong', changed: apiBridgeMain.event() });
      const result = await w.webContents.executeJavaScript(`(() => {
        const namespace = Object.getOwnPropertyDescriptor(navigator, 'electron');
        const api = Object.getOwnPropertyDescriptor(navigator.electron, 'test');
        return [
          namespace.writable,
          namespace.enumerable,
          api.writable,
          Object.isFrozen(api.value),
          Object.isFrozen(api.value.changed),
          api.value.ping.name
        ];
      })()`);
      expect(result).to.deep.equal([false, false, false, true, true, 'ping']);
    });

    it('defines navigator.electron again if page script deleted it', async () => {
      const w = await loadWithApi({ ping: () => 'pong' });
      await w.webContents.executeJavaScript('delete navigator.electron; null');
      w.webContents.mainFrame.apiBridge.pass('other', { ping: () => 'other' });
      await waitInPage(w.webContents, "typeof navigator.electron?.other === 'object'");
      expect(await w.webContents.executeJavaScript('navigator.electron.test.ping()')).to.equal('pong');
    });

    it('does not add navigator.electron to documents without an API', async () => {
      const w = createWindow();
      await w.loadURL(url);
      expect(await w.webContents.executeJavaScript("'electron' in navigator")).to.be.false();
    });

    it('removes navigator.electron with the last API', async () => {
      const w = await loadWithApi({ ping: () => 'pong' });
      w.webContents.mainFrame.apiBridge.revoke('test');
      await waitInPage(w.webContents, "!('electron' in navigator)");
    });

    it('fires electronapichange for changes after load only', async () => {
      const w = createWindow();
      const frame = w.webContents.mainFrame;
      frame.apiBridge.pass('test', { ping: () => 'pong' }, { origin });
      await w.loadURL(`${url}/listen`);
      expect(await w.webContents.executeJavaScript('changes')).to.deep.equal([]);

      frame.apiBridge.pass('other', { ping: () => 'other' });
      await waitInPage(w.webContents, 'changes.length === 1');
      frame.apiBridge.pass('test', { ping: () => 'new' });
      await waitInPage(w.webContents, 'changes.length === 2');
      frame.apiBridge.revoke('other');
      await waitInPage(w.webContents, 'changes.length === 3');
      expect(await w.webContents.executeJavaScript('changes')).to.deep.equal([
        ['other', 'added'],
        ['test', 'replaced'],
        ['other', 'removed']
      ]);
    });

    it('does not tell the page about isolated-world APIs', async () => {
      const w = createWindow();
      await w.loadURL(`${url}/listen`);
      w.webContents.mainFrame.apiBridge.passToIsolatedWorld('hidden', { ping: () => 'pong' });
      w.webContents.mainFrame.apiBridge.pass('marker', { ping: () => 'pong' });
      await waitInPage(w.webContents, 'changes.length === 1');
      expect(await w.webContents.executeJavaScript('[changes, Object.keys(navigator.electron)]')).to.deep.equal([
        [['marker', 'added']],
        ['marker']
      ]);
    });

    it('works in renderers that are not sandboxed', async () => {
      const w = await loadWithApi({ ping: () => 'pong' }, { sandbox: false });
      expect(await w.webContents.executeJavaScript('navigator.electron.test.ping()')).to.equal('pong');
    });

    it('keeps the API across same-origin navigations', async () => {
      const w = await loadWithApi({ ping: () => 'pong' });
      await w.loadURL(`${url}/other`);
      expect(await w.webContents.executeJavaScript('availableAtStart')).to.be.true();
      expect(await w.webContents.executeJavaScript('navigator.electron.test.ping()')).to.equal('pong');
    });

    it('does not give the API to documents of another origin', async () => {
      const w = await loadWithApi({ ping: () => 'pong' });
      await w.loadURL(otherUrl);
      expect(await w.webContents.executeJavaScript("[availableAtStart, 'electron' in navigator]")).to.deep.equal([
        false,
        false
      ]);
    });

    it('gives the API back after navigating back to its origin', async () => {
      const w = await loadWithApi({ ping: () => 'pong' });
      await w.loadURL(otherUrl);
      await w.loadURL(url);
      expect(await w.webContents.executeJavaScript('navigator.electron.test.ping()')).to.equal('pong');
    });

    it('does not give an iframe the API passed to its parent', async () => {
      const w = createWindow();
      w.webContents.mainFrame.apiBridge.pass('test', { ping: () => 'main' }, { origin });
      await w.loadURL(`${url}/iframe`);
      const child = await waitForChildFrame(w.webContents.mainFrame);
      expect(await child.executeJavaScript("'electron' in navigator")).to.be.false();
    });

    it('gives an iframe the API passed to it', async () => {
      const w = createWindow();
      await w.loadURL(`${url}/iframe`);
      const child = await waitForChildFrame(w.webContents.mainFrame);
      child.apiBridge.pass('test', { ping: () => 'child' });
      await waitInPage(child, "typeof navigator.electron?.test === 'object'");
      expect(await child.executeJavaScript('navigator.electron.test.ping()')).to.equal('child');
      expect(await w.webContents.executeJavaScript("'electron' in navigator")).to.be.false();
    });
  });

  describe('frame.apiBridge.revoke()', () => {
    it('removes the API from navigator.electron', async () => {
      const w = await loadWithApi({ ping: () => 'pong' });
      await w.webContents.executeJavaScript('window.kept = navigator.electron.test; null');
      expect(w.webContents.mainFrame.apiBridge.revoke('test')).to.be.true();
      expect(w.webContents.mainFrame.apiBridge.revoke('test')).to.be.false();
      // The revocation reaches the renderer asynchronously.
      await waitInPage(w.webContents, '!navigator.electron?.test');
      const message = await w.webContents.executeJavaScript('kept.ping().catch((e) => e.message)');
      expect(message).to.equal('This API is not available to this frame');
    });

    it('is not needed to replace an API', async () => {
      const w = await loadWithApi({ version: () => 1 });
      await w.webContents.executeJavaScript('window.old = navigator.electron.test; null');
      w.webContents.mainFrame.apiBridge.pass('test', { version: () => 2 });
      await waitInPage(w.webContents, 'navigator.electron.test !== old');
      expect(await w.webContents.executeJavaScript('navigator.electron.test.version()')).to.equal(2);
      expect(await w.webContents.executeJavaScript('old.version().catch((e) => e.message)')).to.match(/not available/);
    });

    it('cancels calls that are still running', async () => {
      const { method, called, finish } = unfinishedMethod();
      const w = await loadWithApi({ wait: method });
      const result = w.webContents.executeJavaScript('navigator.electron.test.wait().then((v) => v, (e) => e.message)');
      const signal = await called;
      w.webContents.mainFrame.apiBridge.revoke('test');
      expect(await result).to.match(/not available/);
      if (!signal.aborted) await once(signal, 'abort');
      expect(signal.aborted).to.be.true();
      finish('late');
    });

    it('unblocks a sync call that is still running', async () => {
      let onCall!: () => void;
      const called = new Promise<void>((resolve) => {
        onCall = resolve;
      });
      const w = await loadWithApi({
        wait: apiBridgeMain.sync(() => {
          onCall();
          return new Promise(() => {});
        })
      });
      const result = w.webContents.executeJavaScript(
        '(() => { try { return navigator.electron.test.wait() } catch (e) { return e.message } })()'
      );
      await called;
      w.webContents.mainFrame.apiBridge.revoke('test');
      expect(await result).to.match(/not available/);
    });
  });
});

// Resolves once |expression| is truthy in the page.
async function waitInPage(target: { executeJavaScript(code: string): Promise<any> }, expression: string) {
  await target.executeJavaScript(
    `(async () => { while (!(${expression})) await new Promise((resolve) => setTimeout(resolve, 10)); })()`
  );
}

async function waitForChildFrame(frame: WebFrameMain): Promise<WebFrameMain> {
  for (let i = 0; i < 100; i++) {
    const child = frame.frames[0];
    if (child && child.url.endsWith('/child')) {
      try {
        if ((await child.executeJavaScript('document.readyState')) === 'complete') return child;
      } catch {}
    }
    await setTimeout(20);
  }
  throw new Error('The child frame did not load');
}

// Resolves with the next window |opener|'s page opens, once it has loaded
// |target|.
function nextWindowLoaded(opener: Electron.WebContents, target: string) {
  return new Promise<BrowserWindow>((resolve) => {
    opener.once('did-create-window', (child) => {
      child.webContents.on('did-finish-load', function loaded() {
        if (child.webContents.getURL() !== target) return;
        child.webContents.off('did-finish-load', loaded);
        resolve(child);
      });
    });
  });
}
