import { expect } from 'chai'
import * as path from 'path'
import { BrowserWindow, ipcMain } from 'electron'
import { closeAllWindows } from './window-helpers'

describe('asar package', () => {
  const fixtures = path.join(__dirname, '..', 'spec', 'fixtures')
  const asarDir = path.join(fixtures, 'test.asar')

  afterEach(closeAllWindows)

  describe('asar protocol', () => {
    it('sets __dirname correctly', function (done) {
      after(function () {
        ipcMain.removeAllListeners('dirname')
      })

      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          nodeIntegration: true
        }
      })
      const p = path.resolve(asarDir, 'web.asar', 'index.html')
      ipcMain.once('dirname', function (event, dirname) {
        expect(dirname).to.equal(path.dirname(p))
        done()
      })
      w.loadFile(p)
    })

    it('loads script tag in html', function (done) {
      after(function () {
        ipcMain.removeAllListeners('ping')
      })

      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          nodeIntegration: true
        }
      })
      const p = path.resolve(asarDir, 'script.asar', 'index.html')
      w.loadFile(p)
      ipcMain.once('ping', function (event, message) {
        expect(message).to.equal('pong')
        done()
      })
    })

    it('loads video tag in html', function (done) {
      this.timeout(60000)

      after(function () {
        ipcMain.removeAllListeners('asar-video')
      })

      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          nodeIntegration: true
        }
      })
      const p = path.resolve(asarDir, 'video.asar', 'index.html')
      w.loadFile(p)
      ipcMain.on('asar-video', function (event, message, error) {
        if (message === 'ended') {
          expect(error).to.be.null()
          done()
        } else if (message === 'error') {
          done(error)
        }
      })
    })
  })
})
