import { BrowserWindow, webContents } from 'electron/main';

import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it } from 'vitest';

import * as childProcess from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import { createRequire } from 'node:module';
import * as os from 'node:os';
import * as path from 'node:path';
import * as tty from 'node:tty';
import { pathToFileURL } from 'node:url';
import * as util from 'node:util';

import {
  copyMacOSFixtureApp,
  getCodesignIdentity,
  shouldRunCodesignTests,
  signApp,
  spawn,
  stripFrameworkSymbols
} from './lib/codesign-helpers.ts';
import { withTempDirectory } from './lib/fs-helpers.ts';
import {
  getRemoteContext,
  ifdescribe,
  ifit,
  isTestingBindingAvailable,
  itremote,
  spawnAndWait,
  startRemoteControlApp,
  useRemoteContext
} from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

import type { EventEmitter } from 'node:stream';

// The startup snapshot spec below eval()s a CommonJS snippet in this scope too.
const require = createRequire(import.meta.url);

const mainFixturesPath = path.resolve(import.meta.dirname, 'fixtures');

describe('node feature', () => {
  const fixtures = path.join(import.meta.dirname, 'fixtures');

  describe('child_process', () => {
    describe('child_process.fork', () => {
      it('Works in browser process', async () => {
        const child = childProcess.fork(path.join(fixtures, 'module', 'ping.js'));
        const message = once(child, 'message');
        child.send('message');
        const [msg] = await message;
        expect(msg).toBe('message');
      });

      it('Has its module search paths restricted', async () => {
        const child = childProcess.fork(path.join(fixtures, 'module', 'module-paths.js'));
        const [msg] = await once(child, 'message');
        expect(msg.length).toBe(2);
      });

      ifit(process.platform === 'darwin')(
        'does not start an app instance when the helper is executed without a process type',
        () => {
          const { status, stderr } = childProcess.spawnSync(
            process.helperExecPath,
            [path.join(fixtures, 'module', 'ping.js')],
            {
              encoding: 'utf-8',
              timeout: 20000
            }
          );
          expect(status).toBe(64);
          expect(stderr).toContain('requires a --type argument');
        }
      );
    });
  });

  describe('child_process in renderer', () => {
    useRemoteContext();

    describe('child_process.fork', () => {
      itremote(
        'works in current process',
        async (fixtures: string) => {
          const child = require('node:child_process').fork(require('node:path').join(fixtures, 'module', 'ping.js'));
          const message = new Promise<any>((resolve) => child.once('message', resolve));
          child.send('message');
          const msg = await message;
          expect(msg).toBe('message');
        },
        [fixtures]
      );

      itremote(
        'preserves args',
        async (fixtures: string) => {
          const args = ['--expose_gc', '-test', '1'];
          const child = require('node:child_process').fork(
            require('node:path').join(fixtures, 'module', 'process_args.js'),
            args
          );
          const message = new Promise<any>((resolve) => child.once('message', resolve));
          child.send('message');
          const msg = await message;
          expect(args).toEqual(msg.slice(2));
        },
        [fixtures]
      );

      itremote(
        'works in forked process',
        async (fixtures: string) => {
          const child = require('node:child_process').fork(
            require('node:path').join(fixtures, 'module', 'fork_ping.js')
          );
          const message = new Promise<any>((resolve) => child.once('message', resolve));
          child.send('message');
          const msg = await message;
          expect(msg).toBe('message');
        },
        [fixtures]
      );

      itremote(
        'works in forked process when options.env is specified',
        async (fixtures: string) => {
          const child = require('node:child_process').fork(
            require('node:path').join(fixtures, 'module', 'fork_ping.js'),
            [],
            {
              path: process.env.PATH
            }
          );
          const message = new Promise<any>((resolve) => child.once('message', resolve));
          child.send('message');
          const msg = await message;
          expect(msg).toBe('message');
        },
        [fixtures]
      );

      itremote(
        'has String::localeCompare working in script',
        async (fixtures: string) => {
          const child = require('node:child_process').fork(
            require('node:path').join(fixtures, 'module', 'locale-compare.js')
          );
          const message = new Promise<any>((resolve) => child.once('message', resolve));
          child.send('message');
          const msg = await message;
          expect(msg).toEqual([0, -1, 1]);
        },
        [fixtures]
      );

      itremote(
        'has setImmediate working in script',
        async (fixtures: string) => {
          const child = require('node:child_process').fork(
            require('node:path').join(fixtures, 'module', 'set-immediate.js')
          );
          const message = new Promise<any>((resolve) => child.once('message', resolve));
          child.send('message');
          const msg = await message;
          expect(msg).toBe('ok');
        },
        [fixtures]
      );

      itremote(
        'pipes stdio',
        async (fixtures: string) => {
          const child = require('node:child_process').fork(
            require('node:path').join(fixtures, 'module', 'process-stdout.js'),
            { silent: true }
          );
          let data = '';
          child.stdout.on('data', (chunk: any) => {
            data += String(chunk);
          });
          const code = await new Promise<any>((resolve) => child.once('close', resolve));
          expect(code).toBe(0);
          expect(data).toBe('pipes stdio');
        },
        [fixtures]
      );

      itremote('works when sending a message to a process forked with the --eval argument', async () => {
        const source = "process.on('message', (message) => { process.send(message) })";
        const forked = require('node:child_process').fork('--eval', [source]);
        const message = new Promise((resolve) => forked.once('message', resolve));
        forked.send('hello');
        const msg = await message;
        expect(msg).toBe('hello');
      });

      it('has the electron version in process.versions', async () => {
        const source = 'process.send(process.versions)';
        const forked = childProcess.fork('--eval', [source]);
        const [message] = await once(forked, 'message');
        expect(message).toHaveProperty('electron');
        expect(message.electron).toBeTypeOf('string');
        expect(message.electron).toMatch(/^\d+\.\d+\.\d+(\S*)?$/);
      });
    });

    describe('child_process.spawn', () => {
      itremote(
        'supports spawning Electron as a node process via the ELECTRON_RUN_AS_NODE env var',
        async (fixtures: string) => {
          const child = require('node:child_process').spawn(
            process.execPath,
            [require('node:path').join(fixtures, 'module', 'run-as-node.js')],
            {
              env: {
                ELECTRON_RUN_AS_NODE: true
              }
            }
          );

          let output = '';
          child.stdout.on('data', (data: any) => {
            output += data;
          });
          try {
            await new Promise((resolve) => child.stdout.once('close', resolve));
            expect(JSON.parse(output)).toEqual({
              stdoutType: 'pipe',
              processType: 'undefined',
              window: 'undefined'
            });
          } finally {
            child.kill();
          }
        },
        [fixtures]
      );
    });

    describe('child_process.exec', () => {
      ifit(process.platform === 'linux')('allows executing a setuid binary from non-sandboxed renderer', async () => {
        // Chrome uses prctl(2) to set the NO_NEW_PRIVILEGES flag on Linux (see
        // https://github.com/torvalds/linux/blob/40fde647cc/Documentation/userspace-api/no_new_privs.rst).
        // We disable this for unsandboxed processes, which the renderer tests
        // are running in. If this test fails with an error like 'effective uid
        // is not 0', then it's likely that our patch to prevent the flag from
        // being set has become ineffective.
        const w = await getRemoteContext();
        const stdout = await w.webContents.executeJavaScript("require('child_process').execSync('sudo --help')");
        expect(stdout).not.toHaveLength(0);
      });
    });
  });

  describe('EventSource', () => {
    itremote('works correctly when nodeIntegration is enabled in the renderer', () => {
      const es = new EventSource('https://example.com');
      expect(es).toHaveProperty('url', expect.any(String));
      expect(es).toHaveProperty('readyState', expect.any(Number));
      expect(es).toHaveProperty('withCredentials', expect.any(Boolean));
    });
  });

  describe('fetch', () => {
    itremote(
      'works correctly when nodeIntegration is enabled in the renderer',
      async (fixtures: string) => {
        const file = require('node:path').join(fixtures, 'hello.txt');
        expect(() => {
          fetch('file://' + file);
        }).not.toThrow();

        expect(() => {
          const formData = new FormData();
          formData.append('username', 'Groucho');
        }).not.toThrow();

        expect(() => {
          const request = new Request('https://example.com', {
            method: 'POST',
            body: JSON.stringify({ foo: 'bar' })
          });
          expect(request.method).toBe('POST');
        }).not.toThrow();

        expect(() => {
          const response = new Response('Hello, world!');
          expect(response.status).toBe(200);
        }).not.toThrow();

        expect(() => {
          const headers = new Headers();
          headers.append('Content-Type', 'text/xml');
        }).not.toThrow();
      },
      [fixtures]
    );

    itremote('uses Blink implementations when nodeIntegration is enabled in the renderer', () => {
      // Node.js' undici-based fetch would return its JS source here; Blink's
      // bindings are native. This guards against Node's bootstrap clobbering
      // the renderer's Web Platform globals.
      for (const ctor of [fetch, FormData, Headers, Request, Response, EventSource]) {
        expect(ctor.toString()).toMatch(/\[native code\]/);
      }
    });
  });

  it('does not hang when using the fs module in the renderer process', async () => {
    const appPath = path.join(mainFixturesPath, 'apps', 'libuv-hang', 'main.js');
    const appProcess = childProcess.spawn(process.execPath, [appPath], {
      cwd: path.join(mainFixturesPath, 'apps', 'libuv-hang'),
      stdio: 'inherit'
    });
    const [code] = await once(appProcess, 'close');
    expect(code).toBe(0);
  });

  describe('contexts', () => {
    describe('setTimeout called under Chromium event loop in browser process', () => {
      it('Can be scheduled in time', () =>
        new Promise<void>((resolve, reject) => {
          const done = (error?: unknown) => (error ? reject(error) : resolve());
          setTimeout(done, 0);
        }));

      it('Can be promisified', () =>
        new Promise<void>((resolve, reject) => {
          const done = (error?: unknown) => (error ? reject(error) : resolve());
          util.promisify(setTimeout)(0).then(done);
        }));
    });

    describe('setInterval called under Chromium event loop in browser process', () => {
      it('can be scheduled in time', () =>
        new Promise<void>((resolve) => {
          let interval: any = null;
          let clearing = false;
          const clear = () => {
            if (interval === null || clearing) return;

            // interval might trigger while clearing (remote is slow sometimes)
            clearing = true;
            clearInterval(interval);
            clearing = false;
            interval = null;
            resolve();
          };
          interval = setInterval(clear, 10);
        }));
    });

    // Native code that settles a promise from outside a task (an X11 reply, an
    // OS event handler) must still have its continuations run before the next
    // task does. Clipboard reads on Linux otherwise sat until something
    // unrelated woke the main loop.
    ifdescribe(isTestingBindingAvailable())('promise settled by native code outside a task', () => {
      it('runs its continuations before the next task', async () => {
        const testing = (process as any)._linkedBinding('electron_common_testing');
        let continued = false;
        const continuedBeforeNextTask = await new Promise<boolean>((resolve) => {
          testing
            .settlePromiseOutsideTask(() => resolve(continued))
            .then(() => {
              continued = true;
            });
        });
        expect(continuedBeforeNextTask).toBe(true);
      });
    });

    const suspendListeners = (emitter: EventEmitter, eventName: string, callback: (...args: any[]) => void) => {
      const listeners = emitter.listeners(eventName) as ((...args: any[]) => void)[];
      emitter.removeAllListeners(eventName);
      emitter.once(eventName, (...args) => {
        emitter.removeAllListeners(eventName);
        for (const listener of listeners) {
          emitter.on(eventName, listener);
        }

        callback(...args);
      });
    };
    describe('error thrown in main process node context', () => {
      it('gets emitted as a process uncaughtException event', async () => {
        fs.readFile(import.meta.filename, () => {
          throw new Error('hello');
        });
        const result = await new Promise((resolve) =>
          suspendListeners(process, 'uncaughtException', (error) => {
            resolve(error.message);
          })
        );
        expect(result).toBe('hello');
      });
    });

    describe('promise rejection in main process node context', () => {
      it('gets emitted as a process unhandledRejection event', async () => {
        fs.readFile(import.meta.filename, () => {
          Promise.reject(new Error('hello'));
        });
        const result = await new Promise((resolve) =>
          suspendListeners(process, 'unhandledRejection', (error) => {
            resolve(error.message);
          })
        );
        expect(result).toBe('hello');
      });

      it('does not log the warning more than once when the rejection is unhandled', async () => {
        const appPath = path.join(mainFixturesPath, 'api', 'unhandled-rejection.js');
        const appProcess = childProcess.spawn(process.execPath, [appPath]);

        let output = '';
        const out = (data: string) => {
          output += data;
          if (/UnhandledPromiseRejectionWarning/.test(data)) {
            appProcess.kill();
          }
        };
        appProcess.stdout!.on('data', out);
        appProcess.stderr!.on('data', out);

        await once(appProcess, 'close');
        expect(/UnhandledPromiseRejectionWarning/.test(output)).toBe(true);
        const matches = output.match(/Error: oops/gm);
        expect(matches).toHaveLength(1);
      });

      it('does not log the warning more than once when the rejection is handled', async () => {
        const appPath = path.join(mainFixturesPath, 'api', 'unhandled-rejection-handled.js');
        const appProcess = childProcess.spawn(process.execPath, [appPath]);

        let output = '';
        const out = (data: string) => {
          output += data;
        };
        appProcess.stdout!.on('data', out);
        appProcess.stderr!.on('data', out);

        const [code] = await once(appProcess, 'close');
        expect(code).toBe(0);
        expect(/UnhandledPromiseRejectionWarning/.test(output)).toBe(false);
        const matches = output.match(/Error: oops/gm);
        expect(matches).toHaveLength(1);
      });
    });
  });

  describe('contexts in renderer', () => {
    useRemoteContext();

    describe('setTimeout in fs callback', () => {
      itremote(
        'does not crash',
        async (filename: string) => {
          await new Promise((resolve) =>
            require('node:fs').readFile(filename, () => {
              setTimeout(resolve, 0);
            })
          );
        },
        [import.meta.filename]
      );
    });

    describe('error thrown in renderer process node context', () => {
      itremote(
        'gets emitted as a process uncaughtException event',
        async (filename: string) => {
          const error = new Error('boo!');
          require('node:fs').readFile(filename, () => {
            throw error;
          });
          await new Promise<void>((resolve, reject) => {
            process.once('uncaughtException', (thrown) => {
              try {
                expect(thrown).toBe(error);
                resolve();
              } catch (e) {
                reject(e);
              }
            });
          });
        },
        [import.meta.filename]
      );
    });

    describe('URL handling in the renderer process', () => {
      itremote(
        'can successfully handle WHATWG URLs constructed by Blink',
        (fixtures: string) => {
          const url = new URL('file://' + require('node:path').resolve(fixtures, 'pages', 'base-page.html'));
          expect(() => {
            require('node:fs').createReadStream(url);
          }).not.toThrow();
        },
        [fixtures]
      );
    });

    describe('setTimeout called under blink env in renderer process', () => {
      itremote('can be scheduled in time', async () => {
        await new Promise((resolve) => setTimeout(resolve, 10));
      });

      itremote('works from the timers module', async () => {
        await new Promise((resolve) => require('node:timers').setTimeout(resolve, 10));
      });
    });

    describe('setInterval called under blink env in renderer process', () => {
      itremote('can be scheduled in time', async () => {
        await new Promise<void>((resolve) => {
          const id = setInterval(() => {
            clearInterval(id);
            resolve();
          }, 10);
        });
      });

      itremote('can be scheduled in time from timers module', async () => {
        const { setInterval, clearInterval } = require('node:timers');
        await new Promise<void>((resolve) => {
          const id = setInterval(() => {
            clearInterval(id);
            resolve();
          }, 10);
        });
      });
    });
  });

  describe('environments sharing a renderer', () => {
    afterEach(closeAllWindows);

    const nodePreferences = { nodeIntegration: true, contextIsolation: false, sandbox: false };

    const churnDuring = (teardown: string) => `new Promise((resolve) => {
      const fs = require('node:fs');
      let immediates = 0;
      const spin = () => { if (++immediates < 200) setImmediate(spin); };
      setImmediate(spin);
      let read = false;
      fs.readFile(__filename, () => { read = true; });
      let echoed = false;
      require('@electron-ci/echo').async(true, (value) => { echoed = value; });
      let closed = 0;
      const watchers = Array.from({ length: 5 }, () => fs.watch(__filename));
      for (const w of watchers) w.on('close', () => closed++);
      for (const w of watchers) w.close();
      ${teardown};
      const done = () => immediates < 200 || !read || !echoed ? setTimeout(done, 10) : resolve({ immediates, read, echoed, closed });
      setTimeout(done, 10);
    })`;

    const loopWork = ['immediate', 'fs', 'napi async work', 'napi threadsafe function'];
    const exerciseLoop = (wc: Electron.WebContents) =>
      wc.executeJavaScript(`Promise.all([
        new Promise((resolve) => setImmediate(() => resolve('immediate'))),
        new Promise((resolve) => require('node:fs').readFile(__filename, () => resolve('fs'))),
        new Promise((resolve) => require('@electron-ci/echo').async('napi async work', resolve)),
        new Promise((resolve) => require('@electron-ci/echo').threadsafe('napi threadsafe function', resolve))
      ])`);

    const openWindow = async (webPreferences = {}) => {
      const w = new BrowserWindow({ show: false, webPreferences: { ...nodePreferences, ...webPreferences } });
      w.webContents.setWindowOpenHandler(() => ({
        action: 'allow',
        overrideBrowserWindowOptions: { show: false, webPreferences: nodePreferences }
      }));
      const errors: string[] = [];
      w.webContents.on('console-message', ({ level, message }) => {
        if (level === 'error') errors.push(message);
      });
      await w.loadFile(path.join(fixtures, 'pages', 'blank.html'));
      return { w, errors };
    };

    const openChild = async (w: BrowserWindow, url: string) => {
      const childCreated = once(w.webContents, 'did-create-window') as Promise<[BrowserWindow, any]>;
      await w.webContents.executeJavaScript(`window.child = window.open(${JSON.stringify(url)}); undefined`);
      const [child] = await childCreated;
      if (child.webContents.isLoading()) await once(child.webContents, 'did-finish-load');
      expect(child.webContents.getOSProcessId()).toBe(w.webContents.getOSProcessId());
      return child;
    };

    it('runs each same-process child window on a working loop of its own', async () => {
      const { w, errors } = await openWindow();
      const child = await openChild(w, 'base-page.html');
      await expect(exerciseLoop(child.webContents)).resolves.toEqual(loopWork);
      const blankChild = await openChild(w, 'about:blank');
      await expect(exerciseLoop(blankChild.webContents)).resolves.toEqual(loopWork);
      expect(errors).toEqual([]);
    });

    it('keeps a child window working while its opener reloads', async () => {
      const { w, errors } = await openWindow();
      const child = await openChild(w, 'base-page.html');
      for (let i = 0; i < 2; i++) {
        const pending = exerciseLoop(child.webContents);
        w.webContents.reload();
        await once(w.webContents, 'did-finish-load');
        await expect(pending).resolves.toEqual(loopWork);
        await expect(exerciseLoop(child.webContents)).resolves.toEqual(loopWork);
      }
      await expect(exerciseLoop(w.webContents)).resolves.toEqual(loopWork);
      expect(errors).toEqual([]);
    });

    it('keeps the opener working when a same-process child window closes', async () => {
      const { w, errors } = await openWindow();
      for (let i = 0; i < 3; i++) {
        const child = await openChild(w, 'base-page.html');
        const closed = once(child, 'closed');
        const result = await w.webContents.executeJavaScript(churnDuring('window.child.close()'));
        await closed;
        expect(result).toEqual({ immediates: 200, read: true, echoed: true, closed: 5 });
      }
      expect(errors).toEqual([]);
    });

    it('keeps the parent frame working when a node-integrated iframe is removed', async () => {
      const { w, errors } = await openWindow({ nodeIntegrationInSubFrames: true });
      for (let i = 0; i < 3; i++) {
        await w.webContents.executeJavaScript(`new Promise((resolve) => {
          const frame = document.createElement('iframe');
          frame.src = 'base-page.html';
          frame.onload = () => resolve();
          document.body.appendChild(window.frame = frame);
        })`);
        const result = await w.webContents.executeJavaScript(churnDuring('window.frame.remove()'));
        expect(result).toEqual({ immediates: 200, read: true, echoed: true, closed: 5 });
      }
      expect(errors).toEqual([]);
    });

    // Regression test for https://github.com/electron/electron/issues/53789.
    it('does not crash when a node-integrated iframe is removed before its loop first runs', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: false,
          nodeIntegrationInSubFrames: true,
          preload: path.join(fixtures, 'module', 'preload-remove-own-frame.js')
        }
      });
      const gone = once(w.webContents, 'render-process-gone') as Promise<
        [Electron.Event, Electron.RenderProcessGoneDetails]
      >;
      await w.loadFile(path.join(fixtures, 'pages', 'blank.html'));
      const removed = w.webContents.executeJavaScript(`new Promise((resolve) => {
        const frames = Array.from({ length: 10 }, () => {
          const frame = document.createElement('iframe');
          frame.src = 'base-page.html';
          return document.body.appendChild(frame);
        });
        const check = () => frames.some((frame) => frame.isConnected) ? setTimeout(check, 10) : resolve(frames.length);
        check();
      })`);
      const result = await Promise.race([removed, gone.then(([, details]) => `render process ${details.reason}`)]);
      expect(result).toBe(10);
    });
  });

  describe('native addons', () => {
    // Regression test for https://github.com/electron/electron/issues/53387:
    // Node.js 24.19.0 made node::ObjectWrap register an environment cleanup
    // hook, and removing it from ~ObjectWrap() during garbage collection
    // aborted the process until nodejs/node#63985 and nodejs/node#65630 were
    // picked up. The addon is loaded in a child process because the failure
    // mode is a CHECK abort.
    //
    // The fixture has to be compiled against Electron's headers, which
    // script/spec-runner.js does after installing the spec modules. That does
    // not happen on the Linux arm CI test jobs: linux-arm skips the spec
    // install (--skipYarnInstall) and on linux-arm64 nothing can compile the
    // headers, as the restored Chromium clang is an x64 binary and the image's
    // GCC rejects the V8 headers
    // (https://github.com/electron/electron/issues/53284). spec-runner sets
    // this variable in those cases and the test is reported as skipped.
    ifit(!process.env.ELECTRON_SKIP_ELECTRON_HEADER_ADDON_SPECS)(
      'node::ObjectWrap instances survive garbage collection and teardown',
      async () => {
        const child = childProcess.spawn(process.execPath, [path.join(fixtures, 'module', 'object-wrap-gc.js')], {
          env: { ...process.env, ELECTRON_RUN_AS_NODE: '1' },
          stdio: ['ignore', 'pipe', 'pipe']
        });
        let stdout = '';
        let stderr = '';
        child.stdout.setEncoding('utf8');
        child.stderr.setEncoding('utf8');
        child.stdout.on('data', (chunk) => {
          stdout += chunk;
        });
        child.stderr.on('data', (chunk) => {
          stderr += chunk;
        });
        const [code, signal] = await once(child, 'close');
        expect(signal, stderr).toBeNull();
        expect(code, stderr).toBe(0);
        expect(stdout.trim()).toBe('ok');
      }
    );
  });

  describe('message loop in renderer', () => {
    useRemoteContext();

    describe('process.nextTick', () => {
      itremote('emits the callback', () => new Promise((resolve) => process.nextTick(resolve)));

      itremote(
        'works in nested calls',
        () =>
          new Promise((resolve) => {
            process.nextTick(() => {
              process.nextTick(() => process.nextTick(resolve));
            });
          })
      );
    });

    describe('setImmediate', () => {
      itremote('emits the callback', () => new Promise((resolve) => setImmediate(resolve)));

      itremote(
        'works in nested calls',
        () =>
          new Promise((resolve) => {
            setImmediate(() => {
              setImmediate(() => setImmediate(resolve));
            });
          })
      );
    });
  });

  ifdescribe(process.platform === 'darwin')('net.connect', () => {
    itremote(
      'emit error when connect to a socket path without listeners',
      async (fixtures: string) => {
        const socketPath = require('node:path').join(require('node:os').tmpdir(), 'electron-test.sock');
        const script = require('node:path').join(fixtures, 'module', 'create_socket.js');
        const child = require('node:child_process').fork(script, [socketPath]);
        const code = await new Promise((resolve) => child.once('exit', resolve));
        expect(code).toBe(0);
        const client = require('node:net').connect(socketPath);
        const error = await new Promise<any>((resolve) => client.once('error', resolve));
        expect(error.code).toBe('ECONNREFUSED');
      },
      [fixtures]
    );
  });

  describe('Buffer', () => {
    useRemoteContext();

    itremote('can be created from Blink external string', () => {
      const p = document.createElement('p');
      p.innerText = '闲云潭影日悠悠，物换星移几度秋';
      const b = Buffer.from(p.innerText);
      expect(b.toString()).toBe('闲云潭影日悠悠，物换星移几度秋');
      expect(Buffer.byteLength(p.innerText)).toBe(45);
    });

    itremote('correctly parses external one-byte UTF8 string', () => {
      const p = document.createElement('p');
      p.innerText = 'Jøhänñéß';
      const b = Buffer.from(p.innerText);
      expect(b.toString()).toBe('Jøhänñéß');
      expect(Buffer.byteLength(p.innerText)).toBe(13);
    });

    itremote('does not crash when creating large Buffers', () => {
      let buffer = Buffer.from(new Array(4096).join(' '));
      expect(buffer.length).toBe(4095);
      buffer = Buffer.from(new Array(4097).join(' '));
      expect(buffer.length).toBe(4096);
    });

    itremote('does not crash for crypto operations', () => {
      const crypto = require('node:crypto');
      const data =
        'lG9E+/g4JmRmedDAnihtBD4Dfaha/GFOjd+xUOQI05UtfVX3DjUXvrS98p7kZQwY3LNhdiFo7MY5rGft8yBuDhKuNNag9vRx/44IuClDhdQ=';
      const key = 'q90K9yBqhWZnAMCMTOJfPQ==';
      const cipherText = '{"error_code":114,"error_message":"Tham số không hợp lệ","data":null}';
      for (let i = 0; i < 10000; ++i) {
        const iv = Buffer.from('0'.repeat(32), 'hex');
        const input = Buffer.from(data, 'base64');
        const decipher = crypto.createDecipheriv('aes-128-cbc', Buffer.from(key, 'base64'), iv);
        const result = Buffer.concat([decipher.update(input), decipher.final()]).toString('utf8');
        expect(cipherText).toBe(result);
      }
    });

    itremote('does not crash when using crypto.diffieHellman() constructors', () => {
      const crypto = require('node:crypto');

      crypto.createDiffieHellman('abc');
      crypto.createDiffieHellman('abc', 2);

      // Needed to test specific DiffieHellman ctors.

      crypto.createDiffieHellman('abc', Buffer.from([2]));
      crypto.createDiffieHellman('abc', '123');
    });

    itremote('does not crash when calling crypto.createPrivateKey() with an unsupported algorithm', () => {
      const crypto = require('node:crypto');

      const ed448 = {
        crv: 'Ed448',
        x: 'KYWcaDwgH77xdAwcbzOgvCVcGMy9I6prRQBhQTTdKXUcr-VquTz7Fd5adJO0wT2VHysF3bk3kBoA',
        d: 'UhC3-vN5vp_g9PnTknXZgfXUez7Xvw-OfuJ0pYkuwzpYkcTvacqoFkV_O05WMHpyXkzH9q2wzx5n',
        kty: 'OKP'
      };

      expect(() => {
        crypto.createPrivateKey({ key: ed448, format: 'jwk' });
      }).toThrow(/Invalid JWK/);
    });
  });

  describe('process.exit', () => {
    it('exits with exit code zero when called without an argument', async () => {
      const rc = await startRemoteControlApp();
      rc.remotely(() => {
        setImmediate(() => process.exit());
      }).catch(() => {});
      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(0);
    });

    it('uses process.exitCode when called without an argument', async () => {
      const rc = await startRemoteControlApp();
      rc.remotely(() => {
        process.exitCode = 42;
        setImmediate(() => process.exit());
      }).catch(() => {});
      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(42);
    });

    it('overrides process.exitCode when called with an argument', async () => {
      const rc = await startRemoteControlApp();
      rc.remotely(() => {
        process.exitCode = 42;
        setImmediate(() => process.exit(11));
      }).catch(() => {});
      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(11);
    });

    it('can be called with a null argument', async () => {
      const rc = await startRemoteControlApp();
      rc.remotely(() => {
        setImmediate(() => process.exit(null));
      }).catch(() => {});
      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(0);
    });

    it('can be called with a number argument', async () => {
      const rc = await startRemoteControlApp();
      rc.remotely(() => {
        setImmediate(() => process.exit(7));
      }).catch(() => {});
      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(7);
    });

    it('throws with an invalid number argument', async () => {
      const rc = await startRemoteControlApp();
      let stdout = '';
      rc.process.stdout!.on('data', (d) => {
        stdout += d.toString();
      });
      rc.remotely(() => {
        setImmediate(() => {
          try {
            process.exit(4.2);
          } catch (err) {
            console.log(err);
            process.exit(99);
          }
        });
      }).catch(() => {});
      const [code] = await once(rc.process, 'close');
      expect(code).toBe(99);
      expect(stdout).toMatch(
        /RangeError \[ERR_OUT_OF_RANGE\]: The value of "code" is out of range. It must be an integer./
      );
    });

    it('can be called with a string argument', async () => {
      const rc = await startRemoteControlApp();
      rc.remotely(() => {
        setImmediate(() => process.exit('12'));
      }).catch(() => {});
      const [code] = await once(rc.process, 'exit');
      expect(code).toBe(12);
    });

    it('throws with an invalid string argument', async () => {
      const rc = await startRemoteControlApp();
      let stdout = '';
      rc.process.stdout!.on('data', (d) => {
        stdout += d.toString();
      });
      rc.remotely(() => {
        setImmediate(() => {
          try {
            process.exit('invalid');
          } catch (err) {
            console.log(err);
            process.exit(99);
          }
        });
      }).catch(() => {});
      const [code] = await once(rc.process, 'close');
      expect(code).toBe(99);
      expect(stdout).toMatch(/TypeError \[ERR_INVALID_ARG_TYPE\]/);
    });
  });

  describe('process.stdout', () => {
    useRemoteContext();

    it('is a real Node stream', () => {
      expect((process.stdout as any)._type).not.toBeUndefined();
    });

    itremote('does not throw an exception when accessed', () => {
      expect(() => process.stdout).not.toThrow();
    });

    itremote('does not throw an exception when calling write()', () => {
      expect(() => {
        process.stdout.write('test');
      }).not.toThrow();
    });

    describe('isTTY', () => {
      itremote("should match Node's TTY classification in the renderer", () => {
        const { isatty } = require('node:tty');

        expect(process.stdout.isTTY === true).toBe(isatty(1));
      });

      ifdescribe(process.platform !== 'win32')('POSIX platforms', () => {
        const parentStdoutIsTTY = tty.isatty(1);

        itremote(
          'should inherit stdout TTY classification from the browser process',
          function (parentStdoutIsTTY: boolean) {
            const { isatty } = require('node:tty');

            expect(isatty(1)).toBe(parentStdoutIsTTY);
          },
          [parentStdoutIsTTY]
        );
      });
    });
  });

  describe('process.stdin', () => {
    useRemoteContext();

    itremote('does not throw an exception when accessed', () => {
      expect(() => process.stdin).not.toThrow();
    });

    itremote('returns null when read from', () => {
      expect(process.stdin.read()).toBeNull();
    });
  });

  describe('process.version', () => {
    itremote('should not have -pre', () => {
      expect(process.version.endsWith('-pre')).toBe(false);
    });
  });

  describe('vm.runInNewContext', () => {
    itremote('should not crash', () => {
      require('node:vm').runInNewContext('');
    });
  });

  describe('crypto', () => {
    useRemoteContext();
    itremote('should list the ripemd160 hash in getHashes', () => {
      expect(require('node:crypto').getHashes()).toContain('ripemd160');
    });

    itremote('should be able to create a ripemd160 hash and use it', () => {
      const hash = require('node:crypto').createHash('ripemd160');
      hash.update('electron-ripemd160');
      expect(hash.digest('hex')).toBe('fa7fec13c624009ab126ebb99eda6525583395fe');
    });

    itremote('should list aes-{128,256}-cfb in getCiphers', () => {
      expect(require('node:crypto').getCiphers()).toEqual(expect.arrayContaining(['aes-128-cfb', 'aes-256-cfb']));
    });

    itremote('should be able to create an aes-128-cfb cipher', () => {
      require('node:crypto').createCipheriv('aes-128-cfb', '0123456789abcdef', '0123456789abcdef');
    });

    itremote('should be able to create an aes-256-cfb cipher', () => {
      require('node:crypto').createCipheriv('aes-256-cfb', '0123456789abcdef0123456789abcdef', '0123456789abcdef');
    });

    itremote('should be able to create a bf-{cbc,cfb,ecb} ciphers', () => {
      require('node:crypto').createCipheriv('bf-cbc', Buffer.from('0123456789abcdef'), Buffer.from('01234567'));
      require('node:crypto').createCipheriv('bf-cfb', Buffer.from('0123456789abcdef'), Buffer.from('01234567'));
      require('node:crypto').createCipheriv('bf-ecb', Buffer.from('0123456789abcdef'), Buffer.from('01234567'));
    });

    itremote('should list des-ede-cbc in getCiphers', () => {
      expect(require('node:crypto').getCiphers()).toContain('des-ede-cbc');
    });

    itremote('should be able to create an des-ede-cbc cipher', () => {
      const key = Buffer.from('0123456789abcdeff1e0d3c2b5a49786', 'hex');
      const iv = Buffer.from('fedcba9876543210', 'hex');
      require('node:crypto').createCipheriv('des-ede-cbc', key, iv);
    });

    itremote('should not crash when getting an ECDH key', () => {
      const ecdh = require('node:crypto').createECDH('prime256v1');
      expect(ecdh.generateKeys()).toBeInstanceOf(Buffer);
      expect(ecdh.getPrivateKey()).toBeInstanceOf(Buffer);
    });

    itremote('should not crash when generating DH keys or fetching DH fields', () => {
      const dh = require('node:crypto').createDiffieHellman('modp15');
      expect(dh.generateKeys()).toBeInstanceOf(Buffer);
      expect(dh.getPublicKey()).toBeInstanceOf(Buffer);
      expect(dh.getPrivateKey()).toBeInstanceOf(Buffer);
      expect(dh.getPrime()).toBeInstanceOf(Buffer);
      expect(dh.getGenerator()).toBeInstanceOf(Buffer);
    });

    itremote('should not crash when creating an ECDH cipher', () => {
      const crypto = require('node:crypto');
      const dh = crypto.createECDH('prime256v1');
      dh.generateKeys();
      dh.setPrivateKey(dh.getPrivateKey());
    });
  });

  itremote('includes the electron version in process.versions', () => {
    expect(process.versions).toHaveProperty('electron');
    expect(process.versions.electron).toBeTypeOf('string');
    expect(process.versions.electron).toMatch(/^\d+\.\d+\.\d+(\S*)?$/);
  });

  itremote('includes the chrome version in process.versions', () => {
    expect(process.versions).toHaveProperty('chrome');
    expect(process.versions.chrome).toBeTypeOf('string');
    expect(process.versions.chrome).toMatch(/^\d+\.\d+\.\d+\.\d+$/);
  });

  describe('NODE_OPTIONS', () => {
    let child: childProcess.ChildProcessWithoutNullStreams;
    let exitPromise: Promise<any[]>;

    it('Fails for options disallowed by Node.js itself', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        afterAll(async () => {
          const [code, signal] = await exitPromise;
          expect(signal).toBe(null);

          // Exit code 9 indicates cli flag parsing failure
          expect(code).toBe(9);
          child.kill();
        });

        const env = { ...process.env, NODE_OPTIONS: '--v8-options' };
        child = childProcess.spawn(process.execPath, { env });
        exitPromise = once(child, 'exit');

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
        child.on('close', () => {
          if (!success) {
            cleanup();
            done(new Error(`Unexpected output: ${output.toString()}`));
          }
        });
      }));

    it('Disallows crypto-related options', () =>
      new Promise<void>((resolve) => {
        afterAll(() => {
          child.kill();
        });

        const appPath = path.join(fixtures, 'module', 'noop.js');
        const env = { ...process.env, NODE_OPTIONS: '--use-openssl-ca' };
        child = childProcess.spawn(process.execPath, ['--enable-logging', appPath], { env });

        let output = '';
        const cleanup = () => {
          child.stderr.removeListener('data', listener);
          child.stdout.removeListener('data', listener);
        };

        const listener = (data: Buffer) => {
          output += data;
          if (/The NODE_OPTION --use-openssl-ca is not supported in Electron/m.test(output)) {
            cleanup();
            resolve();
          }
        };

        child.stderr.on('data', listener);
        child.stdout.on('data', listener);
      }));

    it('does allow --require in non-packaged apps', async () => {
      const appPath = path.join(fixtures, 'module', 'noop.js');
      const env = {
        ...process.env,
        NODE_OPTIONS: `--require=${path.join(fixtures, 'module', 'fail.js')}`
      };
      // App should exit with code 1.
      const child = childProcess.spawn(process.execPath, [appPath], { env });
      const [code] = await once(child, 'exit');
      expect(code).toBe(1);
    });

    it('does allow --require in utility process of non-packaged apps', async () => {
      const appPath = path.join(fixtures, 'apps', 'node-options-utility-process');
      // App should exit with code 1.
      const child = childProcess.spawn(process.execPath, [appPath]);
      const [code] = await once(child, 'exit');
      expect(code).toBe(1);
    });

    it('does not allow --require in utility process of packaged apps', async () => {
      const appPath = path.join(fixtures, 'apps', 'node-options-utility-process');
      // App should exit with code 1.
      const child = childProcess.spawn(process.execPath, [appPath], {
        env: {
          ...process.env,
          ELECTRON_FORCE_IS_PACKAGED: 'true'
        }
      });
      const [code] = await once(child, 'exit');
      expect(code).toBe(0);
    });

    it('does not allow --require in packaged apps', async () => {
      const appPath = path.join(fixtures, 'module', 'noop.js');
      const env = {
        ...process.env,
        ELECTRON_FORCE_IS_PACKAGED: 'true',
        NODE_OPTIONS: `--require=${path.join(fixtures, 'module', 'fail.js')}`
      };
      // App should exit with code 0.
      const child = childProcess.spawn(process.execPath, [appPath], { env });
      const [code] = await once(child, 'exit');
      expect(code).toBe(0);
    });

    it('does not throw --import hook in non-packaged apps', async () => {
      const appPath = path.join(fixtures, 'module', 'noop.js');
      // Since this is the import path - we should always use backward slashes
      // regardless of the `path.sep` value.
      const hookUrl = pathToFileURL(path.join(fixtures, 'module', 'hook.mjs'));
      const env = {
        ...process.env,
        NODE_OPTIONS: `--import=${hookUrl}`
      };
      // App should exit with code 0.
      const child = childProcess.spawn(process.execPath, [appPath], { env });
      const [code] = await once(child, 'exit');
      expect(code).toBe(0);
    });
  });

  ifdescribe(shouldRunCodesignTests)('NODE_OPTIONS in signed app', () => {
    let identity = '';
    // Copying and deep-signing the whole unstripped Electron.app
    // took most of each test's 30s budget on the Intel runners.
    // As in the updater specs, deep-sign one stripped template up front;
    // each test clones it and needs only a shallow re-sign.
    let templateDir = '';
    let templateApp = '';

    beforeAll(
      async () => {
        const result = getCodesignIdentity();
        if (result === null) return; // beforeEach below skips every test
        identity = result;
        templateDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'electron-node-options-template-'));
        templateApp = await copyMacOSFixtureApp(templateDir, null);
        stripFrameworkSymbols(templateApp);
        const { code, out } = await signApp(templateApp, identity);
        if (code !== 0) {
          throw new Error(`Failed to sign template app: ${out}`);
        }
      },
      5 * 60 * 1000
    );

    afterAll(() => {
      if (templateDir) childProcess.spawnSync('rm', ['-r', templateDir]);
    });

    const copySignedApp = async (dir: string, fixture: string | null) => {
      const appPath = await copyMacOSFixtureApp(dir, fixture, { sourceApp: templateApp });
      const { code, out } = await signApp(appPath, identity, { deep: false });
      if (code !== 0) {
        throw new Error(`codesign failed for ${appPath}: ${out}`);
      }
      return appPath;
    };

    beforeEach((ctx) => {
      const result = getCodesignIdentity();
      if (result === null) {
        ctx.skip();
      } else {
        identity = result;
      }
    });

    const script = path.join(fixtures, 'api', 'fork-with-node-options.js');
    const nodeOptionsWarning =
      'Node.js environment variables are disabled because this process is invoked by other apps';

    it('is disabled when invoked by other apps in ELECTRON_RUN_AS_NODE mode', async () => {
      await withTempDirectory(async (dir) => {
        const appPath = await copySignedApp(dir, 'initial');
        // Invoke Electron by using the system node binary as middle layer, so
        // the check of NODE_OPTIONS will think the process is started by other
        // apps.
        const { code, out } = await spawn('node', [script, path.join(appPath, 'Contents/MacOS/Electron')]);
        expect(code).toBe(0);
        expect(out).toContain(nodeOptionsWarning);
      });
    });

    it('is disabled when invoked by alien binary in app bundle in ELECTRON_RUN_AS_NODE mode', async (ctx) => {
      await withTempDirectory(async (dir) => {
        const appPath = await copySignedApp(dir, 'initial');
        // Find system node and copy it to app bundle.
        const nodePath = process.env.PATH?.split(path.delimiter).find((dir) => fs.existsSync(path.join(dir, 'node')));
        if (!nodePath) {
          ctx.skip();
          return;
        }
        const alienBinary = path.join(appPath, 'Contents/MacOS/node');
        await fs.promises.cp(path.join(nodePath, 'node'), alienBinary, { recursive: true });
        // Try to execute electron app from the alien node in app bundle.
        const { code, out } = await spawn(alienBinary, [script, path.join(appPath, 'Contents/MacOS/Electron')]);
        expect(code).toBe(0);
        expect(out).toContain(nodeOptionsWarning);
      });
    });

    it('is respected when invoked from self', async () => {
      await withTempDirectory(async (dir) => {
        const appPath = await copySignedApp(dir, null);
        const appExePath = path.join(appPath, 'Contents/MacOS/Electron');
        const { code, out } = await spawn(appExePath, [script, appExePath]);
        expect(code).toBe(1);
        expect(out).not.toContain(nodeOptionsWarning);
        expect(out).toContain('NODE_OPTIONS passed to child');
      });
    });
  });

  describe('Node.js cli flags', () => {
    let child: childProcess.ChildProcessWithoutNullStreams;
    let exitPromise: Promise<any[]>;

    it('Prohibits crypto-related flags in ELECTRON_RUN_AS_NODE mode', () =>
      new Promise<void>((resolve) => {
        afterAll(async () => {
          const [code, signal] = await exitPromise;
          expect(signal).toBe(null);
          expect(code).toBe(9);
          child.kill();
        });

        child = childProcess.spawn(process.execPath, ['--force-fips'], {
          env: { ELECTRON_RUN_AS_NODE: 'true' }
        });
        exitPromise = once(child, 'exit');

        let output = '';
        const cleanup = () => {
          child.stderr.removeListener('data', listener);
          child.stdout.removeListener('data', listener);
        };

        const listener = (data: Buffer) => {
          output += data;
          if (/.*The Node.js cli flag --force-fips is not supported in Electron/m.test(output)) {
            cleanup();
            resolve();
          }
        };

        child.stderr.on('data', listener);
        child.stdout.on('data', listener);
      }));
  });

  describe('fs.readFile', () => {
    it('can accept a FileHandle as the Path argument', async () => {
      const filePathForHandle = path.resolve(mainFixturesPath, 'dogs-running.txt');
      const fileHandle = await fs.promises.open(filePathForHandle, 'r');

      const file = await fs.promises.readFile(fileHandle, { encoding: 'utf8' });
      expect(file).not.toHaveLength(0);
      await fileHandle.close();
    });
  });

  describe('inspector', () => {
    let child: childProcess.ChildProcessWithoutNullStreams;
    let exitPromise: Promise<any[]> | null;

    afterEach(async () => {
      if (child && exitPromise) {
        const [code, signal] = await exitPromise;
        expect(signal).toBe(null);
        expect(code).toBe(0);
      } else if (child) {
        child.kill();
      }
      child = null as any;
      exitPromise = null as any;
    });

    it('Supports starting the v8 inspector with --inspect/--inspect-brk', () =>
      new Promise<void>((resolve) => {
        child = childProcess.spawn(
          process.execPath,
          ['--inspect-brk', path.join(fixtures, 'module', 'run-as-node.js')],
          {
            env: { ELECTRON_RUN_AS_NODE: 'true' }
          }
        );

        let output = '';
        const cleanup = () => {
          child.stderr.removeListener('data', listener);
          child.stdout.removeListener('data', listener);
        };

        const listener = (data: Buffer) => {
          output += data;
          if (/Debugger listening on ws:/m.test(output)) {
            cleanup();
            resolve();
          }
        };

        child.stderr.on('data', listener);
        child.stdout.on('data', listener);
      }));

    it('Supports starting the v8 inspector with --inspect and a provided port', async () => {
      child = childProcess.spawn(
        process.execPath,
        ['--inspect=17364', path.join(fixtures, 'module', 'run-as-node.js')],
        {
          env: { ELECTRON_RUN_AS_NODE: 'true' }
        }
      );
      exitPromise = once(child, 'exit');

      let output = '';
      const listener = (data: Buffer) => {
        output += data;
      };
      const cleanup = () => {
        child.stderr.removeListener('data', listener);
        child.stdout.removeListener('data', listener);
      };

      child.stderr.on('data', listener);
      child.stdout.on('data', listener);
      await once(child, 'close');
      cleanup();
      if (/^Debugger listening on ws:/m.test(output)) {
        expect(output.trim(), 'should be listening on port 17364').toContain(':17364');
      } else {
        throw new Error(`Unexpected output: ${output.toString()}`);
      }
    });

    it('Does not start the v8 inspector when --inspect is after a -- argument', async () => {
      child = childProcess.spawn(process.execPath, [path.join(fixtures, 'module', 'noop.js'), '--', '--inspect']);
      exitPromise = once(child, 'exit');

      let output = '';
      const listener = (data: Buffer) => {
        output += data;
      };
      child.stderr.on('data', listener);
      child.stdout.on('data', listener);
      await once(child, 'close');
      if (output.trim().startsWith('Debugger listening on ws://')) {
        throw new Error('Inspector was started when it should not have been');
      }
    });

    // IPC Electron child process not supported on Windows.
    ifit(process.platform !== 'win32')(
      'does not crash when quitting with the inspector connected',
      () =>
        new Promise<void>((resolve) => {
          child = childProcess.spawn(process.execPath, [path.join(fixtures, 'module', 'delay-exit'), '--inspect=0'], {
            stdio: ['ipc']
          }) as childProcess.ChildProcessWithoutNullStreams;
          exitPromise = once(child, 'exit');

          const cleanup = () => {
            child.stderr.removeListener('data', listener);
            child.stdout.removeListener('data', listener);
          };

          let output = '';
          const success = false;
          function listener(data: Buffer) {
            output += data;
            console.log(data.toString()); // NOTE: temporary debug logging to try to catch flake.
            const match = /^Debugger listening on (ws:\/\/.+:\d+\/.+)\n/m.exec(output.trim());
            if (match) {
              cleanup();
              // NOTE: temporary debug logging to try to catch flake.
              child.stderr.on('data', (m) => console.log(m.toString()));
              child.stdout.on('data', (m) => console.log(m.toString()));
              const w = (webContents as typeof ElectronInternal.WebContents).create();
              w.loadURL('about:blank')
                .then(() =>
                  w.executeJavaScript(`new Promise(resolve => {
              const connection = new WebSocket(${JSON.stringify(match[1])})
              connection.onopen = () => {
                connection.onclose = () => resolve()
                connection.close()
              }
            })`)
                )
                .then(() => {
                  w.destroy();
                  child.send('plz-quit');
                  resolve();
                });
            }
          }

          child.stderr.on('data', listener);
          child.stdout.on('data', listener);
          child.on('exit', () => {
            if (!success) cleanup();
          });
        })
    );

    it('Supports js binding', async () => {
      child = childProcess.spawn(
        process.execPath,
        ['--inspect', path.join(fixtures, 'module', 'inspector-binding.js')],
        {
          env: { ELECTRON_RUN_AS_NODE: 'true' },
          stdio: ['ipc']
        }
      ) as childProcess.ChildProcessWithoutNullStreams;
      exitPromise = once(child, 'exit');

      const [{ cmd, debuggerEnabled, success }] = await once(child, 'message');
      expect(cmd).toBe('assert');
      expect(debuggerEnabled).toBe(true);
      expect(success).toBe(true);
    });
  });

  itremote('handles assert module assertions as expected', () => {
    const assert = require('node:assert');
    try {
      assert.ok(false);
      expect.fail('assert.ok(false) should throw');
    } catch (err) {
      console.log(err);
      expect(err).toBeInstanceOf(assert.AssertionError);
    }
  });

  it('Can find a module using a package.json main field', async () => {
    const { code, stdout, stderr } = await spawnAndWait(
      process.execPath,
      [path.resolve(fixtures, 'api', 'electron-main-module', 'app.asar')],
      { timeout: 20_000 }
    );
    expect(code, `stdout:\n${stdout}\nstderr:\n${stderr}`).toBe(0);
  });

  it('handles Promise timeouts correctly', async () => {
    const scriptPath = path.join(fixtures, 'module', 'node-promise-timer.js');
    const child = childProcess.spawn(process.execPath, [scriptPath], {
      env: { ELECTRON_RUN_AS_NODE: 'true' }
    });
    const [code, signal] = await once(child, 'exit');
    expect(code).toBe(0);
    expect(signal).toBe(null);
    child.kill();
  });

  it('performs microtask checkpoint correctly', () =>
    new Promise<void>((resolve, reject) => {
      const done = (error?: unknown) => (error ? reject(error) : resolve());
      let timer: NodeJS.Timeout;
      const listener = () => {
        done(new Error('catch block is delayed to next tick'));
      };

      const f3 = async () => {
        return new Promise((resolve, reject) => {
          timer = setTimeout(listener);
          reject(new Error('oops'));
        });
      };

      setTimeout(() => {
        f3().catch(() => {
          clearTimeout(timer);
          done();
        });
      });
    }));

  describe('node:wasi', () => {
    it('does not crash when a wasiImport call is optimized', async () => {
      const child = childProcess.spawn(
        process.execPath,
        [path.join(fixtures, 'module', 'wasi-optimized-import-call.js')],
        {
          env: { ELECTRON_RUN_AS_NODE: 'true' }
        }
      );
      let output = '';
      child.stdout.on('data', (data) => {
        output += data;
      });
      const [code] = await once(child, 'close');
      expect(output).toBe('ok');
      expect(code).toBe(0);
    });
  });

  describe('type stripping', () => {
    it('strips TypeScript types automatically in the main process', async () => {
      const child = childProcess.spawn(process.execPath, [path.join(fixtures, 'type-stripping', 'basic.ts')]);
      const [code] = await once(child, 'exit');
      expect(code).toBe(0);
    });

    it('will not transform TypeScript types without --experimental-transform-types', async () => {
      const child = childProcess.spawn(
        process.execPath,
        [path.join(fixtures, 'type-stripping', 'transform-types-node.ts')],
        {
          env: { ELECTRON_RUN_AS_NODE: 'true' }
        }
      );
      const [code] = await once(child, 'exit');
      expect(code).not.toBe(0);
    });

    it('transforms TypeScript types with --experimental-transform-types', async () => {
      const child = childProcess.spawn(process.execPath, [
        '--experimental-transform-types',
        path.join(fixtures, 'type-stripping', 'transform-types.ts')
      ]);
      const [code] = await once(child, 'exit');
      expect(code).toBe(0);
    });
  });
});

