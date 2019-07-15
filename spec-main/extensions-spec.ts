//import { expect } from 'chai'
import { session, BrowserWindow } from 'electron'
import { closeAllWindows } from './window-helpers'
import * as path from 'path'

const fixtures = path.join(__dirname, 'fixtures')

describe('chrome extensions', () => {
  afterEach(closeAllWindows)
  it('loads an extension', async () => {
    (session.defaultSession as any).loadChromeExtension(path.join(fixtures, 'extensions', 'simple'))
    const w = new BrowserWindow({show: false})
    await w.loadURL(`file://${fixtures}/blank.html`)
  })
})
