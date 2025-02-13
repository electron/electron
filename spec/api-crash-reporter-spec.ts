import { app } from 'electron/main';

import * as Busboy from 'busboy';
import { expect } from 'chai';
import * as uuid from 'uuid';

import * as childProcess from 'node:child_process';
import { EventEmitter } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, ifit, defer, startRemoteControlApp, repeatedly, listen } from './lib/spec-helpers';

const isWindowsOnArm = process.platform === 'win32' && process.arch === 'arm64';
const isLinuxOnArm = process.platform === 'linux' && process.arch.includes('arm');

type CrashInfo = {
  prod: string
  ver: string
  process_type: string // eslint-disable-line camelcase
  ptype: string
  platform: string
  _productName: string
  _version: string
  upload_file_minidump: Buffer // eslint-disable-line camelcase
  guid: string
  mainProcessSpecific: 'mps' | undefined
  rendererSpecific: 'rs' | undefined
  globalParam: 'globalValue' | undefined
  addedThenRemoved: 'to-be-removed' | undefined
  longParam: string | undefined
  'electron.v8-fatal.location': string | undefined
  'electron.v8-fatal.message': string | undefined
}

function checkCrash (expectedProcessType: string, fields: CrashInfo) {
  expect(String(fields.prod)).to.equal('Electron', 'prod');
  expect(String(fields.ver)).to.equal(process.versions.electron, 'ver');
  expect(String(fields.ptype)).to.equal(expectedProcessType, 'ptype');
  expect(String(fields.process_type)).to.equal(expectedProcessType, 'process_type');
  expect(String(fields.platform)).to.equal(process.platform, 'platform');
  expect(String(fields._productName)).to.equal('Zombies', '_productName');
  expect(String(fields._version)).to.equal(app.getVersion(), '_version');
  expect(fields.upload_file_minidump).to.be.an.instanceOf(Buffer);

  // TODO(nornagon): minidumps are sometimes (not always) turning up empty on
  // 32-bit Linux.  Figure out why.
  if (!(process.platform === 'linux' && process.arch === 'ia32')) {
    expect(fields.upload_file_minidump.length).to.be.greaterThan(0);
  }
}

const startServer = async () => {
  const crashes: CrashInfo[] = [];
  function getCrashes () { return crashes; }
  const emitter = new EventEmitter();
  function waitForCrash (): Promise<CrashInfo> {
    return new Promise(resolve => {
      emitter.once('crash', (crash) => {
        resolve(crash);
      });
    });
  }

  const server = http.createServer((req, res) => {
    const busboy = Busboy({ headers: req.headers });
    const fields = {} as Record<string, any>;
    const files = {} as Record<string, Buffer>;
    busboy.on('file', (fieldname, file) => {
      const chunks = [] as Array<Buffer>;
      file.on('data', (chunk) => {
        chunks.push(chunk);
      });
      file.on('end', () => {
        files[fieldname] = Buffer.concat(chunks);
      });
    });
    busboy.on('field', (fieldname, val) => {
      fields[fieldname] = val;
    });
    busboy.on('finish', () => {
      // breakpad id must be 16 hex digits.
      const reportId = Math.random().toString(16).split('.')[1].padStart(16, '0');
      res.end(reportId, async () => {
        req.socket.destroy();
        emitter.emit('crash', { ...fields, ...files });
      });
    });
    req.pipe(busboy);
  });

  const { port } = await listen(server);

  defer(() => { server.close(); });

  return { getCrashes, port, waitForCrash };
};

function runApp (appPath: string, args: Array<string> = []) {
  const appProcess = childProcess.spawn(process.execPath, [appPath, ...args]);
  return new Promise(resolve => {
    appProcess.once('exit', resolve);
  });
}

function runCrashApp (crashType: string, port: number, extraArgs: Array<string> = []) {
  const appPath = path.join(__dirname, 'fixtures', 'apps', 'crash');
  return runApp(appPath, [
    `--crash-type=${crashType}`,
    `--crash-reporter-url=http://127.0.0.1:${port}`,
    ...extraArgs
  ]);
}

