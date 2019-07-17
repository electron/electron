import { expect } from 'chai'
import { session, BrowserWindow } from 'electron'
import { closeAllWindows } from './window-helpers'
import * as http from 'http'
import { AddressInfo } from 'net'
import * as path from 'path'
import { ifdescribe } from './spec-helpers'

const fixtures = path.join(__dirname, 'fixtures')

ifdescribe(process.electronBinding('features').isExtensionsEnabled())('chrome extensions', () => {
  // NB. extensions are only allowed on http://, https:// and ftp:// (!) urls by default.
  let server: http.Server
  let url: string
  before(async () => {
    server = http.createServer((req, res) => res.end())
    await new Promise(resolve => server.listen(0, '127.0.0.1', () => {
      url = `http://127.0.0.1:${(server.address() as AddressInfo).port}`
      resolve()
    }))
  })
  after(() => {
    server.close()
  })

  afterEach(closeAllWindows)
  it('loads an extension', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
    (customSession as any).loadChromeExtension(path.join(fixtures, 'extensions', 'red-bg'))
    const w = new BrowserWindow({show: false, webPreferences: {session: customSession}})
    await w.loadURL(url)
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
    expect(bg).to.equal('red')
  })

  it('confines an extension to the session it was loaded in', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
    (customSession as any).loadChromeExtension(path.join(fixtures, 'extensions', 'red-bg'))
    const w = new BrowserWindow({show: false}) // not in the session
    await w.loadURL(url)
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
    expect(bg).to.equal('')
  })
})
