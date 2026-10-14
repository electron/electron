import { app, BrowserWindow, Menu, session, net as electronNet, type WebContents } from 'electron/main';

import { afterAll, afterEach, assert, beforeAll, beforeEach, describe, expect, it } from 'vitest';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import * as https from 'node:https';
import { createRequire } from 'node:module';
import * as net from 'node:net';
import * as os from 'node:os';
import * as path from 'node:path';
import * as readline from 'node:readline';
import { setTimeout } from 'node:timers/promises';
import { promisify } from 'node:util';

import { collectStreamBody, getResponse } from './lib/net-helpers.ts';
import { defer, ifdescribe, ifit, isWayland, listen, spawnAndWait, waitUntil } from './lib/spec-helpers.ts';
import { runInUtilityProcess } from './lib/utility-process-helpers.ts';
import { closeWindow, closeAllWindows } from './lib/window-helpers.ts';
import {
  makeXdgMockDirectories,
  spawnProtocolInfoWithXdgMock,
  spawnProtocolNameWithXdgMock,
  writeProtocolAssociation
} from './lib/xdg-helpers.ts';

const require = createRequire(import.meta.url);

const fixturesPath = path.resolve(import.meta.dirname, 'fixtures');

const isMacOSx64 = process.platform === 'darwin' && process.arch === 'x64';

describe('electron module', () => {
  it('does not expose internal modules to require', () => {
    expect(() => {
      require('clipboard');
    }).toThrow(/Cannot find module 'clipboard'/);
  });

  describe('require("electron")', () => {
    it('always returns the internal electron module', () => {
      require('electron');
    });
  });
});

