'use strict'

const assert = require('assert')
const {closeWindow} = require('./window-helpers')
const {webContents, TopLevelWindow, WebContentsView} = require('electron').remote

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
})
