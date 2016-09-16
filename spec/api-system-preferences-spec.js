const assert = require('assert')
const {remote} = require('electron')
const {systemPreferences} = remote

describe('systemPreferences module', function () {
  describe('systemPreferences.getAccentColor', function () {
    if (process.platform !== 'win32') {
      return
    }

    it('should return a non-empty string', function () {
      let accentColor = systemPreferences.getAccentColor()
      assert.notEqual(accentColor, null)
      assert(accentColor.length > 0)
    })
  })

  describe('systemPreferences.getUserDefault(key, type)', function () {
    if (process.platform !== 'darwin') {
      return
    }

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
