// For these tests we use a fake DBus daemon to verify powerMonitor module
// interaction with the system bus. This requires python-dbusmock installed and
// running (with the DBUS_SYSTEM_BUS_ADDRESS environment variable set).
// script/test.py will take care of spawning the fake DBus daemon and setting
// DBUS_SYSTEM_BUS_ADDRESS when python-dbusmock is installed.
//
// See https://pypi.python.org/pypi/python-dbusmock for more information about
// python-dbusmock.
const assert = require('assert')
const dbus = require('dbus-native')
const Promise = require('bluebird')

const skip = process.platform !== 'linux' || !process.env.DBUS_SYSTEM_BUS_ADDRESS

// TODO(alexeykuzmin): [Ch66] Crashes on Linux ia32. Fix it and enable back.
xdescribe('powerMonitor', () => {
  let logindMock, dbusMockPowerMonitor, getCalls, emitSignal, reset

  if (!skip) {
    before(async () => {
      const systemBus = dbus.systemBus()
      const loginService = systemBus.getService('org.freedesktop.login1')
      const getInterface = Promise.promisify(loginService.getInterface, {context: loginService})
      logindMock = await getInterface('/org/freedesktop/login1', 'org.freedesktop.DBus.Mock')
      getCalls = Promise.promisify(logindMock.GetCalls, {context: logindMock})
      emitSignal = Promise.promisify(logindMock.EmitSignal, {context: logindMock})
      reset = Promise.promisify(logindMock.Reset, {context: logindMock})
    })

    after(async () => {
      await reset()
    })
  }

  (skip ? describe.skip : describe)('when powerMonitor module is loaded with dbus mock', () => {
    function onceMethodCalled (done) {
      function cb () {
        logindMock.removeListener('MethodCalled', cb)
      }
      done()
      return cb
    }

    before((done) => {
      logindMock.on('MethodCalled', onceMethodCalled(done))
      // lazy load powerMonitor after we listen to MethodCalled mock signal
      dbusMockPowerMonitor = require('electron').remote.powerMonitor
    })

    it('should call Inhibit to delay suspend', async () => {
      const calls = await getCalls()
      assert.equal(calls.length, 1)
      assert.deepEqual(calls[0].slice(1), [
        'Inhibit', [
          [[{type: 's', child: []}], ['sleep']],
          [[{type: 's', child: []}], ['electron']],
          [[{type: 's', child: []}], ['Application cleanup before suspend']],
          [[{type: 's', child: []}], ['delay']]
        ]
      ])
    })

    describe('when PrepareForSleep(true) signal is sent by logind', () => {
      it('should emit "suspend" event', (done) => {
        dbusMockPowerMonitor.once('suspend', () => done())
        emitSignal('org.freedesktop.login1.Manager', 'PrepareForSleep',
          'b', [['b', true]])
      })

      describe('when PrepareForSleep(false) signal is sent by logind', () => {
        it('should emit "resume" event', (done) => {
          dbusMockPowerMonitor.once('resume', () => done())
          emitSignal('org.freedesktop.login1.Manager', 'PrepareForSleep',
            'b', [['b', false]])
        })

        it('should have called Inhibit again', async () => {
          const calls = await getCalls()
          assert.equal(calls.length, 2)
          assert.deepEqual(calls[1].slice(1), [
            'Inhibit', [
              [[{type: 's', child: []}], ['sleep']],
              [[{type: 's', child: []}], ['electron']],
              [[{type: 's', child: []}], ['Application cleanup before suspend']],
              [[{type: 's', child: []}], ['delay']]
            ]
          ])
        })
      })
    })

    describe('when a listener is added to shutdown event', () => {
      before(async () => {
        const calls = await getCalls()
        assert.equal(calls.length, 2)
        dbusMockPowerMonitor.once('shutdown', () => { })
      })

      it('should call Inhibit to delay shutdown', async () => {
        const calls = await getCalls()
        assert.equal(calls.length, 3)
        assert.deepEqual(calls[2].slice(1), [
          'Inhibit', [
            [[{type: 's', child: []}], ['shutdown']],
            [[{type: 's', child: []}], ['electron']],
            [[{type: 's', child: []}], ['Ensure a clean shutdown']],
            [[{type: 's', child: []}], ['delay']]
          ]
        ])
      })

      describe('when PrepareForShutdown(true) signal is sent by logind', () => {
        it('should emit "shutdown" event', (done) => {
          dbusMockPowerMonitor.once('shutdown', () => { done() })
          emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown',
            'b', [['b', true]])
        })
      })
    })
  })

  describe('when powerMonitor module is loaded', () => {
    let powerMonitor
    before(() => {
      powerMonitor = require('electron').remote.powerMonitor
    })

    describe('powerMonitor.querySystemIdleState', () => {
      it('notify current system idle state', (done) => {
        powerMonitor.querySystemIdleState(1, (idleState) => {
          assert.ok(idleState)
          done()
        })
      })

      it('does not accept non positive integer threshold', () => {
        assert.throws(() => {
          powerMonitor.querySystemIdleState(-1, (idleState) => {
          })
        })

        assert.throws(() => {
          powerMonitor.querySystemIdleState(NaN, (idleState) => {
          })
        })

        assert.throws(() => {
          powerMonitor.querySystemIdleState('a', (idleState) => {
          })
        })
      })
    })

    describe('powerMonitor.querySystemIdleTime', () => {
      it('notify current system idle time', (done) => {
        powerMonitor.querySystemIdleTime((idleTime) => {
          assert.ok(idleTime >= 0)
          done()
        })
      })
    })
  })
})
