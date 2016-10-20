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

  describe('systemPreferences.getColor(id)', function () {
    if (process.platform !== 'win32') {
      return
    }

    it('throws an error when the id is invalid', function () {
      assert.throws(function () {
        systemPreferences.getColor('not-a-color')
      }, /Unknown color: not-a-color/)
    })

    it('returns a hex RGB color string', function () {
      assert.equal(/^#[0-9A-F]{6}$/i.test(systemPreferences.getColor('window')), true)
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

  describe('systemPreferences.isInvertedColorScheme()', function () {
    it('returns a boolean', function () {
      assert.equal(typeof systemPreferences.isInvertedColorScheme(), 'boolean')
    })
  })
})
