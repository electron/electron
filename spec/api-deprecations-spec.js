const assert = require('assert')
const {deprecations, deprecate, nativeImage} = require('electron')

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

  it('returns a deprecation handler after one is set', () => {
    const messages = []

    deprecations.setHandler((message) => {
      messages.push(message)
    })

    deprecate.log('this is deprecated')
    assert(typeof deprecations.getHandler() === 'function')
  })

  it('returns a deprecation warning', () => {
    const messages = []

    deprecations.setHandler((message) => {
      messages.push(message)
    })

    deprecate.warn('old', 'new')
    assert.deepEqual(messages, [`'old' is deprecated. Use 'new' instead.`])
  })

  it('renames a method', () => {
    assert.equal(typeof nativeImage.createFromDataUrl, 'undefined')
    assert.equal(typeof nativeImage.createFromDataURL, 'function')

    deprecate.alias(nativeImage, 'createFromDataUrl', 'createFromDataURL')

    assert.equal(typeof nativeImage.createFromDataUrl, 'function')
  })

  it('renames a property', () => {
    let msg
    deprecations.setHandler((m) => { msg = m })

    const oldPropertyName = 'dingyOldName'
    const newPropertyName = 'shinyNewName'

    let value = 0
    let o = { [newPropertyName]: value }
    assert.strictEqual(typeof o[oldPropertyName], 'undefined')
    assert.strictEqual(typeof o[newPropertyName], 'number')

    deprecate.property(o, oldPropertyName, newPropertyName)
    assert.notEqual(typeof msg, 'string')
    o[oldPropertyName] = ++value

    assert.strictEqual(typeof msg, 'string')
    assert.ok(msg.includes(oldPropertyName))
    assert.ok(msg.includes(newPropertyName))

    assert.strictEqual(o[newPropertyName], value)
    assert.strictEqual(o[oldPropertyName], value)
  })

  it('warns if deprecated property is already set', () => {
    let msg
    deprecations.setHandler((m) => { msg = m })

    const oldPropertyName = 'dingyOldName'
    const newPropertyName = 'shinyNewName'
    const value = 0

    let o = { [oldPropertyName]: value }
    deprecate.property(o, oldPropertyName, newPropertyName)

    assert.strictEqual(typeof msg, 'string')
    assert.ok(msg.includes(oldPropertyName))
    assert.ok(msg.includes(newPropertyName))
  })

  it('throws an exception if no deprecation handler is specified', () => {
    assert.throws(() => {
      deprecate.log('this is deprecated')
    }, /this is deprecated/)
  })
})
