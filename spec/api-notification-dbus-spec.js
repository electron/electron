// For these tests we use a fake DBus daemon to verify libnotify interaction
// with the session bus. This requires python-dbusmock to be installed and
// running at $DBUS_SESSION_BUS_ADDRESS.
//
// script/test.py spawns dbusmock, which sets DBUS_SESSION_BUS_ADDRESS.
//
// See https://pypi.python.org/pypi/python-dbusmock for more information about
// python-dbusmock.

const assert = require('assert')
const dbus = require('dbus-native')
const Promise = require('bluebird')

const {remote} = require('electron')
const {app} = remote.require('electron')

const skip = process.platform !== 'linux' || !process.env.DBUS_SESSION_BUS_ADDRESS;

(skip ? describe.skip : describe)('Notifications module - dbus', () => {
  let mock, Notification, getCalls, reset
  const appName = 'api-notification-dbus-spec'
  const serviceName = 'org.freedesktop.Notifications'

  before(async () => {
    // init app
    app.setName(appName)
    app.setDesktopName(appName+'.desktop')
    // init dbus mock
    const path = '/org/freedesktop/Notifications'
    const iface = 'org.freedesktop.DBus.Mock'
    const bus = dbus.sessionBus()
    const service = bus.getService(serviceName)
    const getInterface = Promise.promisify(service.getInterface, {context: service})
    mock = await getInterface(path, iface)
    getCalls = Promise.promisify(mock.GetCalls, {context: mock})
    reset = Promise.promisify(mock.Reset, {context: mock})
  })

  after(async () => {
    await reset()
  })

  describe('Notification module using '+serviceName, () => {
    function onMethodCalled (done) {
      function cb (name) {
        if (name === 'Notify') {
          mock.removeListener('MethodCalled', cb)
          done()
        }
      }
      return cb
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

    it('should call ' + serviceName + ' to display a notification', async () => {
      const calls = await getCalls()
      assert(calls.length >= 1)
      let call = calls[calls.length-1]
      let methodName = call[1]
      let args = call[2]
      assert.equal(methodName, 'Notify')
      assert.equal(args[0][1], appName) // app_name
      assert.equal(args[1][1], 0) // replaces_id
      assert.equal(args[2][1], '') // app_icon
      assert.equal(args[3][1], 'title') // summary
      assert.equal(args[4][1], 'body') // body
      assert.equal(args[5][1], '') // actions
      hints = args[6][1][0] // hints
      assert.equal(JSON.stringify(hints[0]),'["append",[[{"type":"s","child":[]}],["true"]]]')
      assert.equal(JSON.stringify(hints[1]),'["desktop-entry",[[{"type":"s","child":[]}],["'+appName+'"]]]')
    })
  })
})
