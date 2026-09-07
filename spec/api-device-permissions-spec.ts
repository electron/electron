import { BrowserWindow, session, WebFrameMain, Session, WebContents } from 'electron/main';

import { expect } from 'chai';

import * as http from 'node:http';
import { setTimeout } from 'node:timers/promises';

import { listen, waitUntil } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

// These specs drive WebHID, WebUSB and Web Serial through in-process fake
// device managers (see shell/browser/testing/fake_device_managers.h) so that
// the permission, chooser, grant and revocation paths run in CI without
// hardware. What they pin down, for every API and frame topology: the frame
// handed to the app is the frame that asked.
const testing = process._linkedBinding('electron_common_testing');

type Kind = 'hid' | 'usb' | 'serial';

const KINDS: {
  kind: Kind;
  request: string;
  get: string;
  selectEvent: 'select-hid-device' | 'select-usb-device' | 'select-serial-port';
  revokedEvent: 'hid-device-revoked' | 'usb-device-revoked' | 'serial-port-revoked';
  add: (ses: Session, opts?: any) => string;
  remove: (ses: Session, id: string) => void;
}[] = [
  {
    kind: 'hid',
    request: 'navigator.hid.requestDevice({ filters: [] }).then(d => d[0])',
    get: 'navigator.hid.getDevices()',
    selectEvent: 'select-hid-device',
    revokedEvent: 'hid-device-revoked',
    add: (ses, opts = {}) =>
      testing.addFakeHidDevice(ses, { vendorId: 0x1234, productId: 0x0001, name: 'Fake Gamepad', ...opts }),
    remove: (ses, id) => testing.removeFakeHidDevice(ses, id)
  },
  {
    kind: 'usb',
    request: 'navigator.usb.requestDevice({ filters: [] })',
    get: 'navigator.usb.getDevices()',
    selectEvent: 'select-usb-device',
    revokedEvent: 'usb-device-revoked',
    add: (ses, opts = {}) =>
      testing.addFakeUsbDevice(ses, { vendorId: 0x1234, productId: 0x0002, productName: 'Fake Stick', ...opts }),
    remove: (ses, id) => testing.removeFakeUsbDevice(ses, id)
  },
  {
    kind: 'serial',
    request: 'navigator.serial.requestPort({})',
    get: 'navigator.serial.getPorts()',
    selectEvent: 'select-serial-port',
    revokedEvent: 'serial-port-revoked',
    add: (ses, opts = {}) =>
      testing.addFakeSerialPort(ses, { path: '/dev/ttyFAKE0', vendorId: 0x1234, productId: 0x0003, ...opts }),
    remove: (ses, id) => testing.removeFakeSerialPort(ses, id)
  }
];

// Normalises the three chooser events to { frame, pick(first|none) }.
function onSelect(
  ses: Session,
  kind: (typeof KINDS)[number],
  handler: (info: { frame: WebFrameMain | null; webContents: WebContents | null; ids: string[] }) => string | undefined
) {
  const listener = (event: Electron.Event, ...args: any[]) => {
    event.preventDefault();
    if (kind.kind === 'serial') {
      const [portList, webContents, callback, frame] = args;
      callback(handler({ frame, webContents, ids: portList.map((p: any) => p.portId) }) ?? '');
    } else {
      const [details, callback] = args;
      callback(
        handler({ frame: details.frame, webContents: null, ids: details.deviceList.map((d: any) => d.deviceId) })
      );
    }
  };
  ses.on(kind.selectEvent as any, listener);
  return () => ses.removeListener(kind.selectEvent as any, listener);
}

