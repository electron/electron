'use strict'

const chai = require('chai')
const dirtyChai = require('dirty-chai')
const path = require('path')
const cp = require('child_process')
const { closeWindow } = require('./window-helpers')
const { emittedOnce } = require('./events-helpers')

const { expect } = chai
chai.use(dirtyChai)

const { remote } = require('electron')
const { ipcMain, BrowserWindow } = remote

describe('ipc main module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  let w = null

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('ipc.sendSync', () => {
    afterEach(() => { ipcMain.removeAllListeners('send-sync-message') })

    it('does not crash when reply is not sent and browser is destroyed', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })
      ipcMain.once('send-sync-message', (event) => {
        event.returnValue = null
        done()
      })
      w.loadFile(path.join(fixtures, 'api', 'send-sync-message.html'))
    })

    it('does not crash when reply is sent by multiple listeners', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })
      ipcMain.on('send-sync-message', (event) => {
        event.returnValue = null
      })
      ipcMain.on('send-sync-message', (event) => {
        event.returnValue = null
        done()
      })
      w.loadFile(path.join(fixtures, 'api', 'send-sync-message.html'))
    })
  })

  describe('remote listeners', () => {
    it('can be added and removed correctly', () => {
      w = new BrowserWindow({ show: false })
      const listener = () => {}

      w.on('test', listener)
      expect(w.listenerCount('test')).to.equal(1)
      w.removeListener('test', listener)
      expect(w.listenerCount('test')).to.equal(0)
    })
  })

  describe('remote objects registry', () => {
    it('does not dereference until the render view is deleted (regression)', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })

      ipcMain.once('error-message', (event, message) => {
        const correctMsgStart = message.startsWith('Cannot call function \'getURL\' on missing remote object')
        expect(correctMsgStart).to.be.true()
        done()
      })

      w.loadFile(path.join(fixtures, 'api', 'render-view-deleted.html'))
    })
  })

  describe('ipcMain.on', () => {
    it('is not used for internals', async () => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'ipc-main-listeners')
      const electronPath = remote.getGlobal('process').execPath
      const appProcess = cp.spawn(electronPath, [appPath])

      let output = ''
      appProcess.stdout.on('data', (data) => { output += data })

      await emittedOnce(appProcess.stdout, 'end')

      output = JSON.parse(output)
      expect(output).to.deep.equal(['error'])
    })
  })
})
