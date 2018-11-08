'use strict'

const assert = require('assert')
const ChildProcess = require('child_process')
const path = require('path')
const {emittedOnce} = require('./events-helpers')
const {closeWindow} = require('./window-helpers')

const {remote} = require('electron')
const {webContents, TopLevelWindow, WebContentsView} = remote

describe('WebContentsView', () => {
  let w = null
  afterEach(() => closeWindow(w).then(() => { w = null }))

  it('can be used as content view', () => {
    const web = webContents.create({})
    w = new TopLevelWindow({show: false})
    w.setContentView(new WebContentsView(web))
  })

  it('prevents adding same WebContents', () => {
    const web = webContents.create({})
    w = new TopLevelWindow({show: false})
    w.setContentView(new WebContentsView(web))
    assert.throws(() => {
      w.setContentView(new WebContentsView(web))
    }, /The WebContents has already been added to a View/)
  })

  describe('new WebContentsView()', () => {
    it('does not crash on exit', async () => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'leak-exit-webcontentsview.js')
      const electronPath = remote.getGlobal('process').execPath
      const appProcess = ChildProcess.spawn(electronPath, [appPath])
      const [code] = await emittedOnce(appProcess, 'close')
      assert.equal(code, 0)
    })
  })
})
