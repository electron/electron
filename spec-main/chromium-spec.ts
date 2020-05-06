import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import { BrowserWindow, WebContents, session, ipcMain } from 'electron'
import { emittedOnce } from './events-helpers'
import { closeAllWindows } from './window-helpers'
import * as https from 'https'
import * as http from 'http'
import * as path from 'path'
import * as fs from 'fs'
import { EventEmitter } from 'events'
import { promisify } from 'util'

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

  describe('window.open', () => {
    it('denies custom open when nativeWindowOpen: true', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: false,
          nodeIntegration: true,
          nativeWindowOpen: true
        }
      });
      w.loadURL('about:blank');

      const previousListeners = process.listeners('uncaughtException');
      process.removeAllListeners('uncaughtException');
      try {
        const uncaughtException = new Promise<Error>(resolve => {
          process.once('uncaughtException', resolve);
        });
        expect(await w.webContents.executeJavaScript(`(${function () {
          const ipc = process.electronBinding('ipc').ipc;
          return ipc.sendSync(true, 'ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', ['', '', ''])[0];
        }})()`)).to.be.null('null');
        const exception = await uncaughtException;
        expect(exception.message).to.match(/denied: expected native window\.open/);
      } finally {
        previousListeners.forEach(l => process.on('uncaughtException', l));
      }
    });
  });
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

describe('iframe using HTML fullscreen API while window is OS-fullscreened', () => {
  const fullscreenChildHtml = promisify(fs.readFile)(
    path.join(fixturesPath, 'pages', 'fullscreen-oopif.html')
  )
  let w: BrowserWindow, server: http.Server

  before(() => {
    server = http.createServer(async (_req, res) => {
      res.writeHead(200, { 'Content-Type': 'text/html' })
      res.write(await fullscreenChildHtml)
      res.end()
    })

    server.listen(8989, '127.0.0.1')
  })

  beforeEach(() => {
    w = new BrowserWindow({
      show: true,
      fullscreen: true,
      webPreferences: {
        nodeIntegration: true,
        nodeIntegrationInSubFrames: true
      }
    })
  })

  afterEach(async () => {
    await closeAllWindows()
    ;(w as any) = null
    server.close()
  })

  it('can fullscreen from out-of-process iframes (OOPIFs)', done => {
    ipcMain.once('fullscreenChange', async () => {
      const fullscreenWidth = await w.webContents.executeJavaScript(
        "document.querySelector('iframe').offsetWidth"
      )
      expect(fullscreenWidth > 0).to.be.true

      await w.webContents.executeJavaScript(
        "document.querySelector('iframe').contentWindow.postMessage('exitFullscreen', '*')"
      )

      await new Promise(resolve => setTimeout(resolve, 500))

      const width = await w.webContents.executeJavaScript(
        "document.querySelector('iframe').offsetWidth"
      )
      expect(width).to.equal(0)

      done()
    })

    const html =
      '<iframe style="width: 0" frameborder=0 src="http://localhost:8989" allowfullscreen></iframe>'
    w.loadURL(`data:text/html,${html}`)
  })

  it('can fullscreen from in-process iframes', done => {
    ipcMain.once('fullscreenChange', async () => {
      const fullscreenWidth = await w.webContents.executeJavaScript(
        "document.querySelector('iframe').offsetWidth"
      )
      expect(fullscreenWidth > 0).to.true

      await w.webContents.executeJavaScript('document.exitFullscreen()')
      const width = await w.webContents.executeJavaScript(
        "document.querySelector('iframe').offsetWidth"
      )
      expect(width).to.equal(0)
      done()
    })

    w.loadFile(path.join(fixturesPath, 'pages', 'fullscreen-ipif.html'))
  })
})

