'use strict'

const assert = require('assert')
const http = require('http')
const path = require('path')
const {closeWindow} = require('./window-helpers')

const {ipcRenderer, remote} = require('electron')
const {ipcMain, webContents, BrowserWindow} = remote

describe('ipc renderer module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  let w = null

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('ipc.sender.send', () => {
    it('should work when sending an object containing id property', (done) => {
      const obj = {
        id: 1,
        name: 'ly'
      }
      ipcRenderer.once('message', function (event, message) {
        assert.deepEqual(message, obj)
        done()
      })
      ipcRenderer.send('message', obj)
    })

    it('can send instances of Date', (done) => {
      const currentDate = new Date()
      ipcRenderer.once('message', function (event, value) {
        assert.equal(value, currentDate.toISOString())
        done()
      })
      ipcRenderer.send('message', currentDate)
    })

    it('can send instances of Buffer', (done) => {
      const buffer = Buffer.from('hello')
      ipcRenderer.once('message', function (event, message) {
        assert.ok(buffer.equals(message))
        done()
      })
      ipcRenderer.send('message', buffer)
    })

    it('can send objects with DOM class prototypes', (done) => {
      ipcRenderer.once('message', function (event, value) {
        assert.equal(value.protocol, 'file:')
        assert.equal(value.hostname, '')
        done()
      })
      ipcRenderer.send('message', document.location)
    })

    it('can send Electron API objects', (done) => {
      const webContents = remote.getCurrentWebContents()
      ipcRenderer.once('message', function (event, value) {
        assert.deepEqual(value.browserWindowOptions, webContents.browserWindowOptions)
        done()
      })
      ipcRenderer.send('message', webContents)
    })

    it('does not crash on external objects (regression)', (done) => {
      const request = http.request({port: 5000, hostname: '127.0.0.1', method: 'GET', path: '/'})
      const stream = request.agent.sockets['127.0.0.1:5000:'][0]._handle._externalStream
      request.on('error', () => {})
      ipcRenderer.once('message', function (event, requestValue, externalStreamValue) {
        assert.equal(requestValue.method, 'GET')
        assert.equal(requestValue.path, '/')
        assert.equal(externalStreamValue, null)
        done()
      })

      ipcRenderer.send('message', request, stream)
    })

    it('can send objects that both reference the same object', (done) => {
      const child = {hello: 'world'}
      const foo = {name: 'foo', child: child}
      const bar = {name: 'bar', child: child}
      const array = [foo, bar]

      ipcRenderer.once('message', function (event, arrayValue, fooValue, barValue, childValue) {
        assert.deepEqual(arrayValue, array)
        assert.deepEqual(fooValue, foo)
        assert.deepEqual(barValue, bar)
        assert.deepEqual(childValue, child)
        done()
      })
      ipcRenderer.send('message', array, foo, bar, child)
    })

    it('inserts null for cyclic references', (done) => {
      const array = [5]
      array.push(array)

      const child = {hello: 'world'}
      child.child = child

      ipcRenderer.once('message', function (event, arrayValue, childValue) {
        assert.equal(arrayValue[0], 5)
        assert.equal(arrayValue[1], null)

        assert.equal(childValue.hello, 'world')
        assert.equal(childValue.child, null)

        done()
      })
      ipcRenderer.send('message', array, child)
    })
  })

  describe('ipc.sendSync', () => {
    afterEach(() => {
      ipcMain.removeAllListeners('send-sync-message')
    })

    it('can be replied by setting event.returnValue', () => {
      const msg = ipcRenderer.sendSync('echo', 'test')
      assert.equal(msg, 'test')
    })
  })

  describe('ipcRenderer.sendTo', () => {
    let contents = null

    beforeEach(() => { contents = webContents.create({}) })

    afterEach(() => {
      ipcRenderer.removeAllListeners('pong')
      contents.destroy()
      contents = null
    })

    it('sends message to WebContents', (done) => {
      const webContentsId = remote.getCurrentWebContents().id

      ipcRenderer.once('pong', function (event, id) {
        assert.equal(webContentsId, id)
        done()
      })

      contents.once('did-finish-load', () => {
        ipcRenderer.sendTo(contents.id, 'ping', webContentsId)
      })

      contents.loadURL(`file://${path.join(fixtures, 'pages', 'ping-pong.html')}`)
    })
  })

  describe('remote listeners', () => {
    it('detaches listeners subscribed to destroyed renderers, and shows a warning', (done) => {
      w = new BrowserWindow({ show: false })

      w.webContents.once('did-finish-load', () => {
        w.webContents.once('did-finish-load', () => {
          const expectedMessage = [
            'Attempting to call a function in a renderer window that has been closed or released.',
            'Function provided here: remote-event-handler.html:11:33',
            'Remote event names: remote-handler, other-remote-handler'
          ].join('\n')
          const results = ipcRenderer.sendSync('try-emit-web-contents-event', w.webContents.id, 'remote-handler')
          assert.deepEqual(results, {
            warningMessage: expectedMessage,
            listenerCountBefore: 2,
            listenerCountAfter: 1
          })
          done()
        })
        w.webContents.reload()
      })
      w.loadURL(`file://${path.join(fixtures, 'api', 'remote-event-handler.html')}`)
    })
  })

  it('throws an error when removing all the listeners', () => {
    ipcRenderer.on('test-event', () => {})
    assert.equal(ipcRenderer.listenerCount('test-event'), 1)

    assert.throws(() => {
      ipcRenderer.removeAllListeners()
    }, /Removing all listeners from ipcRenderer will make Electron internals stop working/)

    ipcRenderer.removeAllListeners('test-event')
    assert.equal(ipcRenderer.listenerCount('test-event'), 0)
  })
})
