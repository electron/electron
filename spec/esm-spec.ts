import { BrowserWindow } from 'electron';

import { expect } from 'chai';

import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { pathToFileURL } from 'node:url';
import { stripVTControlCharacters } from 'node:util';

const fixtureTimeout = 20000;

// ---------------------------------------------------------------------------
// DO-NOT-MERGE: instrumentation for #51462. The import-meta fixture child
// reaches applicationWillFinishLaunching but never gets
// applicationDidFinishLaunching on macOS-x64 shard 2. This wrapper captures
// LaunchServices state, process list, sample, spindump and AppleEvent debug
// output from the child the moment it is observed hung, then kills it.
// All output goes to spec/artifacts/esm-diag/ which is uploaded as
// test_artifacts_<platform>_<shard>.
// ---------------------------------------------------------------------------
const diagDir = path.resolve(__dirname, 'artifacts', 'esm-diag');
fs.mkdirSync(diagDir, { recursive: true });

const runDiag = (label: string, cmd: string, args: string[]) => {
  try {
    const out = cp.spawnSync(cmd, args, { encoding: 'utf8', timeout: 90_000 });
    const file = path.join(diagDir, `${label}.txt`);
    fs.writeFileSync(
      file,
      `# ${cmd} ${args.join(' ')}\n# exit=${out.status} signal=${out.signal}\n` +
      `--- stdout ---\n${out.stdout ?? ''}\n--- stderr ---\n${out.stderr ?? ''}\n`
    );
    process.stderr.write(`[esm-diag] wrote ${file}\n`);
  } catch (e) {
    process.stderr.write(`[esm-diag] ${label} failed: ${e}\n`);
  }
};

const captureSavedState = (tag: string) => {
  const home = os.homedir();
  // Persistent UI saved-state for the bundle — if this exists with restorecount
  // or windows.plist the open-application handler may run state restoration.
  runDiag(`${tag}-savedstate-ls`, 'sh', ['-c',
    `ls -laR "${home}/Library/Saved Application State/com.github.Electron.savedState" 2>&1 || echo "<no savedState dir>"`]);
  runDiag(`${tag}-savedstate-restorecount`, 'sh', ['-c',
    `plutil -p "${home}/Library/Saved Application State/com.github.Electron.savedState/restorecount.plist" 2>&1 || echo "<no restorecount>"`]);
  runDiag(`${tag}-savedstate-windows`, 'sh', ['-c',
    `plutil -p "${home}/Library/Saved Application State/com.github.Electron.savedState/windows.plist" 2>&1 || echo "<no windows.plist>"`]);
  // Any container saved state too (sandboxed path).
  runDiag(`${tag}-savedstate-container-ls`, 'sh', ['-c',
    `ls -laR "${home}/Library/Containers/com.github.Electron" 2>&1 || echo "<no container>"`]);
  // NSUserDefaults for the bundle.
  runDiag(`${tag}-defaults`, 'defaults', ['read', 'com.github.Electron']);
};

