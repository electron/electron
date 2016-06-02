const assert = require('assert')
const {remote} = require('electron')
const {systemPreferences} = remote

describe('systemPreferences module', function () {
  if (process.platform !== 'darwin') {
    return
  }

  describe('systemPreferences.getUserDefault(key, type)', function () {
    it('returns values for known user defaults', function () {
      let locale = systemPreferences.getUserDefault('AppleLocale', 'string')
      assert.notEqual(locale, null)
      assert(locale.length > 0)

      let languages = systemPreferences.getUserDefault('AppleLanguages', 'array')
      assert.notEqual(languages, null)
      assert(languages.length > 0)
    })
  })

})
