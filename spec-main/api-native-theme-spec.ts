import { expect } from 'chai'
import { nativeTheme, systemPreferences } from 'electron'
import * as os from 'os'
import * as semver from 'semver'

describe('nativeTheme module', () => {
  describe('nativeTheme.shouldUseDarkColors', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseDarkColors).to.be.a('boolean')
    })
  })

  describe('nativeTheme.themeSource', () => {
    afterEach(() => {
      nativeTheme.themeSource = 'system'
    })

    it('is system by default', () => {
      expect(nativeTheme.themeSource).to.equal('system')
    })

    it('should override the value of shouldUseDarkColors', () => {
      nativeTheme.themeSource = 'dark'
      expect(nativeTheme.shouldUseDarkColors).to.equal(true)
      nativeTheme.themeSource = 'light'
      expect(nativeTheme.shouldUseDarkColors).to.equal(false)
    })

    it('should emit the "updated" event when it is set and the resulting "shouldUseDarkColors" value changes', () => {
      nativeTheme.themeSource = 'dark'
      let called = false
      nativeTheme.once('updated', () => {
        called = true
      })
      nativeTheme.themeSource = 'light'
      expect(called).to.equal(true)
    })

    it('should not emit the "updated" event when it is set and the resulting "shouldUseDarkColors" value is the same', () => {
      nativeTheme.themeSource = 'dark'
      let called = false
      nativeTheme.once('updated', () => {
        called = true
      })
      nativeTheme.themeSource = 'dark'
      expect(called).to.equal(false)
    })

    describe('on macOS 10.14', () => {
      before(function () {
        if (process.platform !== 'darwin') return this.skip()
        // Darwin 18.0.0 === Mojave 10.14
        if (semver.lt(os.release(), '18.0.0')) return this.skip()
      })

      it('should update appLevelAppearance when set', () => {
        nativeTheme.themeSource = 'dark'
        expect(systemPreferences.appLevelAppearance).to.equal('dark')
        nativeTheme.themeSource = 'light'
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