describe('Node.js startup snapshot', () => {
  afterEach(closeAllWindows);

  // Everything here is captured into the embedded Node.js startup snapshot by
  // node_mksnapshot, which runs on the build host. Renderer processes bootstrap
  // Node.js from scratch, so they hold the values this binary was actually
  // built for; anything the snapshot took from the host instead shows up as a
  // difference. Serialized through JSON since the bindings have null
  // prototypes.
  const collect = `JSON.stringify({
    arch: process.arch,
    platform: process.platform,
    endianness: require('node:os').endianness(),
    features: process.features,
    config: process.binding('config'),
    constants: process.binding('constants'),
    buffer: require('node:buffer').constants
  })`;
  // defaultCipherList is a CLI option value that only environments owning the
  // process state define, so it never exists in a renderer.
  const comparable = (json: string) => {
    const values = JSON.parse(json);
    delete values.constants.crypto.defaultCipherList;
    return values;
  };
  // eslint-disable-next-line no-eval
  const fromThisProcess = () => comparable(eval(collect));

  const fromFreshEnvironment = async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: { nodeIntegration: true, contextIsolation: false }
    });
    await w.loadURL('about:blank');
    return comparable(await w.webContents.executeJavaScript(collect));
  };

  it('captures the same values as an environment bootstrapped from scratch', async () => {
    expect(fromThisProcess()).toEqual(await fromFreshEnvironment());
  });

  it('captures the same values for ELECTRON_RUN_AS_NODE', async () => {
    const child = childProcess.spawn(process.execPath, ['-p', collect], {
      env: { ...process.env, ELECTRON_RUN_AS_NODE: '1' },
      stdio: ['ignore', 'pipe', 'inherit']
    });
    let stdout = '';
    child.stdout.setEncoding('utf8');
    child.stdout.on('data', (chunk) => {
      stdout += chunk;
    });
    const [code] = await once(child, 'close');
    expect(code).toBe(0);
    expect(comparable(stdout)).toEqual(await fromFreshEnvironment());
  });

  ifit(process.platform !== 'win32')('can open a directory with O_DIRECTORY', async () => {
    await withTempDirectory(async (dir) => {
      const { O_RDONLY, O_DIRECTORY } = fs.constants;
      fs.closeSync(fs.openSync(dir, O_RDONLY | O_DIRECTORY));
    });
  });

  ifit(process.platform !== 'win32')('refuses to follow a symlink with O_NOFOLLOW', async () => {
    await withTempDirectory(async (dir) => {
      const target = path.join(dir, 'target');
      const link = path.join(dir, 'link');
      fs.writeFileSync(target, '');
      fs.symlinkSync(target, link);
      const { O_RDONLY, O_NOFOLLOW } = fs.constants;
      expect(() => fs.openSync(link, O_RDONLY | O_NOFOLLOW)).toThrow(expect.objectContaining({ code: 'ELOOP' }));
    });
  });
});
