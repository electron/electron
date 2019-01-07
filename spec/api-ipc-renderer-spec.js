'use strict'

const chai = require('chai')
const dirtyChai = require('dirty-chai')
const http = require('http')
const path = require('path')
const { closeWindow } = require('./window-helpers')

const { expect } = chai
chai.use(dirtyChai)

const { ipcRenderer, remote } = require('electron')
const { ipcMain, webContents, BrowserWindow } = remote

describe('ipc renderer module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  let w = null

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('ipc.sender.send', () => {
    it('should work when sending an object containing id property', done => {
      const obj = {
        id: 1,
        name: 'ly'
      }
      ipcRenderer.once('message', (event, message) => {
        expect(message).to.deep.equal(obj)
        done()
      })
      ipcRenderer.send('message', obj)
    })

    it('can send instances of Date', done => {
      const currentDate = new Date()
      ipcRenderer.once('message', (event, value) => {
        expect(value).to.equal(currentDate.toISOString())
        done()
      })
      ipcRenderer.send('message', currentDate)
    })

    it('can send instances of Buffer', done => {
      const buffer = Buffer.from('hello')
      ipcRenderer.once('message', (event, message) => {
        expect(buffer.equals(message)).to.be.true()
        done()
      })
      ipcRenderer.send('message', buffer)
    })

    it('can send objects with DOM class prototypes', done => {
      ipcRenderer.once('message', (event, value) => {
        expect(value.protocol).to.equal('file:')
        expect(value.hostname).to.equal('')
        done()
      })
      ipcRenderer.send('message', document.location)
    })

    it('can send Electron API objects', done => {
      const webContents = remote.getCurrentWebContents()
      ipcRenderer.once('message', (event, value) => {
        expect(value.browserWindowOptions).to.deep.equal(webContents.browserWindowOptions)
        done()
      })
      ipcRenderer.send('message', webContents)
    })

    it('does not crash on external objects (regression)', done => {
      const request = http.request({ port: 5000, hostname: '127.0.0.1', method: 'GET', path: '/' })
      const stream = request.agent.sockets['127.0.0.1:5000:'][0]._handle._externalStream
      request.on('error', () => {})
      ipcRenderer.once('message', (event, requestValue, externalStreamValue) => {
        expect(requestValue.method).to.equal('GET')
        expect(requestValue.path).to.equal('/')
        expect(externalStreamValue).to.be.null()
        done()
      })

      ipcRenderer.send('message', request, stream)
    })

    it('can send objects that both reference the same object', done => {
      const child = { hello: 'world' }
      const foo = { name: 'foo', child: child }
      const bar = { name: 'bar', child: child }
      const array = [foo, bar]

      ipcRenderer.once('message', (event, arrayValue, fooValue, barValue, childValue) => {
        expect(arrayValue).to.deep.equal(array)
        expect(fooValue).to.deep.equal(foo)
        expect(barValue).to.deep.equal(bar)
        expect(childValue).to.deep.equal(child)
        done()
      })
      ipcRenderer.send('message', array, foo, bar, child)
    })

    it('inserts null for cyclic references', done => {
      const array = [5]
      array.push(array)

      const child = { hello: 'world' }
      child.child = child

      ipcRenderer.once('message', (event, arrayValue, childValue) => {
        expect(arrayValue[0]).to.equal(5)
        expect(arrayValue[1]).to.be.null()

        expect(childValue.hello).to.equal('world')
        expect(childValue.child).to.be.null()

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
      expect(msg).to.equal('test')
    })
  })

  describe('ipcRenderer.sendTo', () => {
    let contents = null

    afterEach(() => {
      ipcRenderer.removeAllListeners('pong')
      contents.destroy()
      contents = null
    })

    it('sends message to WebContents', done => {
      contents = webContents.create({
        preload: path.join(fixtures, 'module', 'preload-inject-ipc.js')
      })

      const payload = 'Hello World!'

      ipcRenderer.once('pong', (event, data) => {
        expect(payload).to.equal(data)
        done()
      })

      contents.once('did-finish-load', () => {
        ipcRenderer.sendTo(contents.id, 'ping', payload)
      })

      contents.loadFile(path.join(fixtures, 'pages', 'ping-pong.html'))
    })

    it('sends message to WebContents (sanboxed renderer)', done => {
      contents = webContents.create({
        preload: path.join(fixtures, 'module', 'preload-inject-ipc.js'),
        sandbox: true
      })

      const payload = 'Hello World!'

      ipcRenderer.once('pong', (event, data) => {
        expect(payload).to.equal(data)
        done()
      })

      contents.once('did-finish-load', () => {
        ipcRenderer.sendTo(contents.id, 'ping', payload)
      })

      contents.loadFile(path.join(fixtures, 'pages', 'ping-pong.html'))
    })

    it('sends message to WebContents (channel has special chars)', done => {
      contents = webContents.create({
        preload: path.join(fixtures, 'module', 'preload-inject-ipc.js')
      })

      const payload = 'Hello World!'

      ipcRenderer.once('pong-æøåü', (event, data) => {
        expect(payload).to.equal(data)
        done()
      })

      contents.once('did-finish-load', () => {
        ipcRenderer.sendTo(contents.id, 'ping-æøåü', payload)
      })

      contents.loadFile(path.join(fixtures, 'pages', 'ping-pong.html'))
    })
  })

  describe('remote listeners', () => {
    it('detaches listeners subscribed to destroyed renderers, and shows a warning', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })

      w.webContents.once('did-finish-load', () => {
        w.webContents.once('did-finish-load', () => {
          const expectedMessage = [
            'Attempting to call a function in a renderer window that has been closed or released.',
            'Function provided here: remote-event-handler.html:11:33',
            'Remote event names: remote-handler, other-remote-handler'
          ].join('\n')

          const results = ipcRenderer.sendSync('try-emit-web-contents-event', w.webContents.id, 'remote-handler')

          expect(results).to.deep.equal({
            warningMessage: expectedMessage,
            listenerCountBefore: 2,
            listenerCountAfter: 1
          })
          done()
        })

        w.webContents.reload()
      })
      w.loadFile(path.join(fixtures, 'api', 'remote-event-handler.html'))
    })
  })

  describe('ipcRenderer.on', () => {
    it('is not used for internals', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })
      await w.loadURL('about:blank')

      const script = `require('electron').ipcRenderer.eventNames()`
      const result = await w.webContents.executeJavaScript(script)
      expect(result).to.deep.equal([])
    })
  })
})
