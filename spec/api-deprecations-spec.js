const chai = require('chai')
const dirtyChai = require('dirty-chai')
const {deprecations, deprecate, nativeImage} = require('electron')

const {expect} = chai
chai.use(dirtyChai)

describe('deprecations', () => {
  beforeEach(() => {
    deprecations.setHandler(null)
    process.throwDeprecation = true
  })

  it('allows a deprecation handler function to be specified', () => {
    const messages = []

    deprecations.setHandler(message => {
      messages.push(message)
    })

    deprecate.log('this is deprecated')
    expect(messages).to.deep.equal(['this is deprecated'])
  })

  it('returns a deprecation handler after one is set', () => {
    const messages = []

    deprecations.setHandler(message => {
      messages.push(message)
    })

    deprecate.log('this is deprecated')
    expect(deprecations.getHandler()).to.be.a('function')
  })

  it('returns a deprecation warning', () => {
    const messages = []

    deprecations.setHandler(message => {
      messages.push(message)
    })

    deprecate.warn('old', 'new')
    expect(messages).to.deep.equal([`'old' is deprecated. Use 'new' instead.`])
  })

  it('renames a method', () => {
    expect(nativeImage.createFromDataUrl).to.be.undefined()
    expect(nativeImage.createFromDataURL).to.be.a('function')

    deprecate.alias(nativeImage, 'createFromDataUrl', 'createFromDataURL')

    expect(nativeImage.createFromDataUrl).to.be.a('function')
  })

  it('renames a property', () => {
    let msg
    deprecations.setHandler((m) => { msg = m })

    const oldPropertyName = 'dingyOldName'
    const newPropertyName = 'shinyNewName'

    let value = 0
    let o = { [newPropertyName]: value }
    expect(o[oldPropertyName]).to.be.undefined()
    expect(o[newPropertyName]).to.be.a('number')

    deprecate.property(o, oldPropertyName, newPropertyName)
    expect(msg).to.not.be.a('string')
    o[oldPropertyName] = ++value

    expect(msg).to.be.a('string')
    expect(msg).to.include(oldPropertyName)
    expect(msg).to.include(newPropertyName)

    expect(o[newPropertyName]).to.equal(value)
    expect(o[oldPropertyName]).to.equal(value)
  })

  it('warns if deprecated property is already set', () => {
    let msg
    deprecations.setHandler((m) => { msg = m })

    const oldPropertyName = 'dingyOldName'
    const newPropertyName = 'shinyNewName'
    const value = 0

    let o = { [oldPropertyName]: value }
    deprecate.property(o, oldPropertyName, newPropertyName)

    expect(msg).to.be.a('string')
    expect(msg).to.include(oldPropertyName)
    expect(msg).to.include(newPropertyName)
  })

  it('throws an exception if no deprecation handler is specified', () => {
    expect(() => {
      deprecate.log('this is deprecated')
    }).to.throw(/this is deprecated/)
  })
})