function waitForNewFileInDir (dir: string): Promise<string[]> {
  function readdirIfPresent (dir: string): string[] {
    try {
      return fs.readdirSync(dir);
    } catch {
      return [];
    }
  }
  const initialFiles = readdirIfPresent(dir);
  return new Promise(resolve => {
    const ivl = setInterval(() => {
      const newCrashFiles = readdirIfPresent(dir).filter(f => !initialFiles.includes(f));
      if (newCrashFiles.length) {
        clearInterval(ivl);
        resolve(newCrashFiles);
      }
    }, 1000);
  });
}

// TODO(nornagon): Fix tests on linux/arm.
ifdescribe(!isLinuxOnArm && !process.mas && !process.env.DISABLE_CRASH_REPORTER_TESTS)('crashReporter module', function () {
  describe('should send minidump', () => {
    it('when renderer crashes', async () => {
      const { port, waitForCrash } = await startServer();
      runCrashApp('renderer', port);
      const crash = await waitForCrash();
      checkCrash('renderer', crash);
      expect(crash.mainProcessSpecific).to.be.undefined();
    });

    it('when sandboxed renderer crashes', async () => {
      const { port, waitForCrash } = await startServer();
      runCrashApp('sandboxed-renderer', port);
      const crash = await waitForCrash();
      checkCrash('renderer', crash);
      expect(crash.mainProcessSpecific).to.be.undefined();
    });

    // TODO(nornagon): Minidump generation in main/node process on Linux/Arm is
    // broken (//components/crash prints "Failed to generate minidump"). Figure
    // out why.
    ifit(!isLinuxOnArm)('when main process crashes', async () => {
      const { port, waitForCrash } = await startServer();
      runCrashApp('main', port);
      const crash = await waitForCrash();
      checkCrash('browser', crash);
      expect(crash.mainProcessSpecific).to.equal('mps');
    });

    ifit(!isLinuxOnArm)('when a node process crashes', async () => {
      const { port, waitForCrash } = await startServer();
      runCrashApp('node', port);
      const crash = await waitForCrash();
      checkCrash('node', crash);
      expect(crash.mainProcessSpecific).to.be.undefined();
      expect(crash.rendererSpecific).to.be.undefined();
    });

    ifit(!isLinuxOnArm)('when a node process inside a node process crashes', async () => {
      const { port, waitForCrash } = await startServer();
      runCrashApp('node-fork', port);
      const crash = await waitForCrash();
      checkCrash('node', crash);
      expect(crash.mainProcessSpecific).to.be.undefined();
      expect(crash.rendererSpecific).to.be.undefined();
    });

    // Ensures that passing in crashpadHandlerPID flag for Linx child processes
    // does not affect child process args.
    ifit(process.platform === 'linux')('ensure linux child process args are not modified', async () => {
      const { port, waitForCrash } = await startServer();
      let exitCode: number | null = null;
      const appPath = path.join(__dirname, 'fixtures', 'apps', 'crash');
      const crashType = 'node-extra-args';
      const crashProcess = childProcess.spawn(process.execPath, [appPath,
        `--crash-type=${crashType}`,
        `--crash-reporter-url=http://127.0.0.1:${port}`
      ], { stdio: 'inherit' });
      crashProcess.once('close', (code) => {
        exitCode = code;
      });
      await waitForCrash();
      expect(exitCode).to.equal(0);
    });

    describe('with guid', () => {
      for (const processType of ['main', 'renderer', 'sandboxed-renderer']) {
        it(`when ${processType} crashes`, async () => {
          const { port, waitForCrash } = await startServer();
          runCrashApp(processType, port);
          const crash = await waitForCrash();
          expect(crash.guid).to.be.a('string');
        });
      }

      it('is a consistent id', async () => {
        let crash1Guid;
        let crash2Guid;
        {
          const { port, waitForCrash } = await startServer();
          runCrashApp('main', port);
          const crash = await waitForCrash();
          crash1Guid = crash.guid;
        }
        {
          const { port, waitForCrash } = await startServer();
          runCrashApp('main', port);
          const crash = await waitForCrash();
          crash2Guid = crash.guid;
        }
        expect(crash2Guid).to.equal(crash1Guid);
      });
    });

    describe('with extra parameters', () => {
      it('when renderer crashes', async () => {
        const { port, waitForCrash } = await startServer();
        runCrashApp('renderer', port, ['--set-extra-parameters-in-renderer']);
        const crash = await waitForCrash();
        checkCrash('renderer', crash);
        expect(crash.mainProcessSpecific).to.be.undefined();
        expect(crash.rendererSpecific).to.equal('rs');
        expect(crash.addedThenRemoved).to.be.undefined();
      });

      it('when sandboxed renderer crashes', async () => {
        const { port, waitForCrash } = await startServer();
        runCrashApp('sandboxed-renderer', port, ['--set-extra-parameters-in-renderer']);
        const crash = await waitForCrash();
        checkCrash('renderer', crash);
        expect(crash.mainProcessSpecific).to.be.undefined();
        expect(crash.rendererSpecific).to.equal('rs');
        expect(crash.addedThenRemoved).to.be.undefined();
      });

      it('contains v8 crash keys when a v8 crash occurs', async () => {
        const { remotely } = await startRemoteControlApp();
        const { port, waitForCrash } = await startServer();

        await remotely((port: number) => {
          require('electron').crashReporter.start({
            submitURL: `http://127.0.0.1:${port}`,
            compress: false,
            ignoreSystemCrashHandler: true
          });
        }, [port]);

        remotely(() => {
          const { BrowserWindow } = require('electron');
          const bw = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
          bw.loadURL('about:blank');
          bw.webContents.executeJavaScript('process._linkedBinding(\'electron_common_v8_util\').triggerFatalErrorForTesting()');
        });

        const crash = await waitForCrash();
        expect(crash.prod).to.equal('Electron');
        expect(crash._productName).to.equal('electron-test-remote-control');
        expect(crash.process_type).to.equal('renderer');
        expect(crash['electron.v8-fatal.location']).to.equal('v8::Context::New()');
        expect(crash['electron.v8-fatal.message']).to.equal('Circular extension dependency');
      });
    });
  });

  ifdescribe(!isLinuxOnArm)('extra parameter limits', () => {
    function stitchLongCrashParam (crash: any, paramKey: string) {
      if (crash[paramKey]) return crash[paramKey];
      let chunk = 1;
      let stitched = '';
      while (crash[`${paramKey}__${chunk}`]) {
        stitched += crash[`${paramKey}__${chunk}`];
        chunk++;
      }
      return stitched;
    }

    it('should truncate extra values longer than 5 * 4096 characters', async () => {
      const { port, waitForCrash } = await startServer();
      const { remotely } = await startRemoteControlApp();
      remotely((port: number) => {
        require('electron').crashReporter.start({
          submitURL: `http://127.0.0.1:${port}`,
          compress: false,
          ignoreSystemCrashHandler: true,
          extra: { longParam: 'a'.repeat(100000) }
        });
        setTimeout().then(() => process.crash());
      }, port);
      const crash = await waitForCrash();
      expect(stitchLongCrashParam(crash, 'longParam')).to.have.lengthOf(160 * 127, 'crash should have truncated longParam');
    });

    it('should omit extra keys with names longer than the maximum', async () => {
      const kKeyLengthMax = 39;
      const { port, waitForCrash } = await startServer();
      const { remotely } = await startRemoteControlApp();
      remotely((port: number, kKeyLengthMax: number) => {
        require('electron').crashReporter.start({
          submitURL: `http://127.0.0.1:${port}`,
          compress: false,
          ignoreSystemCrashHandler: true,
          extra: {
            ['a'.repeat(kKeyLengthMax + 10)]: 'value',
            ['b'.repeat(kKeyLengthMax)]: 'value',
            'not-long': 'not-long-value'
          }
        });
        require('electron').crashReporter.addExtraParameter('c'.repeat(kKeyLengthMax + 10), 'value');
        setTimeout().then(() => process.crash());
      }, port, kKeyLengthMax);
      const crash = await waitForCrash();
      expect(crash).not.to.have.property('a'.repeat(kKeyLengthMax + 10));
      expect(crash).not.to.have.property('a'.repeat(kKeyLengthMax));
      expect(crash).to.have.property('b'.repeat(kKeyLengthMax), 'value');
      expect(crash).to.have.property('not-long', 'not-long-value');
      expect(crash).not.to.have.property('c'.repeat(kKeyLengthMax + 10));
      expect(crash).not.to.have.property('c'.repeat(kKeyLengthMax));
    });
  });

  describe('globalExtra', () => {
    ifit(!isLinuxOnArm)('should be sent with main process dumps', async () => {
      const { port, waitForCrash } = await startServer();
      runCrashApp('main', port, ['--add-global-param=globalParam:globalValue']);
      const crash = await waitForCrash();
      expect(crash.globalParam).to.equal('globalValue');
    });

    it('should be sent with renderer process dumps', async () => {
      const { port, waitForCrash } = await startServer();
      runCrashApp('renderer', port, ['--add-global-param=globalParam:globalValue']);
      const crash = await waitForCrash();
      expect(crash.globalParam).to.equal('globalValue');
    });

    it('should be sent with sandboxed renderer process dumps', async () => {
      const { port, waitForCrash } = await startServer();
      runCrashApp('sandboxed-renderer', port, ['--add-global-param=globalParam:globalValue']);
      const crash = await waitForCrash();
      expect(crash.globalParam).to.equal('globalValue');
    });

    ifit(!isLinuxOnArm)('should not be overridden by extra in main process', async () => {
      const { port, waitForCrash } = await startServer();
      runCrashApp('main', port, ['--add-global-param=mainProcessSpecific:global']);
      const crash = await waitForCrash();
      expect(crash.mainProcessSpecific).to.equal('global');
    });

    ifit(!isLinuxOnArm)('should not be overridden by extra in renderer process', async () => {
      const { port, waitForCrash } = await startServer();
      runCrashApp('main', port, ['--add-global-param=rendererSpecific:global']);
      const crash = await waitForCrash();
      expect(crash.rendererSpecific).to.equal('global');
    });
  });

  // TODO(nornagon): also test crashing main / sandboxed renderers.
  ifit(!isWindowsOnArm)('should not send a minidump when uploadToServer is false', async () => {
    const { port, waitForCrash, getCrashes } = await startServer();
    waitForCrash().then(() => expect.fail('expected not to receive a dump'));
    await runCrashApp('renderer', port, ['--no-upload']);
    // wait a sec in case the crash reporter is about to upload a crash
    await setTimeout(1000);
    expect(getCrashes()).to.have.length(0);
  });

  describe('getUploadedReports', () => {
    it('returns an array of reports', async () => {
      const { remotely } = await startRemoteControlApp();
      await remotely(() => {
        require('electron').crashReporter.start({ submitURL: 'http://127.0.0.1' });
      });
      const reports = await remotely(() => require('electron').crashReporter.getUploadedReports());
      expect(reports).to.be.an('array');
    });
  });

  // TODO(nornagon): re-enable on woa
  ifdescribe(!isWindowsOnArm)('getLastCrashReport', () => {
    it('returns the last uploaded report', async () => {
      const { remotely } = await startRemoteControlApp();
      const { port, waitForCrash } = await startServer();

      // 0. clear the crash reports directory.
      const dir = await remotely(() => require('electron').app.getPath('crashDumps'));
      try {
        fs.rmdirSync(dir, { recursive: true });
        fs.mkdirSync(dir);
      } catch { /* ignore */ }

      // 1. start the crash reporter.
      await remotely((port: number) => {
        require('electron').crashReporter.start({
          submitURL: `http://127.0.0.1:${port}`,
          compress: false,
          ignoreSystemCrashHandler: true
        });
      }, [port]);
      // 2. generate a crash in the renderer.
      remotely(() => {
        const { BrowserWindow } = require('electron');
        const bw = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
        bw.loadURL('about:blank');
        bw.webContents.executeJavaScript('process.crash()');
      });
      await waitForCrash();
      // 3. get the crash from getLastCrashReport.
      const firstReport = await repeatedly(
        () => remotely(() => require('electron').crashReporter.getLastCrashReport())
      );
      expect(firstReport).to.not.be.null();
      expect(firstReport.date).to.be.an.instanceOf(Date);
      expect((Date.now()) - (+firstReport.date)).to.be.lessThan(30000);
    });
  });

  describe('getParameters', () => {
    it('returns all of the current parameters', async () => {
      const { remotely } = await startRemoteControlApp();
      await remotely(() => {
        require('electron').crashReporter.start({
          submitURL: 'http://127.0.0.1',
          extra: { extra1: 'hi' }
        });
      });
      const parameters = await remotely(() => require('electron').crashReporter.getParameters());
      expect(parameters).to.have.property('extra1', 'hi');
    });

    it('reflects added and removed parameters', async () => {
      const { remotely } = await startRemoteControlApp();
      await remotely(() => {
        require('electron').crashReporter.start({ submitURL: 'http://127.0.0.1' });
        require('electron').crashReporter.addExtraParameter('hello', 'world');
      });
      {
        const parameters = await remotely(() => require('electron').crashReporter.getParameters());
        expect(parameters).to.have.property('hello', 'world');
      }

      await remotely(() => { require('electron').crashReporter.removeExtraParameter('hello'); });

      {
        const parameters = await remotely(() => require('electron').crashReporter.getParameters());
        expect(parameters).not.to.have.property('hello');
      }
    });

    it('can be called in the renderer', async () => {
      const { remotely } = await startRemoteControlApp();
      const rendererParameters = await remotely(async () => {
        const { crashReporter, BrowserWindow } = require('electron');
        crashReporter.start({ submitURL: 'http://' });
        const bw = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
        bw.loadURL('about:blank');
        await bw.webContents.executeJavaScript('require(\'electron\').crashReporter.addExtraParameter(\'hello\', \'world\')');
        return bw.webContents.executeJavaScript('require(\'electron\').crashReporter.getParameters()');
      });
      expect(rendererParameters).to.deep.equal({ hello: 'world' });
    });

    it('can be called in a node child process', async () => {
      function slurp (stream: NodeJS.ReadableStream): Promise<string> {
        return new Promise((resolve, reject) => {
          const chunks: Buffer[] = [];
          stream.on('data', chunk => { chunks.push(chunk); });
          stream.on('end', () => resolve(Buffer.concat(chunks).toString('utf8')));
          stream.on('error', e => reject(e));
        });
      }
      // TODO(nornagon): how to enable crashpad in a node child process...?
      const child = childProcess.fork(path.join(__dirname, 'fixtures', 'module', 'print-crash-parameters.js'), [], { silent: true });
      const output = await slurp(child.stdout!);
      expect(JSON.parse(output)).to.deep.equal({ hello: 'world' });
    });
  });

  describe('crash dumps directory', () => {
    it('is set by default', () => {
      expect(app.getPath('crashDumps')).to.be.a('string');
    });

    it('is inside the user data dir', () => {
      expect(app.getPath('crashDumps')).to.include(app.getPath('userData'));
    });

    function crash (processType: string, remotely: Function) {
      if (processType === 'main') {
        return remotely(() => {
          setTimeout().then(() => { process.crash(); });
        });
      } else if (processType === 'renderer') {
        return remotely(() => {
          const { BrowserWindow } = require('electron');
          const bw = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
          bw.loadURL('about:blank');
          bw.webContents.executeJavaScript('process.crash()');
        });
      } else if (processType === 'sandboxed-renderer') {
        const preloadPath = path.join(__dirname, 'fixtures', 'apps', 'crash', 'sandbox-preload.js');
        return remotely((preload: string) => {
          const { BrowserWindow } = require('electron');
          const bw = new BrowserWindow({ show: false, webPreferences: { sandbox: true, preload, contextIsolation: false } });
          bw.loadURL('about:blank');
        }, preloadPath);
      } else if (processType === 'node') {
        const crashScriptPath = path.join(__dirname, 'fixtures', 'apps', 'crash', 'node-crash.js');
        return remotely((crashScriptPath: string) => {
          const { app } = require('electron');
          const childProcess = require('node:child_process');
          const version = app.getVersion();
          const url = 'http://127.0.0.1';
          childProcess.fork(crashScriptPath, [url, version], { silent: true });
        }, crashScriptPath);
      }
    }

    const processList = process.platform === 'linux'
      ? ['main', 'renderer', 'sandboxed-renderer']
      : ['main', 'renderer', 'sandboxed-renderer', 'node'];
    for (const crashingProcess of processList) {
      describe(`when ${crashingProcess} crashes`, () => {
        it('stores crashes in the crash dump directory when uploadToServer: false', async () => {
          const { remotely } = await startRemoteControlApp();
          const crashesDir = await remotely(() => {
            const { crashReporter, app } = require('electron');
            crashReporter.start({ submitURL: 'http://127.0.0.1', uploadToServer: false, ignoreSystemCrashHandler: true });
            return app.getPath('crashDumps');
          });
          let reportsDir = crashesDir;
          if (process.platform === 'darwin' || process.platform === 'linux') {
            reportsDir = path.join(crashesDir, 'completed');
          } else if (process.platform === 'win32') {
            reportsDir = path.join(crashesDir, 'reports');
          }
          const newFileAppeared = waitForNewFileInDir(reportsDir);
          crash(crashingProcess, remotely);
          const newFiles = await newFileAppeared;
          expect(newFiles.length).to.be.greaterThan(0);
          expect(newFiles[0]).to.match(/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\.dmp$/);
        });

        it('respects an overridden crash dump directory', async () => {
          const { remotely } = await startRemoteControlApp();
          const crashesDir = path.join(app.getPath('temp'), uuid.v4());
          const remoteCrashesDir = await remotely((crashesDir: string) => {
            const { crashReporter, app } = require('electron');
            app.setPath('crashDumps', crashesDir);
            crashReporter.start({ submitURL: 'http://127.0.0.1', uploadToServer: false, ignoreSystemCrashHandler: true });
            return app.getPath('crashDumps');
          }, crashesDir);
          expect(remoteCrashesDir).to.equal(crashesDir);

          let reportsDir = crashesDir;
          if (process.platform === 'darwin' || process.platform === 'linux') {
            reportsDir = path.join(crashesDir, 'completed');
          } else if (process.platform === 'win32') {
            reportsDir = path.join(crashesDir, 'reports');
          }
          const newFileAppeared = waitForNewFileInDir(reportsDir);
          crash(crashingProcess, remotely);
          const newFiles = await newFileAppeared;
          expect(newFiles.length).to.be.greaterThan(0);
          expect(newFiles[0]).to.match(/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\.dmp$/);
        });
      });
    }
  });

  describe('start() option validation', () => {
    it('requires that the submitURL option be specified', async () => {
      const { remotely } = await startRemoteControlApp();
      await expect(remotely(() => {
        const { crashReporter } = require('electron');
        crashReporter.start({} as any);
      })).to.be.rejectedWith('submitURL must be specified when uploadToServer is true');
    });

    it('allows the submitURL option to be omitted when uploadToServer is false', async () => {
      const { remotely } = await startRemoteControlApp();
      await expect(remotely(() => {
        const { crashReporter } = require('electron');
        crashReporter.start({ uploadToServer: false } as any);
      })).to.be.fulfilled();
    });

    it('can be called twice', async () => {
      const { remotely } = await startRemoteControlApp();
      await expect(remotely(() => {
        const { crashReporter } = require('electron');
        crashReporter.start({ submitURL: 'http://127.0.0.1' });
        crashReporter.start({ submitURL: 'http://127.0.0.1' });
      })).to.be.fulfilled();
    });
  });

  describe('getUploadToServer()', () => {
    it('returns true when uploadToServer is set to true (by default)', async () => {
      const { remotely } = await startRemoteControlApp();

      await remotely(() => { require('electron').crashReporter.start({ submitURL: 'http://127.0.0.1' }); });
      const uploadToServer = await remotely(() => require('electron').crashReporter.getUploadToServer());
      expect(uploadToServer).to.be.true();
    });

    it('returns false when uploadToServer is set to false in init', async () => {
      const { remotely } = await startRemoteControlApp();
      await remotely(() => { require('electron').crashReporter.start({ submitURL: 'http://127.0.0.1', uploadToServer: false }); });
      const uploadToServer = await remotely(() => require('electron').crashReporter.getUploadToServer());
      expect(uploadToServer).to.be.false();
    });

    it('is updated by setUploadToServer', async () => {
      const { remotely } = await startRemoteControlApp();
      await remotely(() => { require('electron').crashReporter.start({ submitURL: 'http://127.0.0.1' }); });
      await remotely(() => { require('electron').crashReporter.setUploadToServer(false); });
      expect(await remotely(() => require('electron').crashReporter.getUploadToServer())).to.be.false();
      await remotely(() => { require('electron').crashReporter.setUploadToServer(true); });
      expect(await remotely(() => require('electron').crashReporter.getUploadToServer())).to.be.true();
    });
  });

  describe('when not started', () => {
    it('does not prevent process from crashing', async () => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'cookie-app');
      await runApp(appPath);
    });
  });
});
