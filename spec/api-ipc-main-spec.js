'use strict'

const assert = require('assert')
const path = require('path')
const {closeWindow} = require('./window-helpers')

const {remote} = require('electron')
const {ipcMain, BrowserWindow} = remote

describe('ipc main module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  let w = null

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('ipc.sendSync', () => {
    afterEach(() => { ipcMain.removeAllListeners('send-sync-message') })

    it('does not crash when reply is not sent and browser is destroyed', (done) => {
      w = new BrowserWindow({ show: false })
      ipcMain.once('send-sync-message', (event) => {
        event.returnValue = null
        done()
      })
      w.loadURL(`file://${path.join(fixtures, 'api', 'send-sync-message.html')}`)
    })

    it('does not crash when reply is sent by multiple listeners', (done) => {
      w = new BrowserWindow({ show: false })
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
      w = new BrowserWindow({ show: false })
      const listener = () => {}

      w.on('test', listener)
      assert.equal(w.listenerCount('test'), 1)
      w.removeListener('test', listener)
      assert.equal(w.listenerCount('test'), 0)
    })
  })

  it('throws an error when removing all the listeners', () => {
    ipcMain.on('test-event', () => {})
    assert.equal(ipcMain.listenerCount('test-event'), 1)

    assert.throws(() => {
      ipcMain.removeAllListeners()
    }, /Removing all listeners from ipcMain will make Electron internals stop working/)

    ipcMain.removeAllListeners('test-event')
    assert.equal(ipcMain.listenerCount('test-event'), 0)
  })

  describe('remote objects registry', () => {
    it('does not dereference until the render view is deleted (regression)', (done) => {
      w = new BrowserWindow({ show: false })

      ipcMain.once('error-message', (event, message) => {
        assert(message.startsWith('Cannot call function \'getURL\' on missing remote object'), message)
        done()
      })

      w.loadURL(`file://${path.join(fixtures, 'api', 'render-view-deleted.html')}`)
    })
  })
})
