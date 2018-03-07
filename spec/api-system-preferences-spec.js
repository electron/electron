const assert = require('assert')
const {remote} = require('electron')
const {systemPreferences} = remote

describe('systemPreferences module', () => {
  describe('systemPreferences.getAccentColor', () => {
    before(function () {
      if (process.platform !== 'win32') {
        this.skip()
      }
    })

    it('should return a non-empty string', () => {
      let accentColor = systemPreferences.getAccentColor()
      assert.notEqual(accentColor, null)
      assert(accentColor.length > 0)
    })
  })

  describe('systemPreferences.getColor(id)', () => {
    before(function () {
      if (process.platform !== 'win32') {
        this.skip()
      }
    })

    it('throws an error when the id is invalid', () => {
      assert.throws(() => {
        systemPreferences.getColor('not-a-color')
      }, /Unknown color: not-a-color/)
    })

    it('returns a hex RGB color string', () => {
      assert.equal(/^#[0-9A-F]{6}$/i.test(systemPreferences.getColor('window')), true)
    })
  })

  describe('systemPreferences.registerDefaults(defaults)', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('registers defaults', () => {
      const defaultsMap = [
        { key: 'one', type: 'string', value: 'ONE' },
        { key: 'two', value: 2, type: 'integer' },
        { key: 'three', value: [1, 2, 3], type: 'array' }
      ]

      const defaultsDict = {}
      defaultsMap.forEach(row => { defaultsDict[row.key] = row.value })

      systemPreferences.registerDefaults(defaultsDict)

      for (const userDefault of defaultsMap) {
        const { key, value: expectedValue, type } = userDefault
        const actualValue = systemPreferences.getUserDefault(key, type)
        assert.deepEqual(actualValue, expectedValue)
      }
    })

    it('throws when bad defaults are passed', () => {
      const badDefaults = [
        1,
        null,
        new Date(),
        { 'one': null }
      ]

      for (const badDefault of badDefaults) {
        assert.throws(() => {
          systemPreferences.registerDefaults(badDefault)
        }, 'Invalid userDefault data provided')
      }
    })
  })

  describe('systemPreferences.getUserDefault(key, type)', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('returns values for known user defaults', () => {
      const locale = systemPreferences.getUserDefault('AppleLocale', 'string')
      assert.equal(typeof locale, 'string')
      assert(locale.length > 0)

      const languages = systemPreferences.getUserDefault('AppleLanguages', 'array')
      assert(Array.isArray(languages))
      assert(languages.length > 0)
    })

    it('returns values for unknown user defaults', () => {
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

    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

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

  describe('systemPreferences.setUserDefault(key, type, value)', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('removes keys', () => {
      const KEY = 'SystemPreferencesTest'
      systemPreferences.setUserDefault(KEY, 'string', 'foo')
      systemPreferences.removeUserDefault(KEY)
      assert.equal(systemPreferences.getUserDefault(KEY, 'string'), '')
    })

    it('does not throw for missing keys', () => {
      systemPreferences.removeUserDefault('some-missing-key')
    })
  })

  describe('systemPreferences.isInvertedColorScheme()', () => {
    it('returns a boolean', () => {
      assert.equal(typeof systemPreferences.isInvertedColorScheme(), 'boolean')
    })
  })
})
