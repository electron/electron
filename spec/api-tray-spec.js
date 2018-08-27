const {remote} = require('electron')
const {Menu, Tray, nativeImage} = remote

describe('tray module', () => {
  describe('tray.setContextMenu', () => {
    let tray

    beforeEach(() => {
      tray = new Tray(nativeImage.createEmpty())
    })

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
})
