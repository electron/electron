import { BrowserWindow } from 'electron'

import { expect } from 'chai'
import * as path from 'path'
import { closeWindow } from './window-helpers'
import { emittedOnce } from './events-helpers'

describe('spellchecker', () => {
  let w: BrowserWindow

  beforeEach(async () => {
    w = new BrowserWindow({
      show: false
    })
    await w.loadFile(path.resolve(__dirname, './fixtures/chromium/spellchecker.html'))
  })

  afterEach(async () => {
    await closeWindow(w)
  })

  it('should detect correctly spelled words as correct', async () => {
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "Beautiful and lovely"')
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()')
    const contextMenuPromise = emittedOnce(w.webContents, 'context-menu')
    // Wait for spellchecker to load
    await new Promise(resolve => setTimeout(resolve, 500))
    w.webContents.sendInputEvent({
      type: 'mouseDown',
      button: 'right',
      x: 43,
      y: 42
    })
    const contextMenuParams: Electron.ContextMenuParams = (await contextMenuPromise)[1]
    expect(contextMenuParams.misspelledWord).to.eq('')
    expect(contextMenuParams.dictionarySuggestions).to.have.lengthOf(0)
  })

  it('should detect incorrectly spelled words as incorrect', async () => {
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "Beautifulllll asd asd"')
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()')
    const contextMenuPromise = emittedOnce(w.webContents, 'context-menu')
    // Wait for spellchecker to load
    await new Promise(resolve => setTimeout(resolve, 500))
    w.webContents.sendInputEvent({
      type: 'mouseDown',
      button: 'right',
      x: 43,
      y: 42
    })
    const contextMenuParams: Electron.ContextMenuParams = (await contextMenuPromise)[1]
    expect(contextMenuParams.misspelledWord).to.eq('Beautifulllll')
    expect(contextMenuParams.dictionarySuggestions).to.have.length.of.at.least(1)
  })
})
