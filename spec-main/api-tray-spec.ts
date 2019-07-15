import { expect } from 'chai'
import { Menu, Tray, nativeImage } from 'electron'

describe('tray module', () => {
  let tray: Tray;

  beforeEach(() => {
    tray = new Tray(nativeImage.createEmpty())
  })

  afterEach(() => {
    tray = null as any
  })

  describe('tray.setContextMenu', () => {
    afterEach(() => {
      tray.destroy()
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
      expect(tray.isDestroyed()).to.be.false('tray should not be destroyed')
      tray.destroy()

      expect(tray.isDestroyed()).to.be.true('tray should be destroyed')
    })
  })

  describe('tray.popUpContextMenu', () => {
    afterEach(() => {
      tray.destroy()
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
    })
  })

  describe('tray.setPressedImage', () => {
    it('accepts empty image', () => {
      tray.setPressedImage(nativeImage.createEmpty())

      tray.destroy()
    })
  })

  describe('tray title get/set', () => {
    before(function () {
      if (process.platform !== 'darwin') this.skip()
    })

    afterEach(() => {
      tray.destroy()
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
