const { remote } = require('electron')
const { expect } = require('chai')
const { Menu, Tray, nativeImage } = remote

describe('tray module', () => {
  let tray

  beforeEach(() => {
    tray = new Tray(nativeImage.createEmpty())
  })

  describe('tray.setContextMenu', () => {
    afterEach(() => {
      tray.destroy()
      tray = null
    })

    it('accepts menu instance', () => {
      tray.setContextMenu(new Menu())
    })

    it('accepts null', () => {
      tray.setContextMenu(null)
    })
  })

  describe('tray.destroy()', () => {
    it('destroys a tray', () => {
      expect(tray.isDestroyed()).to.be.false()
      tray.destroy()

      expect(tray.isDestroyed()).to.be.true()
      tray = null
    })
  })

  describe('tray.popUpContextMenu', () => {
    afterEach(() => {
      tray.destroy()
      tray = null
    })

    before(function () {
      if (process.platform !== 'win32') this.skip()
    })

    it('can be called when menu is showing', (done) => {
      tray.setContextMenu(Menu.buildFromTemplate([{ label: 'Test' }]))
      setTimeout(() => {
        tray.popUpContextMenu()
        done()
      })
      tray.popUpContextMenu()
    })
  })

  describe('tray.setImage', () => {
    it('accepts empty image', () => {
      tray.setImage(nativeImage.createEmpty())

      tray.destroy()
      tray = null
    })
  })

  describe('tray.setPressedImage', () => {
    it('accepts empty image', () => {
      tray.setPressedImage(nativeImage.createEmpty())

      tray.destroy()
      tray = null
    })
  })

  describe('tray title get/set', () => {
    before(function () {
      if (process.platform !== 'darwin') this.skip()
    })

    afterEach(() => {
      tray.destroy()
      tray = null
    })

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
