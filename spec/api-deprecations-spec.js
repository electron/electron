'use strict'

const chai = require('chai')
const dirtyChai = require('dirty-chai')
const {deprecations, deprecate} = require('electron')

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

  it('renames a property', () => {
    let msg
    deprecations.setHandler(m => { msg = m })

    const oldProp = 'dingyOldName'
    const newProp = 'shinyNewName'

    let value = 0
    const o = {[newProp]: value}
    expect(o).to.not.have.a.property(oldProp)
    expect(o).to.have.a.property(newProp).that.is.a('number')

    deprecate.renameProperty(o, oldProp, newProp)
    o[oldProp] = ++value

    expect(msg).to.be.a('string')
    expect(msg).to.include(oldProp)
    expect(msg).to.include(newProp)

    expect(o).to.have.a.property(newProp).that.is.equal(value)
    expect(o).to.have.a.property(oldProp).that.is.equal(value)
  })

  it('doesn\'t deprecate a property not on an object', () => {
    const o = {}

    expect(() => {
      deprecate.removeProperty(o, 'iDoNotExist')
    }).to.throw(/iDoNotExist/)
  })

  it('deprecates a property of an object', () => {
    let msg
    deprecations.setHandler(m => { msg = m })

    const prop = 'itMustGo'
    let o = {[prop]: 0}

    deprecate.removeProperty(o, prop)

    const temp = o[prop]

    expect(temp).to.equal(0)
    expect(msg).to.be.a('string')
    expect(msg).to.include(prop)
  })

  it('warns only once per item', () => {
    const messages = []
    deprecations.setHandler(message => { messages.push(message) })

    const key = 'foo'
    const val = 'bar'
    let o = {[key]: val}
    deprecate.removeProperty(o, key)

    for (let i = 0; i < 3; ++i) {
      expect(o[key]).to.equal(val)
      expect(messages).to.have.length(1)
    }
  })

  it('warns if deprecated property is already set', () => {
    let msg
    deprecations.setHandler(m => { msg = m })

    const oldProp = 'dingyOldName'
    const newProp = 'shinyNewName'

    let o = {[oldProp]: 0}
    deprecate.renameProperty(o, oldProp, newProp)

    expect(msg).to.be.a('string')
    expect(msg).to.include(oldProp)
    expect(msg).to.include(newProp)
  })

  it('throws an exception if no deprecation handler is specified', () => {
    expect(() => {
      deprecate.log('this is deprecated')
    }).to.throw(/this is deprecated/)
  })
})
