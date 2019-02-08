'use strict'

const chai = require('chai')
const dirtyChai = require('dirty-chai')
const { deprecations, deprecate } = require('electron')

const { expect } = chai
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
    const o = { [newProp]: value }
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
    const o = { [prop]: 0 }

    deprecate.removeProperty(o, prop)

    const temp = o[prop]

    expect(temp).to.equal(0)
    expect(msg).to.be.a('string')
    expect(msg).to.include(prop)
  })

  it('warns exactly once when a function is deprecated with no replacement', () => {
    let msg
    deprecations.setHandler(m => { msg = m })

    function oldFn () { return 'hello' }
    const deprecatedFn = deprecate.function(oldFn)
    deprecatedFn()

    expect(msg).to.be.a('string')
    expect(msg).to.include('oldFn')
  })

  it('warns exactly once when a function is deprecated with a replacement', () => {
    let msg
    deprecations.setHandler(m => { msg = m })

    function oldFn () { return 'hello' }
    function newFn () { return 'goodbye' }
    const deprecatedFn = deprecate.function(oldFn, newFn)
    deprecatedFn()

    expect(msg).to.be.a('string')
    expect(msg).to.include('oldFn')
    expect(msg).to.include('newFn')
  })

  it('warns only once per item', () => {
    const messages = []
    deprecations.setHandler(message => messages.push(message))

    const key = 'foo'
    const val = 'bar'
    const o = { [key]: val }
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

    const o = { [oldProp]: 0 }
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

  describe('promisify', () => {
    const expected = 'Hello, world!'
    let promiseFunc
    let warnings

    const enableCallbackWarnings = () => {
      warnings = []
      deprecations.setHandler(warning => warnings.push(warning))
      process.enablePromiseAPIs = true
    }

    beforeEach(() => {
      deprecations.setHandler(null)
      process.throwDeprecation = true

      promiseFunc = param => new Promise((resolve, reject) => resolve(param))
    })

    it('acts as a pass-through for promise-based invocations', async () => {
      enableCallbackWarnings()
      promiseFunc = deprecate.promisify(promiseFunc)

      const actual = await promiseFunc(expected)
      expect(actual).to.equal(expected)
      expect(warnings).to.have.lengthOf(0)
    })

    it('only calls back an error if the callback is called with (err, data)', (done) => {
      enableCallbackWarnings()
      let erringPromiseFunc = () => new Promise((resolve, reject) => {
        reject(new Error('fail'))
      })
      erringPromiseFunc = deprecate.promisify(erringPromiseFunc)

      erringPromiseFunc((err, data) => {
        expect(data).to.be.an('undefined')
        expect(err).to.be.an.instanceOf(Error).with.property('message', 'fail')
        erringPromiseFunc(data => {
          expect(data).to.not.be.an.instanceOf(Error)
          expect(data).to.be.an('undefined')
          done()
        })
      })
    })

    it('warns exactly once for callback-based invocations', (done) => {
      enableCallbackWarnings()
      promiseFunc = deprecate.promisify(promiseFunc)

      let callbackCount = 0
      const invocationCount = 3
      const callback = (error, actual) => {
        expect(error).to.be.null()
        expect(actual).to.equal(expected)
        expect(warnings).to.have.lengthOf(1)
        expect(warnings[0]).to.include('promiseFunc')
        callbackCount += 1
        if (callbackCount === invocationCount) {
          done()
        }
      }

      for (let i = 0; i < invocationCount; i += 1) {
        promiseFunc(expected, callback)
      }
    })
  })
})
