const { remote } = require('electron')
const { expect } = require('chai')
const { Menu, Tray, nativeImage } = remote

describe('tray module', () => {
  let tray

  beforeEach(() => {
    tray = new Tray(nativeImage.createEmpty())
  })

  afterEach(() => {
    tray.destroy()
    tray = null
  })

  describe('tray.setContextMenu', () => {
    it('accepts menu instance', () => {
      tray.setContextMenu(new Menu())
    })

    it('accepts null', () => {
      tray.setContextMenu(null)
    })
  })

  describe('tray.setImage', () => {
    it('accepts empty image', () => {
      tray.setImage(nativeImage.createEmpty())
    })
  })

  describe('tray.setPressedImage', () => {
    it('accepts empty image', () => {
      tray.setPressedImage(nativeImage.createEmpty())
    })
  })

  describe('tray title get/set', () => {
    it('sets/gets non-empty title', () => {
      const title = 'Hello World!'
      tray.setTitle(title)
      const newTitle = tray.getTitle()

      expect(newTitle).to.equal(title)
    })

    it('sets/gets empty title', () => {
      const title = ''
      tray.setTitle(title)
      const newTitle = tray.getTitle()

      expect(newTitle).to.equal(title)
    })
  })
})
