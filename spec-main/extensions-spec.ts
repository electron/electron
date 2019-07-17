import { expect } from 'chai'
import { session, BrowserWindow } from 'electron'
import { closeAllWindows } from './window-helpers'
import * as http from 'http'
import { AddressInfo } from 'net'
import * as path from 'path'

const fixtures = path.join(__dirname, 'fixtures')

describe('chrome extensions', () => {
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
    (session.defaultSession as any).loadChromeExtension(path.join(fixtures, 'extensions', 'simple'))
    const w = new BrowserWindow({show: false})
    await w.loadURL(url)
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
    expect(bg).to.equal('red')
  })
})
