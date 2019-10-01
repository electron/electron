import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import { BrowserWindow, WebContents, session, ipcMain } from 'electron'
import { emittedOnce } from './events-helpers';
import { closeAllWindows } from './window-helpers';
import * as https from 'https';
import * as http from 'http';
import * as path from 'path';
import * as fs from 'fs';
import { EventEmitter } from 'events';

const { expect } = chai

chai.use(chaiAsPromised)
const fixturesPath = path.resolve(__dirname, '..', 'spec', 'fixtures')

describe('reporting api', () => {
  it('sends a report for a deprecation', async () => {
    const reports = new EventEmitter

    // The Reporting API only works on https with valid certs. To dodge having
    // to set up a trusted certificate, hack the validator.
    session.defaultSession.setCertificateVerifyProc((req, cb) => {
      cb(0)
    })
    const certPath = path.join(fixturesPath, 'certificates')
    const options = {
      key: fs.readFileSync(path.join(certPath, 'server.key')),
      cert: fs.readFileSync(path.join(certPath, 'server.pem')),
      ca: [
        fs.readFileSync(path.join(certPath, 'rootCA.pem')),
        fs.readFileSync(path.join(certPath, 'intermediateCA.pem'))
      ],
      requestCert: true,
      rejectUnauthorized: false
    }

    const server = https.createServer(options, (req, res) => {
      if (req.url === '/report') {
        let data = ''
        req.on('data', (d) => data += d.toString('utf-8'))
        req.on('end', () => {
          reports.emit('report', JSON.parse(data))
        })
      }
      res.setHeader('Report-To', JSON.stringify({
        group: 'default',
        max_age: 120,
        endpoints: [ {url: `https://localhost:${(server.address() as any).port}/report`} ],
      }))
      res.setHeader('Content-Type', 'text/html')
      // using the deprecated `webkitRequestAnimationFrame` will trigger a
      // "deprecation" report.
      res.end('<script>webkitRequestAnimationFrame(() => {})</script>')
    })
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
    const bw = new BrowserWindow({
      show: false,
    })
    try {
      const reportGenerated = emittedOnce(reports, 'report')
      const url = `https://localhost:${(server.address() as any).port}/a`
      await bw.loadURL(url)
      const [report] = await reportGenerated
      expect(report).to.be.an('array')
      expect(report[0].type).to.equal('deprecation')
      expect(report[0].url).to.equal(url)
      expect(report[0].body.id).to.equal('PrefixedRequestAnimationFrame')
    } finally {
      bw.destroy()
      server.close()
    }
  })
})

describe('window.postMessage', () => {
  afterEach(async () => {
    await closeAllWindows()
  })

  it('sets the source and origin correctly', async () => {
    const w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    w.loadURL(`file://${fixturesPath}/pages/window-open-postMessage-driver.html`)
    const [, message] = await emittedOnce(ipcMain, 'complete')
    expect(message.data).to.equal('testing')
    expect(message.origin).to.equal('file://')
    expect(message.sourceEqualsOpener).to.equal(true)
    expect(message.eventOrigin).to.equal('file://')
  })
})

describe('focus handling', () => {
  let webviewContents: WebContents = null as unknown as WebContents
  let w: BrowserWindow = null as unknown as BrowserWindow

  beforeEach(async () => {
    w = new BrowserWindow({
      show: true,
      webPreferences: {
        nodeIntegration: true,
        webviewTag: true
      }
    })

    const webviewReady = emittedOnce(w.webContents, 'did-attach-webview')
    await w.loadFile(path.join(fixturesPath, 'pages', 'tab-focus-loop-elements.html'))
    const [, wvContents] = await webviewReady
    webviewContents = wvContents
    await emittedOnce(webviewContents, 'did-finish-load')
    w.focus()
  })

  afterEach(() => {
    webviewContents = null as unknown as WebContents
    w.destroy()
    w = null as unknown as BrowserWindow
  })

  const expectFocusChange = async () => {
    const [, focusedElementId] = await emittedOnce(ipcMain, 'focus-changed')
    return focusedElementId
  }

  describe('a TAB press', () => {
    const tabPressEvent: any = {
      type: 'keyDown',
      keyCode: 'Tab'
    }

    it('moves focus to the next focusable item', async () => {
      let focusChange = expectFocusChange()
      w.webContents.sendInputEvent(tabPressEvent)
      let focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-1', `should start focused in element-1, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(tabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-2', `focus should've moved to element-2, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(tabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-wv-element-1', `focus should've moved to the webview's element-1, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      webviewContents.sendInputEvent(tabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-wv-element-2', `focus should've moved to the webview's element-2, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      webviewContents.sendInputEvent(tabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-3', `focus should've moved to element-3, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(tabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-1', `focus should've looped back to element-1, it's instead in ${focusedElementId}`)
    })
  })

  describe('a SHIFT + TAB press', () => {
    const shiftTabPressEvent: any = {
      type: 'keyDown',
      modifiers: ['Shift'],
      keyCode: 'Tab'
    }

    it('moves focus to the previous focusable item', async () => {
      let focusChange = expectFocusChange()
      w.webContents.sendInputEvent(shiftTabPressEvent)
      let focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-3', `should start focused in element-3, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(shiftTabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-wv-element-2', `focus should've moved to the webview's element-2, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      webviewContents.sendInputEvent(shiftTabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-wv-element-1', `focus should've moved to the webview's element-1, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      webviewContents.sendInputEvent(shiftTabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-2', `focus should've moved to element-2, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(shiftTabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-1', `focus should've moved to element-1, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(shiftTabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-3', `focus should've looped back to element-3, it's instead in ${focusedElementId}`)
    })
  })
})

describe('web security', () => {
  afterEach(closeAllWindows)
  let server: http.Server
  let serverUrl: string
  before(async () => {
    server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html')
      res.end('<body>')
    })
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve))
    serverUrl = `http://localhost:${(server.address() as any).port}`
  })
  after(() => {
    server.close()
  })

  it('engages CORB when web security is not disabled', async () => {
    const w = new BrowserWindow({ show: true, webPreferences: { webSecurity: true, nodeIntegration: true } })
    const p = emittedOnce(ipcMain, 'success')
    await w.loadURL(`data:text/html,<script>
        const s = document.createElement('script')
        s.src = "${serverUrl}"
        // The script will load successfully but its body will be emptied out
        // by CORB, so we don't expect a syntax error.
        s.onload = () => { require('electron').ipcRenderer.send('success') }
        document.documentElement.appendChild(s)
      </script>`)
    await p
  })

  it('bypasses CORB when web security is disabled', async () => {
    const w = new BrowserWindow({ show: true, webPreferences: { webSecurity: false, nodeIntegration: true } })
    const p = emittedOnce(ipcMain, 'success')
    await w.loadURL(`data:text/html,
      <script>
        window.onerror = (e) => { require('electron').ipcRenderer.send('success', e) }
      </script>
      <script src="${serverUrl}"></script>`)
    await p
  })
})