describe('enableWebSQL webpreference', () => {
  const standardScheme = (global as any).standardScheme;
  const origin = `${standardScheme}://fake-host`;
  const filePath = path.join(fixturesPath, 'pages', 'storage', 'web_sql.html');
  const sqlPartition = 'web-sql-preference-test';
  const sqlSession = session.fromPartition(sqlPartition);
  const securityError = 'An attempt was made to break through the security policy of the user agent.';
  let contents: WebContents, w: BrowserWindow;

  before(() => {
    sqlSession.protocol.registerFileProtocol(standardScheme, (request, callback) => {
      callback({ path: filePath });
    });
  });

  after(() => {
    sqlSession.protocol.unregisterProtocol(standardScheme);
  });

  afterEach(async () => {
    if (contents) {
      (contents as any).destroy();
      contents = null as any;
    }
    await closeAllWindows();
    (w as any) = null;
  });

  it('default value allows websql', async () => {
    contents = (webContents as any).create({
      session: sqlSession,
      nodeIntegration: true
    });
    contents.loadURL(origin);
    const [, error] = await emittedOnce(ipcMain, 'web-sql-response');
    expect(error).to.be.null();
  });

  it('when set to false can disallow websql', async () => {
    contents = (webContents as any).create({
      session: sqlSession,
      nodeIntegration: true,
      enableWebSQL: false
    });
    contents.loadURL(origin);
    const [, error] = await emittedOnce(ipcMain, 'web-sql-response');
    expect(error).to.equal(securityError);
  });

  it('when set to false does not disable indexedDB', async () => {
    contents = (webContents as any).create({
      session: sqlSession,
      nodeIntegration: true,
      enableWebSQL: false
    });
    contents.loadURL(origin);
    const [, error] = await emittedOnce(ipcMain, 'web-sql-response');
    expect(error).to.equal(securityError);
    const dbName = 'random';
    const result = await contents.executeJavaScript(`
      new Promise((resolve, reject) => {
        try {
          let req = window.indexedDB.open('${dbName}');
          req.onsuccess = (event) => { 
            let db = req.result;
            resolve(db.name);
          }
          req.onerror = (event) => { resolve(event.target.code); }
        } catch (e) {
          resolve(e.message);
        }
      });
    `);
    expect(result).to.equal(dbName);
  });

  it('child webContents can override when the embedder has allowed websql', async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        webviewTag: true,
        session: sqlSession
      }
    });
    w.webContents.loadURL(origin);
    const [, error] = await emittedOnce(ipcMain, 'web-sql-response');
    expect(error).to.be.null();
    const webviewResult = emittedOnce(ipcMain, 'web-sql-response');
    await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => {
        const webview = new WebView();
        webview.setAttribute('src', '${origin}');
        webview.setAttribute('webpreferences', 'enableWebSQL=0');
        webview.setAttribute('partition', '${sqlPartition}');
        webview.setAttribute('nodeIntegration', 'on');
        document.body.appendChild(webview);
        webview.addEventListener('dom-ready', () => resolve());
      });
    `);
    const [, childError] = await webviewResult;
    expect(childError).to.equal(securityError);
  });

  it('child webContents cannot override when the embedder has disallowed websql', async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        enableWebSQL: false,
        webviewTag: true,
        session: sqlSession
      }
    });
    w.webContents.loadURL('data:text/html,<html></html>');
    const webviewResult = emittedOnce(ipcMain, 'web-sql-response');
    await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => {
        const webview = new WebView();
        webview.setAttribute('src', '${origin}');
        webview.setAttribute('webpreferences', 'enableWebSQL=1');
        webview.setAttribute('partition', '${sqlPartition}');
        webview.setAttribute('nodeIntegration', 'on');
        document.body.appendChild(webview);
        webview.addEventListener('dom-ready', () => resolve());
      });
    `);
    const [, childError] = await webviewResult;
    expect(childError).to.equal(securityError);
  });

  it('child webContents can use websql when the embedder has allowed websql', async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        webviewTag: true,
        session: sqlSession
      }
    });
    w.webContents.loadURL(origin);
    const [, error] = await emittedOnce(ipcMain, 'web-sql-response');
    expect(error).to.be.null();
    const webviewResult = emittedOnce(ipcMain, 'web-sql-response');
    await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => {
        const webview = new WebView();
        webview.setAttribute('src', '${origin}');
        webview.setAttribute('webpreferences', 'enableWebSQL=1');
        webview.setAttribute('partition', '${sqlPartition}');
        webview.setAttribute('nodeIntegration', 'on');
        document.body.appendChild(webview);
        webview.addEventListener('dom-ready', () => resolve());
      });
    `);
    const [, childError] = await webviewResult;
    expect(childError).to.be.null();
  });
});
