// End-to-end coverage for setIgnoreMouseEvents() driven by real OS input (see
// spec/fixtures/native-addon/mouse-input): SendInput() on Windows,
// CGEventPost() on macOS and XTEST on Linux X11.
//
// Why real input: webContents.sendInputEvent() feeds Blink directly, so it
// cannot tell whether the OS lets clicks through the window or whether moves
// are forwarded. On Windows a forwarding window is WS_EX_LAYERED |
// WS_EX_TRANSPARENT, so the OS never delivers mouse messages to it and
// NativeWindowViews gets the cursor position from a WH_MOUSE_LL hook instead;
// only SendInput() (or a physical mouse) goes through low level hooks. On
// macOS the window ignores mouse events at the window server and forwarding
// relies on AppKit delivering mouse-moved events to it. On Linux the window
// gets an empty input shape (X11) or input region (Wayland), and the forward
// option is not supported (docs/api/browser-window.md).
import { BaseWindow, BrowserWindow, WebContentsView, screen } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as http from 'node:http';
import { createRequire } from 'node:module';
import { setTimeout } from 'node:timers/promises';

import { defer, ifdescribe, ifit, isWayland, listen, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

const require = createRequire(import.meta.url);

type MouseInput = {
  move(x: number, y: number): Promise<unknown>;
  click(button?: 'left' | 'right' | 'middle'): Promise<unknown>;
  getCursorPos(): { x?: number; y?: number; error?: number };
  isWindowAtPoint(handle: Buffer, x: number, y: number): boolean;
  describeWindowAtPoint(x: number, y: number): Record<string, unknown> | null;
  getDiagnostics(): { available?: boolean; sendInputProbeSent?: number; inputDesktop?: string } & Record<
    string,
    unknown
  >;
};

type PageEvent = { type: string; x?: number; y?: number };
// A BrowserWindow or a WebContentsView showing makePage().
type Page = { webContents: Electron.WebContents };

// Nothing can inject input into a Wayland compositor from a client, so only
// X11 is covered on Linux.
const hasRealInput =
  process.platform === 'win32' || process.platform === 'darwin' || (process.platform === 'linux' && !isWayland);
// { forward: true } is macOS and Windows only.
const canForward = process.platform === 'win32' || process.platform === 'darwin';
// Only these can ask the OS which window it hit tests at a point.
const canHitTest = process.platform === 'win32' || process.platform === 'linux';

// #target turns orange while hovered. Element.matches(':hover') is not usable
// here: it stays false while the (never focused) window is inactive even
// though the hover style is applied, so check the computed style instead.
const HOVER_COLOR = 'rgb(255, 136, 0)';
// body gets a colour of its own while hovered, to check that the cursor
// leaving the window also clears :hover on an element filling the window.
const BODY_HOVER_COLOR = 'rgb(1, 2, 3)';
const makePage = (target: { left: number; top: number; width: number; height: number }) => `
<style>
  html, body { margin: 0; height: 100%; background: #fff; }
  body:hover { color: ${BODY_HOVER_COLOR}; }
  #target { position: absolute; left: ${target.left}px; top: ${target.top}px; width: ${target.width}px; height: ${target.height}px; background: #08f; }
  #target:hover { background: ${HOVER_COLOR}; }
</style>
<div id="target"></div>
<script>
  window.events = [];
  for (const type of ['mouseenter', 'mouseleave']) {
    target.addEventListener(type, () => events.push({ type }));
  }
  for (const type of ['mousemove', 'mousedown']) {
    document.addEventListener(type, (e) => events.push({ type, x: e.clientX, y: e.clientY }));
  }
</script>
`;
const PAGE_HTML = makePage({ left: 50, top: 50, width: 300, height: 300 });
const PAGE = `data:text/html,${encodeURIComponent(PAGE_HTML)}`;

// Window placement in DIPs. OUTSIDE is well clear of the window, MARGIN is
// inside the window but outside #target.
const BOUNDS = { x: 100, y: 100, width: 400, height: 400 };
const OUTSIDE = { x: 30, y: 30 };
const MARGIN = { x: 20, y: 20 };
const TARGET_CENTER = { x: 200, y: 200 };

ifdescribe(hasRealInput && !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS)(
  'BrowserWindow.setIgnoreMouseEvents() with real mouse input',
  { tags: ['serial'] },
  () => {
    let mouse: MouseInput;
    let initialCursor: { x?: number; y?: number } | undefined;

    // Converts DIPs to the coordinates the addon takes: physical pixels on
    // Windows and X11, points (which are DIPs) on macOS.
    const toScreen = (dip: Electron.Point) => (process.platform === 'darwin' ? dip : screen.dipToScreenPoint(dip));
    const inWindow = ({ x, y }: Electron.Point) => ({ x: BOUNDS.x + x, y: BOUNDS.y + y });

    // Moves the real cursor to a point in DIPs and waits until the OS reports
    // it there.
    const moveTo = async (dip: Electron.Point, timeout?: number) => {
      const p = toScreen(dip);
      await mouse.move(p.x, p.y);
      await waitUntil(
        () => {
          const { x, y } = mouse.getCursorPos();
          return x === p.x && y === p.y;
        },
        { timeout }
      );
    };

    // Why input injection does not work on this machine, or undefined if it
    // does.
    const cannotInject = async () => {
      const diagnostics = mouse.getDiagnostics();
      if (process.platform === 'win32') {
        if (diagnostics.sendInputProbeSent !== 1 || diagnostics.inputDesktop !== 'Default') {
          return `SendInput cannot inject mouse input on this machine: ${JSON.stringify(diagnostics)}`;
        }
        return undefined;
      }
      if (!diagnostics.available) {
        return `mouse input is unavailable: ${JSON.stringify(diagnostics)}`;
      }
      // macOS drops posted events silently without the Accessibility
      // permission, so check that a move lands.
      try {
        await moveTo(OUTSIDE, 2000);
      } catch {
        return `injected moves do not move the cursor: ${JSON.stringify({ ...diagnostics, cursor: mouse.getCursorPos() })}`;
      }
      return undefined;
    };

    before(async function () {
      mouse = require('@electron-ci/mouse-input');
      initialCursor = mouse.getCursorPos();
      const reason = await cannotInject();
      if (reason) {
        // On CI this must not turn into a silent skip, or a runner image
        // change would quietly disable the suite.
        if (process.env.CI) throw new Error(reason);
        console.warn(`Skipping real mouse input specs. ${reason}`);
        this.skip();
      }
    });

    after(async () => {
      if (mouse && initialCursor?.x !== undefined && initialCursor?.y !== undefined) {
        await mouse.move(initialCursor.x, initialCursor.y);
      }
    });

    afterEach(closeAllWindows);

    const events = (page: Page): Promise<PageEvent[]> => page.webContents.executeJavaScript('events');
    const count = async (page: Page, type: string) => (await events(page)).filter((e) => e.type === type).length;
    const lastMove = async (page: Page) => (await events(page)).filter((e) => e.type === 'mousemove').at(-1);
    const clearEvents = (page: Page) => page.webContents.executeJavaScript('events.length = 0');
    const isHovered = (page: Page): Promise<boolean> =>
      page.webContents.executeJavaScript(
        `getComputedStyle(document.getElementById('target')).backgroundColor === '${HOVER_COLOR}'`
      );
    const isBodyHovered = (page: Page): Promise<boolean> =>
      page.webContents.executeJavaScript(`getComputedStyle(document.body).color === '${BODY_HOVER_COLOR}'`);

    // Moves the cursor to a point in window client coordinates and waits for
    // the forwarded mousemove to reach the page. |origin| is where the page
    // sits in the window, for a WebContentsView.
    const moveInWindow = async (page: Page, point: Electron.Point, origin: Electron.Point = { x: 0, y: 0 }) => {
      await moveTo(inWindow(point));
      const expected = { x: point.x - origin.x, y: point.y - origin.y };
      // Allow a pixel of DIP rounding on scaled displays.
      const near = (a: number | undefined, b: number) => a !== undefined && Math.abs(a - b) <= 1;
      try {
        await waitUntil(async () => {
          const move = await lastMove(page);
          return near(move?.x, expected.x) && near(move?.y, expected.y);
        });
      } catch {
        const move = await lastMove(page);
        throw new Error(
          `no mousemove at ${JSON.stringify(expected)} reached the page, last was ${JSON.stringify(move)}`
        );
      }
    };

    // Moves the cursor in steps, so the OS sees a realistic series of moves.
    const glide = async (from: Electron.Point, to: Electron.Point, steps = 5) => {
      for (let i = 1; i <= steps; i++) {
        await moveTo({
          x: Math.round(from.x + ((to.x - from.x) * i) / steps),
          y: Math.round(from.y + ((to.y - from.y) * i) / steps)
        });
      }
    };

    // Waits for |condition|; on timeout fails with what the OS hit tests at
    // the physical screen point |p|, since the usual cause is another window
    // covering ours.
    const waitUntilAt = async (p: Electron.Point, what: string, condition: () => boolean | Promise<boolean>) => {
      try {
        await waitUntil(condition);
      } catch {
        const atPoint = JSON.stringify(mouse.describeWindowAtPoint(p.x, p.y));
        throw new Error(`${what} did not happen; window at ${JSON.stringify(p)}: ${atPoint}`);
      }
    };

    const createWindow = async (options: Electron.BrowserWindowConstructorOptions = {}) => {
      const w = new BrowserWindow({ ...BOUNDS, frame: false, show: false, useContentSize: true, ...options });
      await w.loadURL(PAGE);
      return w;
    };

    // The first moves after showing a window, or after its web contents were
    // replaced, do not always arrive (seen on windows-11-arm), so nudge the
    // cursor in the MARGIN until one reaches |page|, then leave it in the
    // MARGIN with the page's event log cleared.
    const warmUp = async (page: Page) => {
      let offset = 0;
      await waitUntil(
        async () => {
          offset = offset === 0 ? 2 : 0;
          await moveTo(inWindow({ x: MARGIN.x + offset, y: MARGIN.y + offset }));
          return (await count(page, 'mousemove')) > 0;
        },
        { rate: 100 }
      );
      await moveInWindow(page, MARGIN);
      await clearEvents(page);
    };

    // Shows |w| frameless and always on top, and makes it forward mouse moves
    // while ignoring them.
    //
    // On Windows the window stays inactive: the hook forwards moves to any
    // window. On macOS it has to be the key window, as Chromium drops
    // mouse-moved events in inactive windows (RenderWidgetHostViewCocoa's
    // -shouldIgnoreMouseEvent:).
    const showForwarding = async (w: BaseWindow) => {
      w.setAlwaysOnTop(true);
      if (process.platform === 'darwin') {
        const focused = once(w, 'focus');
        w.show();
        await focused;
      } else {
        w.showInactive();
      }
      w.setIgnoreMouseEvents(true, { forward: true });
    };

    // Shows a window that forwards mouse moves while ignoring them, and leaves
    // the cursor in its MARGIN once the page has seen a forwarded move.
    const createForwardingWindow = async (options: Electron.BrowserWindowConstructorOptions = {}) => {
      await moveTo(OUTSIDE);
      const w = await createWindow(options);
      await showForwarding(w);
      await warmUp(w);
      return w;
    };

    // Moves from the MARGIN into #target and then around inside it, checking
    // that every move is forwarded and that #target is entered exactly once.
    const expectForwardsWithoutFlapping = async (w: Page) => {
      for (let i = 0; i < 10; i++) {
        await moveInWindow(w, { x: 100 + i * 10, y: 100 + i * 10 });
      }
      await waitUntil(() => isHovered(w));
      expect(await count(w, 'mouseenter')).to.equal(1);
      expect(await count(w, 'mouseleave')).to.equal(0);
    };

    ifit(canHitTest)('removes the window from OS hit testing', async () => {
      await moveTo(OUTSIDE);
      const w = await createWindow();
      w.setAlwaysOnTop(true);
      w.showInactive();
      const p = toScreen(inWindow(TARGET_CENTER));
      // Also confirms the window is on screen before it starts ignoring.
      await waitUntilAt(p, 'hit testing the window before it ignores', () =>
        mouse.isWindowAtPoint(w.getNativeWindowHandle(), p.x, p.y)
      );
      w.setIgnoreMouseEvents(true, { forward: canForward });
      // Synchronous on both: Windows changes the extended style in place, and
      // on X11 the new input shape is applied when the call returns.
      expect(mouse.isWindowAtPoint(w.getNativeWindowHandle(), p.x, p.y)).to.be.false();
    });

    // Only clicks are checked in the window below. Hover there is not: on X11
    // (seen with Electron 44 under Xvfb) a window below an ignoring Electron
    // window gets no mousemove until it is clicked, although the X server
    // delivers the motion events to it.
    it('passes clicks through to the window below', async () => {
      await moveTo(OUTSIDE);
      // acceptFirstMouse: macOS only delivers a click that activates an
      // inactive window when the window asks for it.
      const below = await createWindow({ acceptFirstMouse: true });
      // Topmost like the window above it (which is shown later, so stays on
      // top of it), so that no ordinary window can come between them and
      // take the click: on windows-11-arm the runner's terminal once did.
      below.setAlwaysOnTop(true);
      below.showInactive();
      const above = await createWindow();
      above.setAlwaysOnTop(true);
      above.showInactive();
      above.setIgnoreMouseEvents(true);

      await glide(OUTSIDE, inWindow(TARGET_CENTER));
      // Where the OS can say, make sure the click lands in the window below
      // rather than in whatever else might cover the point (on CI a system
      // window once did, and the click launched an app from it).
      if (canHitTest) {
        const p = toScreen(inWindow(TARGET_CENTER));
        await waitUntilAt(p, 'the window below being hit tested under the ignoring window', () =>
          mouse.isWindowAtPoint(below.getNativeWindowHandle(), p.x, p.y)
        );
      }
      await mouse.click();
      await waitUntilAt(
        toScreen(inWindow(TARGET_CENTER)),
        'a mousedown in the window below',
        async () => (await count(below, 'mousedown')) > 0
      );
      expect(await count(above, 'mousedown')).to.equal(0);
    });

    ifit(canForward)('forwards mouse moves to the page', async () => {
      const w = await createForwardingWindow();
      await glide(inWindow(MARGIN), inWindow(TARGET_CENTER));
      await moveInWindow(w, { x: 210, y: 220 });
      await waitUntil(() => isHovered(w));
      expect(await count(w, 'mouseenter')).to.equal(1);
    });

    // https://github.com/electron/electron/issues/30808
    ifit(canForward)('does not oscillate mouseenter/mouseleave while moving inside an element', async () => {
      const w = await createForwardingWindow();
      await expectForwardsWithoutFlapping(w);
    });

    // Leaves #target through the window's margin. Leaving the window straight
    // from #target is a different case: only moves inside the window are
    // forwarded and on Windows WM_MOUSELEAVE is swallowed while forwarding,
    // see https://github.com/electron/electron/issues/51521.
    ifit(canForward)('clears :hover and fires one mouseleave when the cursor moves off an element', async () => {
      const w = await createForwardingWindow();
      await glide(inWindow(MARGIN), inWindow(TARGET_CENTER));
      await waitUntil(() => isHovered(w));
      await glide(inWindow(TARGET_CENTER), inWindow(MARGIN));
      await moveInWindow(w, MARGIN);
      await waitUntil(async () => !(await isHovered(w)));
      await moveTo(OUTSIDE);
      expect(await count(w, 'mouseenter')).to.equal(1);
      expect(await count(w, 'mouseleave')).to.equal(1);
    });

    // Regression cases for the Windows forwarding rewrite in
    // https://github.com/electron/electron/pull/52633. Forwarding there feeds
    // the low level hook's moves into the aura window tree and tracks the
    // enter/leave itself, instead of posting WM_MOUSEMOVE to the first
    // Chrome_RenderWidgetHostHWND the window ever had.
    ifdescribe(process.platform === 'win32')('forwarding on Windows', () => {
      // Waits until nothing more arrives, so that "exactly one" counts also
      // catch a late duplicate.
      const settle = () => setTimeout(300);

      // Nudges the cursor around |point| (window client coordinates) until
      // |condition| holds: the first moves after showing a window do not
      // always arrive.
      const nudgeUntil = async (point: Electron.Point, condition: () => Promise<boolean>) => {
        let offset = 0;
        await waitUntil(
          async () => {
            offset = offset === 0 ? 2 : 0;
            await moveTo(inWindow({ x: point.x + offset, y: point.y + offset }));
            return condition();
          },
          { rate: 100 }
        );
      };

      // Shows a window below the one under test to catch the clicks that go
      // through it, so that they never land on the desktop. Not focusable, so
      // that the click does not activate it and raise it above the window
      // under test.
      const createClickCatcher = async () => {
        const below = await createWindow({ focusable: false });
        below.setAlwaysOnTop(true);
        below.showInactive();
        return below;
      };

      const crashAndReload = async (w: BrowserWindow) => {
        const gone = once(w.webContents, 'render-process-gone');
        w.webContents.forcefullyCrashRenderer();
        await gone;
        const loaded = once(w.webContents, 'did-finish-load');
        w.webContents.reload();
        await loaded;
      };

      // https://github.com/electron/electron/issues/49982: the renderer's
      // replacement Chrome_RenderWidgetHostHWND was never forwarded to.
      it('keeps forwarding after the renderer crashes and the page is reloaded (#49982)', async function () {
        this.timeout(60000);
        const w = await createForwardingWindow();
        await crashAndReload(w);
        await warmUp(w);
        await expectForwardsWithoutFlapping(w);
      });

      // https://github.com/electron/electron/issues/15376
      it('keeps forwarding after the page is reloaded (#15376)', async function () {
        this.timeout(60000);
        const w = await createForwardingWindow();
        const loaded = once(w.webContents, 'did-finish-load');
        w.webContents.reload();
        await loaded;
        await warmUp(w);
        await expectForwardsWithoutFlapping(w);
      });

      // https://github.com/electron/electron/issues/49982: after a click in
      // the click-through area mouseleave stopped firing.
      it('fires mouseleave after a click-through click once the renderer crashed and reloaded (#49982)', async function () {
        this.timeout(60000);
        await moveTo(OUTSIDE);
        const below = await createClickCatcher();
        const w = await createForwardingWindow();
        await crashAndReload(w);
        await warmUp(w);

        await glide(inWindow(MARGIN), inWindow(TARGET_CENTER));
        await moveInWindow(w, TARGET_CENTER);
        await waitUntil(() => isHovered(w));

        const p = toScreen(inWindow(TARGET_CENTER));
        await waitUntilAt(p, 'the window below being hit tested under the forwarding window', () =>
          mouse.isWindowAtPoint(below.getNativeWindowHandle(), p.x, p.y)
        );
        await mouse.click();
        await waitUntilAt(p, 'a mousedown in the window below', async () => (await count(below, 'mousedown')) > 0);
        expect(await count(w, 'mousedown')).to.equal(0);

        await glide(inWindow(TARGET_CENTER), inWindow(MARGIN));
        await moveInWindow(w, MARGIN);
        await waitUntil(async () => !(await isHovered(w)));
        await settle();
        expect(await count(w, 'mouseenter')).to.equal(1);
        expect(await count(w, 'mouseleave')).to.equal(1);
      });

      // https://github.com/electron/electron/issues/51521: the leaves
      // swallowed while forwarding left Chromium's one-shot TME_LEAVE
      // tracking disarmed, so :hover stuck once the window stopped ignoring.
      it('fires mouseleave when the cursor leaves the window after setIgnoreMouseEvents(false) (#51521)', async () => {
        const w = await createForwardingWindow();
        await glide(inWindow(MARGIN), inWindow(TARGET_CENTER));
        await moveInWindow(w, TARGET_CENTER);
        await waitUntil(() => isHovered(w));

        w.setIgnoreMouseEvents(false);
        // A move the window now gets from the OS itself.
        await moveInWindow(w, { x: TARGET_CENTER.x + 10, y: TARGET_CENTER.y + 10 });
        await moveTo(OUTSIDE);
        await waitUntil(async () => !(await isHovered(w)));
        await waitUntil(async () => !(await isBodyHovered(w)));
        await settle();
        expect(await count(w, 'mouseenter')).to.equal(1);
        expect(await count(w, 'mouseleave')).to.equal(1);
      });

      // https://github.com/electron/electron/issues/51521: only moves inside
      // the window were forwarded, so leaving it straight from an element
      // never cleared :hover.
      it('clears :hover when the cursor leaves the window straight from an element (#51521)', async () => {
        const w = await createForwardingWindow();
        expect(await isBodyHovered(w)).to.be.true();
        await glide(inWindow(MARGIN), inWindow(TARGET_CENTER));
        await moveInWindow(w, TARGET_CENTER);
        await waitUntil(() => isHovered(w));

        // One move, so the hook never reports a position in the MARGIN.
        await moveTo(OUTSIDE);
        await waitUntil(async () => !(await isHovered(w)));
        await waitUntil(async () => !(await isBodyHovered(w)));
        await settle();
        expect(await count(w, 'mouseenter')).to.equal(1);
        expect(await count(w, 'mouseleave')).to.equal(1);
      });

      // A cross-site navigation swaps the renderer, and with it the
      // Chrome_RenderWidgetHostHWND. The two servers are on different ports,
      // and one is reached as localhost, so the navigation is cross-site and
      // not only cross-origin.
      it('keeps forwarding after a cross-origin navigation', async function () {
        this.timeout(60000);
        const serve = async (host: string) => {
          const server = http.createServer((_req, res) => {
            res.setHeader('Content-Type', 'text/html');
            res.end(PAGE_HTML);
          });
          defer(() => server.close());
          const { port } = await listen(server);
          return `http://${host}:${port}/`;
        };
        const first = await serve('127.0.0.1');
        const second = await serve('localhost');

        await moveTo(OUTSIDE);
        const w = new BrowserWindow({ ...BOUNDS, frame: false, show: false, useContentSize: true });
        await w.loadURL(first);
        await showForwarding(w);
        await warmUp(w);
        const firstPid = w.webContents.getOSProcessId();

        await w.loadURL(second);
        expect(w.webContents.getOSProcessId()).to.not.equal(firstPid);
        await warmUp(w);
        await expectForwardsWithoutFlapping(w);
      });

      // https://github.com/electron/electron/issues/51521: enabling
      // forwarding while an element is hovered, then leaving the window.
      it('fires one mouseleave when forwarding starts over a hovered element and the cursor leaves (#51521)', async () => {
        await moveTo(OUTSIDE);
        const w = await createWindow();
        w.setAlwaysOnTop(true);
        w.showInactive();
        // Not ignoring yet: the window gets the moves from the OS.
        await nudgeUntil(TARGET_CENTER, () => isHovered(w));
        await clearEvents(w);

        w.setIgnoreMouseEvents(true, { forward: true });
        await moveTo(OUTSIDE);
        await waitUntil(async () => !(await isHovered(w)));
        await settle();
        expect(await count(w, 'mouseenter')).to.equal(0);
        expect(await count(w, 'mouseleave')).to.equal(1);
      });

      // https://github.com/electron/electron/issues/30808: forwarding enabled
      // before the page (and its Chrome_RenderWidgetHostHWND) exists.
      it('forwards without flapping when enabled before loadURL() (#30808)', async function () {
        this.timeout(60000);
        await moveTo(OUTSIDE);
        const w = new BrowserWindow({ ...BOUNDS, frame: false, show: false, useContentSize: true });
        await showForwarding(w);
        await w.loadURL(PAGE);
        await warmUp(w);
        await expectForwardsWithoutFlapping(w);
      });

      // https://github.com/electron/electron/issues/30808 and
      // https://github.com/electron/electron/issues/49982: the usual pattern
      // of an overlay that stops ignoring while an element is hovered.
      it('toggles cleanly when the page switches ignoring from its mouseenter/mouseleave handlers (#30808, #49982)', async function () {
        this.timeout(90000);
        const w = await createForwardingWindow({ webPreferences: { nodeIntegration: true, contextIsolation: false } });
        let ignoring = true;
        w.webContents.ipc.on('set-ignore', (_event, ignore: boolean) => {
          ignoring = ignore;
          if (ignore) {
            w.setIgnoreMouseEvents(true, { forward: true });
          } else {
            w.setIgnoreMouseEvents(false);
          }
        });
        await w.webContents.executeJavaScript(`{
          const { ipcRenderer } = require('electron');
          target.addEventListener('mouseenter', () => ipcRenderer.send('set-ignore', false));
          target.addEventListener('mouseleave', () => ipcRenderer.send('set-ignore', true));
        }`);

        for (let i = 1; i <= 3; i++) {
          await glide(inWindow(MARGIN), inWindow(TARGET_CENTER));
          await waitUntil(async () => !ignoring && (await isHovered(w)));
          await glide(inWindow(TARGET_CENTER), inWindow(MARGIN));
          await waitUntil(async () => ignoring && !(await isHovered(w)));
          // Forwarding picks up again in the MARGIN.
          await moveInWindow(w, { x: MARGIN.x + 5, y: MARGIN.y + 5 });
          expect(await count(w, 'mouseenter')).to.equal(i, `mouseenter count after pass ${i}`);
          expect(await count(w, 'mouseleave')).to.equal(i, `mouseleave count after pass ${i}`);
        }
        await settle();
        expect(await count(w, 'mouseenter')).to.equal(3);
        expect(await count(w, 'mouseleave')).to.equal(3);
        expect(await isHovered(w)).to.be.false();
      });

      // Every WebContentsView has a Chrome_RenderWidgetHostHWND of its own,
      // and only the first one a window ever had was forwarded to.
      it('forwards to the WebContentsView under the cursor in a BaseWindow', async function () {
        this.timeout(60000);
        await moveTo(OUTSIDE);
        const w = new BaseWindow({ ...BOUNDS, frame: false, show: false });
        const viewPage = `data:text/html,${encodeURIComponent(makePage({ left: 50, top: 50, width: 100, height: 300 }))}`;
        const half = BOUNDS.width / 2;
        const views = [0, half].map((x) => {
          const view = new WebContentsView();
          defer(() => {
            if (!view.webContents.isDestroyed()) view.webContents.destroy();
          });
          view.setBounds({ x, y: 0, width: half, height: BOUNDS.height });
          w.contentView.addChildView(view);
          return view;
        });
        await Promise.all(views.map((view) => view.webContents.loadURL(viewPage)));
        await showForwarding(w);
        // MARGIN is in the first view.
        await warmUp(views[0]);

        const centers = [
          { x: 100, y: 200 },
          { x: half + 100, y: 200 }
        ];
        await glide(inWindow(MARGIN), inWindow(centers[0]));
        await moveInWindow(views[0], centers[0]);
        await waitUntil(() => isHovered(views[0]));

        await glide(inWindow(centers[0]), inWindow(centers[1]));
        await moveInWindow(views[1], { x: centers[1].x + 10, y: centers[1].y }, { x: half, y: 0 });
        await waitUntil(async () => (await isHovered(views[1])) && !(await isHovered(views[0])));

        await moveTo(OUTSIDE);
        await waitUntil(async () => !(await isHovered(views[1])));
        await settle();
        for (const view of views) {
          expect(await count(view, 'mouseenter')).to.equal(1);
          expect(await count(view, 'mouseleave')).to.equal(1);
        }
      });
    });
  }
);