const captureHangDiagnostics = (pid: number, tag: string) => {
  process.stderr.write(`[esm-diag] capturing diagnostics for hung child pid=${pid} tag=${tag}\n`);
  // Saved-state / persistent UI — leading theory after first spindump.
  captureSavedState(tag);
  // Screenshot the whole screen — if AppKit is showing a modal alert ("Reopen
  // windows?" / crash dialog) we'll see it. Also screenshot just the child's
  // windows if any exist.
  runDiag(`${tag}-screencapture`, 'screencapture', ['-x', '-t', 'png', path.join(diagDir, `${tag}-screen.png`)]);
  // Enumerate on-screen windows owned by the hung pid (via CoreGraphics).
  runDiag(`${tag}-cgwindows`, '/usr/bin/python3', ['-c',
    'import Quartz,json,sys;pid=int(sys.argv[1]);' +
    'ws=Quartz.CGWindowListCopyWindowInfo(Quartz.kCGWindowListOptionAll,Quartz.kCGNullWindowID) or [];' +
    'print(json.dumps([dict(w) for w in ws if w.get("kCGWindowOwnerPID")==pid],default=str,indent=2))',
    String(pid)]);
  // All on-screen windows (any owner) so we can spot a system alert.
  runDiag(`${tag}-cgwindows-all`, '/usr/bin/python3', ['-c',
    'import Quartz,json;' +
    'ws=Quartz.CGWindowListCopyWindowInfo(Quartz.kCGWindowListOptionOnScreenOnly,Quartz.kCGNullWindowID) or [];' +
    'print(json.dumps([{k:str(v) for k,v in dict(w).items()} for w in ws],indent=2))']);
  // LaunchServices state.
  runDiag(`${tag}-lsappinfo-list`, 'lsappinfo', ['list']);
  runDiag(`${tag}-lsappinfo-visible`, 'lsappinfo', ['visibleProcessList']);
  runDiag(`${tag}-lsappinfo-pid`, 'lsappinfo', ['info', '-all', `pid=${pid}`]);
  // All running processes.
  runDiag(`${tag}-ps`, 'ps', ['-Af']);
  runDiag(`${tag}-ps-electron`, 'sh', ['-c', 'ps -Af | grep -i electron | grep -v grep']);
  // Thread stacks of the hung child.
  runDiag(`${tag}-sample`, 'sample', [String(pid), '3', '-file', path.join(diagDir, `${tag}-sample.txt`)]);
  // Full spindump.
  runDiag(`${tag}-spindump-run`, 'sudo', [
    'spindump', String(pid), '5', '10',
    '-file', path.join(diagDir, `${tag}-spindump.txt`),
    '-noProcessingWhileSampling'
  ]);
  // Symbolicate the unsymbolicated AppKit frames from THIS spindump (parse the
  // file we just wrote for the AppKit load address and the ??? frame addrs).
  try {
    const sd = fs.readFileSync(path.join(diagDir, `${tag}-spindump.txt`), 'utf8');
    const loadM = sd.match(/0x([0-9a-f]+)\s+-\s+0x[0-9a-f]+\s+com\.apple\.AppKit/);
    const addrs = [...new Set(
      [...sd.matchAll(/\?\?\? \(AppKit \+ \d+\) \[(0x[0-9a-f]+)\]/g)].map(m => m[1])
    )];
    if (loadM && addrs.length) {
      runDiag(`${tag}-atos-appkit`, 'atos', [
        '-o', '/System/Library/Frameworks/AppKit.framework/AppKit',
        '-arch', 'x86_64', '-l', `0x${loadM[1]}`, ...addrs
      ]);
      fs.writeFileSync(path.join(diagDir, `${tag}-atos-addrs.txt`),
        `load=0x${loadM[1]}\n${addrs.join('\n')}\n`);
    }
  } catch (e) {
    process.stderr.write(`[esm-diag] atos parse failed: ${e}\n`);
  }
  // Unified log for talagent / persistent UI / appleevents around the hang.
  runDiag(`${tag}-log-ls`, 'log', [
    'show', '--last', '5m', '--style', 'compact',
    '--predicate',
    'process == "launchservicesd" OR process == "appleeventsd" OR process == "talagentd" OR ' +
    'subsystem == "com.apple.launchservices" OR subsystem == "com.apple.appleevents" OR ' +
    'subsystem == "com.apple.PersistentUI" OR category == "PersistentUI"'
  ]);
};

