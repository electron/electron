import { expect } from 'chai'
import { nativeTheme, systemPreferences } from 'electron'

describe('nativeTheme module', () => {
  describe('nativeTheme.shouldUseDarkColors', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseDarkColors).to.be.a('boolean')
    })
  })

  describe('nativeTheme.shouldUseDarkColorsOverride', () => {
    afterEach(() => {
      nativeTheme.shouldUseDarkColorsOverride = null
    })

    it('is null by default', () => {
      expect(nativeTheme.shouldUseDarkColorsOverride).to.equal(null)
    })

    it('should override the value of shouldUseDarkColors', () => {
      nativeTheme.shouldUseDarkColorsOverride = true
      expect(nativeTheme.shouldUseDarkColors).to.equal(true)
      nativeTheme.shouldUseDarkColorsOverride = false
      expect(nativeTheme.shouldUseDarkColors).to.equal(false)
    })

    it('should emit the "updated" event when it is set and the resulting "shouldUseDarkColors" value changes', () => {
      nativeTheme.shouldUseDarkColorsOverride = true
      let called = false
      nativeTheme.once('updated', () => {
        called = true
      })
      nativeTheme.shouldUseDarkColorsOverride = false
      expect(called).to.equal(true)
    })

    it('should not emit the "updated" event when it is set and the resulting "shouldUseDarkColors" value is the same', () => {
      nativeTheme.shouldUseDarkColorsOverride = true
      let called = false
      nativeTheme.once('updated', () => {
        called = true
      })
      nativeTheme.shouldUseDarkColorsOverride = true
      expect(called).to.equal(false)
    })

    describe('on macOS', () => {
      before(function () {
        if (process.platform !== 'darwin') this.skip()
      })

      it('should update appLevelAppearance when set', () => {
        nativeTheme.shouldUseDarkColorsOverride = true
        expect(systemPreferences.appLevelAppearance).to.equal('dark')
        nativeTheme.shouldUseDarkColorsOverride = false
        expect(systemPreferences.appLevelAppearance).to.equal('light')
      })
    })
  })

  describe('nativeTheme.shouldUseInvertedColorScheme', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseInvertedColorScheme).to.be.a('boolean')
    })
  })

  describe('nativeTheme.shouldUseHighContrastColors', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseHighContrastColors).to.be.a('boolean')
    })
  })
})
