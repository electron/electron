import { expect } from 'chai';
import * as childProcess from 'child_process';
import * as http from 'http';
import * as Busboy from 'busboy';
import * as path from 'path';
import { ifdescribe, ifit } from './spec-helpers';
import * as temp from 'temp';
import { app } from 'electron/main';
import { crashReporter } from 'electron/common';
import { AddressInfo } from 'net';
import { EventEmitter } from 'events';
import * as fs from 'fs';
import * as v8 from 'v8';

temp.track();

const isWindowsOnArm = process.platform === 'win32' && process.arch === 'arm64';

const afterTest: ((() => void) | (() => Promise<void>))[] = [];
async function cleanup () {
  for (const cleanup of afterTest) {
    const r = cleanup();
    if (r instanceof Promise) { await r; }
  }
  afterTest.length = 0;
}

type CrashInfo = {
  prod: string
  ver: string
  process_type: string // eslint-disable-line camelcase
  platform: string
  extra1: string
  extra2: string
  extra3: undefined
  _productName: string
  _companyName: string
  _version: string
  upload_file_minidump: Buffer // eslint-disable-line camelcase
}

function checkCrash (expectedProcessType: string, fields: CrashInfo) {
  expect(String(fields.prod)).to.equal('Electron');
  expect(String(fields.ver)).to.equal(process.versions.electron);
  expect(String(fields.process_type)).to.equal(expectedProcessType);
  expect(String(fields.platform)).to.equal(process.platform);
  expect(String(fields._productName)).to.equal('Zombies');
  expect(String(fields._companyName)).to.equal('Umbrella Corporation');
  expect(String(fields._version)).to.equal(app.getVersion());
  expect(fields.upload_file_minidump).to.be.an.instanceOf(Buffer);
  expect(fields.upload_file_minidump.length).to.be.greaterThan(0);
}

function checkCrashExtra (fields: CrashInfo) {
  expect(String(fields.extra1)).to.equal('extra1');
  expect(String(fields.extra2)).to.equal('extra2');
  expect(fields.extra3).to.be.undefined();
}

