// End-to-end coverage for setIgnoreMouseEvents(true, { forward: true }) on
// Windows, driven by real OS input (see spec/fixtures/native-addon/mouse-input).
//
// Why real input: a forwarding window is WS_EX_LAYERED | WS_EX_TRANSPARENT, so
// the OS never delivers mouse messages to it. NativeWindowViews gets the
// cursor position from a WH_MOUSE_LL hook instead. Only SendInput() (or a
// physical mouse) goes through low level hooks; webContents.sendInputEvent()
// feeds Blink directly and PostMessage() to the HWND skips the hook, so
// neither exercises the forwarding path.
import { BrowserWindow, screen } from 'electron/main';

import { expect } from 'chai';

import { createRequire } from 'node:module';

import { ifdescribe, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

const require = createRequire(import.meta.url);

type MouseInput = {
  move(x: number, y: number): Promise<unknown>;
  getCursorPos(): { x?: number; y?: number; error?: number };
  isWindowAtPoint(handle: Buffer, x: number, y: number): boolean;
  getDiagnostics(): { sendInputProbeSent?: number; inputDesktop?: string } & Record<string, unknown>;
};

type PageEvent = { type: string; x?: number; y?: number };

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
  document.addEventListener('mousemove', (e) => events.push({ type: 'mousemove', x: e.clientX, y: e.clientY }));
</script>
`)}`;

// Window placement in DIPs. OUTSIDE is well clear of the window, MARGIN is
// inside the window but outside #target.
const BOUNDS = { x: 100, y: 100, width: 400, height: 400 };
const OUTSIDE = { x: 30, y: 30 };
const MARGIN = { x: 20, y: 20 };
const TARGET_CENTER = { x: 200, y: 200 };

ifdescribe(process.platform === 'win32' && !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS)(
  'BrowserWindow.setIgnoreMouseEvents(true, { forward: true }) with real mouse input',
  { tags: ['serial'] },
  () => {
    let mouse: MouseInput;
    let initialCursor: { x?: number; y?: number } | undefined;

    before(function () {
      mouse = require('@electron-ci/mouse-input');
      const diagnostics = mouse.getDiagnostics();
      if (diagnostics.sendInputProbeSent !== 1 || diagnostics.inputDesktop !== 'Default') {
        const reason = `SendInput cannot inject mouse input on this machine: ${JSON.stringify(diagnostics)}`;
        // On CI this must not turn into a silent skip, or a runner image
        // change would quietly disable the suite.
        if (process.env.CI) throw new Error(reason);
        console.warn(`Skipping real mouse input specs. ${reason}`);
        this.skip();
      }
      initialCursor = mouse.getCursorPos();
    });

    after(async () => {
      if (mouse && initialCursor?.x !== undefined && initialCursor?.y !== undefined) {
        await mouse.move(initialCursor.x, initialCursor.y);
      }
    });

    afterEach(closeAllWindows);

    const toScreen = (dip: Electron.Point) => screen.dipToScreenPoint(dip);
    const inWindow = ({ x, y }: Electron.Point) => ({ x: BOUNDS.x + x, y: BOUNDS.y + y });

    // Moves the real cursor to a point in DIPs and waits until the OS reports
    // it there.
    const moveTo = async (dip: Electron.Point) => {
      const p = toScreen(dip);
      await mouse.move(p.x, p.y);
      await waitUntil(() => {
        const { x, y } = mouse.getCursorPos();
        return x === p.x && y === p.y;
      });
    };

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

    // Moves the cursor in steps, so the hook sees a realistic series of moves.
    const glide = async (from: Electron.Point, to: Electron.Point, steps = 5) => {
      for (let i = 1; i <= steps; i++) {
        await moveTo({
          x: Math.round(from.x + ((to.x - from.x) * i) / steps),
          y: Math.round(from.y + ((to.y - from.y) * i) / steps)
        });
      }
    };

    // Shows a frameless, inactive, always-on-top window that forwards mouse
    // moves while ignoring them, and leaves the cursor in its MARGIN once the
    // page has seen a forwarded move. The first moves after showing a window
    // do not always arrive (seen on windows-11-arm), so nudge the cursor until
    // one does before handing the window to the test.
    const createForwardingWindow = async () => {
      await moveTo(OUTSIDE);
      const w = new BrowserWindow({ ...BOUNDS, frame: false, show: false, useContentSize: true });
      await w.loadURL(PAGE);
      w.setAlwaysOnTop(true);
      w.showInactive();
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

    it('makes the window click-through, so only the mouse hook sees the cursor', async () => {
      const w = await createForwardingWindow();
      const p = toScreen(inWindow(TARGET_CENTER));
      expect(mouse.isWindowAtPoint(w.getNativeWindowHandle(), p.x, p.y)).to.be.false();
    });

    it('forwards mouse moves to the page', async () => {
      const w = await createForwardingWindow();
      await glide(inWindow(MARGIN), inWindow(TARGET_CENTER));
      await moveInWindow(w, { x: 210, y: 220 });
      await waitUntil(() => isHovered(w));
      expect(await count(w, 'mouseenter')).to.equal(1);
    });

    // https://github.com/electron/electron/issues/30808
    it('does not oscillate mouseenter/mouseleave while moving inside an element', async () => {
      const w = await createForwardingWindow();
      for (let i = 0; i < 10; i++) {
        await moveInWindow(w, { x: 100 + i * 10, y: 100 + i * 10 });
      }
      expect(await count(w, 'mouseenter')).to.equal(1);
      expect(await count(w, 'mouseleave')).to.equal(0);
    });

    // Leaves #target through the window's margin. Leaving the window straight
    // from #target is a different case: only moves inside the window are
    // forwarded and WM_MOUSELEAVE is swallowed while forwarding, see
    // https://github.com/electron/electron/issues/51521.
    it('clears :hover and fires one mouseleave when the cursor moves off an element', async () => {
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
