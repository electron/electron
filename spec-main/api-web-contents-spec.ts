import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as path from 'path'
import { BrowserWindow, ipcMain, webContents } from 'electron'
import { emittedOnce } from './events-helpers';
import { closeWindow } from './window-helpers';

const { expect } = chai

chai.use(chaiAsPromised)

const fixturesPath = path.resolve(__dirname, '../spec/fixtures')

describe('webContents module', () => {
  let w: BrowserWindow = (null as unknown as BrowserWindow)

  beforeEach(() => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        backgroundThrottling: false,
        nodeIntegration: true,
        sandbox: false,
        contextIsolation: false,
        webviewTag: true
      }
    })
  })

  afterEach(async () => {
    await closeWindow(w)
    w = (null as unknown as BrowserWindow)
  })

  describe('getAllWebContents() API', () => {
    it('returns an array of web contents', async () => {
      w.loadFile(path.join(fixturesPath, 'pages', 'webview-zoom-factor.html'))

      await emittedOnce(w.webContents, 'did-attach-webview')

      w.webContents.openDevTools()

      await emittedOnce(w.webContents, 'devtools-opened')

      const all = webContents.getAllWebContents().sort((a, b) => {
        return a.id - b.id
      })

      expect(all).to.have.length(3)
      expect(all[0].getType()).to.equal('window')
      expect(all[all.length - 2].getType()).to.equal('webview')
      expect(all[all.length - 1].getType()).to.equal('remote')
    })
  })

  describe('webContents.send(channel, args...)', () => {
    it('does not block node async APIs when sent before document is ready', (done) => {
      // Please reference https://github.com/electron/electron/issues/19368 if
      // this test fails.
      ipcMain.once('async-node-api-done', () => {
        done()
      })
      w.loadFile(path.join(fixturesPath, 'pages', 'send-after-node.html'))
      setTimeout(() => {
        w.webContents.send("test")
      }, 50)
    })
  })
})
