// For these tests we use a fake DBus daemon to verify libnotify interaction
// with the session bus. This requires python-dbusmock to be installed and
// running at $DBUS_SESSION_BUS_ADDRESS.
//
// script/test.py spawns dbusmock, which sets DBUS_SESSION_BUS_ADDRESS.
//
// See https://pypi.python.org/pypi/python-dbusmock to read about dbusmock.

const assert = require('assert')
const dbus = require('dbus-native')
const Promise = require('bluebird')

const {remote} = require('electron')
const {app} = remote.require('electron')

const skip = process.platform !== 'linux' ||
             process.arch === 'ia32' ||
             !process.env.DBUS_SESSION_BUS_ADDRESS;

(skip ? describe.skip : describe)('Notification module (dbus)', () => {
  let mock, Notification, getCalls, reset
  const realAppName = app.getName()
  const realAppVersion = app.getVersion()
  const appName = 'api-notification-dbus-spec'
  const serviceName = 'org.freedesktop.Notifications'

  before(async () => {
    // init app
    app.setName(appName)
    app.setDesktopName(appName + '.desktop')
    // init dbus
    const path = '/org/freedesktop/Notifications'
    const iface = 'org.freedesktop.DBus.Mock'
    const bus = dbus.sessionBus()
    console.log('session bus: ' + process.env.DBUS_SESSION_BUS_ADDRESS)
    const service = bus.getService(serviceName)
    const getInterface = Promise.promisify(service.getInterface, {context: service})
    mock = await getInterface(path, iface)
    getCalls = Promise.promisify(mock.GetCalls, {context: mock})
    reset = Promise.promisify(mock.Reset, {context: mock})
  })

  after(async () => {
    // cleanup dbus
    await reset()
    // cleanup app
    app.setName(realAppName)
    app.setVersion(realAppVersion)
  })

  describe('Notification module using ' + serviceName, () => {
    function onMethodCalled (done) {
      function cb (name) {
        console.log('onMethodCalled: ' + name)
        if (name === 'Notify') {
          mock.removeListener('MethodCalled', cb)
          console.log('done')
          done()
        }
      }
      return cb
    }

    function unmarshalDBusNotifyHints (dbusHints) {
      let o = {}
      for (let hint of dbusHints) {
        let key = hint[0]
        let value = hint[1][1][0]
        o[key] = value
      }
      return o
    }

    function unmarshalDBusNotifyArgs (dbusArgs) {
      return {
        app_name: dbusArgs[0][1][0],
        replaces_id: dbusArgs[1][1][0],
        app_icon: dbusArgs[2][1][0],
        title: dbusArgs[3][1][0],
        body: dbusArgs[4][1][0],
        actions: dbusArgs[5][1][0],
        hints: unmarshalDBusNotifyHints(dbusArgs[6][1][0])
      }
    }

    before((done) => {
      mock.on('MethodCalled', onMethodCalled(done))
      // lazy load Notification after we listen to MethodCalled mock signal
      Notification = require('electron').remote.Notification
      const n = new Notification({
        title: 'title',
        subtitle: 'subtitle',
        body: 'body',
        replyPlaceholder: 'replyPlaceholder',
        sound: 'sound',
        closeButtonText: 'closeButtonText'
      })
      n.show()
    })

    it('should call ' + serviceName + ' to show notifications', async () => {
      const calls = await getCalls()
      assert(calls.length >= 1)
      let lastCall = calls[calls.length - 1]
      let methodName = lastCall[1]
      assert.equal(methodName, 'Notify')
      let args = unmarshalDBusNotifyArgs(lastCall[2])
      assert.deepEqual(args, {
        app_name: appName,
        replaces_id: 0,
        app_icon: '',
        title: 'title',
        body: 'body',
        actions: [],
        hints: {
          'append': 'true',
          'desktop-entry': appName
        }
      })
    })
  })
})
