const chai = require('chai')
const dirtyChai = require('dirty-chai')

const { remote } = require('electron')
const { systemPreferences } = remote

const { expect } = chai
chai.use(dirtyChai)

describe('systemPreferences module', () => {
  describe('systemPreferences.getAccentColor', () => {
    before(function () {
      if (process.platform !== 'win32') {
        this.skip()
      }
    })

    it('should return a non-empty string', () => {
      const accentColor = systemPreferences.getAccentColor()
      expect(accentColor).to.be.a('string').that.is.not.empty()
    })
  })

  describe('systemPreferences.getColor(id)', () => {
    before(function () {
      if (process.platform !== 'win32') {
        this.skip()
      }
    })

    it('throws an error when the id is invalid', () => {
      expect(() => {
        systemPreferences.getColor('not-a-color')
      }).to.throw('Unknown color: not-a-color')
    })

    it('returns a hex RGB color string', () => {
      expect(systemPreferences.getColor('window')).to.match(/^#[0-9A-F]{6}$/i)
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
        expect(actualValue).to.deep.equal(expectedValue)
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
        expect(() => {
          systemPreferences.registerDefaults(badDefault)
        }).to.throw('Invalid userDefault data provided')
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
      expect(locale).to.be.a('string').that.is.not.empty()

      const languages = systemPreferences.getUserDefault('AppleLanguages', 'array')
      expect(languages).to.be.an('array').that.is.not.empty()
    })

    it('returns values for unknown user defaults', () => {
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'boolean')).to.equal(false)
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'integer')).to.equal(0)
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'float')).to.equal(0)
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'double')).to.equal(0)
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'string')).to.equal('')
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'url')).to.equal('')
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'badtype')).to.be.undefined()
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'array')).to.deep.equal([])
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'dictionary')).to.deep.equal({})
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
      ['dictionary', { 'a': 1, 'b': 2 }]
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
        expect(retrievedValue).to.deep.equal(value)
      }
    })

    it('throws when type and value conflict', () => {
      for (const [type, value] of TEST_CASES) {
        expect(() => {
          systemPreferences.setUserDefault(KEY, type, typeof value === 'string' ? {} : 'foo')
        }).to.throw(`Unable to convert value to: ${type}`)
      }
    })

    it('throws when type is not valid', () => {
      expect(() => {
        systemPreferences.setUserDefault(KEY, 'abc', 'foo')
      }).to.throw('Invalid type: abc')
    })
  })

  describe('systemPreferences.appLevelAppearance', () => {
    before(function () {
      if (process.platform !== 'darwin') this.skip()
    })

    it('has an appLevelAppearance property', () => {
      expect(systemPreferences).to.have.a.property('appLevelAppearance')

      // TODO(codebytere): remove when propertyification is complete
      expect(systemPreferences.setAppLevelAppearance).to.be.a('function')
      expect(() => { systemPreferences.getAppLevelAppearance() }).to.not.throw()
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
      expect(systemPreferences.getUserDefault(KEY, 'string')).to.equal('')
    })

    it('does not throw for missing keys', () => {
      systemPreferences.removeUserDefault('some-missing-key')
    })
  })

  describe('systemPreferences.isInvertedColorScheme()', () => {
    it('returns a boolean', () => {
      expect(systemPreferences.isInvertedColorScheme()).to.be.a('boolean')
    })
  })

  describe('systemPreferences.getAnimationSettings()', () => {
    it('returns an object with all properties', () => {
      const settings = systemPreferences.getAnimationSettings()
      expect(settings).to.be.an('object')
      expect(settings).to.have.a.property('shouldRenderRichAnimation').that.is.a('boolean')
      expect(settings).to.have.a.property('scrollAnimationsEnabledBySystem').that.is.a('boolean')
      expect(settings).to.have.a.property('prefersReducedMotion').that.is.a('boolean')
    })
  })
})
