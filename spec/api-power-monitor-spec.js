// For these tests we use a fake DBus daemon to verify powerMonitor module
// interaction with the system bus. This requires python-dbusmock installed and
// running (with the DBUS_SYSTEM_BUS_ADDRESS environment variable set).
// script/spec-runner.js will take care of spawning the fake DBus daemon and setting
// DBUS_SYSTEM_BUS_ADDRESS when python-dbusmock is installed.
//
// See https://pypi.python.org/pypi/python-dbusmock for more information about
// python-dbusmock.
const chai = require('chai')
const dirtyChai = require('dirty-chai')
const dbus = require('dbus-native')
const Promise = require('bluebird')

const { expect } = chai
chai.use(dirtyChai)

const skip = process.platform !== 'linux' || !process.env.DBUS_SYSTEM_BUS_ADDRESS

describe('powerMonitor', () => {
  let logindMock, dbusMockPowerMonitor, getCalls, emitSignal, reset

  if (!skip) {
    before(async () => {
      const systemBus = dbus.systemBus()
      const loginService = systemBus.getService('org.freedesktop.login1')
      const getInterface = Promise.promisify(loginService.getInterface, { context: loginService })
      logindMock = await getInterface('/org/freedesktop/login1', 'org.freedesktop.DBus.Mock')
      getCalls = Promise.promisify(logindMock.GetCalls, { context: logindMock })
      emitSignal = Promise.promisify(logindMock.EmitSignal, { context: logindMock })
      reset = Promise.promisify(logindMock.Reset, { context: logindMock })
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

    before(done => {
      logindMock.on('MethodCalled', onceMethodCalled(done))
      // lazy load powerMonitor after we listen to MethodCalled mock signal
      dbusMockPowerMonitor = require('electron').remote.powerMonitor
    })

    it('should call Inhibit to delay suspend', async () => {
      const calls = await getCalls()
      expect(calls).to.be.an('array').that.has.lengthOf(1)
      expect(calls[0].slice(1)).to.deep.equal([
        'Inhibit', [
          [[{ type: 's', child: [] }], ['sleep']],
          [[{ type: 's', child: [] }], ['electron']],
          [[{ type: 's', child: [] }], ['Application cleanup before suspend']],
          [[{ type: 's', child: [] }], ['delay']]
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
        it('should emit "resume" event', done => {
          dbusMockPowerMonitor.once('resume', () => done())
          emitSignal('org.freedesktop.login1.Manager', 'PrepareForSleep',
            'b', [['b', false]])
        })

        it('should have called Inhibit again', async () => {
          const calls = await getCalls()
          expect(calls).to.be.an('array').that.has.lengthOf(2)
          expect(calls[1].slice(1)).to.deep.equal([
            'Inhibit', [
              [[{ type: 's', child: [] }], ['sleep']],
              [[{ type: 's', child: [] }], ['electron']],
              [[{ type: 's', child: [] }], ['Application cleanup before suspend']],
              [[{ type: 's', child: [] }], ['delay']]
            ]
          ])
        })
      })
    })

    describe('when a listener is added to shutdown event', () => {
      before(async () => {
        const calls = await getCalls()
        expect(calls).to.be.an('array').that.has.lengthOf(2)
        dbusMockPowerMonitor.once('shutdown', () => { })
      })

      it('should call Inhibit to delay shutdown', async () => {
        const calls = await getCalls()
        expect(calls).to.be.an('array').that.has.lengthOf(3)
        expect(calls[2].slice(1)).to.deep.equal([
          'Inhibit', [
            [[{ type: 's', child: [] }], ['shutdown']],
            [[{ type: 's', child: [] }], ['electron']],
            [[{ type: 's', child: [] }], ['Ensure a clean shutdown']],
            [[{ type: 's', child: [] }], ['delay']]
          ]
        ])
      })

      describe('when PrepareForShutdown(true) signal is sent by logind', () => {
        it('should emit "shutdown" event', done => {
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
      it('notify current system idle state', done => {
        // this function is not mocked out, so we can test the result's
        // form and type but not its value.
        powerMonitor.querySystemIdleState(1, idleState => {
          expect(idleState).to.be.a('string')
          const validIdleStates = [ 'active', 'idle', 'locked', 'unknown' ]
          expect(validIdleStates).to.include(idleState)
          done()
        })
      })

      it('does not accept non positive integer threshold', () => {
        expect(() => {
          powerMonitor.querySystemIdleState(-1, (idleState) => {})
        }).to.throw()

        expect(() => {
          powerMonitor.querySystemIdleState(NaN, (idleState) => {})
        }).to.throw()

        expect(() => {
          powerMonitor.querySystemIdleState('a', (idleState) => {})
        }).to.throw()
      })
    })

    describe('powerMonitor.querySystemIdleTime', () => {
      it('notify current system idle time', done => {
        powerMonitor.querySystemIdleTime(idleTime => {
          expect(idleTime).to.be.at.least(0)
          done()
        })
      })
    })
  })
})
