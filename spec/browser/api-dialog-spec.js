const assert = require('assert')
const {dialog} = require('electron').remote

describe('dialog module', () => {
  describe('showOpenDialog', () => {
    it('throws errors when the options are invalid', () => {
      assert.throws(() => {
        dialog.showOpenDialog({properties: false})
      }, /Properties must be an array/)

      assert.throws(() => {
        dialog.showOpenDialog({title: 300})
      }, /Title must be a string/)

      assert.throws(() => {
        dialog.showOpenDialog({buttonLabel: []})
      }, /Button label must be a string/)

      assert.throws(() => {
        dialog.showOpenDialog({defaultPath: {}})
      }, /Default path must be a string/)

      assert.throws(() => {
        dialog.showOpenDialog({message: {}})
      }, /Message must be a string/)
    })
  })

  describe('showSaveDialog', () => {
    it('throws errors when the options are invalid', () => {
      assert.throws(() => {
        dialog.showSaveDialog({title: 300})
      }, /Title must be a string/)

      assert.throws(() => {
        dialog.showSaveDialog({buttonLabel: []})
      }, /Button label must be a string/)

      assert.throws(() => {
        dialog.showSaveDialog({defaultPath: {}})
      }, /Default path must be a string/)

      assert.throws(() => {
        dialog.showSaveDialog({message: {}})
      }, /Message must be a string/)

      assert.throws(() => {
        dialog.showSaveDialog({nameFieldLabel: {}})
      }, /Name field label must be a string/)
    })
  })

  describe('showMessageBox', () => {
    it('throws errors when the options are invalid', () => {
      assert.throws(() => {
        dialog.showMessageBox(undefined, {type: 'not-a-valid-type'})
      }, /Invalid message box type/)

      assert.throws(() => {
        dialog.showMessageBox(null, {buttons: false})
      }, /Buttons must be an array/)

      assert.throws(() => {
        dialog.showMessageBox({title: 300})
      }, /Title must be a string/)

      assert.throws(() => {
        dialog.showMessageBox({message: []})
      }, /Message must be a string/)

      assert.throws(() => {
        dialog.showMessageBox({detail: 3.14})
      }, /Detail must be a string/)

      assert.throws(() => {
        dialog.showMessageBox({checkboxLabel: false})
      }, /checkboxLabel must be a string/)
    })
  })

  describe('showErrorBox', () => {
    it('throws errors when the options are invalid', () => {
      assert.throws(() => {
        dialog.showErrorBox()
      }, /Insufficient number of arguments/)

      assert.throws(() => {
        dialog.showErrorBox(3, 'four')
      }, /Error processing argument at index 0/)

      assert.throws(() => {
        dialog.showErrorBox('three', 4)
      }, /Error processing argument at index 1/)
    })
  })

  describe('showCertificateTrustDialog', () => {
    it('throws errors when the options are invalid', () => {
      assert.throws(() => {
        dialog.showCertificateTrustDialog()
      }, /options must be an object/)

      assert.throws(() => {
        dialog.showCertificateTrustDialog({})
      }, /certificate must be an object/)

      assert.throws(() => {
        dialog.showCertificateTrustDialog({certificate: {}, message: false})
      }, /message must be a string/)
    })
  })
})
