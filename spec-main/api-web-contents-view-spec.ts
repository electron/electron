import { expect } from 'chai'
import * as ChildProcess from 'child_process'
import * as path from 'path'
import { emittedOnce } from './events-helpers'
import { closeWindow } from './window-helpers'

import { webContents, TopLevelWindow, WebContentsView } from 'electron'

describe('WebContentsView', () => {
  let w: TopLevelWindow
  afterEach(() => closeWindow(w as any).then(() => { w = null as unknown as TopLevelWindow }))

  it('can be used as content view', () => {
    const web = (webContents as any).create({})
    w = new TopLevelWindow({ show: false })
    w.setContentView(new WebContentsView(web))
  })

  it('prevents adding same WebContents', () => {
    const web = (webContents as any).create({})
    w = new TopLevelWindow({ show: false })
    w.setContentView(new WebContentsView(web))
    expect(() => {
      w.setContentView(new WebContentsView(web))
    }).to.throw('The WebContents has already been added to a View')
  })

  describe('new WebContentsView()', () => {
    it('does not crash on exit', async () => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'leak-exit-webcontentsview.js')
      const electronPath = process.execPath
      const appProcess = ChildProcess.spawn(electronPath, [appPath])
      const [code] = await emittedOnce(appProcess, 'exit')
      expect(code).to.equal(0)
    })
  })
})
