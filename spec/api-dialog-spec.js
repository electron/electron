const { expect } = require('chai')
const { dialog } = require('electron').remote

describe('dialog module', () => {
  describe('showOpenDialog', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showOpenDialog({ properties: false })
      }).to.throw(/Properties must be an array/)

      expect(() => {
        dialog.showOpenDialog({ title: 300 })
      }).to.throw(/Title must be a string/)

      expect(() => {
        dialog.showOpenDialog({ buttonLabel: [] })
      }).to.throw(/Button label must be a string/)

      expect(() => {
        dialog.showOpenDialog({ defaultPath: {} })
      }).to.throw(/Default path must be a string/)

      expect(() => {
        dialog.showOpenDialog({ message: {} })
      }).to.throw(/Message must be a string/)
    })
  })

  describe('showSaveDialog', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showSaveDialog({ title: 300 })
      }).to.throw(/Title must be a string/)

      expect(() => {
        dialog.showSaveDialog({ buttonLabel: [] })
      }).to.throw(/Button label must be a string/)

      expect(() => {
        dialog.showSaveDialog({ defaultPath: {} })
      }).to.throw(/Default path must be a string/)

      expect(() => {
        dialog.showSaveDialog({ message: {} })
      }).to.throw(/Message must be a string/)

      expect(() => {
        dialog.showSaveDialog({ nameFieldLabel: {} })
      }).to.throw(/Name field label must be a string/)
    })
  })

  describe('showMessageBox', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showMessageBox(undefined, { type: 'not-a-valid-type' })
      }).to.throw(/Invalid message box type/)

      expect(() => {
        dialog.showMessageBox(null, { buttons: false })
      }).to.throw(/Buttons must be an array/)

      expect(() => {
        dialog.showMessageBox({ title: 300 })
      }).to.throw(/Title must be a string/)

      expect(() => {
        dialog.showMessageBox({ message: [] })
      }).to.throw(/Message must be a string/)

      expect(() => {
        dialog.showMessageBox({ detail: 3.14 })
      }).to.throw(/Detail must be a string/)

      expect(() => {
        dialog.showMessageBox({ checkboxLabel: false })
      }).to.throw(/checkboxLabel must be a string/)
    })
  })

  describe('showErrorBox', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showErrorBox()
      }).to.throw(/Insufficient number of arguments/)

      expect(() => {
        dialog.showErrorBox(3, 'four')
      }).to.throw(/Error processing argument at index 0/)

      expect(() => {
        dialog.showErrorBox('three', 4)
      }).to.throw(/Error processing argument at index 1/)
    })
  })

  describe('showCertificateTrustDialog', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showCertificateTrustDialog()
      }).to.throw(/options must be an object/)

      expect(() => {
        dialog.showCertificateTrustDialog({})
      }).to.throw(/certificate must be an object/)

      expect(() => {
        dialog.showCertificateTrustDialog({ certificate: {}, message: false })
      }).to.throw(/message must be a string/)
    })
  })
})