const startRemoteControlApp = async () => {
  const appPath = path.join(__dirname, 'fixtures', 'apps', 'remote-control');
  const appProcess = childProcess.spawn(process.execPath, [appPath]);
  const port = await new Promise<number>(resolve => {
    appProcess.stdout.on('data', d => {
      const m = /Listening: (\d+)/.exec(d.toString());
      if (m && m[1] != null) {
        resolve(Number(m[1]));
      }
    });
  });
  function remoteEval (js: string): any {
    return new Promise((resolve, reject) => {
      const req = http.request({
        host: '127.0.0.1',
        port,
        method: 'POST'
      }, res => {
        const chunks = [] as Buffer[];
        res.on('data', chunk => { chunks.push(chunk); });
        res.on('end', () => {
          const ret = v8.deserialize(Buffer.concat(chunks));
          if (Object.prototype.hasOwnProperty.call(ret, 'error')) {
            reject(new Error(`remote error: ${ret.error}\n\nTriggered at:`));
          } else {
            resolve(ret.result);
          }
        });
      });
      req.write(js);
      req.end();
    });
  }
  afterTest.push(() => { appProcess.kill('SIGINT'); });
  return { remoteEval };
};

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
    const busboy = new Busboy({ headers: req.headers });
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
      const reportId = 'abc-123-def-456-abc-789-abc-123-abcd';
      res.end(reportId, async () => {
        req.socket.destroy();
        emitter.emit('crash', { ...fields, ...files });
      });
    });
    req.pipe(busboy);
  });

  await new Promise(resolve => {
    server.listen(0, '127.0.0.1', () => { resolve(); });
  });

  const port = (server.address() as AddressInfo).port;

  afterTest.push(() => { server.close(); });

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
    } catch (e) {
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

// TODO(alexeykuzmin): [Ch66] This test fails on Linux. Fix it and enable back.
ifdescribe(!process.mas && !process.env.DISABLE_CRASH_REPORTER_TESTS && process.platform !== 'linux')('crashReporter module', function () {
  afterEach(cleanup);

  it('should send minidump when renderer crashes', async () => {
    const { port, waitForCrash } = await startServer();
    runCrashApp('renderer', port);
    const crash = await waitForCrash();
    checkCrash('renderer', crash);
  });

  it('should send minidump when sandboxed renderer crashes', async () => {
    const { port, waitForCrash } = await startServer();
    runCrashApp('sandboxed-renderer', port);
    const crash = await waitForCrash();
    checkCrash('renderer', crash);
    checkCrashExtra(crash);
  });

  it('should send minidump with updated parameters when renderer crashes', async () => {
    const { port, waitForCrash } = await startServer();
    runCrashApp('renderer', port, ['--set-extra-parameters-in-renderer']);
    const crash = await waitForCrash();
    checkCrash('renderer', crash);
    expect(crash.extra1).to.be.undefined();
    expect(crash.extra2).to.equal('extra2');
    expect(crash.extra3).to.equal('added');
  });

  it('should send minidump with updated parameters when sandboxed renderer crashes', async () => {
    const { port, waitForCrash } = await startServer();
    runCrashApp('sandboxed-renderer', port, ['--set-extra-parameters-in-renderer']);
    const crash = await waitForCrash();
    checkCrash('renderer', crash);
    expect(crash.extra1).to.be.undefined();
    expect(crash.extra2).to.equal('extra2');
    expect(crash.extra3).to.equal('added');
  });

  it('should send minidump when main process crashes', async () => {
    const { port, waitForCrash } = await startServer();
    runCrashApp('main', port);
    const crash = await waitForCrash();
    checkCrash('browser', crash);
    checkCrashExtra(crash);
  });

  it('should send minidump when a node process crashes', async () => {
    const { port, waitForCrash } = await startServer();
    runCrashApp('node', port);
    const crash = await waitForCrash();
    checkCrash('node', crash);
    checkCrashExtra(crash);
  });

  // TODO(jeremy): re-enable on woa
  ifit(!isWindowsOnArm)('should not send a minidump when uploadToServer is false', async () => {
    const { port, getCrashes } = await startServer();
    const crashesDir = path.join(app.getPath('temp'), 'Zombies Crashes');
    const completedCrashesDir = path.join(crashesDir, 'completed');
    const crashAppeared = waitForNewFileInDir(completedCrashesDir);
    await runCrashApp('renderer', port, ['--no-upload']);
    await crashAppeared;
    // wait a sec in case crashpad is about to upload a crash
    await new Promise(resolve => setTimeout(resolve, 1000));
    expect(getCrashes()).to.have.length(0);
  });

  describe('start() option validation', () => {
    it('requires that the companyName option be specified', () => {
      expect(() => {
        crashReporter.start({ companyName: 'dummy' } as any);
      }).to.throw('submitURL is a required option to crashReporter.start');
    });

    it('requires that the submitURL option be specified', () => {
      expect(() => {
        crashReporter.start({ submitURL: 'dummy' } as any);
      }).to.throw('companyName is a required option to crashReporter.start');
    });

    it('can be called twice', async () => {
      const { remoteEval } = await startRemoteControlApp();

      await remoteEval(`require('electron').crashReporter.start({companyName: "Umbrella Corporation", submitURL: "http://127.0.0.1"})`);
      await remoteEval(`require('electron').crashReporter.start({companyName: "Umbrella Corporation", submitURL: "http://127.0.0.1"})`);
    });
  });

  describe('getCrashesDirectory', () => {
    it('correctly returns the directory', async () => {
      const { remoteEval } = await startRemoteControlApp();
      await remoteEval(`require('electron').crashReporter.start({companyName: "Umbrella Corporation", submitURL: "http://127.0.0.1"})`);
      const crashesDir = await remoteEval(`require('electron').crashReporter.getCrashesDirectory()`);
      const dir = path.join(app.getPath('temp'), 'remote-control Crashes');
      expect(crashesDir).to.equal(dir);
    });
  });

  describe('getUploadedReports', () => {
    it('returns an array of reports', async () => {
      const { remoteEval } = await startRemoteControlApp();
      await remoteEval(`require('electron').crashReporter.start({companyName: "Umbrella Corporation", submitURL: "http://127.0.0.1"})`);
      const reports = await remoteEval(`require('electron').crashReporter.getUploadedReports()`);
      expect(reports).to.be.an('array');
    });
  });

  // TODO(jeremy): re-enable on woa
  ifdescribe(!isWindowsOnArm)('getLastCrashReport', () => {
    it('returns the last uploaded report', async () => {
      const { remoteEval } = await startRemoteControlApp();
      const { port, waitForCrash } = await startServer();

      // 0. clear the crash reports directory.
      const dir = path.join(app.getPath('temp'), 'remote-control Crashes');
      try {
        fs.rmdirSync(dir, { recursive: true });
      } catch (e) { /* ignore */ }

      // 1. start the crash reporter.
      await remoteEval(`require('electron').crashReporter.start({companyName: "Umbrella Corporation", submitURL: "http://127.0.0.1:${port}", ignoreSystemCrashHandler: true})`);
      // 2. generate a crash.
      remoteEval(`(function() { const {BrowserWindow} = require('electron'); const bw = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}}); bw.loadURL('about:blank'); bw.webContents.executeJavaScript('process.crash()') })()`);
      await waitForCrash();
      // 3. get the crash from getLastCrashReport.
      const firstReport = await remoteEval(`require('electron').crashReporter.getLastCrashReport()`);
      expect(firstReport).to.not.be.null();
      expect(firstReport.date).to.be.an.instanceOf(Date);
    });
  });

  describe('getUploadToServer()', () => {
    it('returns true when uploadToServer is set to true (by default)', async () => {
      const { remoteEval } = await startRemoteControlApp();

      await remoteEval(`require('electron').crashReporter.start({companyName: "Umbrella Corporation", submitURL: "http://127.0.0.1"})`);
      const uploadToServer = await remoteEval(`require('electron').crashReporter.getUploadToServer()`);
      expect(uploadToServer).to.be.true();
    });

    it('returns false when uploadToServer is set to false in init', async () => {
      const { remoteEval } = await startRemoteControlApp();
      await remoteEval(`require('electron').crashReporter.start({companyName: "Umbrella Corporation", submitURL: "http://127.0.0.1", uploadToServer: false})`);
      const uploadToServer = await remoteEval(`require('electron').crashReporter.getUploadToServer()`);
      expect(uploadToServer).to.be.false();
    });

    it('is updated by setUploadToServer', async () => {
      const { remoteEval } = await startRemoteControlApp();
      await remoteEval(`require('electron').crashReporter.start({companyName: "Umbrella Corporation", submitURL: "http://127.0.0.1"})`);
      await remoteEval(`require('electron').crashReporter.setUploadToServer(false)`);
      expect(await remoteEval(`require('electron').crashReporter.getUploadToServer()`)).to.be.false();
      await remoteEval(`require('electron').crashReporter.setUploadToServer(true)`);
      expect(await remoteEval(`require('electron').crashReporter.getUploadToServer()`)).to.be.true();
    });
  });

  describe('Parameters', () => {
    it('returns all of the current parameters', async () => {
      const { remoteEval } = await startRemoteControlApp();
      await remoteEval(`require('electron').crashReporter.start({companyName: "Umbrella Corporation", submitURL: "http://127.0.0.1", extra: {"extra1": "hi"}})`);
      const parameters = await remoteEval(`require('electron').crashReporter.getParameters()`);
      expect(parameters).to.be.an('object');
      expect(parameters.extra1).to.equal('hi');
    });

    it('adds and removes parameters', async () => {
      const { remoteEval } = await startRemoteControlApp();
      await remoteEval(`require('electron').crashReporter.start({companyName: "Umbrella Corporation", submitURL: "http://127.0.0.1"})`);
      await remoteEval(`require('electron').crashReporter.addExtraParameter('hello', 'world')`);
      {
        const parameters = await remoteEval(`require('electron').crashReporter.getParameters()`);
        expect(parameters).to.have.property('hello');
        expect(parameters.hello).to.equal('world');
      }

      {
        await remoteEval(`require('electron').crashReporter.removeExtraParameter('hello')`);
        const parameters = await remoteEval(`require('electron').crashReporter.getParameters()`);
        expect(parameters).not.to.have.property('hello');
      }
    });
  });

  describe('when not started', () => {
    it('does not prevent process from crashing', async () => {
      const appPath = path.join(__dirname, '..', 'spec', 'fixtures', 'api', 'cookie-app');
      await runApp(appPath);
    });
  });
});