describe('app module', () => {
  let server: https.Server;
  let secureUrl: string;
  const certPath = path.join(fixturesPath, 'certificates');

  beforeAll(async () => {
    const options = {
      key: fs.readFileSync(path.join(certPath, 'server.key')),
      cert: fs.readFileSync(path.join(certPath, 'server.pem')),
      ca: [
        fs.readFileSync(path.join(certPath, 'rootCA.pem')),
        fs.readFileSync(path.join(certPath, 'intermediateCA.pem'))
      ],
      requestCert: true,
      rejectUnauthorized: false
    };

    server = https.createServer(options, (req, res) => {
      if ((req as any).client.authorized) {
        res.writeHead(200);
        res.end('<title>authorized</title>');
      } else {
        res.writeHead(401);
        res.end('<title>denied</title>');
      }
    });

    secureUrl = (await listen(server)).url;
  });

  afterAll(
    () =>
      new Promise<void>((resolve) => {
        server.close(() => resolve());
      })
  );

  describe('app.getVersion()', () => {
    it('returns the version field of package.json', () => {
      expect(app.getVersion()).toBe('0.1.0');
    });
  });

  describe('app.setVersion(version)', () => {
    it('overrides the version', () => {
      expect(app.getVersion()).toBe('0.1.0');
      app.setVersion('test-version');

      expect(app.getVersion()).toBe('test-version');
      app.setVersion('0.1.0');
    });
  });

  describe('app name APIs', () => {
    const originalName = app.name;
    afterEach(() => (app.name = originalName));

    describe('with properties', () => {
      it('returns the name field of package.json', () => {
        expect(app.name).toBe('Electron Test Main');
      });

      it('overrides the name', () => {
        expect(app.name).toBe('Electron Test Main');
        app.name = 'electron-test-name';

        expect(app.name).toBe('electron-test-name');
      });
    });

    describe('with functions', () => {
      it('returns the name field of package.json', () => {
        expect(app.getName()).toBe('Electron Test Main');
      });

      it('overrides the name', () => {
        expect(app.getName()).toBe('Electron Test Main');
        app.setName('electron-test-name');

        expect(app.getName()).toBe('electron-test-name');
      });
    });
  });

  ifdescribe(process.platform === 'linux')('app.setDesktopName(name)', () => {
    it('sets the desktop name to the CHROME_DESKTOP environment variable', () => {
      const original = process.env.CHROME_DESKTOP;
      defer(() => {
        if (original === undefined) {
          delete process.env.CHROME_DESKTOP;
        } else {
          process.env.CHROME_DESKTOP = original;
        }
      });

      app.setDesktopName('electron-test-set-desktop-name.desktop');
      expect(process.env.CHROME_DESKTOP).toBe('electron-test-set-desktop-name.desktop');
    });
  });

  describe('app.getLocale()', () => {
    it('should not be empty', () => {
      expect(app.getLocale()).not.toBe('');
    });
  });

  describe('app.getSystemLocale()', () => {
    it('should not be empty', () => {
      expect(app.getSystemLocale()).not.toBe('');
    });
  });

  describe('app.getPreferredSystemLanguages()', () => {
    ifit(process.platform !== 'linux')('should not be empty', () => {
      expect(app.getPreferredSystemLanguages().length).not.toBe(0);
    });

    ifit(process.platform === 'linux')('should be empty or contain C entry', () => {
      const languages = app.getPreferredSystemLanguages();
      if (languages.length) {
        expect(languages).not.toContain('C');
      }
    });
  });

  describe('app.getLocaleCountryCode()', () => {
    it('should be empty or have length of two', () => {
      const localeCountryCode = app.getLocaleCountryCode();
      expect(localeCountryCode).toBeTypeOf('string');
      expect([0, 2]).toContain(localeCountryCode.length);
    });
  });

  describe('app.isHardwareAccelerationEnabled()', () => {
    it('should be a boolean', () => {
      expect(app.isHardwareAccelerationEnabled()).toBeTypeOf('boolean');
    });
  });

  ifdescribe(process.platform === 'win32')('app.setToastActivatorCLSID()', () => {
    it('throws on invalid format', () => {
      expect(() => {
        app.setToastActivatorCLSID('1234567890');
      }).toThrow(/Invalid CLSID format/);
    });
  });

  describe('app.isPackaged', () => {
    it('should be false during tests', () => {
      expect(app.isPackaged).toBe(false);
    });
  });

  ifdescribe(process.platform === 'darwin')('app.isInApplicationsFolder()', () => {
    it('should be false during tests', () => {
      expect(app.isInApplicationsFolder()).toBe(false);
    });
  });

  ifdescribe(process.platform === 'linux' && fs.existsSync('/usr/bin/dbus-daemon'))(
    'when a D-Bus bus goes away',
    () => {
      it('exits cleanly instead of crashing', async () => {
        const daemon = cp.spawn('dbus-daemon', ['--session', '--nofork', '--print-address']);
        defer(() => daemon.kill());
        const [address] = await once(daemon.stdout, 'data');
        const bus = address.toString().trim();
        const env = { ...process.env, DBUS_SESSION_BUS_ADDRESS: bus, DBUS_SYSTEM_BUS_ADDRESS: bus };

        const appProcess = cp.spawn(process.execPath, [path.join(fixturesPath, 'api', 'session-bus-lost')], { env });
        defer(() => appProcess.kill());
        const exited = once(appProcess, 'exit');
        await once(appProcess.stdout, 'data');
        await waitUntil(() => {
          const names = cp
            .spawnSync(
              'dbus-send',
              ['--session', '--dest=org.freedesktop.DBus', '--print-reply', '/', 'org.freedesktop.DBus.ListNames'],
              { env }
            )
            .stdout.toString();
          return (names.match(/string ":1\./g) ?? []).length >= 2;
        });

        daemon.kill();
        const [code, signal] = await exited;
        expect(signal).toBeNull();
        expect(code).toBe(0);
      });
    }
  );

  describe('app.exit(exitCode)', () => {
    let appProcess: cp.ChildProcess | null = null;

    afterEach(() => {
      if (appProcess) appProcess.kill();
    });

    it('emits a process exit event with the code', async () => {
      const appPath = path.join(fixturesPath, 'api', 'quit-app');
      const electronPath = process.execPath;
      let output = '';

      appProcess = cp.spawn(electronPath, [appPath]);
      if (appProcess && appProcess.stdout) {
        appProcess.stdout.on('data', (data) => {
          output += data;
        });
      }
      const [code] = await once(appProcess, 'close');

      if (process.platform !== 'win32') {
        expect(output).toContain('Exit event with code: 123');
      }
      expect(code).toBe(123);
    });

    it('closes all windows', async () => {
      const appPath = path.join(fixturesPath, 'api', 'exit-closes-all-windows-app');
      const electronPath = process.execPath;

      appProcess = cp.spawn(electronPath, [appPath]);
      const [code, signal] = await once(appProcess, 'exit');

      expect(signal, 'exit signal should be null, if you see this please tag @MarshallOfSound').toBe(null);
      expect(code, 'exit code should be 123, if you see this please tag @MarshallOfSound').toBe(123);
    });

    // Exiting before 'ready' leaves browser start-up state unfreed by design,
    // which LeakSanitizer reports and turns into exit code 1.
    ifit(!process.env.IS_ASAN)('exits cleanly when called before ready right after loading tls', async () => {
      const appPath = path.join(fixturesPath, 'api', 'exit-before-ready-after-tls');
      // This guards against a shutdown race that was lost roughly one run in
      // five, so go a few rounds.
      for (let i = 0; i < 15; i++) {
        appProcess = cp.spawn(process.execPath, [appPath]);
        let stderr = '';
        appProcess.stderr!.on('data', (data) => {
          stderr += data;
        });
        const [code, signal] = await once(appProcess, 'close');
        appProcess = null;
        const message = `run ${i}: code=${code} signal=${signal}\n${stderr}`;
        expect(signal, message).toBe(null);
        expect(code, message).toBe(123);
        expect(stderr, message).not.toMatch(/Received signal \d+|Ignoring extra certs/);
      }
    });

    // A missing display makes toolkit initialisation fail before the main loop
    // runs; that early return must still tear the JS environment down cleanly.
    // Skipped under ASan for the same reason as the test above.
    ifit(process.platform === 'linux' && !process.env.IS_ASAN)(
      'exits with code 1 when no display is available',
      async () => {
        const appPath = path.join(fixturesPath, 'api', 'no-display');
        const env = { ...process.env };
        delete env.DISPLAY;
        delete env.WAYLAND_DISPLAY;
        appProcess = cp.spawn(process.execPath, [appPath, '--ozone-platform=x11'], { env });
        let stderr = '';
        appProcess.stderr!.on('data', (data) => {
          stderr += data;
        });
        const [code, signal] = await once(appProcess, 'close');
        appProcess = null;
        const message = `code=${code} signal=${signal}\n${stderr}`;
        expect(signal, message).toBe(null);
        expect(code, message).toBe(1);
        expect(stderr, message).not.toMatch(/Received signal \d+|Check failed/);
      }
    );

    ifit(['darwin', 'linux'].includes(process.platform))('exits gracefully', async () => {
      const electronPath = process.execPath;
      const appPath = path.join(fixturesPath, 'api', 'singleton');
      appProcess = cp.spawn(electronPath, [appPath]);

      // Singleton will send us greeting data to let us know it's running.
      // After that, ask it to exit gracefully and confirm that it does.
      if (appProcess && appProcess.stdout) {
        appProcess.stdout.on('data', () => appProcess!.kill());
      }
      const [code, signal] = await once(appProcess, 'exit');

      const message = `code:\n${code}\nsignal:\n${signal}`;
      expect(code, message).toBe(0);
      expect(signal, message).toBe(null);
    });
  });

  ifdescribe(process.platform === 'darwin')('app.setActivationPolicy', () => {
    it('throws an error on invalid application policies', () => {
      expect(() => {
        app.setActivationPolicy('terrible' as any);
      }).toThrow(/Invalid activation policy: must be one of 'regular', 'accessory', or 'prohibited'/);
    });
  });

  describe('app.requestSingleInstanceLock', () => {
    interface SingleInstanceLockTestArgs {
      args: string[];
      expectedAdditionalData: unknown;
    }

    it('prevents the second launch of app', { timeout: 120000 }, async () => {
      const appPath = path.join(fixturesPath, 'api', 'singleton-data');
      // The two copies quit within milliseconds of each other, so record each
      // exit as soon as the copy is spawned: an 'exit' emitted before a
      // listener is attached is lost, and the test would wait for it until it
      // timed out.
      const first = cp.spawn(process.execPath, [appPath]);
      let firstCode: number | null = null;
      first.on('exit', (code) => {
        firstCode = code;
      });
      defer(() => {
        if (first.exitCode === null && first.signalCode === null) first.kill();
      });
      let firstOutput = '';
      first.stdout.on('data', (data) => {
        firstOutput += data;
      });
      await waitUntil(() => firstOutput.length > 0, { timeout: 30000 });
      // Start second app when received output.
      const second = cp.spawn(process.execPath, [appPath]);
      let secondCode: number | null = null;
      second.on('exit', (code) => {
        secondCode = code;
      });
      defer(() => {
        if (second.exitCode === null && second.signalCode === null) second.kill();
      });
      await waitUntil(() => secondCode !== null, { timeout: 30000 });
      expect(secondCode).toBe(1);
      await waitUntil(() => firstCode !== null, { timeout: 30000 });
      expect(firstCode).toBe(0);
    });

    it('returns true when setting non-existent user data folder', async () => {
      const appPath = path.join(fixturesPath, 'api', 'singleton-userdata');
      const instance = cp.spawn(process.execPath, [appPath]);
      const [code] = await once(instance, 'exit');
      expect(code).toBe(0);
    });

    async function testArgumentPassing(testArgs: SingleInstanceLockTestArgs) {
      const appPath = path.join(fixturesPath, 'api', 'singleton-data');
      const first = cp.spawn(process.execPath, [appPath, ...testArgs.args]);
      const firstExited = once(first, 'exit');
      // A copy left running holds the single instance lock, and a retry
      // could never get it, so make sure both copies are gone after the test.
      defer(async () => {
        if (first.exitCode === null && first.signalCode === null) {
          first.kill();
          await firstExited;
        }
      });

      // Wait for the first app to boot.
      const firstStdoutLines = readline.createInterface({ input: first.stdout });
      while ((await once(firstStdoutLines, 'line')).toString() !== 'started') {
        // wait.
      }
      const additionalDataPromise = once(firstStdoutLines, 'line');

      const secondInstanceArgs = [process.execPath, appPath, ...testArgs.args, '--some-switch', 'some-arg'];
      const second = cp.spawn(secondInstanceArgs[0], secondInstanceArgs.slice(1));
      const secondExited = once(second, 'exit');
      defer(async () => {
        if (second.exitCode === null && second.signalCode === null) {
          second.kill();
          await secondExited;
        }
      });

      const [code2] = await secondExited;
      expect(code2).toBe(1);
      const [code1] = await firstExited;
      expect(code1).toBe(0);
      const dataFromSecondInstance = await additionalDataPromise;
      const [args, additionalData] = dataFromSecondInstance[0].toString('ascii').split('||');
      const secondInstanceArgsReceived: string[] = JSON.parse(args.toString('ascii'));
      const secondInstanceDataReceived = JSON.parse(additionalData.toString('ascii'));

      // Ensure secondInstanceArgs is a subset of secondInstanceArgsReceived
      for (const arg of secondInstanceArgs) {
        expect(secondInstanceArgsReceived, `argument ${arg} is missing from received second args`).toContain(arg);
      }
      expect(
        secondInstanceDataReceived,
        `received data ${JSON.stringify(secondInstanceDataReceived)} is not equal to expected data ${JSON.stringify(testArgs.expectedAdditionalData)}.`
      ).toEqual(testArgs.expectedAdditionalData);
    }

    it('passes arguments to the second-instance event no additional data', async () => {
      await testArgumentPassing({
        args: [],
        expectedAdditionalData: null
      });
    });

    it('sends and receives JSON object data', async () => {
      const expectedAdditionalData = {
        level: 1,
        testkey: 'testvalue1',
        inner: {
          level: 2,
          testkey: 'testvalue2'
        }
      };
      await testArgumentPassing({
        args: ['--send-data'],
        expectedAdditionalData
      });
    });

    it('sends and receives data larger than the singleton message buffer', async () => {
      await testArgumentPassing({
        args: ['--send-data', '--data-size=300000'],
        expectedAdditionalData: 'x'.repeat(300000)
      });
    });

    ifit(process.platform !== 'win32')('passes long arguments to the second-instance event', async () => {
      await testArgumentPassing({
        args: [`--long-arg=${'a'.repeat(50000)}`],
        expectedAdditionalData: null
      });
    });

    it('sends and receives numerical data', async () => {
      await testArgumentPassing({
        args: ['--send-data', '--data-content=2'],
        expectedAdditionalData: 2
      });
    });

    it('sends and receives string data', async () => {
      await testArgumentPassing({
        args: ['--send-data', '--data-content="data"'],
        expectedAdditionalData: 'data'
      });
    });

    it('preserves NUL followed by whitespace in additional data', async () => {
      // The real invariant here is the V8-serialized format of the data. Using
      // a string here is just a convenient way to test the invariant.
      const expectedAdditionalData = {
        value: 'foo\0\tbar'
      };

      await testArgumentPassing({
        args: ['--send-data', `--data-content=${JSON.stringify(expectedAdditionalData)}`],
        expectedAdditionalData
      });
    });

    it('sends and receives boolean data', async () => {
      await testArgumentPassing({
        args: ['--send-data', '--data-content=false'],
        expectedAdditionalData: false
      });
    });

    it('sends and receives array data', async () => {
      await testArgumentPassing({
        args: ['--send-data', '--data-content=[2, 3, 4]'],
        expectedAdditionalData: [2, 3, 4]
      });
    });

    it('sends and receives mixed array data', async () => {
      await testArgumentPassing({
        args: ['--send-data', '--data-content=["2", true, 4]'],
        expectedAdditionalData: ['2', true, 4]
      });
    });

    it('sends and receives null data', async () => {
      await testArgumentPassing({
        args: ['--send-data', '--data-content=null'],
        expectedAdditionalData: null
      });
    });

    it('cannot send or receive undefined data', async () => {
      try {
        await testArgumentPassing({
          args: [
            '--send-ack',
            '--ack-content="undefined"',
            '--prevent-default',
            '--send-data',
            '--data-content="undefined"'
          ],
          expectedAdditionalData: undefined
        });
        assert(false);
      } catch {
        // This is expected.
      }
    });
  });

  // GitHub Actions macOS-13 runners used for x64 seem to have a problem with this test.
  ifdescribe(process.platform !== 'linux' && !isMacOSx64)('app.{add|get|clear}RecentDocument(s)', () => {
    const tempFiles = [
      path.join(fixturesPath, 'foo.txt'),
      path.join(fixturesPath, 'bar.txt'),
      path.join(fixturesPath, 'baz.txt'),
      path.join(fixturesPath, 'документ-文件.txt')
    ];

    afterEach(() => {
      app.clearRecentDocuments();
      for (const file of tempFiles) {
        fs.unlinkSync(file);
      }
    });

    beforeEach(() => {
      for (const file of tempFiles) {
        fs.writeFileSync(file, 'Lorem Ipsum');
      }
    });

    it('can add a recent document', async () => {
      app.addRecentDocument(tempFiles[0]);
      await setTimeout(2000);
      expect(app.getRecentDocuments()).toEqual(expect.arrayContaining([tempFiles[0]]));
    });

    it('returns recent documents whose path is not ASCII', async () => {
      app.addRecentDocument(tempFiles[3]);
      await setTimeout(2000);
      expect(app.getRecentDocuments()).toEqual(expect.arrayContaining([tempFiles[3]]));
    });

    it('can clear recent documents', async () => {
      app.addRecentDocument(tempFiles[1]);
      app.addRecentDocument(tempFiles[2]);
      await setTimeout(2000);
      expect(app.getRecentDocuments()).toEqual(expect.arrayContaining([tempFiles[1], tempFiles[2]]));
      app.clearRecentDocuments();
      await setTimeout(2000);
      expect(app.getRecentDocuments()).toEqual([]);
    });
  });

  describe('app.relaunch', () => {
    let server: net.Server | null = null;
    const socketPath =
      process.platform === 'win32' ? '\\\\.\\pipe\\electron-app-relaunch' : '/tmp/electron-app-relaunch';

    beforeEach(
      () =>
        new Promise<void>((resolve) => {
          fs.unlink(socketPath, () => {
            server = net.createServer();
            server.listen(socketPath);
            resolve();
          });
        })
    );

    afterEach(
      () =>
        new Promise<void>((resolve) => {
          server!.close(() => {
            if (process.platform === 'win32') {
              resolve();
            } else {
              fs.unlink(socketPath, () => resolve());
            }
          });
        })
    );

    it('relaunches the app', { timeout: 120000 }, () => {
      return new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());

        let state = 'none';
        server!.once('error', (error) => done(error));
        server!.on('connection', (client) => {
          client.once('data', (data) => {
            if (String(data) === '--first' && state === 'none') {
              state = 'first-launch';
            } else if (String(data) === '--second' && state === 'first-launch') {
              state = 'second-launch';
            } else if (String(data) === '--third' && state === 'second-launch') {
              done();
            } else {
              done(`Unexpected state: "${state}", data: "${data}"`);
            }
          });
        });

        const appPath = path.join(fixturesPath, 'api', 'relaunch');
        const child = cp.spawn(process.execPath, [appPath, '--first']);
        child.stdout.on('data', (c) => console.log(c.toString()));
        child.stderr.on('data', (c) => console.log(c.toString()));
        child.on('exit', (code, signal) => {
          if (code !== 0) {
            console.log(`Process exited with code "${code}" signal "${signal}"`);
          }
        });
      });
    });
  });

  ifdescribe(process.platform === 'darwin')('app.setUserActivity(type, userInfo)', () => {
    it('sets the current activity', () => {
      app.setUserActivity('com.electron.testActivity', { testData: '123' });
      expect(app.getCurrentActivityType()).toBe('com.electron.testActivity');
    });
  });

  describe('certificate-error event', () => {
    afterEach(closeAllWindows);
    it('is emitted when visiting a server with a self-signed cert', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL(secureUrl);
      await once(app, 'certificate-error');
    });

    describe('when denied', () => {
      beforeAll(() => {
        app.on('certificate-error', (event, webContents, url, error, certificate, callback) => {
          callback(false);
        });
      });

      afterAll(() => {
        app.removeAllListeners('certificate-error');
      });

      it('causes did-fail-load', async () => {
        const w = new BrowserWindow({ show: false });
        w.loadURL(secureUrl);
        await once(w.webContents, 'did-fail-load');
      });
    });
  });

  // xdescribe('app.importCertificate', () => {
  //   let w = null

  //   before(function () {
  //     if (process.platform !== 'linux') {
  //       this.skip()
  //     }
  //   })

  //   afterEach(() => closeWindow(w).then(() => { w = null }))

  //   it('can import certificate into platform cert store', done => {
  //     const options = {
  //       certificate: path.join(certPath, 'client.p12'),
  //       password: 'electron'
  //     }

  //     w = new BrowserWindow({
  //       show: false,
  //       webPreferences: {
  //         nodeIntegration: true
  //       }
  //     })

  //     w.webContents.on('did-finish-load', () => {
  //       expect(w.webContents.getTitle()).to.equal('authorized')
  //       done()
  //     })

  //     ipcRenderer.once('select-client-certificate', (event, webContentsId, list) => {
  //       expect(webContentsId).to.equal(w.webContents.id)
  //       expect(list).to.have.lengthOf(1)

  //       expect(list[0]).to.deep.equal({
  //         issuerName: 'Intermediate CA',
  //         subjectName: 'Client Cert',
  //         issuer: { commonName: 'Intermediate CA' },
  //         subject: { commonName: 'Client Cert' }
  //       })

  //       event.sender.send('client-certificate-response', list[0])
  //     })

  //     app.importCertificate(options, result => {
  //       expect(result).toNotExist()
  //       ipcRenderer.sendSync('set-client-certificate-option', false)
  //       w.loadURL(secureUrl)
  //     })
  //   })
  // })

  describe('BrowserWindow events', () => {
    let w: BrowserWindow = null as any;

    afterEach(() => {
      closeWindow(w).then(() => {
        w = null as any;
      });
    });

    it('should emit browser-window-focus event when window is focused', async () => {
      const emitted = once(app, 'browser-window-focus') as Promise<[any, BrowserWindow]>;
      w = new BrowserWindow({ show: false });
      w.emit('focus');
      const [, window] = await emitted;
      expect(window.id).toBe(w.id);
    });

    it('should emit browser-window-blur event when window is blurred', async () => {
      const emitted = once(app, 'browser-window-blur') as Promise<[any, BrowserWindow]>;
      w = new BrowserWindow({ show: false });
      w.emit('blur');
      const [, window] = await emitted;
      expect(window.id).toBe(w.id);
    });

    it('should emit browser-window-created event when window is created', async () => {
      const emitted = once(app, 'browser-window-created') as Promise<[any, BrowserWindow]>;
      w = new BrowserWindow({ show: false });
      const [, window] = await emitted;
      expect(window.id).toBe(w.id);
    });

    it('should emit web-contents-created event when a webContents is created', async () => {
      const emitted = once(app, 'web-contents-created') as Promise<[any, WebContents]>;
      w = new BrowserWindow({ show: false });
      const [, webContents] = await emitted;
      expect(webContents.id).toBe(w.webContents.id);
    });

    // FIXME: re-enable this test on win32.
    ifit(process.platform !== 'win32' && !isWayland)(
      'should emit render-process-gone event when renderer crashes',
      async () => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
          }
        });
        await w.loadURL('about:blank');

        const emitted = once(app, 'render-process-gone') as Promise<
          [any, WebContents, Electron.RenderProcessGoneDetails]
        >;
        w.webContents.executeJavaScript('process.crash()');

        const [, webContents, details] = await emitted;
        expect(webContents).toBe(w.webContents);
        expect(['crashed', 'abnormal-exit']).toContain(details.reason);
      }
    );
  });

  describe('app.badgeCount', () => {
    const platformIsSupported = process.platform === 'darwin' || process.platform === 'linux';

    const expectedBadgeCount = 42;

    afterAll(() => {
      app.badgeCount = 0;
    });

    ifdescribe(platformIsSupported)('on supported platform', () => {
      describe('with properties', () => {
        it('sets a badge count', () => {
          app.badgeCount = expectedBadgeCount;
          expect(app.badgeCount).toBe(expectedBadgeCount);
        });
      });

      describe('with functions', () => {
        it('sets a numerical badge count', () => {
          app.setBadgeCount(expectedBadgeCount);
          expect(app.getBadgeCount()).toBe(expectedBadgeCount);
        });
        // A badge count is required on Linux; only macOS displays a plain
        // dot when no count is provided.
        ifit(process.platform === 'darwin')('sets an non numeric (dot) badge count', () => {
          app.setBadgeCount();
          // Badge count should be zero when non numeric (dot) is requested
          expect(app.getBadgeCount()).toBe(0);
        });
      });
    });
  });

  ifdescribe(
    process.platform !== 'linux' && !process.mas && (process.platform !== 'darwin' || process.arch === 'arm64')
  )('app.get/setLoginItemSettings API', () => {
    const isMac = process.platform === 'darwin';
    const isWin = process.platform === 'win32';

    const updateExe = path.resolve(path.dirname(process.execPath), '..', 'Update.exe');
    const processStartArgs = [
      '--processStart',
      `"${path.basename(process.execPath)}"`,
      '--process-start-args',
      '"--hidden"'
    ];
    const regAddArgs = [
      'ADD',
      'HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run',
      '/v',
      'additionalEntry',
      '/t',
      'REG_BINARY',
      '/f',
      '/d'
    ];

    beforeEach(() => {
      app.setLoginItemSettings({ openAtLogin: false });
      app.setLoginItemSettings({ openAtLogin: false, path: updateExe, args: processStartArgs });
      app.setLoginItemSettings({ name: 'additionalEntry', openAtLogin: false });
    });

    afterEach(() => {
      app.setLoginItemSettings({ openAtLogin: false });
      app.setLoginItemSettings({ openAtLogin: false, path: updateExe, args: processStartArgs });
      app.setLoginItemSettings({ name: 'additionalEntry', openAtLogin: false });
    });

    ifit(!isWin)('sets and returns the app as a login item', () => {
      app.setLoginItemSettings({ openAtLogin: true });

      const settings = app.getLoginItemSettings();
      expect(settings.openAtLogin).toBe(true);
      expect(settings.wasOpenedAtLogin).toBe(false);
      expect(settings.status).toBe('enabled');
    });

    ifit(isWin)('sets and returns the app as a login item (windows)', () => {
      app.setLoginItemSettings({ openAtLogin: true, enabled: true });
      expect(app.getLoginItemSettings()).toEqual({
        openAtLogin: true,
        wasOpenedAtLogin: false,
        executableWillLaunchAtLogin: true,
        launchItems: [
          {
            name: 'electron.app.Electron',
            path: process.execPath,
            args: [],
            scope: 'user',
            enabled: true
          }
        ]
      });

      app.setLoginItemSettings({ openAtLogin: false });
      app.setLoginItemSettings({ openAtLogin: true, enabled: false });
      expect(app.getLoginItemSettings()).toEqual({
        openAtLogin: true,
        wasOpenedAtLogin: false,
        executableWillLaunchAtLogin: false,
        launchItems: [
          {
            name: 'electron.app.Electron',
            path: process.execPath,
            args: [],
            scope: 'user',
            enabled: false
          }
        ]
      });
    });

    it('correctly sets and unsets the LoginItem', () => {
      expect(app.getLoginItemSettings().openAtLogin).toBe(false);

      app.setLoginItemSettings({ openAtLogin: true });
      expect(app.getLoginItemSettings().openAtLogin).toBe(true);

      app.setLoginItemSettings({ openAtLogin: false });
      expect(app.getLoginItemSettings().openAtLogin).toBe(false);
    });

    ifdescribe(isMac)('using SMAppService', () => {
      it('can set a login item', () => {
        app.setLoginItemSettings({
          openAtLogin: true,
          type: 'mainAppService'
        });

        expect(app.getLoginItemSettings()).toEqual({
          status: 'enabled',
          openAtLogin: true,
          wasOpenedAtLogin: false,
          executableWillLaunchAtLogin: false
        });
      });

      it('throws when setting non-default type with no name', () => {
        expect(() => {
          app.setLoginItemSettings({
            openAtLogin: true,
            type: 'daemonService'
          });
        }).toThrow(/'name' is required when type is not mainAppService/);
      });

      it('throws when getting non-default type with no name', () => {
        expect(() => {
          app.getLoginItemSettings({
            type: 'daemonService'
          });
        }).toThrow(/'name' is required when type is not mainAppService/);
      });

      it('does not crash when the service name is not valid UTF-8', () => {
        expect(() => {
          app.setLoginItemSettings({
            openAtLogin: false,
            type: 'daemonService',
            serviceName: '\uD800'
          });
        }).not.toThrow();
      });

      it('can unset a login item', () => {
        app.setLoginItemSettings({
          openAtLogin: true,
          type: 'mainAppService'
        });

        app.setLoginItemSettings({
          openAtLogin: false,
          type: 'mainAppService'
        });

        expect(app.getLoginItemSettings()).toEqual({
          status: 'not-registered',
          openAtLogin: false,
          wasOpenedAtLogin: false,
          executableWillLaunchAtLogin: false
        });
      });
    });

    ifit(isWin)('allows you to pass a custom executable and arguments', () => {
      app.setLoginItemSettings({ openAtLogin: true, path: updateExe, args: processStartArgs, enabled: true });
      expect(app.getLoginItemSettings().openAtLogin).toBe(false);
      const openAtLoginTrueEnabledTrue = app.getLoginItemSettings({
        path: updateExe,
        args: processStartArgs
      });

      expect(openAtLoginTrueEnabledTrue.openAtLogin).toBe(true);
      expect(openAtLoginTrueEnabledTrue.executableWillLaunchAtLogin).toBe(true);

      app.setLoginItemSettings({ openAtLogin: true, path: updateExe, args: processStartArgs, enabled: false });
      const openAtLoginTrueEnabledFalse = app.getLoginItemSettings({
        path: updateExe,
        args: processStartArgs
      });

      expect(openAtLoginTrueEnabledFalse.openAtLogin).toBe(true);
      expect(openAtLoginTrueEnabledFalse.executableWillLaunchAtLogin).toBe(false);

      app.setLoginItemSettings({ openAtLogin: false, path: updateExe, args: processStartArgs, enabled: false });
      const openAtLoginFalseEnabledFalse = app.getLoginItemSettings({
        path: updateExe,
        args: processStartArgs
      });

      expect(openAtLoginFalseEnabledFalse.openAtLogin).toBe(false);
      expect(openAtLoginFalseEnabledFalse.executableWillLaunchAtLogin).toBe(false);
    });

    ifit(isWin)('allows you to pass a custom name', () => {
      app.setLoginItemSettings({ openAtLogin: true });
      app.setLoginItemSettings({ openAtLogin: true, name: 'additionalEntry', enabled: false });
      expect(app.getLoginItemSettings()).toEqual({
        openAtLogin: true,
        wasOpenedAtLogin: false,
        executableWillLaunchAtLogin: true,
        launchItems: [
          {
            name: 'additionalEntry',
            path: process.execPath,
            args: [],
            scope: 'user',
            enabled: false
          },
          {
            name: 'electron.app.Electron',
            path: process.execPath,
            args: [],
            scope: 'user',
            enabled: true
          }
        ]
      });

      app.setLoginItemSettings({ openAtLogin: false, name: 'additionalEntry' });
      expect(app.getLoginItemSettings()).toEqual({
        openAtLogin: true,
        wasOpenedAtLogin: false,
        executableWillLaunchAtLogin: true,
        launchItems: [
          {
            name: 'electron.app.Electron',
            path: process.execPath,
            args: [],
            scope: 'user',
            enabled: true
          }
        ]
      });
    });

    ifit(isWin)('finds launch items independent of args', () => {
      app.setLoginItemSettings({ openAtLogin: true, args: ['arg1'] });
      app.setLoginItemSettings({ openAtLogin: true, name: 'additionalEntry', enabled: false, args: ['arg2'] });
      expect(app.getLoginItemSettings()).toEqual({
        openAtLogin: false,
        wasOpenedAtLogin: false,
        executableWillLaunchAtLogin: true,
        launchItems: [
          {
            name: 'additionalEntry',
            path: process.execPath,
            args: ['arg2'],
            scope: 'user',
            enabled: false
          },
          {
            name: 'electron.app.Electron',
            path: process.execPath,
            args: ['arg1'],
            scope: 'user',
            enabled: true
          }
        ]
      });
    });

    ifit(isWin)('finds launch items independent of path quotation or casing', () => {
      const expectation = {
        openAtLogin: false,
        wasOpenedAtLogin: false,
        executableWillLaunchAtLogin: true,
        launchItems: [
          {
            name: 'additionalEntry',
            path: 'C:\\electron\\myapp.exe',
            args: ['arg1'],
            scope: 'user',
            enabled: true
          }
        ]
      };

      app.setLoginItemSettings({
        openAtLogin: true,
        name: 'additionalEntry',
        enabled: true,
        path: 'C:\\electron\\myapp.exe',
        args: ['arg1']
      });
      expect(app.getLoginItemSettings({ path: '"C:\\electron\\MYAPP.exe"' })).toEqual(expectation);

      app.setLoginItemSettings({ openAtLogin: false, name: 'additionalEntry' });
      app.setLoginItemSettings({
        openAtLogin: true,
        name: 'additionalEntry',
        enabled: true,
        path: '"C:\\electron\\MYAPP.exe"',
        args: ['arg1']
      });
      expect(app.getLoginItemSettings({ path: 'C:\\electron\\myapp.exe' })).toEqual({
        ...expectation,
        launchItems: [
          {
            name: 'additionalEntry',
            path: 'C:\\electron\\MYAPP.exe',
            args: ['arg1'],
            scope: 'user',
            enabled: true
          }
        ]
      });
    });

    ifit(isWin)('detects disabled by TaskManager', async () => {
      app.setLoginItemSettings({ openAtLogin: true, name: 'additionalEntry', enabled: true, args: ['arg1'] });
      const appProcess = cp.spawn('reg', [...regAddArgs, '030000000000000000000000']);
      await once(appProcess, 'exit');
      expect(app.getLoginItemSettings()).toEqual({
        openAtLogin: false,
        wasOpenedAtLogin: false,
        executableWillLaunchAtLogin: false,
        launchItems: [
          {
            name: 'additionalEntry',
            path: process.execPath,
            args: ['arg1'],
            scope: 'user',
            enabled: false
          }
        ]
      });
    });

    ifit(isWin)('detects enabled by TaskManager', async () => {
      const expectation = {
        openAtLogin: false,
        wasOpenedAtLogin: false,
        executableWillLaunchAtLogin: true,
        launchItems: [
          {
            name: 'additionalEntry',
            path: process.execPath,
            args: ['arg1'],
            scope: 'user',
            enabled: true
          }
        ]
      };

      app.setLoginItemSettings({ openAtLogin: true, name: 'additionalEntry', enabled: false, args: ['arg1'] });
      let appProcess = cp.spawn('reg', [...regAddArgs, '020000000000000000000000']);
      await once(appProcess, 'exit');
      expect(app.getLoginItemSettings()).toEqual(expectation);

      app.setLoginItemSettings({ openAtLogin: true, name: 'additionalEntry', enabled: false, args: ['arg1'] });
      appProcess = cp.spawn('reg', [...regAddArgs, '000000000000000000000000']);
      await once(appProcess, 'exit');
      expect(app.getLoginItemSettings()).toEqual(expectation);
    });
  });

  ifdescribe(process.platform !== 'linux')('accessibility support functionality', () => {
    // These tests toggle a process-wide AXMode. Turn it back off so the rest of
    // the suite doesn't run with renderer accessibility enabled.
    afterEach(() => {
      app.setAccessibilitySupportEnabled(false);
    });

    it('is mutable', () => {
      const values = [false, true, false];
      const setters: Array<(arg: boolean) => void> = [
        (value) => {
          app.accessibilitySupportEnabled = value;
        },
        (value) => app.setAccessibilitySupportEnabled(value)
      ];
      const getters: Array<() => boolean> = [
        () => app.accessibilitySupportEnabled,
        () => app.isAccessibilitySupportEnabled()
      ];
      for (const value of values) {
        for (const set of setters) {
          set(value);
          for (const get of getters) expect(get()).toEqual(value);
        }
      }
    });

    it('getAccessibilitySupportFeatures returns an array with accessibility properties', () => {
      const values = [
        'nativeAPIs',
        'webContents',
        'inlineTextBoxes',
        'extendedProperties',
        'screenReader',
        'html',
        'labelImages',
        'pdfPrinting'
      ];

      app.setAccessibilitySupportEnabled(false);

      const disabled = app.getAccessibilitySupportFeatures();
      expect(disabled).toBeInstanceOf(Array);
      expect(disabled.includes('complete')).toBe(false);

      app.setAccessibilitySupportEnabled(true);
      const enabled = app.getAccessibilitySupportFeatures();
      expect(enabled).toBeInstanceOf(Array);
      expect(enabled.length).toBeGreaterThan(0);

      const boolEnabled = app.isAccessibilitySupportEnabled();
      if (boolEnabled) {
        expect(enabled.some((f) => values.includes(f))).toBe(true);
      }
    });

    it('setAccessibilitySupportFeatures can enable a subset of features', () => {
      app.setAccessibilitySupportEnabled(false);
      expect(app.isAccessibilitySupportEnabled()).toBe(false);
      expect(app.getAccessibilitySupportFeatures()).toEqual([]);

      const subsetA = ['webContents', 'html'];
      app.setAccessibilitySupportFeatures(subsetA);
      const afterSubsetA = app.getAccessibilitySupportFeatures();
      expect(afterSubsetA).toEqual(subsetA);

      const subsetB = ['nativeAPIs', 'webContents', 'inlineTextBoxes', 'extendedProperties'];
      app.setAccessibilitySupportFeatures(subsetB);
      const afterSubsetB = app.getAccessibilitySupportFeatures();
      expect(afterSubsetB).toEqual(subsetB);
    });

    it('throws when an unknown accessibility feature is requested', () => {
      expect(() => {
        app.setAccessibilitySupportFeatures(['unknownFeature']);
      }).toThrow('Unknown accessibility feature: unknownFeature');
    });
  });

  ifdescribe(process.platform === 'win32')('setJumpList(categories)', () => {
    it('throws an error when categories is not null or an array', () => {
      expect(() => {
        app.setJumpList('string' as any);
      }).toThrow('Argument must be null or an array of categories');
    });

    it('can get jump list settings', () => {
      const settings = app.getJumpListSettings();
      expect(settings).toEqual({ minItems: 10, removedItems: [] });
    });

    it('can set a jump list with an array of categories', () => {
      expect(() => {
        app.setJumpList([
          { type: 'frequent' },
          {
            items: [
              {
                type: 'task',
                title: 'New Project',
                program: process.execPath,
                args: '--new-project',
                description: 'Create a new project.'
              },
              { type: 'separator' },
              {
                type: 'task',
                title: 'Recover Project',
                program: process.execPath,
                args: '--recover-project',
                description: 'Recover Project'
              }
            ]
          }
        ]);
      }).not.toThrow();
    });
  });

  describe('getAppPath', () => {
    it('works for directories with package.json', async () => {
      const { appPath } = await runTestApp('app-path');
      expect(appPath).toBe(path.resolve(fixturesPath, 'api/app-path'));
    });

    it('works for directories with index.js', async () => {
      const { appPath } = await runTestApp('app-path/lib');
      expect(appPath).toBe(path.resolve(fixturesPath, 'api/app-path/lib'));
    });

    it('works for files without extension', async () => {
      const { appPath } = await runTestApp('app-path/lib/index');
      expect(appPath).toBe(path.resolve(fixturesPath, 'api/app-path/lib'));
    });

    it('works for files', async () => {
      const { appPath } = await runTestApp('app-path/lib/index.js');
      expect(appPath).toBe(path.resolve(fixturesPath, 'api/app-path/lib'));
    });
  });

  describe('getPath(name)', () => {
    const originalMusicPath = app.getPath('music');
    afterEach(() => app.setPath('music', originalMusicPath));

    it('returns paths that exist', () => {
      const paths = [
        fs.existsSync(app.getPath('exe')),
        fs.existsSync(app.getPath('home')),
        fs.existsSync(app.getPath('temp'))
      ];
      expect(paths).toEqual([true, true, true]);
    });

    if (process.platform === 'darwin') {
      it('throws an error when trying to get the assets path on macOS', () => {
        expect(() => {
          app.getPath('assets' as any);
        }).toThrow(/Failed to get 'assets' path/);
      });
    } else {
      it('returns an assets path that is identical to the module path', () => {
        const assetsPath = app.getPath('assets');
        expect(fs.existsSync(assetsPath)).toBe(true);
        expect(assetsPath).toBe(path.dirname(app.getPath('module')));
      });
    }

    it('throws an error when the name is invalid', () => {
      expect(() => {
        app.getPath('does-not-exist' as any);
      }).toThrow(/Failed to get 'does-not-exist' path/);
    });

    it('returns the overridden path', () => {
      app.setPath('music', import.meta.dirname);
      expect(app.getPath('music')).toBe(import.meta.dirname);
    });

    if (process.platform === 'win32') {
      it('gets the folder for recent files', () => {
        const recent = app.getPath('recent');

        // We expect that one of our test machines have overridden this
        // to be something crazy, it'll always include the word "Recent"
        // unless people have been registry-hacking like crazy
        expect(recent).toContain('Recent');
      });

      it('can override the recent files path', () => {
        app.setPath('recent', 'C:\\fake-path');
        expect(app.getPath('recent')).toBe('C:\\fake-path');
      });
    }

    it('uses the app name in getPath(userData)', () => {
      expect(app.getPath('userData')).toContain(app.name);
    });
  });

  describe('setPath(name, path)', () => {
    const originalMusicPath = app.getPath('music');
    afterEach(() => app.setPath('music', originalMusicPath));

    it('throws when a relative path is passed', () => {
      const badPath = 'hey/hi/hello';

      expect(() => {
        app.setPath('music', badPath);
      }).toThrow(/Path must be absolute/);
    });

    it('does not create a new directory by default', () => {
      const badPath = path.join(import.meta.dirname, 'music');

      expect(fs.existsSync(badPath)).toBe(false);
      app.setPath('music', badPath);
      expect(fs.existsSync(badPath)).toBe(false);

      expect(() => {
        app.getPath(badPath as any);
      }).toThrow();
    });

    describe('sessionData', () => {
      const appPath = path.join(import.meta.dirname, 'fixtures', 'apps', 'set-path');
      const appName = JSON.parse(fs.readFileSync(path.join(appPath, 'package.json'), 'utf8')).name;
      const userDataPath = path.join(app.getPath('appData'), appName);
      const tempBrowserDataPath = path.join(app.getPath('temp'), appName);

      const sessionFiles = ['Preferences', 'Code Cache', 'Local Storage', 'IndexedDB', 'Service Worker'];
      const hasSessionFiles = (dir: string) => {
        for (const file of sessionFiles) {
          if (!fs.existsSync(path.join(dir, file))) {
            return false;
          }
        }
        return true;
      };

      beforeEach(() => {
        fs.rmSync(userDataPath, { force: true, recursive: true });
        fs.rmSync(tempBrowserDataPath, { force: true, recursive: true });
      });

      // The app quits once it has written its session data.
      const runApp = async (args: string[]) => {
        const { code, stdout, stderr } = await spawnAndWait(process.execPath, [appPath, ...args], { timeout: 20_000 });
        expect(code, `stdout:\n${stdout}\nstderr:\n${stderr}`).toBe(0);
      };

      it('writes to userData by default', async () => {
        expect(hasSessionFiles(userDataPath)).toBe(false);
        await runApp([]);
        expect(hasSessionFiles(userDataPath)).toBe(true);
      });

      it('can be changed', async () => {
        expect(hasSessionFiles(userDataPath)).toBe(false);
        await runApp(['sessionData', tempBrowserDataPath]);
        expect(hasSessionFiles(userDataPath)).toBe(false);
        expect(hasSessionFiles(tempBrowserDataPath)).toBe(true);
      });

      it('changing userData affects default sessionData', async () => {
        expect(hasSessionFiles(userDataPath)).toBe(false);
        await runApp(['userData', tempBrowserDataPath]);
        expect(hasSessionFiles(userDataPath)).toBe(false);
        expect(hasSessionFiles(tempBrowserDataPath)).toBe(true);
      });
    });
  });

  describe('setAppLogsPath(path)', () => {
    it('throws when a relative path is passed', () => {
      const badPath = 'hey/hi/hello';

      expect(() => {
        app.setAppLogsPath(badPath);
      }).toThrow(/Path must be absolute/);
    });
  });

  ifdescribe(process.platform !== 'linux')('select-client-certificate event', () => {
    let w: BrowserWindow;

    beforeAll(() => {
      session.fromPartition('empty-certificate').setCertificateVerifyProc((req, cb) => {
        cb(0);
      });
    });

    beforeEach(() => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'empty-certificate'
        }
      });
    });

    afterEach(() =>
      closeWindow(w).then(() => {
        w = null as any;
      })
    );

    afterAll(() => session.fromPartition('empty-certificate').setCertificateVerifyProc(null));

    it('can respond with empty certificate list', async () => {
      app.once('select-client-certificate', function (event, webContents, url, list, callback) {
        console.log('select-client-certificate emitted');
        event.preventDefault();
        callback();
      });
      await w.webContents.loadURL(secureUrl);
      expect(w.webContents.getTitle()).toBe('denied');
    });
  });

  ifdescribe(process.platform === 'win32')('setAsDefaultProtocolClient(protocol, path, args)', () => {
    const protocol = 'electron-test';
    const updateExe = path.resolve(path.dirname(process.execPath), '..', 'Update.exe');
    const processStartArgs = [
      '--processStart',
      `"${path.basename(process.execPath)}"`,
      '--process-start-args',
      '"--hidden"'
    ];

    let Winreg: any;
    let classesKey: any;

    beforeAll(() => {
      Winreg = require('winreg');

      classesKey = new Winreg({
        hive: Winreg.HKCU,
        key: '\\Software\\Classes\\'
      });
    });

    afterAll(
      () =>
        new Promise<void>((resolve) => {
          if (process.platform !== 'win32') {
            resolve();
          } else {
            const protocolKey = new Winreg({
              hive: Winreg.HKCU,
              key: `\\Software\\Classes\\${protocol}`
            });

            // The last test leaves the registry dirty,
            // delete the protocol key for those of us who test at home
            protocolKey.destroy(() => resolve());
          }
        })
    );

    beforeEach(() => {
      app.removeAsDefaultProtocolClient(protocol);
      app.removeAsDefaultProtocolClient(protocol, updateExe, processStartArgs);
    });

    afterEach(() => {
      app.removeAsDefaultProtocolClient(protocol);
      expect(app.isDefaultProtocolClient(protocol)).toBe(false);

      app.removeAsDefaultProtocolClient(protocol, updateExe, processStartArgs);
      expect(app.isDefaultProtocolClient(protocol, updateExe, processStartArgs)).toBe(false);
    });

    it('sets the app as the default protocol client', () => {
      expect(app.isDefaultProtocolClient(protocol)).toBe(false);
      app.setAsDefaultProtocolClient(protocol);
      expect(app.isDefaultProtocolClient(protocol)).toBe(true);
    });

    it('allows a custom path and args to be specified', () => {
      expect(app.isDefaultProtocolClient(protocol, updateExe, processStartArgs)).toBe(false);
      app.setAsDefaultProtocolClient(protocol, updateExe, processStartArgs);

      expect(app.isDefaultProtocolClient(protocol, updateExe, processStartArgs)).toBe(true);
      expect(app.isDefaultProtocolClient(protocol)).toBe(false);
    });

    it('creates a registry entry for the protocol class', async () => {
      app.setAsDefaultProtocolClient(protocol);

      const keys = (await promisify(classesKey.keys).call(classesKey)) as any[];
      const exists = keys.some((key) => key.key.includes(protocol));
      expect(exists).toBe(true);
    });

    it('completely removes a registry entry for the protocol class', async () => {
      app.setAsDefaultProtocolClient(protocol);
      app.removeAsDefaultProtocolClient(protocol);

      const keys = (await promisify(classesKey.keys).call(classesKey)) as any[];
      const exists = keys.some((key) => key.key.includes(protocol));
      expect(exists).toBe(false);
    });

    it('only unsets a class registry key if it contains other data', async () => {
      app.setAsDefaultProtocolClient(protocol);

      const protocolKey = new Winreg({
        hive: Winreg.HKCU,
        key: `\\Software\\Classes\\${protocol}`
      });

      await promisify(protocolKey.set).call(protocolKey, 'test-value', 'REG_BINARY', '123');
      app.removeAsDefaultProtocolClient(protocol);

      const keys = (await promisify(classesKey.keys).call(classesKey)) as any[];
      const exists = keys.some((key) => key.key.includes(protocol));
      expect(exists).toBe(true);
    });

    it('sets the default client such that getApplicationNameForProtocol returns Electron', () => {
      app.setAsDefaultProtocolClient(protocol);
      expect(app.getApplicationNameForProtocol(`${protocol}://`)).toBe('Electron');
    });
  });

  describe('getApplicationNameForProtocol()', () => {
    // TODO: Linux CI doesn't have registered http & https handlers
    ifit(!(process.env.CI && process.platform === 'linux') && !isWayland)(
      'returns application names for common protocols',
      () => {
        // We can't expect particular app names here, but these protocols should
        // at least have _something_ registered. Except on our Linux CI
        // environment apparently.
        const protocols = ['http://', 'https://'];
        for (const protocol of protocols) {
          expect(app.getApplicationNameForProtocol(protocol)).not.toBe('');
        }
      }
    );

    it('returns an empty string for a bogus protocol', () => {
      expect(app.getApplicationNameForProtocol('bogus-protocol://')).toBe('');
    });

    ifdescribe(process.platform === 'linux')('on Linux with mocked XDG dirs', () => {
      const desktopFileId = 'mock-browser.desktop';
      const mockDisplayName = 'Mock Browser';
      const mockScheme = 'mockproto';
      const mockMimeType = `x-scheme-handler/${mockScheme}`;

      let xdgDir: string;
      let xdgDataHome: string;
      let xdgConfigHome: string;
      beforeAll(() => {
        ({ xdgDir, xdgDataHome, xdgConfigHome } = makeXdgMockDirectories('electron-xdg-name-'));
        writeProtocolAssociation(
          xdgDataHome,
          xdgConfigHome,
          desktopFileId,
          mockDisplayName,
          '/usr/bin/true %u',
          mockMimeType
        );
      });

      afterAll(() => {
        fs.rmSync(xdgDir, { recursive: true, force: true });
      });

      it('returns the display name for a registered protocol', async () => {
        const name = await spawnProtocolNameWithXdgMock(`${mockScheme}://`, xdgDataHome, xdgConfigHome);
        expect(name).toBe(mockDisplayName);
      });

      it('returns an empty string for an unregistered protocol', async () => {
        const name = await spawnProtocolNameWithXdgMock('unregistered-proto://', xdgDataHome, xdgConfigHome);
        expect(name).toBe('');
      });
    });
  });

  describe('getApplicationInfoForProtocol()', () => {
    const fixtureIcon = path.join(fixturesPath, 'assets', '1x1.png');
    const desktopFileId = 'mock-browser.desktop';
    const mockDisplayName = 'Mock Browser';
    const mockScheme = 'mockproto';
    const mockMimeType = `x-scheme-handler/${mockScheme}`;

    let xdgDir: string;
    let xdgDataHome: string;
    let xdgConfigHome: string;
    let xdgBinDir: string;

    beforeAll(() => {
      if (process.platform !== 'linux') {
        return;
      }

      ({ xdgDir, xdgDataHome, xdgConfigHome, xdgBinDir } = makeXdgMockDirectories('electron-xdg-app-info-'));
      writeProtocolAssociation(
        xdgDataHome,
        xdgConfigHome,
        desktopFileId,
        mockDisplayName,
        '/usr/bin/true %u',
        mockMimeType,
        fixtureIcon
      );
    });

    afterAll(() => {
      if (process.platform === 'linux') {
        fs.rmSync(xdgDir, { recursive: true, force: true });
      }
    });

    it('returns promise rejection for a bogus protocol', async () => {
      await expect(app.getApplicationInfoForProtocol('bogus-protocol://')).rejects.toThrow(
        'Unable to retrieve installation path to app'
      );
    });

    it('returns resolved promise with appPath, displayName and icon', async () => {
      if (process.platform === 'linux') {
        const appInfo = await spawnProtocolInfoWithXdgMock(`${mockScheme}://`, xdgDataHome, xdgConfigHome);
        expect(appInfo.name).toBe(mockDisplayName);
        expect(appInfo.path).toBe('/usr/bin/true');
        expect(appInfo.hasIcon).toBe(true);
        return;
      }

      const appInfo = await app.getApplicationInfoForProtocol('https://');
      expect(appInfo.path).not.toBeUndefined();
      expect(appInfo.name).not.toBeUndefined();
      expect(appInfo.icon).not.toBeUndefined();
    });

    ifit(process.platform === 'linux')('resolves an executable name via PATH', async () => {
      const pathLookupExecutable = 'mock-browser';
      const pathLookupExecutablePath = path.join(xdgBinDir, pathLookupExecutable);
      const pathLookupDisplayName = 'Mock Browser PATH';
      const pathLookupScheme = 'mockproto-path';
      const pathLookupMimeType = `x-scheme-handler/${pathLookupScheme}`;

      fs.writeFileSync(pathLookupExecutablePath, '#!/bin/sh\nexit 0\n');
      fs.chmodSync(pathLookupExecutablePath, 0o755);
      writeProtocolAssociation(
        xdgDataHome,
        xdgConfigHome,
        'mock-browser-path.desktop',
        pathLookupDisplayName,
        `${pathLookupExecutable} %u`,
        pathLookupMimeType,
        fixtureIcon
      );

      const appInfo = await spawnProtocolInfoWithXdgMock(`${pathLookupScheme}://`, xdgDataHome, xdgConfigHome, {
        PATH: [xdgBinDir, process.env.PATH].filter(Boolean).join(':')
      });
      expect(appInfo.name).toBe(pathLookupDisplayName);
      expect(appInfo.path).toBe(pathLookupExecutablePath);
      expect(appInfo.hasIcon).toBe(true);
    });
  });

  describe('isDefaultProtocolClient()', () => {
    it('returns false for a bogus protocol', () => {
      expect(app.isDefaultProtocolClient('bogus-protocol://')).toBe(false);
    });
  });

  ifdescribe(process.platform === 'linux')('default protocol client APIs', () => {
    const protocol = 'electron-test-linux';
    const desktopFileId = 'electron-test.desktop';
    const protocolMimeType = `x-scheme-handler/${protocol}`;

    // GIO caches XDG directory paths at process startup, so we must
    // operate on the directories it is actually monitoring rather than
    // creating isolated temp dirs.
    const gioDataHome = process.env.XDG_DATA_HOME || path.join(os.homedir(), '.local', 'share');
    const gioConfigHome = process.env.XDG_CONFIG_HOME || path.join(os.homedir(), '.config');
    const desktopFileDst = path.join(gioDataHome, 'applications', desktopFileId);
    const mimeappsListPath = path.join(gioConfigHome, 'mimeapps.list');

    beforeEach(() => {
      const oldDesktopName = process.env.CHROME_DESKTOP;

      // Install the test .desktop file where GIO can discover it.
      fs.mkdirSync(path.dirname(desktopFileDst), { recursive: true });
      fs.copyFileSync(
        path.join(fixturesPath, 'api', 'xdg-mock', 'data', 'applications', desktopFileId),
        desktopFileDst
      );

      app.setDesktopName(desktopFileId);

      defer(() => {
        // Restore CHROME_DESKTOP.
        if (oldDesktopName !== undefined) {
          process.env.CHROME_DESKTOP = oldDesktopName;
        } else {
          delete process.env.CHROME_DESKTOP;
        }

        // Remove the test .desktop file.
        try {
          fs.unlinkSync(desktopFileDst);
        } catch {}

        // Remove any association for the test protocol from mimeapps.list.
        if (fs.existsSync(mimeappsListPath)) {
          const content = fs.readFileSync(mimeappsListPath, 'utf8');
          const cleaned = content
            .split('\n')
            .filter((line) => !line.includes(protocolMimeType))
            .join('\n');
          if (cleaned !== content) {
            fs.writeFileSync(mimeappsListPath, cleaned);
          }
        }
      });
    });

    it('sets and queries the default protocol client', async () => {
      expect(app.isDefaultProtocolClient(protocol)).toBe(false);

      // GIO needs to discover the newly installed .desktop file via inotify.
      await waitUntil(() => app.setAsDefaultProtocolClient(protocol));

      await waitUntil(() => app.isDefaultProtocolClient(protocol));
      expect(app.isDefaultProtocolClient(protocol)).toBe(true);

      // Changing identity should make the check return false.
      app.setDesktopName('other-app.desktop');
      expect(app.isDefaultProtocolClient(protocol)).toBe(false);
    });
  });

  describe('protocol scheme validation', () => {
    it('rejects empty protocol names', () => {
      expect(app.setAsDefaultProtocolClient('')).toBe(false);
      expect(app.isDefaultProtocolClient('')).toBe(false);
      expect(app.removeAsDefaultProtocolClient('')).toBe(false);
    });

    it('rejects non-conformant protocol names ', () => {
      // Starting with a digit.
      expect(app.setAsDefaultProtocolClient('0badscheme')).toBe(false);
      // Starting with a hyphen.
      expect(app.setAsDefaultProtocolClient('-badscheme')).toBe(false);
      // Containing backslashes.
      expect(app.setAsDefaultProtocolClient('http\\shell\\open\\command')).toBe(false);
      // Containing forward slashes.
      expect(app.setAsDefaultProtocolClient('bad/protocol')).toBe(false);
      // Containing spaces.
      expect(app.setAsDefaultProtocolClient('bad protocol')).toBe(false);
      // Containing colons.
      expect(app.setAsDefaultProtocolClient('bad:protocol')).toBe(false);
    });
  });

  ifdescribe(process.platform === 'win32')('app launch through uri', () => {
    it('does not launch for argument following a URL', async () => {
      const appPath = path.join(fixturesPath, 'api', 'quit-app');
      // App should exit with non 123 code.
      const first = cp.spawn(process.execPath, [appPath, 'electron-test:?', 'abc']);
      const [code] = await once(first, 'exit');
      expect(code).not.toBe(123);
    });

    it('launches successfully for argument following a file path', async () => {
      const appPath = path.join(fixturesPath, 'api', 'quit-app');
      // App should exit with code 123.
      const first = cp.spawn(process.execPath, [appPath, 'e:\\abc', 'abc']);
      const [code] = await once(first, 'exit');
      expect(code).toBe(123);
    });

    it('launches successfully for multiple URIs following --', async () => {
      const appPath = path.join(fixturesPath, 'api', 'quit-app');
      // App should exit with code 123.
      const first = cp.spawn(process.execPath, [appPath, '--', 'http://electronjs.org', 'electron-test://testdata']);
      const [code] = await once(first, 'exit');
      expect(code).toBe(123);
    });
  });

  // FIXME Get these specs running on Linux CI
  ifdescribe(process.platform !== 'linux')('getFileIcon() API', () => {
    const iconPath = path.join(import.meta.dirname, 'fixtures/assets/icon.ico');
    const sizes = {
      small: 16,
      normal: 32,
      large: process.platform === 'win32' ? 32 : 48
    };

    it('fetches a non-empty icon', async () => {
      const icon = await app.getFileIcon(iconPath);
      expect(icon.isEmpty()).toBe(false);
    });

    it('fetches normal icon size by default', async () => {
      const icon = await app.getFileIcon(iconPath);
      const size = icon.getSize();

      expect(size.height).toBe(sizes.normal);
      expect(size.width).toBe(sizes.normal);
    });

    describe('size option', () => {
      it('fetches a small icon', async () => {
        const icon = await app.getFileIcon(iconPath, { size: 'small' });
        const size = icon.getSize();

        expect(size.height).toBe(sizes.small);
        expect(size.width).toBe(sizes.small);
      });

      it('fetches a normal icon', async () => {
        const icon = await app.getFileIcon(iconPath, { size: 'normal' });
        const size = icon.getSize();

        expect(size.height).toBe(sizes.normal);
        expect(size.width).toBe(sizes.normal);
      });

      it('fetches a large icon', async () => {
        // macOS does not support large icons
        if (process.platform === 'darwin') return;

        const icon = await app.getFileIcon(iconPath, { size: 'large' });
        const size = icon.getSize();

        expect(size.height).toBe(sizes.large);
        expect(size.width).toBe(sizes.large);
      });
    });
  });

  describe('getAppMetrics() API', () => {
    it('returns memory and cpu stats of all running electron processes', () => {
      const appMetrics = app.getAppMetrics();
      expect(appMetrics).toBeInstanceOf(Array);
      expect(appMetrics.length, 'App memory info object is not > 0').toBeGreaterThanOrEqual(1);

      const types = [];
      for (const entry of appMetrics) {
        expect(entry.pid, 'pid is not > 0').toBeGreaterThan(0);
        expect(entry.type).toBeTypeOf('string');
        expect(entry.type).not.toBe('');
        expect(entry.creationTime).toBeTypeOf('number');
        expect(entry.creationTime).toBeGreaterThan(0);

        types.push(entry.type);
        expect(entry.cpu).toHaveProperty('percentCPUUsage', expect.any(Number));
        expect(entry.cpu).toHaveProperty('cumulativeCPUUsage', expect.any(Number));
        expect(entry.cpu).toHaveProperty('idleWakeupsPerSecond', expect.any(Number));

        expect(entry.memory).toHaveProperty('workingSetSize');
        expect(entry.memory.workingSetSize).toBeGreaterThan(0);
        expect(entry.memory).toHaveProperty('peakWorkingSetSize');
        expect(entry.memory.peakWorkingSetSize).toBeGreaterThan(0);

        if (entry.type === 'Utility' || entry.type === 'GPU') {
          expect(entry.serviceName).toBeTypeOf('string');
          expect(entry.serviceName).not.toBe('');
        }

        if (entry.type === 'Utility') {
          expect(entry).toHaveProperty('name', expect.any(String));
        }

        if (process.platform === 'win32') {
          expect(entry.memory).toHaveProperty('privateBytes');
          expect(entry.memory.privateBytes).toBeGreaterThan(0);
        }

        if (process.platform !== 'linux') {
          expect(entry.sandboxed).toBeTypeOf('boolean');
        }

        if (process.platform === 'win32') {
          expect(entry.integrityLevel).toBeTypeOf('string');
        }
      }

      if (process.platform === 'darwin') {
        expect(types).toContain('GPU');
      }

      expect(types).toContain('Browser');
    });
  });

  ifdescribe(process.platform === 'linux')('GDK_BACKEND', () => {
    const fixture = path.join(fixturesPath, 'apps', 'gdk-backend');
    const run = async (env: NodeJS.ProcessEnv) => {
      const child = cp.spawn(process.execPath, [fixture], { env, stdio: ['ignore', 'pipe', 'ignore'] });
      defer(() => {
        if (child.exitCode === null && child.signalCode === null) child.kill();
      });
      let out = '';
      child.stdout.on('data', (chunk) => {
        out += chunk;
      });
      await waitUntil(() => /GDK_BACKEND in child: .*\n/.test(out));
      return out.match(/GDK_BACKEND in child: (.*)\n/)![1];
    };

    it('is not added to the environment that child processes inherit', async () => {
      const { GDK_BACKEND: _, ...env } = process.env;
      expect(await run(env)).toBe('<unset>');
    });

    it('is left as the user set it', async () => {
      expect(await run({ ...process.env, GDK_BACKEND: 'x11' })).toBe('x11');
    });
  });

  describe('getGPUFeatureStatus() API', () => {
    it('returns the graphic features statuses', () => {
      const features = app.getGPUFeatureStatus();
      expect(features).toHaveProperty('webgl', expect.any(String));
      expect(features).toHaveProperty('gpu_compositing', expect.any(String));
    });
  });

  ifdescribe(!process.env.IS_ASAN)('getGPUInfo() API', () => {
    const appPath = path.join(fixturesPath, 'api', 'gpu-info.js');

    const getGPUInfo = async (type: string) => {
      const appProcess = cp.spawn(process.execPath, [appPath, type]);
      let gpuInfoData = '';
      let errorData = '';
      appProcess.stdout.on('data', (data) => {
        gpuInfoData += data;
      });
      appProcess.stderr.on('data', (data) => {
        errorData += data;
      });
      const [exitCode] = await once(appProcess, 'close');
      if (exitCode === 0) {
        try {
          const [, json] = /HERE COMES THE JSON: (.+) AND THERE IT WAS/.exec(gpuInfoData)!;
          // return info data on successful exit
          return JSON.parse(json);
        } catch (e) {
          console.error('Failed to interpret the following as JSON:');
          console.error(gpuInfoData);
          throw e;
        }
      } else {
        // return error if not clean exit
        throw new Error(errorData);
      }
    };
    const verifyBasicGPUInfo = async (gpuInfo: any) => {
      // Devices information is always present in the available info.
      expect(gpuInfo).toHaveProperty('gpuDevice');
      expect(gpuInfo.gpuDevice).toBeInstanceOf(Array);
      expect(gpuInfo.gpuDevice).not.toBe([]);

      const device = gpuInfo.gpuDevice[0];
      expect(device).toBeTypeOf('object');
      expect(device).toHaveProperty('deviceId');
      expect(device.deviceId).toBeTypeOf('number');
      expect(device.deviceId).not.toBeLessThan(0);
    };

    // On a headless runner with no usable GPU
    // (e.g. CI, where ANGLE is statically linked on Linux), the GPU process
    // fails to initialize and GPU access is disabled. Treat that as a skip
    // rather than a failure.
    const isGpuUnavailable = (error: Error) =>
      /GPU access (?:not allowed|is disabled)/i.test(error.message) ||
      /Exiting GPU process due to errors during initialization/i.test(error.message);

    it('succeeds with basic GPUInfo', async (ctx) => {
      let gpuInfo;
      try {
        gpuInfo = await getGPUInfo('basic');
      } catch (error) {
        if (isGpuUnavailable(error as Error)) return ctx.skip();
        throw error;
      }
      await verifyBasicGPUInfo(gpuInfo);
    });

    it('succeeds with complete GPUInfo', async (ctx) => {
      let completeInfo;
      try {
        completeInfo = await getGPUInfo('complete');
      } catch (error) {
        if (isGpuUnavailable(error as Error)) return ctx.skip();
        throw error;
      }
      if (process.platform === 'linux') {
        // For linux and macOS complete info is same as basic info
        await verifyBasicGPUInfo(completeInfo);
        const basicInfo = await getGPUInfo('basic');
        expect(completeInfo).toEqual(basicInfo);
      } else {
        // Gl version is present in the complete info.
        expect(completeInfo).toHaveProperty('auxAttributes', expect.any(Object));
        if (completeInfo.gpuDevice.active) {
          expect(completeInfo.auxAttributes).toHaveProperty('glVersion');
          expect(completeInfo.auxAttributes.glVersion).toBeTypeOf('string');
          expect(completeInfo.auxAttributes.glVersion).not.toBe([]);
        }
      }
    });

    it('fails for invalid info_type', () => {
      const invalidType = 'invalid';
      const expectedErrorMessage = "Invalid info type. Use 'basic' or 'complete'";
      return expect(app.getGPUInfo(invalidType as any)).rejects.toThrow(expectedErrorMessage);
    });
  });

  ifdescribe(!(process.platform === 'linux' && process.arch === 'arm64'))('sandbox options', () => {
    let appProcess: cp.ChildProcess = null as any;
    let server: net.Server = null as any;
    const socketPath =
      process.platform === 'win32' ? '\\\\.\\pipe\\electron-mixed-sandbox' : '/tmp/electron-mixed-sandbox';

    beforeEach(
      () =>
        new Promise<void>((resolve) => {
          fs.unlink(socketPath, () => {
            server = net.createServer();
            server.listen(socketPath);
            resolve();
          });
        })
    );

    afterEach(
      () =>
        new Promise<void>((resolve) => {
          if (appProcess != null) appProcess.kill();

          if (server) {
            server.close(() => {
              if (process.platform === 'win32') {
                resolve();
              } else {
                fs.unlink(socketPath, () => resolve());
              }
            });
          } else {
            resolve();
          }
        })
    );

    describe('when app.enableSandbox() is called', () => {
      it('adds --enable-sandbox to all renderer processes', () =>
        new Promise<void>((resolve, reject) => {
          const done = (error?: unknown) => (error ? reject(error) : resolve());
          const appPath = path.join(fixturesPath, 'api', 'mixed-sandbox-app');
          appProcess = cp.spawn(process.execPath, [appPath, '--app-enable-sandbox'], { stdio: 'inherit' });

          server.once('error', (error) => {
            done(error);
          });

          server.on('connection', (client) => {
            client.once('data', (data) => {
              const argv = JSON.parse(data.toString());
              expect(argv.sandbox).toContain('--enable-sandbox');
              expect(argv.sandbox).not.toContain('--no-sandbox');

              expect(argv.noSandbox).toContain('--enable-sandbox');
              expect(argv.noSandbox).not.toContain('--no-sandbox');

              expect(argv.noSandboxDevtools).toBe(true);
              expect(argv.sandboxDevtools).toBe(true);

              done();
            });
          });
        }));
    });

    describe('when the app is launched with --enable-sandbox', () => {
      it('adds --enable-sandbox to all renderer processes', () =>
        new Promise<void>((resolve, reject) => {
          const done = (error?: unknown) => (error ? reject(error) : resolve());
          const appPath = path.join(fixturesPath, 'api', 'mixed-sandbox-app');
          appProcess = cp.spawn(process.execPath, [appPath, '--enable-sandbox'], { stdio: 'inherit' });

          server.once('error', (error) => {
            done(error);
          });

          server.on('connection', (client) => {
            client.once('data', (data) => {
              const argv = JSON.parse(data.toString());
              expect(argv.sandbox).toContain('--enable-sandbox');
              expect(argv.sandbox).not.toContain('--no-sandbox');

              expect(argv.noSandbox).toContain('--enable-sandbox');
              expect(argv.noSandbox).not.toContain('--no-sandbox');

              expect(argv.noSandboxDevtools).toBe(true);
              expect(argv.sandboxDevtools).toBe(true);

              done();
            });
          });
        }));
    });
  });

  describe('disableDomainBlockingFor3DAPIs() API', () => {
    it('throws when called after app is ready', () => {
      expect(() => {
        app.disableDomainBlockingFor3DAPIs();
      }).toThrow(/before app is ready/);
    });
  });

  // Activation, hiding and the dock are per-machine state on macOS.
  //
  // A spec worker can start out as the active app without having shown a
  // window: macOS hands it activation when the spec process before it quits.
  // app.hide() from that inherited state can be ignored for good, so take
  // activation through a window of our own first and only then hide.
  const activateThroughOwnWindow = async () => {
    const w = new BrowserWindow({ width: 200, height: 200, show: false });
    w.show();
    await waitUntil(() => app.isActive() && w.isFocused());
    await closeWindow(w);
  };

  ifdescribe(process.platform === 'darwin')('app isActive API', { tags: ['serial'] }, () => {
    describe('app.isActive', () => {
      afterEach(closeAllWindows);

      it('returns true when the app becomes active', async () => {
        if (app.isActive()) {
          await activateThroughOwnWindow();
          app.hide();
          await waitUntil(() => !app.isActive());
        }

        const w = new BrowserWindow({
          width: 200,
          height: 200,
          show: false
        });

        w.show();

        await waitUntil(() => app.isActive());

        w.close();
        app.hide();
      });
    });
  });

  ifdescribe(process.platform === 'darwin')('app hide and show API', { tags: ['serial'] }, () => {
    describe('app.isHidden', () => {
      it('returns true when the app is hidden', async () => {
        if (app.isActive() && !app.isHidden()) {
          await activateThroughOwnWindow();
        }
        app.hide();
        await waitUntil(() => app.isHidden());
      });
      it('returns false when the app is shown', async () => {
        app.show();
        await waitUntil(() => !app.isHidden());
      });
    });
  });

  ifdescribe(process.platform === 'darwin')('dock APIs', { tags: ['serial'] }, () => {
    afterAll(async () => {
      await app.dock?.show();
    });

    describe('dock.setMenu', () => {
      it('can be retrieved via dock.getMenu', () => {
        expect(app.dock?.getMenu()).toBe(null);
        const menu = new Menu();
        app.dock?.setMenu(menu);
        expect(app.dock?.getMenu()).toBe(menu);
      });

      it('keeps references to the menu', () => {
        app.dock?.setMenu(new Menu());
        const v8Util = process._linkedBinding('electron_common_v8_util');
        v8Util.requestGarbageCollectionForTesting();
      });
    });

    describe('dock.setIcon', () => {
      it('throws a descriptive error for a bad icon path', () => {
        const badPath = path.resolve('I', 'Do', 'Not', 'Exist');
        expect(() => {
          app.dock?.setIcon(badPath);
        }).toThrow(/Failed to load image from path (.+)/);
      });
    });

    describe('dock.bounce', () => {
      it('should return -1 for unknown bounce type', () => {
        expect(app.dock?.bounce('bad type' as any)).toBe(-1);
      });

      it('should return a positive number for informational type', () => {
        if (!app.isActive()) {
          expect(app.dock?.bounce('informational')).toBeGreaterThanOrEqual(0);
        }
      });

      it('should return a positive number for critical type', () => {
        if (!app.isActive()) {
          expect(app.dock?.bounce('critical')).toBeGreaterThanOrEqual(0);
        }
      });
    });

    describe('dock.cancelBounce', () => {
      it('should not throw', () => {
        app.dock?.cancelBounce(app.dock?.bounce('critical'));
      });
    });

    describe('dock.setBadge', () => {
      afterAll(() => {
        app.dock?.setBadge('');
      });

      it('should not throw', () => {
        app.dock?.setBadge('1');
      });

      it('should be retrievable via getBadge', () => {
        app.dock?.setBadge('test');
        expect(app.dock?.getBadge()).toBe('test');
      });
    });

    describe('dock.hide', () => {
      it('should not throw', () => {
        app.dock?.hide();
        expect(app.dock?.isVisible()).toBe(false);
      });
    });

    // Note that dock.show tests should run after dock.hide tests, to work
    // around a bug of macOS.
    // See https://github.com/electron/electron/pull/25269 for more.
    describe('dock.show', () => {
      it('should not throw', () => {
        return app.dock?.show().then(() => {
          expect(app.dock?.isVisible()).toBe(true);
        });
      });

      it('returns a Promise', () => {
        expect(app.dock?.show()).toBeInstanceOf(Promise);
      });

      it('eventually fulfills', async () => {
        await expect(app.dock?.show()).resolves.toBe(undefined);
      });
    });
  });

  describe('whenReady', () => {
    it('returns a Promise', () => {
      expect(app.whenReady()).toBeInstanceOf(Promise);
    });

    it('becomes fulfilled if the app is already ready', async () => {
      expect(app.isReady()).toBe(true);
      await expect(app.whenReady()).resolves.toBe(undefined);
    });
  });

  describe('app.applicationMenu', () => {
    it('has the applicationMenu property', () => {
      expect(app).toHaveProperty('applicationMenu');
    });
  });

  describe('commandLine.hasSwitch', () => {
    afterEach(() => app.commandLine.removeSwitch('foobar1'));

    it('returns true when present', () => {
      app.commandLine.appendSwitch('foobar1');
      expect(app.commandLine.hasSwitch('foobar1')).toBe(true);
    });

    it('returns false when not present', () => {
      expect(app.commandLine.hasSwitch('foobar2')).toBe(false);
    });
  });

  describe('commandLine.hasSwitch (existing argv)', () => {
    it('returns true when present', async () => {
      const { hasSwitch } = await runTestApp('command-line', '--foobar');
      expect(hasSwitch).toBe(true);
    });

    it('returns false when not present', async () => {
      const { hasSwitch } = await runTestApp('command-line');
      expect(hasSwitch).toBe(false);
    });
  });

  describe('commandLine.getSwitchValue', () => {
    afterEach(() => {
      app.commandLine.removeSwitch('foobar');
      app.commandLine.removeSwitch('foobar1');
    });

    it('returns the value when present', () => {
      app.commandLine.appendSwitch('foobar', 'æøåü');
      expect(app.commandLine.getSwitchValue('foobar')).toBe('æøåü');
    });

    it('returns an empty string when present without value', () => {
      app.commandLine.appendSwitch('foobar1');
      expect(app.commandLine.getSwitchValue('foobar1')).toBe('');
    });

    it('returns an empty string when not present', () => {
      expect(app.commandLine.getSwitchValue('foobar2')).toBe('');
    });
  });

  describe('commandLine.getSwitchValue (existing argv)', () => {
    it('returns the value when present', async () => {
      const { getSwitchValue } = await runTestApp('command-line', '--foobar=test');
      expect(getSwitchValue).toBe('test');
    });

    it('returns an empty string when present without value', async () => {
      const { getSwitchValue } = await runTestApp('command-line', '--foobar');
      expect(getSwitchValue).toBe('');
    });

    it('returns an empty string when not present', async () => {
      const { getSwitchValue } = await runTestApp('command-line');
      expect(getSwitchValue).toBe('');
    });
  });

  describe('commandLine.removeSwitch', () => {
    it('no-ops a non-existent switch', async () => {
      expect(app.commandLine.hasSwitch('foobar3')).toBe(false);
      app.commandLine.removeSwitch('foobar3');
      expect(app.commandLine.hasSwitch('foobar3')).toBe(false);
    });

    it('removes an existing switch', async () => {
      app.commandLine.appendSwitch('foobar3', 'test');
      expect(app.commandLine.hasSwitch('foobar3')).toBe(true);
      app.commandLine.removeSwitch('foobar3');
      expect(app.commandLine.hasSwitch('foobar3')).toBe(false);
    });
  });

  ifdescribe(process.platform === 'darwin')('app.setSecureKeyboardEntryEnabled', () => {
    it('changes Secure Keyboard Entry is enabled', () => {
      app.setSecureKeyboardEntryEnabled(true);
      expect(app.isSecureKeyboardEntryEnabled()).toBe(true);
      app.setSecureKeyboardEntryEnabled(false);
      expect(app.isSecureKeyboardEntryEnabled()).toBe(false);
    });
  });

  describe('configureHostResolver', () => {
    afterAll(() => {
      // Returns to the default configuration.
      app.configureHostResolver({});
    });

    it('fails on bad arguments', () => {
      expect(() => {
        (app.configureHostResolver as any)();
      }).toThrow();
      expect(() => {
        app.configureHostResolver({
          secureDnsMode: 'notAValidValue' as any
        });
      }).toThrow();
      expect(() => {
        app.configureHostResolver({
          secureDnsServers: [123 as any]
        });
      }).toThrow();
    });

    it('affects dns lookup behavior', async () => {
      // 1. resolve a domain name to check that things are working
      await new Promise((resolve, reject) => {
        electronNet
          .request({
            method: 'HEAD',
            url: 'https://www.electronjs.org'
          })
          .on('response', resolve)
          .on('error', reject)
          .end();
      });
      // 2. change the host resolver configuration to something that will
      // always fail
      app.configureHostResolver({
        secureDnsMode: 'secure',
        secureDnsServers: ['https://127.0.0.1:1234']
      });
      // 3. check that resolving domain names now fails
      await expect(
        new Promise((resolve, reject) => {
          electronNet
            .request({
              method: 'HEAD',
              // Needs to be a slightly different domain to above, otherwise the
              // response will come from the cache.
              url: 'https://electronjs.org'
            })
            .on('response', resolve)
            .on('error', reject)
            .end();
        })
      ).rejects.toThrow(/ERR_NAME_NOT_RESOLVED/);
    });
  });

  describe('about panel', () => {
    it('app.setAboutPanelOptions() does not crash', () => {
      app.setAboutPanelOptions({
        applicationName: 'electron!!',
        version: '1.2.3'
      });
    });

    it('app.showAboutPanel() does not crash & runs asynchronously', () => {
      app.showAboutPanel();
    });
  });

  describe('app.setProxy(options)', () => {
    let server: http.Server;

    afterEach(async () => {
      if (server) {
        server.close();
      }
      await app.setProxy({ mode: 'direct' as const });
    });

    it('allows configuring proxy settings', async () => {
      const config = { proxyRules: 'http=myproxy:80' };
      await app.setProxy(config);
      const proxy = await app.resolveProxy('http://example.com/');
      expect(proxy).toBe('PROXY myproxy:80');
    });

    it('allows removing the implicit bypass rules for localhost', async () => {
      const config = {
        proxyRules: 'http=myproxy:80',
        proxyBypassRules: '<-loopback>'
      };

      await app.setProxy(config);
      const proxy = await app.resolveProxy('http://localhost');
      expect(proxy).toBe('PROXY myproxy:80');
    });

    it('allows configuring proxy settings with pacScript', async () => {
      server = http.createServer((req, res) => {
        const pac = `
          function FindProxyForURL(url, host) {
            return "PROXY myproxy:8132";
          }
        `;
        res.writeHead(200, {
          'Content-Type': 'application/x-ns-proxy-autoconfig'
        });
        res.end(pac);
      });
      const { url } = await listen(server);
      {
        const config = { pacScript: url };
        await app.setProxy(config);
        const proxy = await app.resolveProxy('https://google.com');
        expect(proxy).toBe('PROXY myproxy:8132');
      }
      {
        const config = { mode: 'pac_script' as any, pacScript: url };
        await app.setProxy(config);
        const proxy = await app.resolveProxy('https://google.com');
        expect(proxy).toBe('PROXY myproxy:8132');
      }
    });

    it('allows bypassing proxy settings', async () => {
      const config = {
        proxyRules: 'http=myproxy:80',
        proxyBypassRules: '<local>'
      };
      await app.setProxy(config);
      const proxy = await app.resolveProxy('http://example/');
      expect(proxy).toBe('DIRECT');
    });

    it('allows configuring proxy settings with mode `direct`', async () => {
      const config = { mode: 'direct' as const, proxyRules: 'http=myproxy:80' };
      await app.setProxy(config);
      const proxy = await app.resolveProxy('http://example.com/');
      expect(proxy).toBe('DIRECT');
    });

    it('allows configuring proxy settings with mode `auto_detect`', async () => {
      const config = { mode: 'auto_detect' as const };
      await app.setProxy(config);
    });

    it('allows configuring proxy settings with mode `pac_script`', async () => {
      const config = { mode: 'pac_script' as const };
      await app.setProxy(config);
      const proxy = await app.resolveProxy('http://example.com/');
      expect(proxy).toBe('DIRECT');
    });

    it('allows configuring proxy settings with mode `fixed_servers`', async () => {
      const config = { mode: 'fixed_servers' as const, proxyRules: 'http=myproxy:80' };
      await app.setProxy(config);
      const proxy = await app.resolveProxy('http://example.com/');
      expect(proxy).toBe('PROXY myproxy:80');
    });

    it('allows configuring proxy settings with mode `system`', async () => {
      const config = { mode: 'system' as const };
      await app.setProxy(config);
    });

    it('disallows configuring proxy settings with mode `invalid`', async () => {
      const config = { mode: 'invalid' as any };
      await expect(app.setProxy(config)).rejects.toThrow(/Invalid mode/);
    });

    it('impacts proxy for requests made from utility process', async () => {
      const fn = async () => {
        const urlRequest = electronNet.request('http://example.com/');
        const response = await getResponse(urlRequest);
        expect(response.statusCode).toBe(200);
        const message = await collectStreamBody(response);
        expect(message).toBe('ok from proxy\n');
      };
      server = http.createServer((req, res) => {
        res.writeHead(200);
        res.end('ok from proxy\n');
      });
      const { port, hostname } = await listen(server);
      const config = { mode: 'fixed_servers' as const, proxyRules: `http=${hostname}:${port}` };
      await app.setProxy(config);
      const proxy = await app.resolveProxy('http://example.com/');
      expect(proxy).toBe(`PROXY ${hostname}:${port}`);
      await runInUtilityProcess(fn);
    });

    it('does not impact proxy for requests made from main process', async () => {
      server = http.createServer((req, res) => {
        res.writeHead(200);
        res.end('ok from server\n');
      });
      const { url } = await listen(server);
      const config = { mode: 'fixed_servers' as const, proxyRules: 'http=myproxy:80' };
      await app.setProxy(config);
      const proxy = await app.resolveProxy('http://example.com/');
      expect(proxy).toBe('PROXY myproxy:80');
      const urlRequest = electronNet.request(url);
      const response = await getResponse(urlRequest);
      expect(response.statusCode).toBe(200);
      const message = await collectStreamBody(response);
      expect(message).toBe('ok from server\n');
    });
  });
});

