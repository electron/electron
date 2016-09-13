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

    require('electron').deprecate.log('this is deprecated')

    assert.deepEqual(messages, ['this is deprecated'])
  })

  it('throws an exception if no deprecation handler is specified', function () {
    assert.throws(function () {
      require('electron').deprecate.log('this is deprecated')
    }, /this is deprecated/)
  })
})
