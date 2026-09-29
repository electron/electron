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
import { BrowserWindow, screen } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import { createRequire } from 'node:module';

import { ifdescribe, ifit, isWayland, waitUntil } from './lib/spec-helpers.ts';
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
const PAGE = `data:text/html,${encodeURIComponent(`
<style>
  html, body { margin: 0; height: 100%; background: #fff; }
  #target { position: absolute; left: 50px; top: 50px; width: 300px; height: 300px; background: #08f; }
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
`)}`;

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

    const events = (w: BrowserWindow): Promise<PageEvent[]> => w.webContents.executeJavaScript('events');
    const count = async (w: BrowserWindow, type: string) => (await events(w)).filter((e) => e.type === type).length;
    const lastMove = async (w: BrowserWindow) => (await events(w)).filter((e) => e.type === 'mousemove').at(-1);
    const isHovered = (w: BrowserWindow): Promise<boolean> =>
      w.webContents.executeJavaScript(
        `getComputedStyle(document.getElementById('target')).backgroundColor === '${HOVER_COLOR}'`
      );

    // Moves the cursor to a point in window client coordinates and waits for
    // the forwarded mousemove to reach the page.
    const moveInWindow = async (w: BrowserWindow, point: Electron.Point) => {
      await moveTo(inWindow(point));
      // Allow a pixel of DIP rounding on scaled displays.
      const near = (a: number | undefined, b: number) => a !== undefined && Math.abs(a - b) <= 1;
      try {
        await waitUntil(async () => {
          const move = await lastMove(w);
          return near(move?.x, point.x) && near(move?.y, point.y);
        });
      } catch {
        const move = await lastMove(w);
        throw new Error(`no mousemove at ${JSON.stringify(point)} reached the page, last was ${JSON.stringify(move)}`);
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

    // Shows a frameless, always-on-top window that forwards mouse moves while
    // ignoring them, and leaves the cursor in its MARGIN once the page has
    // seen a forwarded move.
    //
    // On Windows the window stays inactive: the hook forwards moves to any
    // window. On macOS it has to be the key window, as Chromium drops
    // mouse-moved events in inactive windows (RenderWidgetHostViewCocoa's
    // -shouldIgnoreMouseEvent:).
    //
    // The first moves after showing a window do not always arrive (seen on
    // windows-11-arm), so nudge the cursor until one does before handing the
    // window to the test.
    const createForwardingWindow = async () => {
      await moveTo(OUTSIDE);
      const w = await createWindow();
      w.setAlwaysOnTop(true);
      if (process.platform === 'darwin') {
        const focused = once(w, 'focus');
        w.show();
        await focused;
      } else {
        w.showInactive();
      }
      w.setIgnoreMouseEvents(true, { forward: true });

      let offset = 0;
      await waitUntil(
        async () => {
          offset = offset === 0 ? 2 : 0;
          await moveTo(inWindow({ x: MARGIN.x + offset, y: MARGIN.y + offset }));
          return (await count(w, 'mousemove')) > 0;
        },
        { rate: 100 }
      );
      await moveInWindow(w, MARGIN);
      await w.webContents.executeJavaScript('events.length = 0');
      return w;
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
      for (let i = 0; i < 10; i++) {
        await moveInWindow(w, { x: 100 + i * 10, y: 100 + i * 10 });
      }
      expect(await count(w, 'mouseenter')).to.equal(1);
      expect(await count(w, 'mouseleave')).to.equal(0);
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
  }
);