describe('default behavior', () => {
  describe('application menu', () => {
    it('creates the default menu if the app does not set it', async () => {
      const result = await runTestApp('default-menu');
      expect(result).toBe(false);
    });

    it('does not create the default menu if the app sets a custom menu', async () => {
      const result = await runTestApp('default-menu', '--custom-menu');
      expect(result).toBe(true);
    });

    it('does not create the default menu if the app sets a null menu', async () => {
      const result = await runTestApp('default-menu', '--null-menu');
      expect(result).toBe(true);
    });
  });

  describe('window-all-closed', () => {
    afterEach(closeAllWindows);

    it('quits when the app does not handle the event', async () => {
      const result = await runTestApp('window-all-closed');
      expect(result).toBe(false);
    });

    it('does not quit when the app handles the event', async () => {
      const result = await runTestApp('window-all-closed', '--handle-event');
      expect(result).toBe(true);
    });

    it('should omit closed windows from getAllWindows', async () => {
      const w = new BrowserWindow({ show: false });
      const len = new Promise((resolve) => {
        app.on('window-all-closed', () => {
          resolve(BrowserWindow.getAllWindows().length);
        });
      });
      w.close();
      expect(await len).toBe(0);
    });
  });

  describe('user agent fallback', () => {
    let initialValue: string;

    beforeAll(() => {
      initialValue = app.userAgentFallback!;
    });

    it('should have a reasonable default', () => {
      expect(initialValue).toContain(`Electron/${process.versions.electron}`);
      expect(initialValue).toContain(`Chrome/${process.versions.chrome}`);
    });

    it('should be overridable', () => {
      app.userAgentFallback = 'test-agent/123';
      expect(app.userAgentFallback).toBe('test-agent/123');
    });

    it('should be restorable', () => {
      app.userAgentFallback = 'test-agent/123';
      app.userAgentFallback = '';
      expect(app.userAgentFallback).toBe(initialValue);
    });
  });

  describe('login event', () => {
    afterEach(closeAllWindows);
    let server: http.Server;
    let serverUrl: string;

    beforeAll(async () => {
      server = http.createServer((request, response) => {
        if (request.headers.authorization) {
          return response.end('ok');
        }
        response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
      });

      serverUrl = (await listen(server)).url;
    });

    afterAll(() => {
      server.close();
    });

    it('should emit a login event on app when a WebContents hits a 401', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL(serverUrl);
      const [, webContents] = (await once(app, 'login')) as [any, WebContents];
      expect(webContents).toBe(w.webContents);
    });
  });

  describe('running under ARM64 translation', () => {
    it('does not throw an error', () => {
      if (process.platform === 'darwin' || process.platform === 'win32') {
        expect(app.runningUnderARM64Translation).not.toBeUndefined();
        expect(() => {
          return app.runningUnderARM64Translation;
        }).not.toThrow();
      } else {
        expect(app.runningUnderARM64Translation).toBeUndefined();
      }
    });
  });
});

async function runTestApp(name: string, ...args: any[]) {
  const appPath = path.join(fixturesPath, 'api', name);
  const electronPath = process.execPath;
  const appProcess = cp.spawn(electronPath, [appPath, ...args]);

  let output = '';
  appProcess.stdout.on('data', (data) => {
    output += data;
  });

  await once(appProcess.stdout, 'end');

  return JSON.parse(output);
}