const runFixture = async (appPath: string, args: string[] = []) => {
  const tag = `${path.basename(appPath)}-${Date.now()}`;
  // AEDebugSends/Receives make CoreFoundation log every Apple Event the child
  // sends or receives (incl. the kAEOpenApplication launch event) to stderr.
  const child = cp.spawn(process.execPath, [appPath, ...args], {
    stdio: 'pipe',
    env: {
      ...process.env,
      AEDebugSends: '1',
      AEDebugReceives: '1',
      AEDebugVerbose: '1'
    }
  });

  const stdout: Buffer[] = [];
  const stderr: Buffer[] = [];
  child.stdout.on('data', (c) => stdout.push(c));
  child.stderr.on('data', (c) => { stderr.push(c); process.stderr.write(c); });

  let timedOut = false;
  const diagTimer = setTimeout(() => {
    if (child.exitCode !== null || child.signalCode !== null) return;
    timedOut = true;
    if (process.platform === 'darwin') {
      captureHangDiagnostics(child.pid!, tag);
    }
    process.stderr.write(`[esm-diag] killing hung child pid=${child.pid}\n`);
    try { child.kill('SIGKILL'); } catch { /* ignore */ }
  }, fixtureTimeout);

  const [code, signal] = await new Promise<[number | null, NodeJS.Signals | null]>((resolve, reject) => {
    child.on('error', reject);
    child.on('close', (code, signal) => resolve([code, signal]));
  }).finally(() => clearTimeout(diagTimer));

  // Persist child stderr (contains AEDebug output) for every fixture so we can
  // diff a passing fixture's AE log against the hung one.
  fs.writeFileSync(
    path.join(diagDir, `${tag}-child-stderr.txt`),
    Buffer.concat(stderr).toString()
  );

  const out = stripVTControlCharacters(Buffer.concat(stdout).toString().trim());
  const err = stripVTControlCharacters(Buffer.concat(stderr).toString().trim());
  if (timedOut) {
    throw new Error(
      `Timed out after ${fixtureTimeout}ms waiting for ${appPath} (pid=${child.pid}). ` +
      `Diagnostics in ${diagDir}.\n\nstdout:\n${out}\n\nstderr:\n${err}`
    );
  }
  return { code, signal, stdout: out, stderr: err };
};

// Baseline LaunchServices/process/saved-state captured BEFORE any esm fixture
// runs, so we can diff against the hang-time capture.
if (process.platform === 'darwin') {
  runDiag('baseline-lsappinfo-list', 'lsappinfo', ['list']);
  runDiag('baseline-ps-electron', 'sh', ['-c', 'ps -Af | grep -i electron | grep -v grep']);
  captureSavedState('baseline');
}

const fixturePath = path.resolve(__dirname, 'fixtures', 'esm');

