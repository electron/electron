const {remote} = require('electron')
const {Menu, Tray, nativeImage} = remote

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

  describe('tray.setTitle', () => {
    it('accepts non-empty string', () => {
      tray.setTitle('Hello World!')
    })

    it('accepts empty string', () => {
      tray.setTitle('')
    })
  })
})
