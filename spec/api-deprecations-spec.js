const assert = require('assert')
const {deprecations, deprecate} = require('electron')

describe('deprecations', () => {
  beforeEach(() => {
    deprecations.setHandler(null)
    process.throwDeprecation = true
  })

  it('allows a deprecation handler function to be specified', () => {
    const messages = []

    deprecations.setHandler((message) => {
      messages.push(message)
    })

    deprecate.log('this is deprecated')
    assert.deepEqual(messages, ['this is deprecated'])
  })

  it('throws an exception if no deprecation handler is specified', () => {
    assert.throws(() => {
      deprecate.log('this is deprecated')
    }, /this is deprecated/)
  })
})
