import { expect } from 'chai';
import * as childProcess from 'child_process';
import * as path from 'path';
import * as util from 'util';
import { emittedOnce } from './events-helpers';
import { ifdescribe, ifit } from './spec-helpers';
import { webContents, WebContents } from 'electron/main';

const features = process.electronBinding('features');

describe('node feature', () => {
  const fixtures = path.join(__dirname, '..', 'spec', 'fixtures');
  describe('child_process', () => {
    describe('child_process.fork', () => {
      it('Works in browser process', (done) => {
        const child = childProcess.fork(path.join(fixtures, 'module', 'ping.js'));
        child.on('message', (msg) => {
          expect(msg).to.equal('message');
          done();
        });
        child.send('message');
      });
    });
  });

  describe('contexts', () => {
    describe('setTimeout called under Chromium event loop in browser process', () => {
      it('Can be scheduled in time', (done) => {
        setTimeout(done, 0);
      });

      it('Can be promisified', (done) => {
        util.promisify(setTimeout)(0).then(done);
      });
    });

    describe('setInterval called under Chromium event loop in browser process', () => {
      it('can be scheduled in time', (done) => {
        let interval: any = null;
        let clearing = false;
        const clear = () => {
          if (interval === null || clearing) return;

          // interval might trigger while clearing (remote is slow sometimes)
          clearing = true;
          clearInterval(interval);
          clearing = false;
          interval = null;
          done();
        };
        interval = setInterval(clear, 10);
      });
    });
  });

  describe('NODE_OPTIONS', () => {
    let child: childProcess.ChildProcessWithoutNullStreams;
    let exitPromise: Promise<any[]>;

    it('Fails for options disallowed by Node.js itself', (done) => {
      after(async () => {
        const [code, signal] = await exitPromise;
        expect(signal).to.equal(null);

        // Exit code 9 indicates cli flag parsing failure
        expect(code).to.equal(9);
        child.kill();
      });

      const env = Object.assign({}, process.env, { NODE_OPTIONS: '--v8-options' });
      child = childProcess.spawn(process.execPath, { env });
      exitPromise = emittedOnce(child, 'exit');

      let output = '';
      let success = false;
      const cleanup = () => {
        child.stderr.removeListener('data', listener);
        child.stdout.removeListener('data', listener);
      };

      const listener = (data: Buffer) => {
        output += data;
        if (/electron: --v8-options is not allowed in NODE_OPTIONS/m.test(output)) {
          success = true;
          cleanup();
          done();
        }
      };

      child.stderr.on('data', listener);
      child.stdout.on('data', listener);
      child.on('exit', () => {
        if (!success) {
          cleanup();
          done(new Error(`Unexpected output: ${output.toString()}`));
        }
      });
    });

    it('Disallows crypto-related options', (done) => {
      after(() => {
        child.kill();
      });

      const env = Object.assign({}, process.env, { NODE_OPTIONS: '--use-openssl-ca' });
      child = childProcess.spawn(process.execPath, ['--enable-logging'], { env });

      let output = '';
      const cleanup = () => {
        child.stderr.removeListener('data', listener);
        child.stdout.removeListener('data', listener);
      };

      const listener = (data: Buffer) => {
        output += data;
        if (/The NODE_OPTION --use-openssl-ca is not supported in Electron/m.test(output)) {
          cleanup();
          done();
        }
      };

      child.stderr.on('data', listener);
      child.stdout.on('data', listener);
    });
  });

  describe('Node.js cli flags', () => {
    let child: childProcess.ChildProcessWithoutNullStreams;
    let exitPromise: Promise<any[]>;

    ifit(features.isRunAsNodeEnabled())('Prohibits crypto-related flags in ELECTRON_RUN_AS_NODE mode', (done) => {
      after(async () => {
        const [code, signal] = await exitPromise;
        expect(signal).to.equal(null);
        expect(code).to.equal(9);
        child.kill();
      });

      child = childProcess.spawn(process.execPath, ['--force-fips'], {
        env: { ELECTRON_RUN_AS_NODE: 'true' }
      });
      exitPromise = emittedOnce(child, 'exit');

      let output = '';
      const cleanup = () => {
        child.stderr.removeListener('data', listener);
        child.stdout.removeListener('data', listener);
      };

      const listener = (data: Buffer) => {
        output += data;
        if (/.*The Node.js cli flag --force-fips is not supported in Electron/m.test(output)) {
          cleanup();
          done();
        }
      };

      child.stderr.on('data', listener);
      child.stdout.on('data', listener);
    });
  });

  ifdescribe(features.isRunAsNodeEnabled())('inspector', () => {
    let child: childProcess.ChildProcessWithoutNullStreams;
    let exitPromise: Promise<any[]>;

    afterEach(async () => {
      if (child && exitPromise) {
        const [code, signal] = await exitPromise;
        expect(signal).to.equal(null);
        expect(code).to.equal(0);
      } else if (child) {
        child.kill();
      }
      child = null as any;
      exitPromise = null as any;
    });

    it('Supports starting the v8 inspector with --inspect/--inspect-brk', (done) => {
      child = childProcess.spawn(process.execPath, ['--inspect-brk', path.join(fixtures, 'module', 'run-as-node.js')], {
        env: { ELECTRON_RUN_AS_NODE: 'true' }
      });

      let output = '';
      const cleanup = () => {
        child.stderr.removeListener('data', listener);
        child.stdout.removeListener('data', listener);
      };

      const listener = (data: Buffer) => {
        output += data;
        if (/Debugger listening on ws:/m.test(output)) {
          cleanup();
          done();
        }
      };

      child.stderr.on('data', listener);
      child.stdout.on('data', listener);
    });

    it('Supports starting the v8 inspector with --inspect and a provided port', (done) => {
      child = childProcess.spawn(process.execPath, ['--inspect=17364', path.join(fixtures, 'module', 'run-as-node.js')], {
        env: { ELECTRON_RUN_AS_NODE: 'true' }
      });
      exitPromise = emittedOnce(child, 'exit');

      let output = '';
      const listener = (data: Buffer) => { output += data; };
      const cleanup = () => {
        child.stderr.removeListener('data', listener);
        child.stdout.removeListener('data', listener);
      };

      child.stderr.on('data', listener);
      child.stdout.on('data', listener);
      child.on('exit', () => {
        cleanup();
        if (/^Debugger listening on ws:/m.test(output)) {
          expect(output.trim()).to.contain(':17364', 'should be listening on port 17364');
          done();
        } else {
          done(new Error(`Unexpected output: ${output.toString()}`));
        }
      });
    });

    it('Does not start the v8 inspector when --inspect is after a -- argument', (done) => {
      child = childProcess.spawn(process.execPath, [path.join(fixtures, 'module', 'noop.js'), '--', '--inspect']);
      exitPromise = emittedOnce(child, 'exit');

      let output = '';
      const listener = (data: Buffer) => { output += data; };
      child.stderr.on('data', listener);
      child.stdout.on('data', listener);
      child.on('exit', () => {
        if (output.trim().startsWith('Debugger listening on ws://')) {
          done(new Error('Inspector was started when it should not have been'));
        } else {
          done();
        }
      });
    });

    // IPC Electron child process not supported on Windows
    ifit(process.platform !== 'win32')('does not crash when quitting with the inspector connected', function (done) {
      child = childProcess.spawn(process.execPath, [path.join(fixtures, 'module', 'delay-exit'), '--inspect=0'], {
        stdio: ['ipc']
      }) as childProcess.ChildProcessWithoutNullStreams;
      exitPromise = emittedOnce(child, 'exit');

      const cleanup = () => {
        child.stderr.removeListener('data', listener);
        child.stdout.removeListener('data', listener);
      };

      let output = '';
      const success = false;
      function listener (data: Buffer) {
        output += data;
        console.log(data.toString()); // NOTE: temporary debug logging to try to catch flake.
        const match = /^Debugger listening on (ws:\/\/.+:\d+\/.+)\n/m.exec(output.trim());
        if (match) {
          cleanup();
          // NOTE: temporary debug logging to try to catch flake.
          child.stderr.on('data', (m) => console.log(m.toString()));
          child.stdout.on('data', (m) => console.log(m.toString()));
          const w = (webContents as any).create({}) as WebContents;
          w.loadURL('about:blank')
            .then(() => w.executeJavaScript(`new Promise(resolve => {
              const connection = new WebSocket(${JSON.stringify(match[1])})
              connection.onopen = () => {
                connection.onclose = () => resolve()
                connection.close()
              }
            })`))
            .then(() => {
              (w as any).destroy();
              child.send('plz-quit');
              done();
            });
        }
      }

      child.stderr.on('data', listener);
      child.stdout.on('data', listener);
      child.on('exit', () => {
        if (!success) cleanup();
      });
    });

    it('Supports js binding', (done) => {
      child = childProcess.spawn(process.execPath, ['--inspect', path.join(fixtures, 'module', 'inspector-binding.js')], {
        env: { ELECTRON_RUN_AS_NODE: 'true' },
        stdio: ['ipc']
      }) as childProcess.ChildProcessWithoutNullStreams;
      exitPromise = emittedOnce(child, 'exit');

      child.on('message', ({ cmd, debuggerEnabled, success }) => {
        if (cmd === 'assert') {
          expect(debuggerEnabled).to.be.true();
          expect(success).to.be.true();
          done();
        }
      });
    });
  });

  it('Can find a module using a package.json main field', () => {
    const result = childProcess.spawnSync(process.execPath, [path.resolve(fixtures, 'api', 'electron-main-module', 'app.asar')]);
    expect(result.status).to.equal(0);
  });

  ifit(features.isRunAsNodeEnabled())('handles Promise timeouts correctly', (done) => {
    const scriptPath = path.join(fixtures, 'module', 'node-promise-timer.js');
    const child = childProcess.spawn(process.execPath, [scriptPath], {
      env: { ELECTRON_RUN_AS_NODE: 'true' }
    });
    emittedOnce(child, 'exit').then(([code, signal]) => {
      expect(code).to.equal(0);
      expect(signal).to.equal(null);
      child.kill();
      done();
    });
  });
});