describe('esm', () => {
  describe('main process', () => {
    it('should load an esm entrypoint', async () => {
      const result = await runFixture(path.resolve(fixturePath, 'entrypoint.mjs'));
      expect(result.code).to.equal(0);
      expect(result.stdout).to.equal('ESM Launch, ready: false');
    });

    it('should load an esm entrypoint based on type=module', async () => {
      const result = await runFixture(path.resolve(fixturePath, 'package'));
      expect(result.code).to.equal(0);
      expect(result.stdout).to.equal('ESM Package Launch, ready: false');
    });

    it('should wait for a top-level await before declaring the app ready', async () => {
      const result = await runFixture(path.resolve(fixturePath, 'top-level-await.mjs'));
      expect(result.code).to.equal(0);
      expect(result.stdout).to.equal('Top level await, ready: false');
    });

    it('should allow usage of pre-app-ready apis in top-level await', async () => {
      const result = await runFixture(path.resolve(fixturePath, 'pre-app-ready-apis.mjs'));
      expect(result.code).to.equal(0);
    });

    it('should allow use of dynamic import', async () => {
      const result = await runFixture(path.resolve(fixturePath, 'dynamic.mjs'));
      expect(result.code).to.equal(0);
      expect(result.stdout).to.equal('Exit with app, ready: false');
    });

    it("import 'electron/lol' should throw", async () => {
      const result = await runFixture(path.resolve(fixturePath, 'electron-modules', 'import-lol.mjs'));
      expect(result.code).to.equal(1);
      expect(result.stderr).to.match(/Error \[ERR_MODULE_NOT_FOUND\]/);
    });

    it("import 'electron/main' should not throw", async () => {
      const result = await runFixture(path.resolve(fixturePath, 'electron-modules', 'import-main.mjs'));
      expect(result.code).to.equal(0);
    });

    it("import 'electron/renderer' should not throw", async () => {
      const result = await runFixture(path.resolve(fixturePath, 'electron-modules', 'import-renderer.mjs'));
      expect(result.code).to.equal(0);
    });

    it("import 'electron/common' should not throw", async () => {
      const result = await runFixture(path.resolve(fixturePath, 'electron-modules', 'import-common.mjs'));
      expect(result.code).to.equal(0);
    });

    it("import 'electron/utility' should not throw", async () => {
      const result = await runFixture(path.resolve(fixturePath, 'electron-modules', 'import-utility.mjs'));
      expect(result.code).to.equal(0);
    });
  });

  describe('renderer process', () => {
    let w: BrowserWindow | null = null;
    const tempDirs: string[] = [];

    afterEach(async () => {
      if (w) w.close();
      w = null;
      while (tempDirs.length) {
        await fs.promises.rm(tempDirs.pop()!, { force: true, recursive: true });
      }
    });

    async function loadWindowWithPreload(preload: string, webPreferences: Electron.WebPreferences) {
      const tmpDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'e-spec-preload-'));
      tempDirs.push(tmpDir);
      const preloadPath = path.resolve(tmpDir, 'preload.mjs');
      await fs.promises.writeFile(preloadPath, preload);

      w = new BrowserWindow({
        show: false,
        webPreferences: {
          ...webPreferences,
          preload: preloadPath
        }
      });

      let error: Error | null = null;
      w.webContents.on('preload-error', (_, __, err) => {
        error = err;
      });

      await w.loadFile(path.resolve(fixturePath, 'empty.html'));

      return [w.webContents, error] as [Electron.WebContents, Error | null];
    }

    describe('nodeIntegration', () => {
      let badFilePath = '';

      beforeEach(async () => {
        badFilePath = path.resolve(path.resolve(os.tmpdir(), 'bad-file.badjs'));
        await fs.promises.writeFile(badFilePath, 'const foo = "bar";');
      });

      afterEach(async () => {
        await fs.promises.unlink(badFilePath);
      });

      it('should support an esm entrypoint', async () => {
        const [webContents] = await loadWindowWithPreload(
          'import { resolve } from "path"; window.resolvePath = resolve;',
          {
            nodeIntegration: true,
            sandbox: false,
            contextIsolation: false
          }
        );

        const exposedType = await webContents.executeJavaScript('typeof window.resolvePath');
        expect(exposedType).to.equal('function');
      });

      it('should delay load until the ESM import chain is complete', async () => {
        const [webContents] = await loadWindowWithPreload(
          `import { resolve } from "path";
        await new Promise(r => setTimeout(r, 500));
        window.resolvePath = resolve;`,
          {
            nodeIntegration: true,
            sandbox: false,
            contextIsolation: false
          }
        );

        const exposedType = await webContents.executeJavaScript('typeof window.resolvePath');
        expect(exposedType).to.equal('function');
      });

      it('should support a top-level await fetch blocking the page load', async () => {
        const [webContents] = await loadWindowWithPreload(
          `
        const r = await fetch("package/package.json");
        window.packageJson = await r.json();`,
          {
            nodeIntegration: true,
            sandbox: false,
            contextIsolation: false
          }
        );

        const packageJson = await webContents.executeJavaScript('window.packageJson');
        expect(packageJson).to.deep.equal(require('./fixtures/esm/package/package.json'));
      });

      const hostsUrl = pathToFileURL(
        process.platform === 'win32' ? 'C:\\Windows\\System32\\drivers\\etc\\hosts' : '/etc/hosts'
      );

      describe('without context isolation', () => {
        it('should use Blinks dynamic loader in the main world', async () => {
          const [webContents] = await loadWindowWithPreload('', {
            nodeIntegration: true,
            sandbox: false,
            contextIsolation: false
          });

          let error: Error | null = null;
          try {
            await webContents.executeJavaScript(`import(${JSON.stringify(hostsUrl)})`);
          } catch (err) {
            error = err as Error;
          }

          expect(error).to.not.equal(null);
          // This is a blink specific error message
          expect(error?.message).to.include('Failed to fetch dynamically imported module');
        });

        it('should use Node.js ESM dynamic loader in the preload', async () => {
          const [, preloadError] = await loadWindowWithPreload(
            `await import(${JSON.stringify(pathToFileURL(badFilePath))})`,
            {
              nodeIntegration: true,
              sandbox: false,
              contextIsolation: false
            }
          );

          expect(preloadError).to.not.equal(null);
          // This is a node.js specific error message
          expect(preloadError!.toString()).to.include('Unknown file extension');
        });

        it('should use import.meta callback handling from Node.js for Node.js modules', async function () {
          // DO-NOT-MERGE: #51462 A/B inlined here so split-tests.js shard
          // assignment is not perturbed. Disable mocha retries — we do our own.
          this.retries(0);
          const fp = path.resolve(fixturePath, 'import-meta');
          const ss = path.join(os.homedir(), 'Library', 'Saved Application State', 'com.github.Electron.savedState');

          const attempt = async (label: string, args: string[], pre?: () => void) => {
            process.stderr.write(`\n[esm-diag] ===== attempt: ${label} =====\n`);
            pre?.();
            try {
              const r = await runFixture(fp, args);
              process.stderr.write(`[esm-diag] attempt ${label}: PASSED code=${r.code}\n`);
              return { ok: r.code === 0, code: r.code };
            } catch (e) {
              const firstLine = String((e as Error).message).slice(0, 200);
              process.stderr.write(`[esm-diag] attempt ${label}: HUNG (${firstLine})\n`);
              return { ok: false, code: null };
            }
          };

          // A: stock — expected to hang on shard 2.
          const a = await attempt('A-stock', []);
          // B: with -ApplePersistenceIgnoreState YES.
          const b = await attempt('B-ApplePersistenceIgnoreState', [
            '-ApplePersistenceIgnoreState', 'YES', '-NSQuitAlwaysKeepsWindows', 'NO'
          ]);
          // C: after rm -rf savedState.
          const c = await attempt('C-after-rm-savedState', [], () => {
            runDiag('preclean-savedstate-ls', 'sh', ['-c', `ls -laR "${ss}" 2>&1 || echo "<none>"`]);
            try { fs.rmSync(ss, { recursive: true, force: true }); } catch { /* ignore */ }
          });

          fs.writeFileSync(path.join(diagDir, 'ab-summary.txt'),
            `A-stock: ${a.ok ? 'PASS' : 'HANG'}\n` +
            `B-ApplePersistenceIgnoreState: ${b.ok ? 'PASS' : 'HANG'}\n` +
            `C-after-rm-savedState: ${c.ok ? 'PASS' : 'HANG'}\n`);
          process.stderr.write(`[esm-diag] A/B summary: A=${a.ok} B=${b.ok} C=${c.ok}\n`);

          // Original assertion so the test still reports correctly.
          expect(a.code).to.equal(0);
        });
      });

      describe('with context isolation', () => {
        it('should use Node.js ESM dynamic loader in the isolated context', async () => {
          const [, preloadError] = await loadWindowWithPreload(
            `await import(${JSON.stringify(pathToFileURL(badFilePath))})`,
            {
              nodeIntegration: true,
              sandbox: false,
              contextIsolation: true
            }
          );

          expect(preloadError).to.not.equal(null);
          // This is a node.js specific error message
          expect(preloadError!.toString()).to.include('Unknown file extension');
        });

        it('should use Blinks dynamic loader in the main world', async () => {
          const [webContents] = await loadWindowWithPreload('', {
            nodeIntegration: true,
            sandbox: false,
            contextIsolation: true
          });

          let error: Error | null = null;
          try {
            await webContents.executeJavaScript(`import(${JSON.stringify(hostsUrl)})`);
          } catch (err) {
            error = err as Error;
          }

          expect(error).to.not.equal(null);
          // This is a blink specific error message
          expect(error?.message).to.include('Failed to fetch dynamically imported module');
        });
      });
    });

    describe('electron modules', () => {
      it("import 'electron/lol' should throw", async () => {
        const [, error] = await loadWindowWithPreload('import { ipcRenderer } from "electron/lol";', {
          sandbox: false
        });
        expect(error).to.not.equal(null);
        expect(error?.message).to.match(/Cannot find package 'electron'/);
      });

      it("import 'electron/main' should not throw", async () => {
        const [, error] = await loadWindowWithPreload('import { ipcRenderer } from "electron/main";', {
          sandbox: false
        });
        expect(error).to.equal(null);
      });

      it("import 'electron/renderer' should not throw", async () => {
        const [, error] = await loadWindowWithPreload('import { ipcRenderer } from "electron/renderer";', {
          sandbox: false
        });
        expect(error).to.equal(null);
      });

      it("import 'electron/common' should not throw", async () => {
        const [, error] = await loadWindowWithPreload('import { ipcRenderer } from "electron/common";', {
          sandbox: false
        });
        expect(error).to.equal(null);
      });

      it("import 'electron/utility' should not throw", async () => {
        const [, error] = await loadWindowWithPreload('import { ipcRenderer } from "electron/utility";', {
          sandbox: false
        });
        expect(error).to.equal(null);
      });
    });
  });
});
