const { expect } = require('chai')
const { remote } = require('electron')
const path = require('path')

const { emittedNTimes, emittedOnce } = require('./events-helpers')
const { closeWindow } = require('./window-helpers')

const { BrowserWindow } = remote

describe('renderer nodeIntegrationInSubFrames', () => {
  const generateTests = (sandboxEnabled) => {
    describe(`with sandbox ${sandboxEnabled ? 'enabled' : 'disabled'}`, () => {
      let w

      beforeEach(async () => {
        await closeWindow(w)
        w = new BrowserWindow({
          show: false,
          width: 400,
          height: 400,
          webPreferences: {
            sandbox: sandboxEnabled,
            preload: path.resolve(__dirname, 'fixtures/sub-frames/preload.js'),
            nodeIntegrationInSubFrames: true
          }
        })
      })

      afterEach(() => {
        return closeWindow(w).then(() => { w = null })
      })

      it('should load preload scripts in top level iframes', async () => {
        const detailsPromise = emittedNTimes(remote.ipcMain, 'preload-ran', 2)
        w.loadFile(path.resolve(__dirname, 'fixtures/sub-frames/frame-container.html'))
        const [event1, event2] = await detailsPromise
        expect(event1[0].frameId).to.not.equal(event2[0].frameId)
        expect(event1[0].frameId).to.equal(event1[2])
        expect(event2[0].frameId).to.equal(event2[2])
      })

      it('should load preload scripts in nested iframes', async () => {
        const detailsPromise = emittedNTimes(remote.ipcMain, 'preload-ran', 3)
        w.loadFile(path.resolve(__dirname, 'fixtures/sub-frames/frame-with-frame-container.html'))
        const [event1, event2, event3] = await detailsPromise
        expect(event1[0].frameId).to.not.equal(event2[0].frameId)
        expect(event1[0].frameId).to.not.equal(event3[0].frameId)
        expect(event2[0].frameId).to.not.equal(event3[0].frameId)
        expect(event1[0].frameId).to.equal(event1[2])
        expect(event2[0].frameId).to.equal(event2[2])
        expect(event3[0].frameId).to.equal(event3[2])
      })

      it('should correctly reply to the main frame with using event.reply', async () => {
        const detailsPromise = emittedNTimes(remote.ipcMain, 'preload-ran', 2)
        w.loadFile(path.resolve(__dirname, 'fixtures/sub-frames/frame-container.html'))
        const [event1] = await detailsPromise
        const pongPromise = emittedOnce(remote.ipcMain, 'preload-pong')
        event1[0].reply('preload-ping')
        const details = await pongPromise
        expect(details[1]).to.equal(event1[0].frameId)
      })

      it('should correctly reply to the sub-frames with using event.reply', async () => {
        const detailsPromise = emittedNTimes(remote.ipcMain, 'preload-ran', 2)
        w.loadFile(path.resolve(__dirname, 'fixtures/sub-frames/frame-container.html'))
        const [, event2] = await detailsPromise
        const pongPromise = emittedOnce(remote.ipcMain, 'preload-pong')
        event2[0].reply('preload-ping')
        const details = await pongPromise
        expect(details[1]).to.equal(event2[0].frameId)
      })

      it('should correctly reply to the nested sub-frames with using event.reply', async () => {
        const detailsPromise = emittedNTimes(remote.ipcMain, 'preload-ran', 3)
        w.loadFile(path.resolve(__dirname, 'fixtures/sub-frames/frame-with-frame-container.html'))
        const [,, event3] = await detailsPromise
        const pongPromise = emittedOnce(remote.ipcMain, 'preload-pong')
        event3[0].reply('preload-ping')
        const details = await pongPromise
        expect(details[1]).to.equal(event3[0].frameId)
      })
    })
  }

  generateTests(false)
  generateTests(true)
})
