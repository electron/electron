import { systemPreferences } from 'electron';
import { BrowserWindow, MessageChannelMain, utilityProcess, app, session } from 'electron/main';

import { expect } from 'chai';

import * as childProcess from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs/promises';
import * as http from 'node:http';
import * as os from 'node:os';
import * as path from 'node:path';
import { setImmediate } from 'node:timers/promises';
import { pathToFileURL } from 'node:url';

import { respondOnce, randomString, kOneKiloByte } from './lib/net-helpers';
import { ifit, listen, startRemoteControlApp } from './lib/spec-helpers';
import { closeWindow } from './lib/window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures', 'api', 'utility-process');
const isWindowsOnArm = process.platform === 'win32' && process.arch === 'arm64';
const isWindows32Bit = process.platform === 'win32' && process.arch === 'ia32';

describe('utilityProcess module', () => {
  describe('UtilityProcess constructor', () => {
    it('throws when empty script path is provided', async () => {
      expect(() => {
        utilityProcess.fork('');
      }).to.throw();
    });

    it('throws when options.stdio is not valid', async () => {
      expect(() => {
        utilityProcess.fork(path.join(fixturesPath, 'empty.js'), [], {
          execArgv: ['--test', '--test2'],
          serviceName: 'test',
          stdio: 'ipc'
        });
      }).to.throw(/stdio must be of the following values: inherit, pipe, ignore/);

      expect(() => {
        utilityProcess.fork(path.join(fixturesPath, 'empty.js'), [], {
          execArgv: ['--test', '--test2'],
          serviceName: 'test',
          stdio: ['ignore', 'ignore']
        });
      }).to.throw(/configuration missing for stdin, stdout or stderr/);

      expect(() => {
        utilityProcess.fork(path.join(fixturesPath, 'empty.js'), [], {
          execArgv: ['--test', '--test2'],
          serviceName: 'test',
          stdio: ['pipe', 'inherit', 'inherit']
        });
      }).to.throw(/stdin value other than ignore is not supported/);
    });
  });

  describe('lifecycle events', () => {
    it("emits 'spawn' when child process successfully launches", async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'empty.js'));
      await once(child, 'spawn');
    });

    it("emits 'exit' when child process exits gracefully", (done) => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'empty.js'));
      child.on('exit', (code) => {
        expect(code).to.equal(0);
        done();
      });
    });

    it("emits 'exit' when the child process file does not exist", (done) => {
      const child = utilityProcess.fork('nonexistent');
      child.on('exit', (code) => {
        expect(code).to.equal(1);
        done();
      });
    });

    ifit(!isWindows32Bit)('emits the correct error code when child process exits nonzero', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'empty.js'));
      await once(child, 'spawn');
      const exit = once(child, 'exit');
      process.kill(child.pid!);
      const [code] = await exit;
      expect(code).to.not.equal(0);
    });

    ifit(!isWindows32Bit)('emits the correct error code when child process is killed', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'empty.js'));
      await once(child, 'spawn');
      const exit = once(child, 'exit');
      process.kill(child.pid!);
      const [code] = await exit;
      expect(code).to.not.equal(0);
    });

    ifit(!isWindows32Bit)("emits 'exit' when child process crashes", async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'crash.js'));
      // SIGSEGV code can differ across pipelines but should never be 0.
      const [code] = await once(child, 'exit');
      expect(code).to.not.equal(0);
    });

    ifit(!isWindows32Bit)("emits 'exit' corresponding to the child process", async () => {
      const child1 = utilityProcess.fork(path.join(fixturesPath, 'endless.js'));
      await once(child1, 'spawn');
      const child2 = utilityProcess.fork(path.join(fixturesPath, 'crash.js'));
      await once(child2, 'exit');
      expect(child1.kill()).to.be.true();
      await once(child1, 'exit');
    });

    it("emits 'exit' when there is uncaught exception", async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'exception.js'));
      const [code] = await once(child, 'exit');
      expect(code).to.equal(1);
    });

    it("emits 'exit' when there is uncaught exception in ESM", async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'exception.mjs'));
      const [code] = await once(child, 'exit');
      expect(code).to.equal(1);
    });

    it("emits 'exit' when process.exit is called", async () => {
      const exitCode = 2;
      const child = utilityProcess.fork(path.join(fixturesPath, 'custom-exit.js'), [`--exitCode=${exitCode}`]);
      const [code] = await once(child, 'exit');
      expect(code).to.equal(exitCode);
    });

    ifit(process.platform === 'win32')('emits correct exit code when high bit is set on Windows', async () => {
      // NTSTATUS code with high bit set should not be mangled by sign extension.
      const exitCode = 0xc0000005;
      const child = utilityProcess.fork(path.join(fixturesPath, 'custom-exit.js'), [`--exitCode=${exitCode}`]);
      const [code] = await once(child, 'exit');
      expect(code).to.equal(exitCode);
    });

    ifit(process.platform !== 'win32')('emits correct exit code when child process crashes on posix', async () => {
      // Crash exit codes should not be sign-extended to large 64-bit values.
      const child = utilityProcess.fork(path.join(fixturesPath, 'crash.js'));
      const [code] = await once(child, 'exit');
      expect(code).to.not.equal(0);
      expect(code).to.be.lessThanOrEqual(0xffffffff);
    });

    it('does not run JS after process.exit is called', async () => {
      const file = path.join(os.tmpdir(), `no-js-after-exit-log-${Math.random()}`);
      const child = utilityProcess.fork(path.join(fixturesPath, 'no-js-after-exit.js'), [`--testPath=${file}`]);
      const [code] = await once(child, 'exit');
      expect(code).to.equal(1);
      let handle = null;
      const lines = [];
      try {
        handle = await fs.open(file);
        for await (const line of handle.readLines()) {
          lines.push(line);
        }
      } finally {
        await handle?.close();
        await fs.rm(file, { force: true });
      }
      expect(lines.length).to.equal(1);
      expect(lines[0]).to.equal('before exit');
    });

    // 32-bit system will not have V8 Sandbox enabled.
    // WoA testing does not have VS toolchain configured to build native addons.
    ifit(process.arch !== 'ia32' && process.arch !== 'arm' && !isWindowsOnArm)(
      "emits 'error' when fatal error is triggered from V8",
      async () => {
        const child = utilityProcess.fork(path.join(fixturesPath, 'external-ab-test.js'));
        const [type, location, report] = await once(child, 'error');
        const [code] = await once(child, 'exit');
        expect(type).to.equal('FatalError');
        expect(location).to.equal('v8_ArrayBuffer_NewBackingStore');
        const reportJSON = JSON.parse(report);
        expect(reportJSON.header.trigger).to.equal('v8_ArrayBuffer_NewBackingStore');
        const addonPath = path.join(
          require.resolve('@electron-ci/external-ab'),
          '..',
          '..',
          'build',
          'Release',
          'external_ab.node'
        );
        expect(reportJSON.sharedObjects).to.include(path.toNamespacedPath(addonPath));
        expect(code).to.not.equal(0);
      }
    );
  });

  describe("app 'child-process-gone' event", () => {
    const waitForCrash = (name: string) => {
      return new Promise<Electron.Details>((resolve) => {
        app.on('child-process-gone', function onCrash(_event, details) {
          if (details.name === name) {
            app.off('child-process-gone', onCrash);
            resolve(details);
          }
        });
      });
    };

    ifit(!isWindows32Bit)('with default serviceName', async () => {
      const name = 'Node Utility Process';
      const crashPromise = waitForCrash(name);
      utilityProcess.fork(path.join(fixturesPath, 'crash.js'));
      const details = await crashPromise;
      expect(details.type).to.equal('Utility');
      expect(details.serviceName).to.equal('node.mojom.NodeService');
      expect(details.name).to.equal(name);
      expect(details.reason).to.be.oneOf(['crashed', 'abnormal-exit']);
    });

    ifit(!isWindows32Bit)('with custom serviceName', async () => {
      const name = crypto.randomUUID();
      const crashPromise = waitForCrash(name);
      utilityProcess.fork(path.join(fixturesPath, 'crash.js'), [], { serviceName: name });
      const details = await crashPromise;
      expect(details.type).to.equal('Utility');
      expect(details.serviceName).to.equal('node.mojom.NodeService');
      expect(details.name).to.equal(name);
      expect(details.reason).to.be.oneOf(['crashed', 'abnormal-exit']);
    });

    ifit(!isWindows32Bit)('does not keep stale observers for crashed processes without JS references', async () => {
      const v8Util = (process as any)._linkedBinding('electron_common_v8_util');
      const logExpectedCrash = (phase: string) => {
        console.error(
          `[expected crash] utilityProcess regression forcing ${phase} crash; signal 11 + backtrace expected`
        );
      };
      const waitForCollection = async (weakChild: WeakRef<ReturnType<typeof utilityProcess.fork>>) => {
        for (let i = 0; i < 30; ++i) {
          await setImmediate();
          v8Util.requestGarbageCollectionForTesting();
          if (weakChild.deref() === undefined) {
            return true;
          }
        }
        return false;
      };

      const name = 'Node Utility Process';
      const firstCrash = waitForCrash(name);
      logExpectedCrash('first');
      let child: ReturnType<typeof utilityProcess.fork> | null = utilityProcess.fork(
        path.join(fixturesPath, 'crash.js')
      );
      const weakChild = new WeakRef(child!);
      child = null;

      const firstDetails = await firstCrash;
      expect(firstDetails.reason).to.be.oneOf(['crashed', 'abnormal-exit']);
      expect(await waitForCollection(weakChild)).to.equal(true);

      const secondCrash = waitForCrash(name);
      logExpectedCrash('second');
      utilityProcess.fork(path.join(fixturesPath, 'crash.js'));
      const secondDetails = await secondCrash;
      expect(secondDetails.reason).to.be.oneOf(['crashed', 'abnormal-exit']);
    });
  });

  describe('app.getAppMetrics()', () => {
    it('with default serviceName', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'endless.js'));
      await once(child, 'spawn');
      expect(child.pid).to.not.be.null();

      await setImmediate();

      const details = app.getAppMetrics().find((item) => item.pid === child.pid)!;
      expect(details).to.be.an('object');
      expect(details.type).to.equal('Utility');
      expect(details.serviceName).to.to.equal('node.mojom.NodeService');
      expect(details.name).to.equal('Node Utility Process');
    });

    it('with custom serviceName', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'endless.js'), [], { serviceName: 'Hello World!' });
      await once(child, 'spawn');
      expect(child.pid).to.not.be.null();

      await setImmediate();

      const details = app.getAppMetrics().find((item) => item.pid === child.pid)!;
      expect(details).to.be.an('object');
      expect(details.type).to.equal('Utility');
      expect(details.serviceName).to.to.equal('node.mojom.NodeService');
      expect(details.name).to.equal('Hello World!');
    });
  });

  describe('kill() API', () => {
    it('terminates the child process gracefully', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'endless.js'), [], {
        serviceName: 'endless'
      });
      await once(child, 'spawn');
      expect(child.kill()).to.be.true();
      const [code] = await once(child, 'exit');
      expect(code).to.equal(0);
    });
  });

  describe('esm', () => {
    it('is launches an mjs file', async () => {
      const fixtureFile = path.join(fixturesPath, 'esm.mjs');
      const child = utilityProcess.fork(fixtureFile, [], {
        stdio: 'pipe'
      });
      expect(child.stdout).to.not.be.null();
      let log = '';
      child.stdout!.on('data', (chunk) => {
        log += chunk.toString('utf8');
      });
      await once(child, 'exit');
      expect(log).to.equal(pathToFileURL(fixtureFile) + '\n');
    });

    it("import 'electron/lol' should throw", async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'electron-modules', 'import-lol.mjs'), [], {
        stdio: ['ignore', 'ignore', 'pipe']
      });
      let stderr = '';
      child.stderr!.on('data', (data) => {
        stderr += data.toString('utf8');
      });
      const [code] = await once(child, 'exit');
      expect(code).to.equal(1);
      expect(stderr).to.match(/Error \[ERR_MODULE_NOT_FOUND\]/);
    });

    it("import 'electron/main' should not throw", async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'electron-modules', 'import-main.mjs'));
      const [code] = await once(child, 'exit');
      expect(code).to.equal(0);
    });

    it("import 'electron/renderer' should not throw", async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'electron-modules', 'import-renderer.mjs'));
      const [code] = await once(child, 'exit');
      expect(code).to.equal(0);
    });

    it("import 'electron/common' should not throw", async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'electron-modules', 'import-common.mjs'));
      const [code] = await once(child, 'exit');
      expect(code).to.equal(0);
    });

    it("import 'electron/utility' should not throw", async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'electron-modules', 'import-utility.mjs'));
      const [code] = await once(child, 'exit');
      expect(code).to.equal(0);
    });
  });

  describe('pid property', () => {
    it('is valid when child process launches successfully', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'empty.js'));
      await once(child, 'spawn');
      expect(child).to.have.property('pid').that.is.a('number');
    });

    it('is undefined when child process fails to launch', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'does-not-exist.js'));
      expect(child.pid).to.be.undefined();
    });

    it('is undefined before the child process is spawned succesfully', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'empty.js'));
      expect(child.pid).to.be.undefined();
      await once(child, 'spawn');
      child.kill();
    });

    it('is undefined when child process is killed', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'empty.js'));
      await once(child, 'spawn');

      expect(child).to.have.property('pid').that.is.a('number');
      expect(child.kill()).to.be.true();

      await once(child, 'exit');
      expect(child.pid).to.be.undefined();
    });
  });

  describe('stdout property', () => {
    it('is null when child process launches with default stdio', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'));
      await once(child, 'spawn');
      expect(child.stdout).to.be.null();
      expect(child.stderr).to.be.null();
      await once(child, 'exit');
    });

    it('is null when child process launches with ignore stdio configuration', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'), [], {
        stdio: 'ignore'
      });
      await once(child, 'spawn');
      expect(child.stdout).to.be.null();
      expect(child.stderr).to.be.null();
      await once(child, 'exit');
    });

    it('is valid when child process launches with pipe stdio configuration', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'), [], {
        stdio: 'pipe'
      });
      expect(child.stdout).to.not.be.null();
      let log = '';
      child.stdout!.on('data', (chunk) => {
        log += chunk.toString('utf8');
      });
      await once(child, 'exit');
      expect(log).to.equal('hello\n');
    });
  });

  describe('stderr property', () => {
    it('is null when child process launches with default stdio', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'));
      await once(child, 'spawn');
      expect(child.stdout).to.be.null();
      expect(child.stderr).to.be.null();
      await once(child, 'exit');
    });

    it('is null when child process launches with ignore stdio configuration', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'), [], {
        stdio: 'ignore'
      });
      await once(child, 'spawn');
      expect(child.stderr).to.be.null();
      await once(child, 'exit');
    });

    ifit(!isWindowsOnArm)('is valid when child process launches with pipe stdio configuration', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'), [], {
        stdio: ['ignore', 'pipe', 'pipe']
      });
      expect(child.stderr).to.not.be.null();
      let log = '';
      child.stderr!.on('data', (chunk) => {
        log += chunk.toString('utf8');
      });
      await once(child, 'exit');
      expect(log).to.equal('world');
    });
  });

  describe('postMessage() API', () => {
    it('establishes a default ipc channel with the child process', async () => {
      const result = 'I will be echoed.';
      const child = utilityProcess.fork(path.join(fixturesPath, 'post-message.js'));
      await once(child, 'spawn');
      child.postMessage(result);
      const [data] = await once(child, 'message');
      expect(data).to.equal(result);
      const exit = once(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
    });

    it('supports queuing messages on the receiving end', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'post-message-queue.js'));
      const p = once(child, 'spawn');
      child.postMessage('This message');
      child.postMessage(' is');
      child.postMessage(' queued');
      await p;
      const [data] = await once(child, 'message');
      expect(data).to.equal('This message is queued');
      const exit = once(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
    });

    it('handles the parent port trying to send an non-clonable object', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'non-cloneable.js'));
      await once(child, 'spawn');
      child.postMessage('non-cloneable');
      const [data] = await once(child, 'message');
      expect(data).to.equal('caught-non-cloneable');
      const exit = once(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
    });
  });

  describe('behavior', () => {
    it('supports starting the v8 inspector with --inspect-brk', (done) => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'), [], {
        stdio: 'pipe',
        execArgv: ['--inspect-brk']
      });

      let output = '';
      const cleanup = () => {
        child.stderr!.removeListener('data', listener);
        child.stdout!.removeListener('data', listener);
        child.once('exit', () => {
          done();
        });
        child.kill();
      };

      const listener = (data: Buffer) => {
        output += data;
        if (/Debugger listening on ws:/m.test(output)) {
          cleanup();
        }
      };

      child.stderr!.on('data', listener);
      child.stdout!.on('data', listener);
    });

    it('supports starting the v8 inspector with --inspect and a provided port', (done) => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'), [], {
        stdio: 'pipe',
        execArgv: ['--inspect=17364']
      });

      let output = '';
      const cleanup = () => {
        child.stderr!.removeListener('data', listener);
        child.stdout!.removeListener('data', listener);
        child.once('exit', () => {
          done();
        });
        child.kill();
      };

      const listener = (data: Buffer) => {
        output += data;
        if (/Debugger listening on ws:/m.test(output)) {
          expect(output.trim()).to.contain(':17364', 'should be listening on port 17364');
          cleanup();
        }
      };

      child.stderr!.on('data', listener);
      child.stdout!.on('data', listener);
    });

    it('supports changing dns verbatim with --dns-result-order', (done) => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'dns-result-order.js'), [], {
        stdio: 'pipe',
        execArgv: ['--dns-result-order=ipv4first']
      });

      let output = '';
      const cleanup = () => {
        child.stderr!.removeListener('data', listener);
        child.stdout!.removeListener('data', listener);
        child.once('exit', () => {
          done();
        });
        child.kill();
      };

      const listener = (data: Buffer) => {
        output += data;
        expect(output.trim()).to.contain('ipv4first', 'default verbatim should be ipv4first');
        cleanup();
      };

      child.stderr!.on('data', listener);
      child.stdout!.on('data', listener);
    });

    ifit(process.platform !== 'win32')('supports redirecting stdout to parent process', async () => {
      const result = 'Output from utility process';
      const appProcess = childProcess.spawn(process.execPath, [
        path.join(fixturesPath, 'inherit-stdout'),
        `--payload=${result}`
      ]);
      let output = '';
      appProcess.stdout.on('data', (data: Buffer) => {
        output += data;
      });
      await once(appProcess, 'exit');
      expect(output).to.equal(result);
    });

    ifit(process.platform !== 'win32')('supports redirecting stderr to parent process', async () => {
      const result = 'Error from utility process';
      const appProcess = childProcess.spawn(process.execPath, [
        path.join(fixturesPath, 'inherit-stderr'),
        `--payload=${result}`
      ]);
      let output = '';
      appProcess.stderr.on('data', (data: Buffer) => {
        output += data;
      });
      await once(appProcess, 'exit');
      expect(output).to.include(result);
    });

    ifit(process.platform !== 'linux')('can access exposed main process modules from the utility process', async () => {
      const message = 'Message from utility process';
      const child = utilityProcess.fork(path.join(fixturesPath, 'expose-main-process-module.js'));
      await once(child, 'spawn');
      child.postMessage(message);
      const [data] = await once(child, 'message');
      expect(data).to.equal(systemPreferences.getMediaAccessStatus('screen'));
      const exit = once(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
    });

    it('can establish communication channel with sandboxed renderer', async () => {
      const result = 'Message from sandboxed renderer';
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          preload: path.join(fixturesPath, 'preload.js')
        }
      });
      await w.loadFile(path.join(__dirname, 'fixtures', 'blank.html'));
      // Create Message port pair for Renderer <-> Utility Process.
      const { port1: rendererPort, port2: childPort1 } = new MessageChannelMain();
      w.webContents.postMessage('port', result, [rendererPort]);
      // Send renderer and main channel port to utility process.
      const child = utilityProcess.fork(path.join(fixturesPath, 'receive-message.js'));
      await once(child, 'spawn');
      child.postMessage('', [childPort1]);
      const [data] = await once(child, 'message');
      expect(data).to.equal(result);
      // Cleanup.
      const exit = once(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
      await closeWindow(w);
    });

    ifit(process.platform === 'linux')('allows executing a setuid binary with child_process', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'suid.js'));
      await once(child, 'spawn');
      const [data] = await once(child, 'message');
      expect(data).to.not.be.empty();
      const exit = once(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
    });

    it('inherits parent env as default', async () => {
      const appProcess = childProcess.spawn(process.execPath, [path.join(fixturesPath, 'env-app')], {
        env: {
          FROM: 'parent',
          ...process.env
        }
      });
      let output = '';
      appProcess.stdout.on('data', (data: Buffer) => {
        output += data;
      });
      await once(appProcess.stdout, 'end');
      const result = process.platform === 'win32' ? '\r\nparent' : 'parent';
      expect(output).to.equal(result);
    });

    // TODO(codebytere): figure out why this is failing in ASAN- builds on Linux.
    ifit(!process.env.IS_ASAN)('does not inherit parent env when custom env is provided', async () => {
      const appProcess = childProcess.spawn(
        process.execPath,
        [path.join(fixturesPath, 'env-app'), '--create-custom-env'],
        {
          env: {
            FROM: 'parent',
            ...process.env
          }
        }
      );
      let output = '';
      appProcess.stdout.on('data', (data: Buffer) => {
        output += data;
      });
      await once(appProcess.stdout, 'end');
      const result = process.platform === 'win32' ? '\r\nchild' : 'child';
      expect(output).to.equal(result);
    });

    it('changes working directory with cwd', async () => {
      const child = utilityProcess.fork('./log.js', [], {
        cwd: fixturesPath,
        stdio: ['ignore', 'pipe', 'ignore']
      });
      await once(child, 'spawn');
      expect(child.stdout).to.not.be.null();
      let log = '';
      child.stdout!.on('data', (chunk) => {
        log += chunk.toString('utf8');
      });
      await once(child, 'exit');
      expect(log).to.equal('hello\n');
    });

    it('does not crash when running eval', async () => {
      const child = utilityProcess.fork('./eval.js', [], {
        cwd: fixturesPath,
        stdio: 'ignore'
      });
      await once(child, 'spawn');
      const [data] = await once(child, 'message');
      expect(data).to.equal(42);
      // Cleanup.
      const exit = once(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
    });

    it('should emit the app#login event when 401', async () => {
      const { remotely } = await startRemoteControlApp();
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
        }
        response.writeHead(200).end('ok');
      });
      const [loginAuthInfo, statusCode] = await remotely(
        async (serverUrl: string, fixture: string) => {
          const { app, utilityProcess } = require('electron');
          const { once } = require('node:events');
          const child = utilityProcess.fork(fixture, [`--server-url=${serverUrl}`], {
            stdio: 'ignore',
            respondToAuthRequestsFromMainProcess: true
          });
          await once(child, 'spawn');
          const [ev, , , authInfo, cb] = await once(app, 'login');
          ev.preventDefault();
          cb('dummy', 'pass');
          const [result] = await once(child, 'message');
          return [authInfo, ...result];
        },
        serverUrl,
        path.join(fixturesPath, 'net.js')
      );
      expect(statusCode).to.equal(200);
      expect(loginAuthInfo!.realm).to.equal('Foo');
      expect(loginAuthInfo!.scheme).to.equal('basic');
    });

    it('should receive 401 response when cancelling authentication via app#login event', async () => {
      const { remotely } = await startRemoteControlApp();
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' });
          response.end('unauthenticated');
        } else {
          response.writeHead(200).end('ok');
        }
      });
      const [authDetails, responseBody, statusCode] = await remotely(
        async (serverUrl: string, fixture: string) => {
          const { app, utilityProcess } = require('electron');
          const { once } = require('node:events');
          const child = utilityProcess.fork(fixture, [`--server-url=${serverUrl}`], {
            stdio: 'ignore',
            respondToAuthRequestsFromMainProcess: true
          });
          await once(child, 'spawn');
          const [, , details, , cb] = await once(app, 'login');
          cb();
          const [response] = await once(child, 'message');
          const [responseBody] = await once(child, 'message');
          return [details, responseBody, ...response];
        },
        serverUrl,
        path.join(fixturesPath, 'net.js')
      );
      expect(authDetails.url).to.equal(serverUrl);
      expect(statusCode).to.equal(401);
      expect(responseBody).to.equal('unauthenticated');
    });

    it('should upload body when 401', async () => {
      const { remotely } = await startRemoteControlApp();
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
        }
        response.writeHead(200);
        request.on('data', (chunk) => response.write(chunk));
        request.on('end', () => response.end());
      });
      const requestData = randomString(kOneKiloByte);
      const [authDetails, responseBody, statusCode] = await remotely(
        async (serverUrl: string, requestData: string, fixture: string) => {
          const { app, utilityProcess } = require('electron');
          const { once } = require('node:events');
          const child = utilityProcess.fork(fixture, [`--server-url=${serverUrl}`, '--request-data'], {
            stdio: 'ignore',
            respondToAuthRequestsFromMainProcess: true
          });
          await once(child, 'spawn');
          await once(child, 'message');
          child.postMessage(requestData);
          const [, , details, , cb] = await once(app, 'login');
          cb('user', 'pass');
          const [response] = await once(child, 'message');
          const [responseBody] = await once(child, 'message');
          return [details, responseBody, ...response];
        },
        serverUrl,
        requestData,
        path.join(fixturesPath, 'net.js')
      );
      expect(authDetails.url).to.equal(serverUrl);
      expect(statusCode).to.equal(200);
      expect(responseBody).to.equal(requestData);
    });

    it('should not emit the app#login event when 401 with {"credentials":"omit"}', async () => {
      const rc = await startRemoteControlApp();
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
        }
        response.writeHead(200).end('ok');
      });
      const [statusCode, responseHeaders] = await rc.remotely(
        async (serverUrl: string, fixture: string) => {
          const { app, utilityProcess } = require('electron');
          const { once } = require('node:events');
          let gracefulExit = true;
          const child = utilityProcess.fork(fixture, [`--server-url=${serverUrl}`, '--omit-credentials'], {
            stdio: 'ignore',
            respondToAuthRequestsFromMainProcess: true
          });
          await once(child, 'spawn');
          app.on('login', () => {
            gracefulExit = false;
          });
          const [result] = await once(child, 'message');
          setTimeout(() => {
            if (gracefulExit) {
              app.quit();
            } else {
              process.exit(1);
            }
          });
          return result;
        },
        serverUrl,
        path.join(fixturesPath, 'net.js')
      );
      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);
      expect(statusCode).to.equal(401);
      expect(responseHeaders['www-authenticate']).to.equal('Basic realm="Foo"');
    });

    it('should not emit the app#login event with default respondToAuthRequestsFromMainProcess', async () => {
      const rc = await startRemoteControlApp();
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
        }
        response.writeHead(200).end('ok');
      });
      const [loginAuthInfo, statusCode] = await rc.remotely(
        async (serverUrl: string, fixture: string) => {
          const { app, utilityProcess } = require('electron');
          const { once } = require('node:events');
          let gracefulExit = true;
          const child = utilityProcess.fork(fixture, [`--server-url=${serverUrl}`, '--use-net-login-event'], {
            stdio: 'ignore'
          });
          await once(child, 'spawn');
          app.on('login', () => {
            gracefulExit = false;
          });
          const [authInfo] = await once(child, 'message');
          const [result] = await once(child, 'message');
          setTimeout(() => {
            if (gracefulExit) {
              app.quit();
            } else {
              process.exit(1);
            }
          });
          return [authInfo, ...result];
        },
        serverUrl,
        path.join(fixturesPath, 'net.js')
      );
      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);
      expect(statusCode).to.equal(200);
      expect(loginAuthInfo!.realm).to.equal('Foo');
      expect(loginAuthInfo!.scheme).to.equal('basic');
    });

    it('should emit the app#login event when creating requests with fetch API', async () => {
      const { remotely } = await startRemoteControlApp();
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
        }
        response.writeHead(200).end('ok');
      });
      const [loginAuthInfo, statusCode] = await remotely(
        async (serverUrl: string, fixture: string) => {
          const { app, utilityProcess } = require('electron');
          const { once } = require('node:events');
          const child = utilityProcess.fork(fixture, [`--server-url=${serverUrl}`, '--use-fetch-api'], {
            stdio: 'ignore',
            respondToAuthRequestsFromMainProcess: true
          });
          await once(child, 'spawn');
          const [ev, , , authInfo, cb] = await once(app, 'login');
          ev.preventDefault();
          cb('dummy', 'pass');
          const [response] = await once(child, 'message');
          return [authInfo, ...response];
        },
        serverUrl,
        path.join(fixturesPath, 'net.js')
      );
      expect(statusCode).to.equal(200);
      expect(loginAuthInfo!.realm).to.equal('Foo');
      expect(loginAuthInfo!.scheme).to.equal('basic');
    });

    it('supports generating snapshots via v8.setHeapSnapshotNearHeapLimit', async () => {
      const tmpDir = await fs.mkdtemp(path.resolve(os.tmpdir(), 'electron-spec-utility-oom-'));
      const child = utilityProcess.fork(path.join(fixturesPath, 'oom-grow.js'), [], {
        stdio: 'ignore',
        execArgv: [`--diagnostic-dir=${tmpDir}`, '--js-flags=--max-old-space-size=50'],
        env: {
          NODE_DEBUG_NATIVE: 'diagnostic'
        }
      });
      await once(child, 'spawn');
      await once(child, 'exit');
      const files = (await fs.readdir(tmpDir)).filter((file) => file.endsWith('.heapsnapshot'));
      expect(files.length).to.be.equal(1);
      const stat = await fs.stat(path.join(tmpDir, files[0]));
      expect(stat.size).to.be.greaterThan(0);
      await fs.rm(tmpDir, { recursive: true });
    });

    it('supports --no-experimental-global-navigator flag', async () => {
      {
        const child = utilityProcess.fork(path.join(fixturesPath, 'navigator.js'), [], {
          stdio: 'ignore'
        });
        await once(child, 'spawn');
        const [data] = await once(child, 'message');
        expect(data).to.be.true();
        const exit = once(child, 'exit');
        expect(child.kill()).to.be.true();
        await exit;
      }
      {
        const child = utilityProcess.fork(path.join(fixturesPath, 'navigator.js'), [], {
          stdio: 'ignore',
          execArgv: ['--no-experimental-global-navigator']
        });
        await once(child, 'spawn');
        const [data] = await once(child, 'message');
        expect(data).to.be.false();
        const exit = once(child, 'exit');
        expect(child.kill()).to.be.true();
        await exit;
      }
    });

    // Note: This doesn't test that disclaiming works (that requires stubbing / mocking TCC which is
    // just straight up not possible generically). This just tests that utility processes still launch
    // when disclaimed.
    ifit(process.platform === 'darwin')('supports disclaim option on macOS', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'post-message.js'), [], {
        disclaim: true
      });
      await once(child, 'spawn');
      expect(child.pid).to.be.a('number');
      // Verify the process can communicate normally
      const testMessage = 'test-disclaim';
      child.postMessage(testMessage);
      const [data] = await once(child, 'message');
      expect(data).to.equal(testMessage);
      const exit = once(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
    });
  });

  describe('session', () => {
    const killChild = async (child: Electron.UtilityProcess) => {
      if (child.pid == null) return;
      const exited = once(child, 'exit');
      try {
        child.kill();
      } catch {
      }
      await exited;
    };

    it('can use a session object for network requests', async () => {
      const customSession = session.fromPartition(`utility-session-test-${Math.random()}`);
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.end('session-response');
      });
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
        session: customSession
      });
      try {
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch', url: serverUrl });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.body).to.equal('session-response');
      } finally {
        await killChild(child);
      }
    });

    it('can use a partition string for network requests', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.end('partition-response');
      });
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
        partition: `utility-partition-test-${Math.random()}`
      });
      try {
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch', url: serverUrl });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.body).to.equal('partition-response');
      } finally {
        await killChild(child);
      }
    });

    it('uses HTTP cache when session is provided', async () => {
      let requestCount = 0;
      const server = http.createServer((request, response) => {
        requestCount++;
        response.writeHead(200, {
          'Cache-Control': 'max-age=3600',
          'Content-Type': 'text/plain',
          'X-Request-Count': String(requestCount)
        });
        response.end(`response-${requestCount}`);
      });
      const { url } = await listen(server);
      const customSession = session.fromPartition(`utility-cache-test-${Math.random()}`);
      const cacheFlags: boolean[] = [];
      customSession.webRequest.onResponseStarted((details) => {
        cacheFlags.push(details.fromCache);
      });
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
        session: customSession
      });
      try {
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch-cached', url: `${url}/cached` });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.first.body).to.equal('response-1');
        expect(data.second.body).to.equal('response-1');
        expect(requestCount).to.equal(1);
        // Verify cache flags: first request from network, second from cache
        expect(cacheFlags).to.deep.equal([false, true]);
      } finally {
        await killChild(child);
        customSession.webRequest.onResponseStarted(null);
        await customSession.clearCache();
        server.close();
      }
    });

    it('does not use HTTP cache when using the system network context (no session)', async () => {
      let requestCount = 0;
      const server = http.createServer((request, response) => {
        requestCount++;
        response.writeHead(200, {
          'Cache-Control': 'max-age=3600',
          'Content-Type': 'text/plain'
        });
        response.end(`response-${requestCount}`);
      });
      const { url } = await listen(server);
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'));
      try {
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch-cached', url: `${url}/no-cache` });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.first.body).to.equal('response-1');
        expect(data.second.body).to.equal('response-2');
        expect(requestCount).to.equal(2);
      } finally {
        await killChild(child);
        server.close();
      }
    });

    it('respects cache: "no-store" fetch option to bypass cache', async () => {
      let requestCount = 0;
      const server = http.createServer((request, response) => {
        requestCount++;
        response.writeHead(200, {
          'Cache-Control': 'max-age=3600',
          'Content-Type': 'text/plain'
        });
        response.end(`response-${requestCount}`);
      });
      const { url } = await listen(server);
      const customSession = session.fromPartition(`utility-cache-nostore-${Math.random()}`);
      const cacheFlags: boolean[] = [];
      customSession.webRequest.onResponseStarted((details) => {
        cacheFlags.push(details.fromCache);
      });
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
        session: customSession
      });
      try {
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch-cached', url: `${url}/nostore`, cacheMode: 'no-store' });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.first.body).to.equal('response-1');
        expect(data.second.body).to.equal('response-2');
        expect(requestCount).to.equal(2);
        // Neither response should be from cache
        expect(cacheFlags).to.deep.equal([false, false]);
      } finally {
        await killChild(child);
        customSession.webRequest.onResponseStarted(null);
        await customSession.clearCache();
        server.close();
      }
    });

    it('respects cache: "no-cache" fetch option to revalidate', async () => {
      let requestCount = 0;
      const server = http.createServer((request, response) => {
        requestCount++;
        response.writeHead(200, {
          'Cache-Control': 'max-age=3600',
          'Content-Type': 'text/plain',
          ETag: '"test-etag"'
        });
        response.end(`response-${requestCount}`);
      });
      const { url } = await listen(server);
      const customSession = session.fromPartition(`utility-cache-nocache-${Math.random()}`);
      const cacheFlags: boolean[] = [];
      customSession.webRequest.onResponseStarted((details) => {
        cacheFlags.push(details.fromCache);
      });
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
        session: customSession
      });
      try {
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch-cached', url: `${url}/nocache`, cacheMode: 'no-cache' });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.first.body).to.equal('response-1');
        expect(requestCount).to.equal(2);
        // First from network, second revalidated (not from cache)
        expect(cacheFlags).to.deep.equal([false, false]);
      } finally {
        await killChild(child);
        customSession.webRequest.onResponseStarted(null);
        await customSession.clearCache();
        server.close();
      }
    });

    it('respects cache: "force-cache" fetch option to use stale cache', async () => {
      let requestCount = 0;
      const server = http.createServer((request, response) => {
        requestCount++;
        response.writeHead(200, {
          'Cache-Control': 'max-age=3600',
          'Content-Type': 'text/plain'
        });
        response.end(`response-${requestCount}`);
      });
      const { url } = await listen(server);
      const customSession = session.fromPartition(`utility-cache-forcecache-${Math.random()}`);
      const cacheFlags: boolean[] = [];
      customSession.webRequest.onResponseStarted((details) => {
        cacheFlags.push(details.fromCache);
      });
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
        session: customSession
      });
      try {
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch-cached', url: `${url}/forcecache`, cacheMode: 'force-cache' });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.first.body).to.equal('response-1');
        expect(data.second.body).to.equal('response-1');
        expect(requestCount).to.equal(1);
        // First from network, second from cache
        expect(cacheFlags).to.deep.equal([false, true]);
      } finally {
        await killChild(child);
        customSession.webRequest.onResponseStarted(null);
        await customSession.clearCache();
        server.close();
      }
    });

    it('respects cache: "reload" fetch option to bypass cache entirely', async () => {
      let requestCount = 0;
      const server = http.createServer((request, response) => {
        requestCount++;
        response.writeHead(200, {
          'Cache-Control': 'max-age=3600',
          'Content-Type': 'text/plain'
        });
        response.end(`response-${requestCount}`);
      });
      const { url } = await listen(server);
      const customSession = session.fromPartition(`utility-cache-reload-${Math.random()}`);
      const cacheFlags: boolean[] = [];
      customSession.webRequest.onResponseStarted((details) => {
        cacheFlags.push(details.fromCache);
      });
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
        session: customSession
      });
      try {
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch-cached', url: `${url}/reload`, cacheMode: 'reload' });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.first.body).to.equal('response-1');
        expect(data.second.body).to.equal('response-2');
        expect(requestCount).to.equal(2);
        // Neither response should be from cache
        expect(cacheFlags).to.deep.equal([false, false]);
      } finally {
        await killChild(child);
        customSession.webRequest.onResponseStarted(null);
        await customSession.clearCache();
        server.close();
      }
    });

    it('isolates cookies between different sessions', async () => {
      const sess1 = session.fromPartition(`utility-cookie-test-1-${Math.random()}`);
      const sess2 = session.fromPartition(`utility-cookie-test-2-${Math.random()}`);

      const server = http.createServer((request, response) => {
        const cookie = request.headers.cookie || 'none';
        response.writeHead(200, {
          'Content-Type': 'text/plain',
          'Set-Cookie': 'testcookie=value; Path=/'
        });
        response.end(cookie);
      });
      const { url } = await listen(server);
      let child1: Electron.UtilityProcess | undefined;
      let child2: Electron.UtilityProcess | undefined;
      try {
        await sess1.cookies.set({ url, name: 'testcookie', value: 'sess1value' });
        child1 = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
          session: sess1
        });
        await once(child1, 'spawn');
        await once(child1, 'message');
        child1.postMessage({ type: 'fetch', url: `${url}/cookies`, options: { credentials: 'include' } });
        const [data1] = await once(child1, 'message');
        expect(data1.ok).to.be.true();
        expect(data1.body).to.include('testcookie=sess1value');
        child2 = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
          session: sess2
        });
        await once(child2, 'spawn');
        await once(child2, 'message');
        child2.postMessage({ type: 'fetch', url: `${url}/cookies`, options: { credentials: 'include' } });
        const [data2] = await once(child2, 'message');
        expect(data2.ok).to.be.true();
        expect(data2.body).to.equal('none');
      } finally {
        if (child1) await killChild(child1);
        if (child2) await killChild(child2);
        await sess1.clearStorageData();
        await sess2.clearStorageData();
        server.close();
      }
    });

    it('shares cookies when utility processes use the same session', async () => {
      const sharedSession = session.fromPartition(`utility-shared-session-${Math.random()}`);
      const server = http.createServer((request, response) => {
        const cookie = request.headers.cookie || 'none';
        response.writeHead(200, { 'Content-Type': 'text/plain' });
        response.end(cookie);
      });
      const { url } = await listen(server);
      let child1: Electron.UtilityProcess | undefined;
      let child2: Electron.UtilityProcess | undefined;
      try {
        await sharedSession.cookies.set({ url, name: 'shared', value: 'cookie123' });
        child1 = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
          session: sharedSession
        });
        await once(child1, 'spawn');
        await once(child1, 'message');
        child1.postMessage({ type: 'fetch', url: `${url}/shared`, options: { credentials: 'include' } });
        const [data1] = await once(child1, 'message');
        expect(data1.ok).to.be.true();
        expect(data1.body).to.include('shared=cookie123');
        child2 = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
          session: sharedSession
        });
        await once(child2, 'spawn');
        await once(child2, 'message');
        child2.postMessage({ type: 'fetch', url: `${url}/shared`, options: { credentials: 'include' } });
        const [data2] = await once(child2, 'message');
        expect(data2.ok).to.be.true();
        expect(data2.body).to.include('shared=cookie123');
      } finally {
        if (child1) await killChild(child1);
        if (child2) await killChild(child2);
        await sharedSession.clearStorageData();
        server.close();
      }
    });

    it('session option takes precedence over partition', async () => {
      const customSession = session.fromPartition(`utility-precedence-session-${Math.random()}`);
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.end('precedence-ok');
      });
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
        session: customSession,
        partition: 'this-should-be-ignored'
      } as any);
      try {
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch', url: serverUrl });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.body).to.equal('precedence-ok');
      } finally {
        await killChild(child);
      }
    });

    it('session webRequest handlers intercept utility process requests', async () => {
      const customSession = session.fromPartition(`utility-webrequest-test-${Math.random()}`);
      const server = http.createServer((request, response) => {
        response.writeHead(200, { 'Content-Type': 'text/plain' });
        response.end(`header: ${request.headers['x-custom-header'] || 'missing'}`);
      });
      const { url } = await listen(server);
      customSession.webRequest.onBeforeSendHeaders((details, callback) => {
        details.requestHeaders['X-Custom-Header'] = 'intercepted';
        callback({ requestHeaders: details.requestHeaders });
      });
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
        session: customSession
      });
      try {
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch', url: `${url}/webrequest` });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.body).to.equal('header: intercepted');
      } finally {
        await killChild(child);
        customSession.webRequest.onBeforeSendHeaders(null);
        server.close();
      }
    });

    it('fires ClientRequest login event when respondToAuthRequestsFromMainProcess is false', async () => {
      const customSession = session.fromPartition(`utility-login-client-${Math.random()}`);
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
        }
        response.writeHead(200).end('authenticated');
      });
      const child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
        session: customSession
      });
      try {
        await once(child, 'spawn');
        await once(child, 'message'); // ready
        child.postMessage({ type: 'net-request-login', url: serverUrl });
        const [loginMsg] = await once(child, 'message');
        expect(loginMsg.event).to.equal('login');
        expect(loginMsg.authInfo.realm).to.equal('Foo');
        expect(loginMsg.authInfo.scheme).to.equal('basic');
        const [responseMsg] = await once(child, 'message');
        expect(responseMsg.event).to.equal('response');
        expect(responseMsg.statusCode).to.equal(200);
      } finally {
        await killChild(child);
      }
    });

    it('fires app login event when respondToAuthRequestsFromMainProcess is true without session', async () => {
      const { remotely } = await startRemoteControlApp();
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
        }
        response.writeHead(200).end('authenticated');
      });
      const [loginAuthInfo, statusCode] = await remotely(
        async (serverUrl: string, fixture: string) => {
          const { app, utilityProcess } = require('electron');
          const { once } = require('node:events');
          const child = utilityProcess.fork(fixture, [], {
            stdio: 'ignore',
            respondToAuthRequestsFromMainProcess: true
          });
          await once(child, 'spawn');
          await once(child, 'message');
          child.postMessage({ type: 'fetch-auth', url: serverUrl });
          const [ev, , , authInfo, cb] = await once(app, 'login');
          ev.preventDefault();
          cb('user', 'pass');
          const [result] = await once(child, 'message');
          return [authInfo, result.status];
        },
        serverUrl,
        path.join(fixturesPath, 'net-session.js')
      );
      expect(statusCode).to.equal(200);
      expect(loginAuthInfo!.realm).to.equal('Foo');
      expect(loginAuthInfo!.scheme).to.equal('basic');
    });

    it('fires utility process login event when respondToAuthRequestsFromMainProcess is true with session', async () => {
      const { remotely } = await startRemoteControlApp();
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end();
        }
        response.writeHead(200).end('authenticated');
      });
      const [loginAuthInfo, statusCode, appLoginFired] = await remotely(
        async (serverUrl: string, fixture: string) => {
          const { app, session, utilityProcess } = require('electron');
          const { once } = require('node:events');
          const customSession = session.fromPartition(`utility-login-session-${Math.random()}`);
          let appLoginFired = false;
          app.on('login', () => {
            appLoginFired = true;
          });
          const child = utilityProcess.fork(fixture, [], {
            stdio: 'ignore',
            respondToAuthRequestsFromMainProcess: true,
            session: customSession
          });
          await once(child, 'spawn');
          await once(child, 'message');
          child.postMessage({ type: 'fetch-auth', url: serverUrl });
          const [ev, , authInfo, cb] = await once(child, 'login');
          ev.preventDefault();
          cb('user', 'pass');
          const [result] = await once(child, 'message');
          return [authInfo, result.status, appLoginFired];
        },
        serverUrl,
        path.join(fixturesPath, 'net-session.js')
      );
      expect(statusCode).to.equal(200);
      expect(loginAuthInfo!.realm).to.equal('Foo');
      expect(loginAuthInfo!.scheme).to.equal('basic');
      expect(appLoginFired).to.be.false();
    });

    it('resolves hosts using the session network context, not the default', async () => {
      const proxyServer = http.createServer((request, response) => {
        response.writeHead(200, { 'Content-Type': 'text/plain' });
        response.end(`proxied:${request.headers.host}`);
      });
      const { port: proxyPort } = await listen(proxyServer);

      const customSession = session.fromPartition(`utility-resolver-test-${Math.random()}`);
      let child: Electron.UtilityProcess | undefined;
      try {
        await customSession.setProxy({
          proxyRules: `http=127.0.0.1:${proxyPort}`,
          proxyBypassRules: '<-loopback>'
        });

        await session.defaultSession.setProxy({});

        child = utilityProcess.fork(path.join(fixturesPath, 'net-session.js'), [], {
          session: customSession
        });
        await once(child, 'spawn');
        await once(child, 'message');
        child.postMessage({ type: 'fetch', url: 'http://non-existent-host.test:12345/path' });
        const [data] = await once(child, 'message');
        expect(data.ok).to.be.true();
        expect(data.body).to.equal('proxied:non-existent-host.test:12345');
      } finally {
        if (child) await killChild(child);
        await session.defaultSession.setProxy({});
        proxyServer.close();
      }
    });

    it('does not use system network proxy set via app.setProxy when session is provided', async () => {
      const { remotely } = await startRemoteControlApp();
      const systemProxyServer = http.createServer((request, response) => {
        response.writeHead(200, { 'Content-Type': 'text/plain' });
        response.end('system-proxy');
      });
      const { port: systemProxyPort } = await listen(systemProxyServer);

      const targetServer = http.createServer((request, response) => {
        response.writeHead(200, { 'Content-Type': 'text/plain' });
        response.end('direct-response');
      });
      const { url: targetUrl } = await listen(targetServer);

      try {
        const body = await remotely(
          async (targetUrl: string, systemProxyPort: number, fixture: string) => {
            const { app, session, utilityProcess } = require('electron');
            const { once } = require('node:events');

            await app.setProxy({
              proxyRules: `http=127.0.0.1:${systemProxyPort}`,
              proxyBypassRules: '<-loopback>'
            });

            const customSession = session.fromPartition(`utility-app-proxy-test-${Math.random()}`);
            await customSession.setProxy({});

            const child = utilityProcess.fork(fixture, [], {
              stdio: 'ignore',
              session: customSession
            });
            await once(child, 'spawn');
            await once(child, 'message');
            child.postMessage({ type: 'fetch', url: targetUrl });
            const [data] = await once(child, 'message');
            const exit = once(child, 'exit');
            child.kill();
            await exit;
            return data.body;
          },
          targetUrl,
          systemProxyPort,
          path.join(fixturesPath, 'net-session.js')
        );
        expect(body).to.equal('direct-response');
      } finally {
        targetServer.close();
        systemProxyServer.close();
      }
    });
  });
});
