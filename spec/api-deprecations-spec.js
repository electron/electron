const assert = require('assert')
const deprecations = require('electron').deprecations

describe('deprecations', function () {
  beforeEach(function () {
    deprecations.setHandler(null)
    process.throwDeprecation = true
  })

  it('allows a deprecation handler function to be specified', function () {
    var messages = []

    deprecations.setHandler(function (message) {
      messages.push(message)
    })

    require('electron').webFrame.registerUrlSchemeAsSecure('some-scheme')

    assert.deepEqual(messages, ['registerUrlSchemeAsSecure is deprecated. Use registerURLSchemeAsSecure instead.'])
  })

  it('throws an exception if no deprecation handler is specified', function () {
    assert.throws(function () {
      require('electron').webFrame.registerUrlSchemeAsPrivileged('some-scheme')
    }, 'registerUrlSchemeAsPrivileged is deprecated. Use registerURLSchemeAsPrivileged instead.')
  })
})
