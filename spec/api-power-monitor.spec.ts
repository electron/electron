import * as dbus from 'dbus-native';
// For these tests we use a fake DBus daemon to verify powerMonitor module
// interaction with the system bus. This requires python-dbusmock installed and
// running (with the DBUS_SYSTEM_BUS_ADDRESS environment variable set).
// script/spec-runner.js will take care of spawning the fake DBus daemon and setting
// DBUS_SYSTEM_BUS_ADDRESS when python-dbusmock is installed.
//
// See https://pypi.python.org/pypi/python-dbusmock for more information about
// python-dbusmock.
import { afterAll, beforeAll, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import { createRequire } from 'node:module';
import { setTimeout } from 'node:timers/promises';
import { promisify } from 'node:util';

import { ifdescribe, ifit, startRemoteControlApp } from './lib/spec-helpers.ts';

const require = createRequire(import.meta.url);

describe('powerMonitor', { tags: ['serial'] }, () => {
  let logindMock: any, dbusMockPowerMonitor: any, getCalls: any, emitSignal: any, reset: any;

  ifdescribe(process.platform === 'linux' && process.env.DBUS_SYSTEM_BUS_ADDRESS != null)(
    'when powerMonitor module is loaded with dbus mock',
    () => {
      beforeAll(async () => {
        const systemBus = dbus.systemBus();
        const loginService = systemBus.getService('org.freedesktop.login1');
        const getInterface = promisify(loginService.getInterface.bind(loginService));
        logindMock = await getInterface('/org/freedesktop/login1', 'org.freedesktop.DBus.Mock');
        getCalls = promisify(logindMock.GetCalls.bind(logindMock));
        emitSignal = promisify(logindMock.EmitSignal.bind(logindMock));
        reset = promisify(logindMock.Reset.bind(logindMock));
        // The mock outlives this process; forget calls other Electron processes
        // of the same test run have made so the counts below are ours alone.
        await promisify(logindMock.ClearCalls.bind(logindMock))();
      });

      afterAll(async () => {
        await reset();
      });

      function onceMethodCalled(done: () => void) {
        function cb() {
          logindMock.removeListener('MethodCalled', cb);
        }
        done();
        return cb;
      }

      beforeAll(
        () =>
          new Promise<void>((resolve, reject) => {
            const done = (error?: unknown) => (error ? reject(error) : resolve());
            logindMock.on('MethodCalled', onceMethodCalled(done));
            // lazy load powerMonitor after we listen to MethodCalled mock signal
            dbusMockPowerMonitor = require('electron').powerMonitor;
          })
      );

      it('should take the shutdown delay lock at startup, without powerMonitor', async () => {
        const callsBefore = (await getCalls()).length;
        // This app never touches powerMonitor.
        await startRemoteControlApp();
        let calls: any[] = [];
        for (let retries = 3; retries > 0; retries--) {
          calls = await getCalls();
          if (calls.length >= callsBefore + 1) break;
          await setTimeout(1000);
        }
        expect(calls).toHaveLength(callsBefore + 1);
        expect(calls[callsBefore].slice(1)).toEqual([
          'Inhibit',
          [
            [[{ type: 's', child: [] }], ['shutdown']],
            [[{ type: 's', child: [] }], ['electron']],
            [[{ type: 's', child: [] }], ['Ensure a clean shutdown']],
            [[{ type: 's', child: [] }], ['delay']]
          ]
        ]);
      });

      it('should take the sleep delay lock when powerMonitor is used', async () => {
        const callsBefore = (await getCalls()).length;
        // Any listener instantiates the native PowerMonitor.
        dbusMockPowerMonitor.on('dummy-event', () => {});
        let calls: any[] = [];
        for (let retries = 3; retries > 0; retries--) {
          calls = await getCalls();
          if (calls.length >= callsBefore + 1) break;
          await setTimeout(1000);
        }
        expect(calls).toHaveLength(callsBefore + 1);
        expect(calls[callsBefore].slice(1)).toEqual([
          'Inhibit',
          [
            [[{ type: 's', child: [] }], ['sleep']],
            [[{ type: 's', child: [] }], ['electron']],
            [[{ type: 's', child: [] }], ['Application cleanup before suspend']],
            [[{ type: 's', child: [] }], ['delay']]
          ]
        ]);
      });

      describe('when PrepareForSleep(true) signal is sent by logind', () => {
        it('should emit "suspend" event', async () => {
          const suspend = once(dbusMockPowerMonitor, 'suspend');
          emitSignal('org.freedesktop.login1.Manager', 'PrepareForSleep', 'b', [['b', true]]);
          await suspend;
        });

        describe('when PrepareForSleep(false) signal is sent by logind', () => {
          let callsBefore = 0;

          beforeAll(async () => {
            callsBefore = (await getCalls()).length;
          });

          it('should emit "resume" event', async () => {
            const resume = once(dbusMockPowerMonitor, 'resume');
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForSleep', 'b', [['b', false]]);
            await resume;
          });

          it('should have called Inhibit again for sleep', async () => {
            let calls = await getCalls();
            for (let retries = 3; retries > 0 && calls.length < callsBefore + 1; retries--) {
              await setTimeout(1000);
              calls = await getCalls();
            }
            expect(calls).toBeInstanceOf(Array);
            // The sleep lock is released on suspend and taken again on resume.
            expect(calls).toHaveLength(callsBefore + 1);
            expect(calls[callsBefore].slice(1)).toEqual([
              'Inhibit',
              [
                [[{ type: 's', child: [] }], ['sleep']],
                [[{ type: 's', child: [] }], ['electron']],
                [[{ type: 's', child: [] }], ['Application cleanup before suspend']],
                [[{ type: 's', child: [] }], ['delay']]
              ]
            ]);
          });
        });
      });

      describe('when PrepareForShutdown(true) signal is sent by logind', () => {
        it('should emit "shutdown" event', () =>
          new Promise<void>((resolve) => {
            dbusMockPowerMonitor.once('shutdown', (event: Electron.Event) => {
              // Keep this process alive; Electron would otherwise quit.
              event.preventDefault();
              resolve();
            });
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', true]]);
          }));

        it('should report one session end once, and again after a cancel', async () => {
          let count = 0;
          const onShutdown = (event: Electron.Event) => {
            event.preventDefault();
            count++;
          };
          dbusMockPowerMonitor.on('shutdown', onShutdown);
          const waitForCount = async (expected: number) => {
            let retries = 20;
            while (retries-- > 0) {
              if (count >= expected) break;
              await setTimeout(50);
            }
            return count;
          };
          try {
            // Clear the latch left by the previous test.
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', false]]);
            await setTimeout(100);

            // A reboot ends the session, so logind and the portal can both
            // report it. The app must hear about it once.
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', true]]);
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', true]]);
            expect(await waitForCount(1)).toBe(1);
            await setTimeout(200);
            expect(count).toBe(1);

            // A shutdown that was called off is a new cycle.
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', false]]);
            await setTimeout(100);
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', true]]);
            expect(await waitForCount(2)).toBe(2);
          } finally {
            dbusMockPowerMonitor.off('shutdown', onShutdown);
          }
        });

        it('should quit when nothing delays the shutdown', async () => {
          const preventDefault = (event: Electron.Event) => event.preventDefault();
          // This process has a powerMonitor too, so it would quit as well.
          dbusMockPowerMonitor.on('shutdown', preventDefault);
          try {
            const callsBefore = (await getCalls()).length;
            const remoteApp = await startRemoteControlApp();
            // Create a powerMonitor without a "shutdown" listener.
            await remoteApp.remotely(() => {
              require('electron').powerMonitor.on('suspend', () => {});
            });
            // Both locks are held once the app is connected to logind, so it
            // will also see the signal.
            let calls: any[] = [];
            for (let retries = 3; retries > 0; retries--) {
              calls = await getCalls();
              if (calls.length >= callsBefore + 2) break;
              await setTimeout(1000);
            }
            expect(calls).toHaveLength(callsBefore + 2);
            const exited = once(remoteApp.process, 'exit');
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', true]]);
            const [code] = await exited;
            expect(code).toBe(0);
          } finally {
            dbusMockPowerMonitor.off('shutdown', preventDefault);
          }
        });

        it('should keep running when "shutdown" is the first listener', async () => {
          const preventDefault = (event: Electron.Event) => event.preventDefault();
          // This process has a powerMonitor too, so it would quit as well.
          dbusMockPowerMonitor.on('shutdown', preventDefault);
          try {
            const callsBefore = (await getCalls()).length;
            const remoteApp = await startRemoteControlApp();
            // "shutdown" is the first listener the app registers.
            await remoteApp.remotely(() => {
              require('electron').powerMonitor.on('shutdown', (event: Electron.Event) => {
                event.preventDefault();
                (globalThis as any).sawShutdown = true;
              });
            });
            let calls: any[] = [];
            for (let retries = 3; retries > 0; retries--) {
              calls = await getCalls();
              if (calls.length >= callsBefore + 2) break;
              await setTimeout(1000);
            }
            expect(calls).toHaveLength(callsBefore + 2);
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', true]]);
            let sawShutdown = false;
            for (let retries = 20; retries > 0 && !sawShutdown; retries--) {
              await setTimeout(100);
              try {
                sawShutdown = await remoteApp.remotely(() => (globalThis as any).sawShutdown === true);
              } catch {
                // The app quit instead of handling the event.
                break;
              }
            }
            expect(sawShutdown).toBe(true);
          } finally {
            dbusMockPowerMonitor.off('shutdown', preventDefault);
          }
        });

        it('should handle "shutdown" registered while the main script runs', async () => {
          const preventDefault = (event: Electron.Event) => event.preventDefault();
          // This process has a powerMonitor too, so it would quit as well.
          dbusMockPowerMonitor.on('shutdown', preventDefault);
          try {
            const callsBefore = (await getCalls()).length;
            // Registered at the top level of the app's main script, before the
            // app is ready.
            const bootEval =
              'require("electron").powerMonitor.on("shutdown", (event) => { event.preventDefault(); globalThis.sawShutdown = true; })';
            const remoteApp = await startRemoteControlApp([`--boot-eval=${bootEval}`]);
            let calls: any[] = [];
            for (let retries = 3; retries > 0; retries--) {
              calls = await getCalls();
              if (calls.length >= callsBefore + 2) break;
              await setTimeout(1000);
            }
            expect(calls).toHaveLength(callsBefore + 2);
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', true]]);
            let sawShutdown = false;
            for (let retries = 20; retries > 0 && !sawShutdown; retries--) {
              await setTimeout(100);
              try {
                sawShutdown = await remoteApp.remoteEval('globalThis.sawShutdown === true');
              } catch {
                // The app quit instead of handling the event.
                break;
              }
            }
            expect(sawShutdown).toBe(true);
          } finally {
            dbusMockPowerMonitor.off('shutdown', preventDefault);
          }
        });

        it('should release the shutdown inhibitor when the quit is cancelled', async () => {
          const preventDefault = (event: Electron.Event) => event.preventDefault();
          // This process has a powerMonitor too, so it would quit as well.
          dbusMockPowerMonitor.on('shutdown', preventDefault);
          try {
            const callsBefore = (await getCalls()).length;
            const bootEval = 'require("electron").app.on("before-quit", (event) => event.preventDefault())';
            await startRemoteControlApp([`--boot-eval=${bootEval}`]);
            const waitForCalls = async (expected: number) => {
              let calls: any[] = [];
              for (let retries = 3; retries > 0; retries--) {
                calls = await getCalls();
                if (calls.length >= expected) break;
                await setTimeout(1000);
              }
              return calls;
            };
            expect(await waitForCalls(callsBefore + 1)).toHaveLength(callsBefore + 1);

            // The shutdown is announced and the app cancels the quit, which
            // drops its delay lock. A cancelled shutdown is a new cycle, so the
            // lock is taken again.
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', true]]);
            await setTimeout(500);
            emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown', 'b', [['b', false]]);
            expect(await waitForCalls(callsBefore + 2)).toHaveLength(callsBefore + 2);
          } finally {
            dbusMockPowerMonitor.off('shutdown', preventDefault);
          }
        });
      });

      describe('when the portal reports a session end', () => {
        let getPortalCalls: () => Promise<any[]>;
        let clearPortalCalls: () => Promise<void>;
        let emitInhibitState: (state: number) => Promise<void>;

        beforeAll(async () => {
          const sessionBus = dbus.sessionBus();
          const service = sessionBus.getService('org.freedesktop.portal.Desktop');
          const getInterface = promisify(service.getInterface.bind(service));
          const portalMock: any = await getInterface('/org/freedesktop/portal/desktop', 'org.freedesktop.DBus.Mock');
          getPortalCalls = promisify(portalMock.GetCalls.bind(portalMock));
          clearPortalCalls = promisify(portalMock.ClearCalls.bind(portalMock));
          emitInhibitState = promisify(portalMock.EmitInhibitState.bind(portalMock));
        });

        it('should quit when nothing delays the logout', async () => {
          const preventDefault = (event: Electron.Event) => event.preventDefault();
          // This process has a powerMonitor too, so it would quit as well.
          dbusMockPowerMonitor.on('shutdown', preventDefault);
          try {
            await clearPortalCalls();
            const remoteApp = await startRemoteControlApp();
            // Wait for the app's monitor session, so it will see the signal.
            let calls: any[] = [];
            for (let retries = 10; retries > 0; retries--) {
              calls = await getPortalCalls();
              if (calls.some((call) => call[1] === 'CreateMonitor')) break;
              await setTimeout(200);
            }
            expect(calls.some((call) => call[1] === 'CreateMonitor')).toBe(true);

            const exited = once(remoteApp.process, 'exit');
            await emitInhibitState(3);
            const [code] = await exited;
            expect(code).toBe(0);
          } finally {
            dbusMockPowerMonitor.off('shutdown', preventDefault);
          }
        });

        it('should emit "shutdown" and stay alive when the default is prevented', async () => {
          const preventDefault = (event: Electron.Event) => event.preventDefault();
          // This process has a powerMonitor too, so it would quit as well.
          dbusMockPowerMonitor.on('shutdown', preventDefault);
          try {
            await clearPortalCalls();
            const bootEval =
              'require("electron").powerMonitor.on("shutdown", (event) => { event.preventDefault(); globalThis.sawShutdown = true; })';
            const remoteApp = await startRemoteControlApp([`--boot-eval=${bootEval}`]);
            let calls: any[] = [];
            for (let retries = 10; retries > 0; retries--) {
              calls = await getPortalCalls();
              if (calls.some((call) => call[1] === 'CreateMonitor')) break;
              await setTimeout(200);
            }
            expect(calls.some((call) => call[1] === 'CreateMonitor')).toBe(true);

            await emitInhibitState(3);
            let sawShutdown = false;
            for (let retries = 20; retries > 0 && !sawShutdown; retries--) {
              await setTimeout(100);
              try {
                sawShutdown = await remoteApp.remoteEval('globalThis.sawShutdown === true');
              } catch {
                // The app quit instead of handling the event.
                break;
              }
            }
            expect(sawShutdown).toBe(true);
          } finally {
            dbusMockPowerMonitor.off('shutdown', preventDefault);
          }
        });
      });
    }
  );

  ifit(process.platform === 'darwin')(
    'should emit "shutdown" event on macOS when the workspace signals power off',
    async () => {
      const remoteApp = await startRemoteControlApp();
      const emitted = await remoteApp.remotely(async () => {
        const { app, powerMonitor, systemPreferences } = require('electron');
        const { once } = require('node:events');
        try {
          const shutdown = once(powerMonitor, 'shutdown', { signal: AbortSignal.timeout(5000) });
          systemPreferences.postWorkspaceNotification('NSWorkspaceWillPowerOffNotification', {});
          await shutdown;
          return true;
        } catch (err: any) {
          if (err.name === 'AbortError') {
            return false;
          } else {
            throw err;
          }
        } finally {
          setImmediate(() => app.quit());
        }
      });
      expect(emitted).toBe(true);

      const [code] = await once(remoteApp.process, 'exit');
      expect(code).toBe(0);
    }
  );

  it('is usable before app ready', async () => {
    const remoteApp = await startRemoteControlApp([
      '--boot-eval=globalThis.initialValue=require("electron").powerMonitor.getSystemIdleTime()'
    ]);
    expect(await remoteApp.remoteEval('globalThis.initialValue')).toBeTypeOf('number');
  });

  describe('when powerMonitor module is loaded', () => {
    let powerMonitor: typeof Electron.powerMonitor;
    beforeAll(() => {
      powerMonitor = require('electron').powerMonitor;
    });
    describe('powerMonitor.getSystemIdleState', () => {
      it('gets current system idle state', () => {
        // this function is not mocked out, so we can test the result's
        // form and type but not its value.
        const idleState = powerMonitor.getSystemIdleState(1);
        expect(idleState).toBeTypeOf('string');
        const validIdleStates = ['active', 'idle', 'locked', 'unknown'];
        expect(validIdleStates).toContain(idleState);
      });

      it('does not accept non positive integer threshold', () => {
        expect(() => {
          powerMonitor.getSystemIdleState(-1);
        }).toThrow(/must be greater than 0/);

        expect(() => {
          powerMonitor.getSystemIdleState(NaN);
        }).toThrow(/conversion failure/);

        expect(() => {
          powerMonitor.getSystemIdleState('a' as any);
        }).toThrow(/conversion failure/);
      });
    });

    describe('powerMonitor.getSystemIdleTime', () => {
      it('returns current system idle time', () => {
        const idleTime = powerMonitor.getSystemIdleTime();
        expect(idleTime).toBeGreaterThanOrEqual(0);
      });
    });

    describe('powerMonitor.getCurrentThermalState', () => {
      it('returns a valid state', () => {
        expect(['unknown', 'nominal', 'fair', 'serious', 'critical']).toContain(powerMonitor.getCurrentThermalState());
      });
    });

    describe('powerMonitor.onBatteryPower', () => {
      it('returns a boolean', () => {
        expect(powerMonitor.onBatteryPower).toBeTypeOf('boolean');
        expect(powerMonitor.isOnBatteryPower()).toBeTypeOf('boolean');
      });
    });
  });
});
