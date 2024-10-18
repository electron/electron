// For these tests we use a fake DBus daemon to verify powerMonitor module
// interaction with the system bus. This requires python-dbusmock installed and
// running (with the DBUS_SYSTEM_BUS_ADDRESS environment variable set).
// script/spec-runner.js will take care of spawning the fake DBus daemon and setting
// DBUS_SYSTEM_BUS_ADDRESS when python-dbusmock is installed.
//
// See https://pypi.python.org/pypi/python-dbusmock for more information about
// python-dbusmock.
import { expect } from 'chai';
import * as dbus from 'dbus-native';

import { once } from 'node:events';
import { setTimeout } from 'node:timers/promises';
import { promisify } from 'node:util';

import { ifdescribe, startRemoteControlApp } from './lib/spec-helpers';

describe('powerMonitor', () => {
  let logindMock: any, dbusMockPowerMonitor: any, getCalls: any, emitSignal: any, reset: any;

  ifdescribe(process.platform === 'linux' && process.env.DBUS_SYSTEM_BUS_ADDRESS != null)('when powerMonitor module is loaded with dbus mock', () => {
    before(async () => {
      const systemBus = dbus.systemBus();
      const loginService = systemBus.getService('org.freedesktop.login1');
      const getInterface = promisify(loginService.getInterface.bind(loginService));
      logindMock = await getInterface('/org/freedesktop/login1', 'org.freedesktop.DBus.Mock');
      getCalls = promisify(logindMock.GetCalls.bind(logindMock));
      emitSignal = promisify(logindMock.EmitSignal.bind(logindMock));
      reset = promisify(logindMock.Reset.bind(logindMock));
    });

    after(async () => {
      await reset();
    });

    function onceMethodCalled (done: () => void) {
      function cb () {
        logindMock.removeListener('MethodCalled', cb);
      }
      done();
      return cb;
    }

    before(done => {
      logindMock.on('MethodCalled', onceMethodCalled(done));
      // lazy load powerMonitor after we listen to MethodCalled mock signal
      dbusMockPowerMonitor = require('electron').powerMonitor;
    });

    it('should call Inhibit to delay suspend once a listener is added', async () => {
      // No calls to dbus until a listener is added
      {
        const calls = await getCalls();
        expect(calls).to.be.an('array').that.has.lengthOf(0);
      }
      // Add a dummy listener to engage the monitors
      dbusMockPowerMonitor.on('dummy-event', () => {});
      try {
        let retriesRemaining = 3;
        // There doesn't seem to be a way to get a notification when a call
        // happens, so poll `getCalls` a few times to reduce flake.
        let calls: any[] = [];
        while (retriesRemaining-- > 0) {
          calls = await getCalls();
          if (calls.length > 0) break;
          await setTimeout(1000);
        }
        expect(calls).to.be.an('array').that.has.lengthOf(1);
        expect(calls[0].slice(1)).to.deep.equal([
          'Inhibit', [
            [[{ type: 's', child: [] }], ['sleep']],
            [[{ type: 's', child: [] }], ['electron']],
            [[{ type: 's', child: [] }], ['Application cleanup before suspend']],
            [[{ type: 's', child: [] }], ['delay']]
          ]
        ]);
      } finally {
        dbusMockPowerMonitor.removeAllListeners('dummy-event');
      }
    });

    describe('when PrepareForSleep(true) signal is sent by logind', () => {
      it('should emit "suspend" event', async () => {
        const suspend = once(dbusMockPowerMonitor, 'suspend');
        emitSignal('org.freedesktop.login1.Manager', 'PrepareForSleep',
          'b', [['b', true]]);
        await suspend;
      });

      describe('when PrepareForSleep(false) signal is sent by logind', () => {
        it('should emit "resume" event', async () => {
          const resume = once(dbusMockPowerMonitor, 'resume');
          emitSignal('org.freedesktop.login1.Manager', 'PrepareForSleep',
            'b', [['b', false]]);
          await resume;
        });

        it('should have called Inhibit again', async () => {
          const calls = await getCalls();
          expect(calls).to.be.an('array').that.has.lengthOf(2);
          expect(calls[1].slice(1)).to.deep.equal([
            'Inhibit', [
              [[{ type: 's', child: [] }], ['sleep']],
              [[{ type: 's', child: [] }], ['electron']],
              [[{ type: 's', child: [] }], ['Application cleanup before suspend']],
              [[{ type: 's', child: [] }], ['delay']]
            ]
          ]);
        });
      });
    });

    describe('when a listener is added to shutdown event', () => {
      before(async () => {
        const calls = await getCalls();
        expect(calls).to.be.an('array').that.has.lengthOf(2);
        dbusMockPowerMonitor.once('shutdown', () => { });
      });

      it('should call Inhibit to delay shutdown', async () => {
        const calls = await getCalls();
        expect(calls).to.be.an('array').that.has.lengthOf(3);
        expect(calls[2].slice(1)).to.deep.equal([
          'Inhibit', [
            [[{ type: 's', child: [] }], ['shutdown']],
            [[{ type: 's', child: [] }], ['electron']],
            [[{ type: 's', child: [] }], ['Ensure a clean shutdown']],
            [[{ type: 's', child: [] }], ['delay']]
          ]
        ]);
      });

      describe('when PrepareForShutdown(true) signal is sent by logind', () => {
        it('should emit "shutdown" event', done => {
          dbusMockPowerMonitor.once('shutdown', () => { done(); });
          emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown',
            'b', [['b', true]]);
        });
      });
    });
  });

  it('is usable before app ready', async () => {
    const remoteApp = await startRemoteControlApp(['--boot-eval=globalThis.initialValue=require("electron").powerMonitor.getSystemIdleTime()']);
    expect(await remoteApp.remoteEval('globalThis.initialValue')).to.be.a('number');
  });

  describe('when powerMonitor module is loaded', () => {
    let powerMonitor: typeof Electron.powerMonitor;
    before(() => {
      powerMonitor = require('electron').powerMonitor;
    });
    describe('powerMonitor.getSystemIdleState', () => {
      it('gets current system idle state', () => {
        // this function is not mocked out, so we can test the result's
        // form and type but not its value.
        const idleState = powerMonitor.getSystemIdleState(1);
        expect(idleState).to.be.a('string');
        const validIdleStates = ['active', 'idle', 'locked', 'unknown'];
        expect(validIdleStates).to.include(idleState);
      });

      it('does not accept non positive integer threshold', () => {
        expect(() => {
          powerMonitor.getSystemIdleState(-1);
        }).to.throw(/must be greater than 0/);

        expect(() => {
          powerMonitor.getSystemIdleState(NaN);
        }).to.throw(/conversion failure/);

        expect(() => {
          powerMonitor.getSystemIdleState('a' as any);
        }).to.throw(/conversion failure/);
      });
    });

    describe('powerMonitor.getSystemIdleTime', () => {
      it('returns current system idle time', () => {
        const idleTime = powerMonitor.getSystemIdleTime();
        expect(idleTime).to.be.at.least(0);
      });
    });

    describe('powerMonitor.getCurrentThermalState', () => {
      it('returns a valid state', () => {
        expect(powerMonitor.getCurrentThermalState()).to.be.oneOf(['unknown', 'nominal', 'fair', 'serious', 'critical']);
      });
    });

    describe('powerMonitor.onBatteryPower', () => {
      it('returns a boolean', () => {
        expect(powerMonitor.onBatteryPower).to.be.a('boolean');
        expect(powerMonitor.isOnBatteryPower()).to.be.a('boolean');
      });
    });
  });
});
