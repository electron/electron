import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as path from 'path'
import { BrowserWindow, webContents } from 'electron'
import { emittedOnce } from './events-helpers';
import { closeWindow } from './window-helpers';

const { expect } = chai

chai.use(chaiAsPromised)

const fixturesPath = path.resolve(__dirname, '..', 'spec', 'fixtures')

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

  describe('will-prevent-unload event', () => {
    it('does not emit if beforeunload returns undefined', (done) => {
      w.once('closed', () => done())
      w.webContents.once('will-prevent-unload', (e) => {
        expect.fail('should not have fired')
      })
      w.loadFile(path.join(fixturesPath, 'api', 'close-beforeunload-undefined.html'))
    })

    it('emits if beforeunload returns false', (done) => {
      w.webContents.once('will-prevent-unload', () => done())
      w.loadFile(path.join(fixturesPath, 'api', 'close-beforeunload-false.html'))
    })

    it('supports calling preventDefault on will-prevent-unload events', (done) => {
      w.webContents.once('will-prevent-unload', event => event.preventDefault())
      w.once('closed', () => done())
      w.loadFile(path.join(fixturesPath, 'api', 'close-beforeunload-false.html'))
    })
  })
})
