'use strict'

const chai = require('chai')
const dirtyChai = require('dirty-chai')
const path = require('path')
const {closeWindow} = require('./window-helpers')

const {expect} = chai
chai.use(dirtyChai)

const {remote} = require('electron')
const {ipcMain, BrowserWindow} = remote

const createWindow = () => new BrowserWindow({
  show: false,
  webPreferences: {
    nodeIntegration: true
  }
})

describe('ipc main module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  let w = null

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('ipc.sendSync', () => {
    afterEach(() => { ipcMain.removeAllListeners('send-sync-message') })

    it('does not crash when reply is not sent and browser is destroyed', (done) => {
      w = createWindow()
      ipcMain.once('send-sync-message', (event) => {
        event.returnValue = null
        done()
      })
      w.loadURL(`file://${path.join(fixtures, 'api', 'send-sync-message.html')}`)
    })

    it('does not crash when reply is sent by multiple listeners', (done) => {
      w = createWindow()
      ipcMain.on('send-sync-message', (event) => {
        event.returnValue = null
      })
      ipcMain.on('send-sync-message', (event) => {
        event.returnValue = null
        done()
      })
      w.loadURL(`file://${path.join(fixtures, 'api', 'send-sync-message.html')}`)
    })
  })

  describe('remote listeners', () => {
    it('can be added and removed correctly', () => {
      w = createWindow()
      const listener = () => {}

      w.on('test', listener)
      expect(w.listenerCount('test')).to.equal(1)
      w.removeListener('test', listener)
      expect(w.listenerCount('test')).to.equal(0)
    })
  })

  it('throws an error when removing all the listeners', () => {
    ipcMain.on('test-event', () => {})
    expect(ipcMain.listenerCount('test-event')).to.equal(1)

    expect(() => {
      ipcMain.removeAllListeners()
    }).to.throw(/Removing all listeners from ipcMain will make Electron internals stop working/)

    ipcMain.removeAllListeners('test-event')
    expect(ipcMain.listenerCount('test-event')).to.equal(0)
  })

  describe('remote objects registry', () => {
    it('does not dereference until the render view is deleted (regression)', (done) => {
      w = createWindow()

      ipcMain.once('error-message', (event, message) => {
        const correctMsgStart = message.startsWith('Cannot call function \'getURL\' on missing remote object')
        expect(correctMsgStart).to.be.true()
        done()
      })

      w.loadURL(`file://${path.join(fixtures, 'api', 'render-view-deleted.html')}`)
    })
  })
})
