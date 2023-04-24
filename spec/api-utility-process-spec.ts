import { expect } from 'chai';
import * as childProcess from 'child_process';
import * as path from 'path';
import { BrowserWindow, MessageChannelMain, utilityProcess } from 'electron/main';
import { emittedOnce } from './events-helpers';
import { ifit } from './spec-helpers';
import { closeWindow } from './window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures', 'api', 'utility-process');
const isWindowsOnArm = process.platform === 'win32' && process.arch === 'arm64';

describe('utilityProcess module', () => {
  describe('UtilityProcess constructor', () => {
    it('throws when empty script path is provided', async () => {
      expect(() => {
        /* eslint-disable no-new */
        utilityProcess.fork('');
        /* eslint-disable no-new */
      }).to.throw();
    });

    it('throws when options.stdio is not valid', async () => {
      expect(() => {
        /* eslint-disable no-new */
        utilityProcess.fork(path.join(fixturesPath, 'empty.js'), [], {
          execArgv: ['--test', '--test2'],
          serviceName: 'test',
          stdio: 'ipc'
        });
        /* eslint-disable no-new */
      }).to.throw(/stdio must be of the following values: inherit, pipe, ignore/);

      expect(() => {
        /* eslint-disable no-new */
        utilityProcess.fork(path.join(fixturesPath, 'empty.js'), [], {
          execArgv: ['--test', '--test2'],
          serviceName: 'test',
          stdio: ['ignore', 'ignore']
        });
        /* eslint-disable no-new */
      }).to.throw(/configuration missing for stdin, stdout or stderr/);

      expect(() => {
        /* eslint-disable no-new */
        utilityProcess.fork(path.join(fixturesPath, 'empty.js'), [], {
          execArgv: ['--test', '--test2'],
          serviceName: 'test',
          stdio: ['pipe', 'inherit', 'inherit']
        });
        /* eslint-disable no-new */
      }).to.throw(/stdin value other than ignore is not supported/);
    });
  });

  describe('lifecycle events', () => {
    it('emits \'spawn\' when child process successfully launches', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'empty.js'));
      await emittedOnce(child, 'spawn');
    });

    it('emits \'exit\' when child process exits gracefully', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'empty.js'));
      const [code] = await emittedOnce(child, 'exit');
      expect(code).to.equal(0);
    });

    it('emits \'exit\' when child process crashes', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'crash.js'));
      // Do not check for exit code in this case,
      // SIGSEGV code can be 139 or 11 across our different CI pipeline.
      await emittedOnce(child, 'exit');
    });

    it('emits \'exit\' corresponding to the child process', async () => {
      const child1 = utilityProcess.fork(path.join(fixturesPath, 'endless.js'));
      await emittedOnce(child1, 'spawn');
      const child2 = utilityProcess.fork(path.join(fixturesPath, 'crash.js'));
      await emittedOnce(child2, 'exit');
      expect(child1.kill()).to.be.true();
      await emittedOnce(child1, 'exit');
    });

    it('emits \'exit\' when there is uncaught exception', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'exception.js'));
      const [code] = await emittedOnce(child, 'exit');
      expect(code).to.equal(1);
    });

    it('emits \'exit\' when process.exit is called', async () => {
      const exitCode = 2;
      const child = utilityProcess.fork(path.join(fixturesPath, 'custom-exit.js'), [`--exitCode=${exitCode}`]);
      const [code] = await emittedOnce(child, 'exit');
      expect(code).to.equal(exitCode);
    });
  });

  describe('kill() API', () => {
    it('terminates the child process gracefully', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'endless.js'), [], {
        serviceName: 'endless'
      });
      await emittedOnce(child, 'spawn');
      expect(child.kill()).to.be.true();
      await emittedOnce(child, 'exit');
    });
  });

  describe('pid property', () => {
    it('is valid when child process launches successfully', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'empty.js'));
      await emittedOnce(child, 'spawn');
      expect(child.pid).to.not.be.null();
    });

    it('is undefined when child process fails to launch', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'does-not-exist.js'));
      expect(child.pid).to.be.undefined();
    });
  });

  describe('stdout property', () => {
    it('is null when child process launches with default stdio', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'));
      await emittedOnce(child, 'spawn');
      expect(child.stdout).to.be.null();
      expect(child.stderr).to.be.null();
      await emittedOnce(child, 'exit');
    });

    it('is null when child process launches with ignore stdio configuration', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'), [], {
        stdio: 'ignore'
      });
      await emittedOnce(child, 'spawn');
      expect(child.stdout).to.be.null();
      expect(child.stderr).to.be.null();
      await emittedOnce(child, 'exit');
    });

    it('is valid when child process launches with pipe stdio configuration', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'), [], {
        stdio: 'pipe'
      });
      await emittedOnce(child, 'spawn');
      expect(child.stdout).to.not.be.null();
      let log = '';
      child.stdout!.on('data', (chunk) => {
        log += chunk.toString('utf8');
      });
      await emittedOnce(child, 'exit');
      expect(log).to.equal('hello\n');
    });
  });

  describe('stderr property', () => {
    it('is null when child process launches with default stdio', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'));
      await emittedOnce(child, 'spawn');
      expect(child.stdout).to.be.null();
      expect(child.stderr).to.be.null();
      await emittedOnce(child, 'exit');
    });

    it('is null when child process launches with ignore stdio configuration', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'), [], {
        stdio: 'ignore'
      });
      await emittedOnce(child, 'spawn');
      expect(child.stderr).to.be.null();
      await emittedOnce(child, 'exit');
    });

    ifit(!isWindowsOnArm)('is valid when child process launches with pipe stdio configuration', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'log.js'), [], {
        stdio: ['ignore', 'pipe', 'pipe']
      });
      await emittedOnce(child, 'spawn');
      expect(child.stderr).to.not.be.null();
      let log = '';
      child.stderr!.on('data', (chunk) => {
        log += chunk.toString('utf8');
      });
      await emittedOnce(child, 'exit');
      expect(log).to.equal('world');
    });
  });

  describe('postMessage() API', () => {
    it('establishes a default ipc channel with the child process', async () => {
      const result = 'I will be echoed.';
      const child = utilityProcess.fork(path.join(fixturesPath, 'post-message.js'));
      await emittedOnce(child, 'spawn');
      child.postMessage(result);
      const [data] = await emittedOnce(child, 'message');
      expect(data).to.equal(result);
      const exit = emittedOnce(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
    });

    it('supports queuing messages on the receiving end', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'post-message-queue.js'));
      const p = emittedOnce(child, 'spawn');
      child.postMessage('This message');
      child.postMessage(' is');
      child.postMessage(' queued');
      await p;
      const [data] = await emittedOnce(child, 'message');
      expect(data).to.equal('This message is queued');
      const exit = emittedOnce(child, 'exit');
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
        child.once('exit', () => { done(); });
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
        child.once('exit', () => { done(); });
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

    ifit(process.platform !== 'win32')('supports redirecting stdout to parent process', async () => {
      const result = 'Output from utility process';
      const appProcess = childProcess.spawn(process.execPath, [path.join(fixturesPath, 'inherit-stdout'), `--payload=${result}`]);
      let output = '';
      appProcess.stdout.on('data', (data: Buffer) => { output += data; });
      await emittedOnce(appProcess, 'exit');
      expect(output).to.equal(result);
    });

    ifit(process.platform !== 'win32')('supports redirecting stderr to parent process', async () => {
      const result = 'Error from utility process';
      const appProcess = childProcess.spawn(process.execPath, [path.join(fixturesPath, 'inherit-stderr'), `--payload=${result}`]);
      let output = '';
      appProcess.stderr.on('data', (data: Buffer) => { output += data; });
      await emittedOnce(appProcess, 'exit');
      expect(output).to.include(result);
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
      await emittedOnce(child, 'spawn');
      child.postMessage('', [childPort1]);
      const [data] = await emittedOnce(child, 'message');
      expect(data).to.equal(result);
      // Cleanup.
      const exit = emittedOnce(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
      await closeWindow(w);
    });

    ifit(process.platform === 'linux')('allows executing a setuid binary with child_process', async () => {
      const child = utilityProcess.fork(path.join(fixturesPath, 'suid.js'));
      await emittedOnce(child, 'spawn');
      const [data] = await emittedOnce(child, 'message');
      expect(data).to.not.be.empty();
      const exit = emittedOnce(child, 'exit');
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
      appProcess.stdout.on('data', (data: Buffer) => { output += data; });
      await emittedOnce(appProcess.stdout, 'end');
      const result = process.platform === 'win32' ? '\r\nparent' : 'parent';
      expect(output).to.equal(result);
    });

    it('does not inherit parent env when custom env is provided', async () => {
      const appProcess = childProcess.spawn(process.execPath, [path.join(fixturesPath, 'env-app'), '--create-custom-env'], {
        env: {
          FROM: 'parent',
          ...process.env
        }
      });
      let output = '';
      appProcess.stdout.on('data', (data: Buffer) => { output += data; });
      await emittedOnce(appProcess.stdout, 'end');
      const result = process.platform === 'win32' ? '\r\nchild' : 'child';
      expect(output).to.equal(result);
    });

    it('changes working directory with cwd', async () => {
      const child = utilityProcess.fork('./log.js', [], {
        cwd: fixturesPath,
        stdio: ['ignore', 'pipe', 'ignore']
      });
      await emittedOnce(child, 'spawn');
      expect(child.stdout).to.not.be.null();
      let log = '';
      child.stdout!.on('data', (chunk) => {
        log += chunk.toString('utf8');
      });
      await emittedOnce(child, 'exit');
      expect(log).to.equal('hello\n');
    });

    it('does not crash when running eval', async () => {
      const child = utilityProcess.fork('./eval.js', [], {
        cwd: fixturesPath,
        stdio: 'ignore'
      });
      await emittedOnce(child, 'spawn');
      const [data] = await emittedOnce(child, 'message');
      expect(data).to.equal(42);
      // Cleanup.
      const exit = emittedOnce(child, 'exit');
      expect(child.kill()).to.be.true();
      await exit;
    });
  });
});
