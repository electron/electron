const {expect} = require('chai')
const {dialog} = require('electron').remote

describe.only('dialog module', () => {
  describe('showOpenDialog', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showOpenDialog({options: {properties: false}})
      }).to.throw(/Properties must be an array/)

      expect(() => {
        dialog.showOpenDialog({options: {title: 300}})
      }).to.throw(/title must be a string/)

      expect(() => {
        dialog.showOpenDialog({options: {buttonLabel: []}})
      }).to.throw(/buttonLabel must be a string/)

      expect(() => {
        dialog.showOpenDialog({options: {defaultPath: {}}})
      }).to.throw(/defaultPath must be a string/)

      expect(() => {
        dialog.showOpenDialog({options: {message: {}}})
      }).to.throw(/message must be a string/)
    })
  })

  describe('showSaveDialog', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showSaveDialog({options: {title: 300}})
      }).to.throw(/title must be a string/)

      expect(() => {
        dialog.showSaveDialog({options: {buttonLabel: []}})
      }).to.throw(/buttonLabel must be a string/)

      expect(() => {
        dialog.showSaveDialog({options: {defaultPath: {}}})
      }).to.throw(/defaultPath must be a string/)

      expect(() => {
        dialog.showSaveDialog({options: {message: {}}})
      }).to.throw(/message must be a string/)

      expect(() => {
        dialog.showSaveDialog({options: {nameFieldLabel: {}}})
      }).to.throw(/nameFieldLabel must be a string/)
    })
  })

  describe('showMessageBox', () => {
    it('throws errors when the options are invalid', () => {
      expect(() => {
        dialog.showMessageBox({window: undefined, options: {type: 'not-a-valid-type'}})
      }).to.throw(/Invalid message box type/)

      expect(() => {
        dialog.showMessageBox({window: null, options: {buttons: false}})
      }).to.throw(/buttons must be an array/)

      expect(() => {
        dialog.showMessageBox({options: {title: 300}})
      }).to.throw(/title must be a string/)

      expect(() => {
        dialog.showMessageBox({options: {message: []}})
      }).to.throw(/message must be a string/)

      expect(() => {
        dialog.showMessageBox({options: {detail: 3.14}})
      }).to.throw(/detail must be a string/)

      expect(() => {
        dialog.showMessageBox({options: {checkboxLabel: false}})
      }).to.throw(/checkboxLabe must be a string/)
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
        dialog.showCertificateTrustDialog({options: {}})
      }).to.throw(/certificate must be an object/)

      expect(() => {
        dialog.showCertificateTrustDialog({options: {certificate: {}, message: false}})
      }).to.throw(/message must be a string/)
    })
  })
})
