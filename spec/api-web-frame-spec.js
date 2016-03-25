const assert = require('assert')
const path = require('path')
const webFrame = require('electron').webFrame

describe('webFrame module', function () {
  var fixtures = path.resolve(__dirname, 'fixtures')
  describe('webFrame.registerURLSchemeAsPrivileged', function () {
    it('supports fetch api', function (done) {
      webFrame.registerURLSchemeAsPrivileged('file')
      var url = 'file://' + fixtures + '/assets/logo.png'
      fetch(url).then(function (response) {
        assert(response.ok)
        done()
      }).catch(function (err) {
        done('unexpected error : ' + err)
      })
    })
  })
})
