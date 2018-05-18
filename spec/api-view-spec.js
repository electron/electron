'use strict'

const {closeWindow} = require('./window-helpers')
const {TopLevelWindow, View} = require('electron').remote

describe('View', () => {
  let w = null
  afterEach(() => closeWindow(w).then(() => { w = null }))

  it('can be used as content view', () => {
    w = new TopLevelWindow({show: false})
    w.setContentView(new View())
  })
})
