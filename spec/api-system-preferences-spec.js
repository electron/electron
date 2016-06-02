const assert = require('assert')
const {remote} = require('electron')
const {systemPreferences} = remote

describe('systemPreferences module', function () {
  if (process.platform !== 'darwin') {
    return
  }

  it('returns user defaults', function () {
    assert.notEqual(systemPreferences.getUserDefault('AppleInterfaceStyle', 'string'), null)
    assert.notEqual(systemPreferences.getUserDefault('AppleAquaColorVariant', 'integer'), null)
  })

  it('returns defaults under the global domain', function () {
    let locale = systemPreferences.getGlobalDefault('AppleLocale')
    assert.notEqual(locale, null)
    assert(locale.length > 0)

    let languages = systemPreferences.getGlobalDefault('AppleLanguages')
    assert.notEqual(languages, null)
    assert(languages.length > 0)
  })

})