describe('device permission attribution (hid / usb / serial)', () => {
  let serverA: http.Server;
  let serverB: http.Server;
  let urlA: string; // top-level
  let urlB: string; // cross-origin (different host)

  before(async () => {
    const handler = (req: http.IncomingMessage, res: http.ServerResponse) => {
      res.setHeader('Content-Type', 'text/html');
      const u = new URL(req.url!, 'http://x');
      if (u.pathname === '/leaf') {
        res.end('<!doctype html><p>leaf</p>');
      } else {
        const sandbox = u.searchParams.get('sandbox');
        res.end(
          `<!doctype html><iframe allow="hid; usb; serial" ${sandbox ? `sandbox="${sandbox}"` : ''} src="${urlB}/leaf"></iframe>`
        );
      }
    };
    serverA = http.createServer(handler);
    serverB = http.createServer(handler);
    urlA = (await listen(serverA)).url;
    urlB = (await listen(serverB)).url.replace('127.0.0.1', 'localhost');
  });

  after(() => {
    serverA.close();
    serverB.close();
  });

  let counter = 0;
  let ses: Session;
  beforeEach(() => {
    ses = session.fromPartition(`device-permissions-${++counter}`);
    testing.useFakeDeviceManagers(ses);
  });

  afterEach(async () => {
    ses.setPermissionCheckHandler(null);
    ses.setDevicePermissionHandler(null);
    for (const k of KINDS) ses.removeAllListeners(k.selectEvent);
    await closeAllWindows();
  });

  type Topology = 'main frame' | 'cross-origin iframe' | 'sandboxed iframe';
  async function open(topology: Topology) {
    const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    if (topology === 'main frame') {
      await w.loadURL(`${urlA}/leaf`);
      return { w, frame: w.webContents.mainFrame };
    }
    await w.loadURL(`${urlA}/?${topology === 'sandboxed iframe' ? 'sandbox=allow-scripts' : ''}`);
    await waitUntil(
      () => w.webContents.mainFrame.frames.length === 1 && w.webContents.mainFrame.frames[0].url.endsWith('/leaf')
    );
    return { w, frame: w.webContents.mainFrame.frames[0] };
  }

  const run = (frame: WebFrameMain, code: string) => frame.executeJavaScript(`(async () => { ${code} })()`, true);

  for (const kind of KINDS) {
    describe(kind.kind, () => {
      for (const topology of ['main frame', 'cross-origin iframe', 'sandboxed iframe'] as Topology[]) {
        it(`attributes the permission check, chooser and device check to the requesting ${topology}`, async () => {
          kind.add(ses);
          const { w, frame } = await open(topology);
          const expected = {
            url: frame.url,
            origin: frame.origin,
            // An opaque origin has no URL form; the app reads frame.origin === 'null'.
            originUrl: frame.origin === 'null' ? '' : `${frame.origin}/`,
            isMainFrame: frame === w.webContents.mainFrame
          };

          const checks: any[] = [];
          ses.setPermissionCheckHandler((wc, permission, requestingOrigin, details) => {
            if (permission === kind.kind) checks.push({ wc, requestingOrigin, details });
            return true;
          });
          const selects: any[] = [];
          onSelect(ses, kind, (info) => {
            selects.push(info);
            return info.ids[0];
          });
          const deviceChecks: any[] = [];
          ses.setDevicePermissionHandler((details) => {
            if (details.deviceType === kind.kind) deviceChecks.push(details);
            return !!(details as any).selected;
          });

          const opened = await run(
            frame,
            `const d = await ${kind.request}; await d.open(${kind.kind === 'serial' ? '{ baudRate: 9600 }' : ''}); return ${kind.kind === 'serial' ? '!!d.readable' : 'd.opened'};`
          );
          expect(opened).to.equal(true);

          expect(checks).to.not.be.empty();
          for (const c of checks) {
            expect(c.wc).to.equal(w.webContents);
            expect(c.requestingOrigin).to.equal(expected.originUrl);
            expect(c.details.requestingUrl).to.equal(expected.url);
            expect(c.details.isMainFrame).to.equal(expected.isMainFrame);
            expect(c.details.frame).to.equal(frame);
            expect(c.details.embeddingOrigin).to.equal(`${w.webContents.mainFrame.origin}/`);
          }
          expect(selects).to.have.lengthOf(1);
          expect(selects[0].frame).to.equal(frame);
          expect(selects[0].ids).to.have.lengthOf(1);
          if (kind.kind === 'serial') expect(selects[0].webContents).to.equal(w.webContents);

          expect(deviceChecks).to.not.be.empty();
          for (const d of deviceChecks) {
            expect(d.origin).to.equal(expected.origin);
            expect(d.frame).to.equal(frame);
            expect(d.selected).to.equal(true);
          }
          // The handler can correlate with what the chooser reported.
          const idKey = kind.kind === 'serial' ? 'portId' : 'deviceId';
          expect(deviceChecks[0].device[idKey]).to.equal(selects[0].ids[0]);
        });
      }

      it('without a device permission handler, a chooser selection grants access and forget() revokes it for the requesting origin', async () => {
        kind.add(ses);
        const { frame } = await open('cross-origin iframe');
        onSelect(ses, kind, (info) => info.ids[0]);
        const revoked: any[] = [];
        ses.on(kind.revokedEvent as any, (_e: any, details: any) => revoked.push(details));

        expect(await run(frame, `const d = await ${kind.request}; window.d = d; return !!d;`)).to.equal(true);
        expect(await run(frame, `return (await ${kind.get}).length`)).to.equal(1);
        await run(frame, 'await window.d.forget()');
        expect(await run(frame, `return (await ${kind.get}).length`)).to.equal(0);
        expect(revoked).to.have.lengthOf(1);
        // Attributed to the iframe's origin, not the embedder's.
        expect(revoked[0].origin).to.equal(frame.origin);
        if (kind.kind === 'serial') expect(revoked[0].frame).to.equal(frame);
      });

      it('a device permission handler is consulted for chooser-selected devices and can veto them', async () => {
        kind.add(ses);
        const { frame } = await open('main frame');
        onSelect(ses, kind, (info) => info.ids[0]);
        let allow = true;
        const seen: any[] = [];
        ses.setDevicePermissionHandler((details) => {
          seen.push(details);
          return allow && !!(details as any).selected;
        });
        expect(await run(frame, `const d = await ${kind.request}; window.d = d; return !!d;`)).to.equal(true);
        expect(await run(frame, `return (await ${kind.get}).length`)).to.equal(1);
        expect(seen.every((d) => d.selected === true)).to.equal(true);
        allow = false;
        expect(await run(frame, `return (await ${kind.get}).length`)).to.equal(0);
        if (kind.kind !== 'usb') {
          // HID and serial re-check on open(); a WebUSB device object holds its
          // connection from creation, so only enumeration is affected there.
          const openResult = await run(
            frame,
            `try { await window.d.open(${kind.kind === 'serial' ? '{ baudRate: 9600 }' : ''}); return 'opened'; } catch (e) { return e.name; }`
          );
          expect(openResult).to.not.equal('opened');
        }
      });

      it('denying the permission check cuts off access to already-granted devices', async () => {
        kind.add(ses);
        const { frame } = await open('main frame');
        onSelect(ses, kind, (info) => info.ids[0]);
        expect(await run(frame, `return !!(await ${kind.request})`)).to.equal(true);
        expect(await run(frame, `return (await ${kind.get}).length`)).to.equal(1);
        ses.setPermissionCheckHandler((_wc, permission) => permission !== kind.kind);
        expect(await run(frame, `return (await ${kind.get}).length`)).to.equal(0);
      });

      it('forget() in one document closes the device in another document of the same origin', async () => {
        kind.add(ses);
        const a = await open('main frame');
        const b = await open('main frame');
        onSelect(ses, kind, (info) => info.ids[0]);
        const openCode = `const d = await ${kind.request}; window.d = d; await d.open(${kind.kind === 'serial' ? '{ baudRate: 9600 }' : ''}); return true;`;
        expect(await run(a.frame, openCode)).to.equal(true);
        expect(await run(b.frame, openCode)).to.equal(true);
        expect(testing.fakeDeviceOpenCount(ses, kind.kind)).to.equal(2);
        await run(a.frame, 'await window.d.forget()');
        // Revocation reaches the other document's service, which drops its
        // connection to the device.
        await waitUntil(() => testing.fakeDeviceOpenCount(ses, kind.kind) === 0);
        expect(await run(b.frame, `return (await ${kind.get}).length`)).to.equal(0);
        if (kind.kind !== 'hid') {
          // Blink reflects the closed pipe for USB and serial (HIDDevice.opened does not track it).
          const isOpen = kind.kind === 'serial' ? '!!window.d.readable' : 'window.d.opened';
          await waitUntil(async () => (await run(b.frame, `return ${isOpen}`)) === false);
        }
      });

      it('does not crash when the chooser handler destroys the requesting window without answering', async () => {
        kind.add(ses);
        const { w, frame } = await open('main frame');
        const off = onSelect(ses, kind, () => {
          w.destroy();
          return undefined;
        });
        run(frame, `await ${kind.request}`).catch(() => {});
        await waitUntil(() => w.isDestroyed());
        off();
        await setTimeout(100);
        // Reaching here without a browser crash is the assertion; open another
        // chooser to make sure the delegate's state is consistent.
        const again = await open('main frame');
        onSelect(ses, kind, (info) => info.ids[0]);
        expect(await run(again.frame, `return !!(await ${kind.request})`)).to.equal(true);
      });
    });
  }

  describe('usb', () => {
    it('reports device configurations and interfaces as documented', async () => {
      KINDS[1].add(ses);
      const { frame } = await open('main frame');
      onSelect(ses, KINDS[1], (info) => info.ids[0]);
      let device: any;
      ses.setDevicePermissionHandler((details) => {
        device = details.device;
        return true;
      });
      await run(frame, `await ${KINDS[1].request}`);
      expect(device).to.be.an('object');
      expect(device.configurations).to.be.an('array').with.lengthOf(1);
      expect(device.configurations[0].interfaces).to.be.an('array').with.lengthOf(1);
      expect(device.configuration).to.be.an('object');
      expect(device.configuration.configurationValue).to.equal(1);
    });

    it('setUSBProtectedClassesHandler receives the requesting frame and origin', async () => {
      KINDS[1].add(ses);
      const { frame } = await open('cross-origin iframe');
      onSelect(ses, KINDS[1], (info) => info.ids[0]);
      const calls: any[] = [];
      ses.setUSBProtectedClassesHandler((details) => {
        calls.push(details);
        return details.protectedClasses;
      });
      await run(
        frame,
        `const d = await ${KINDS[1].request}; await d.open(); await d.claimInterface(0).catch(() => {});`
      );
      ses.setUSBProtectedClassesHandler(null);
      expect(calls).to.not.be.empty();
      expect(calls[0].origin).to.equal(frame.origin);
      expect(calls[0].frame).to.equal(frame);
    });
  });

  describe('serial', () => {
    it('emits serial-port-added with the requesting frame while a chooser is open', async () => {
      const kind = KINDS[2];
      const { w, frame } = await open('cross-origin iframe');
      const added = new Promise<any[]>((resolve) =>
        ses.once('serial-port-added' as any, (_e: any, ...args: any[]) => resolve(args))
      );
      let pick: ((id: string) => void) | undefined;
      ses.on('select-serial-port', (event, _list, _wc, callback) => {
        event.preventDefault();
        pick = callback;
      });
      const request = run(frame, `return !!(await ${kind.request})`);
      await waitUntil(() => !!pick);
      const token = kind.add(ses);
      const [port, webContents, addedFrame] = await added;
      expect(port.portId).to.equal(token);
      expect(webContents).to.equal(w.webContents);
      expect(addedFrame).to.equal(frame);
      pick!(token);
      expect(await request).to.equal(true);
    });

    it('delivers connect / disconnect for connected-state changes', async () => {
      const kind = KINDS[2];
      const token = kind.add(ses);
      const { frame } = await open('main frame');
      onSelect(ses, kind, (info) => info.ids[0]);
      await run(
        frame,
        `window.events = []; navigator.serial.addEventListener('connect', () => window.events.push('connect')); navigator.serial.addEventListener('disconnect', () => window.events.push('disconnect')); window.p = await ${kind.request};`
      );
      testing.setFakeSerialPortConnected(ses, token, false);
      await waitUntil(async () => (await run(frame, 'return window.events.join()')) === 'disconnect');
      expect(await run(frame, 'return window.p.connected')).to.equal(false);
      testing.setFakeSerialPortConnected(ses, token, true);
      await waitUntil(async () => (await run(frame, 'return window.events.join()')) === 'disconnect,connect');
    });
  });
});
