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
      const locale = systemPreferences.getUserDefault('AppleLocale', 'string')
      assert.equal(typeof locale, 'string')
      assert(locale.length > 0)

      const languages = systemPreferences.getUserDefault('AppleLanguages', 'array')
      assert(Array.isArray(languages))
      assert(languages.length > 0)
    })

    it('returns values for unknown user defaults', function () {
      assert.equal(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'boolean'), false)
      assert.equal(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'integer'), 0)
      assert.equal(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'float'), 0)
      assert.equal(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'double'), 0)
      assert.equal(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'string'), '')
      assert.equal(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'url'), '')
      assert.equal(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'badtype'), undefined)
      assert.deepEqual(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'array'), [])
      assert.deepEqual(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'dictionary'), {})
    })
  })

  describe('systemPreferences.setUserDefault(key, type, value)', () => {
    if (process.platform !== 'darwin') {
      return
    }

    const KEY = 'SystemPreferencesTest'

    const TEST_CASES = [
      ['string', 'abc'],
      ['boolean', true],
      ['float', 2.5],
      ['double', 10.1],
      ['integer', 11],
      ['url', 'https://github.com/electron'],
      ['array', [1, 2, 3]],
      ['dictionary', {'a': 1, 'b': 2}]
    ]

    it('sets values', () => {
      for (const [type, value] of TEST_CASES) {
        systemPreferences.setUserDefault(KEY, type, value)
        const retrievedValue = systemPreferences.getUserDefault(KEY, type)
        assert.deepEqual(retrievedValue, value)
      }
    })

    it('throws when type and value conflict', () => {
      for (const [type, value] of TEST_CASES) {
        assert.throws(() => {
          systemPreferences.setUserDefault(KEY, type, typeof value === 'string' ? {} : 'foo')
        }, `Unable to convert value to: ${type}`)
      }
    })

    it('throws when type is not valid', () => {
      assert.throws(() => {
        systemPreferences.setUserDefault(KEY, 'abc', 'foo')
      }, 'Invalid type: abc')
    })
  })

  describe('systemPreferences.isInvertedColorScheme()', function () {
    it('returns a boolean', function () {
      assert.equal(typeof systemPreferences.isInvertedColorScheme(), 'boolean')
    })
  })
})
