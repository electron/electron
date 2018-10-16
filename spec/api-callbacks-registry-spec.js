const chai = require('chai')
const dirtyChai = require('dirty-chai')

const { expect } = chai
chai.use(dirtyChai)

const CallbacksRegistry = require('../lib/renderer/callbacks-registry')

describe('CallbacksRegistry module', () => {
  let registry = null

  beforeEach(() => {
    registry = new CallbacksRegistry()
  })

  it('adds a callback to the registry', () => {
    const cb = () => [1, 2, 3, 4, 5]
    const key = registry.add(cb)

    expect(key).to.exist()
  })

  it('returns a specified callback if it is in the registry', () => {
    const cb = () => [1, 2, 3, 4, 5]
    const key = registry.add(cb)
    const callback = registry.get(key)

    expect(callback.toString()).equal(cb.toString())
  })

  it('returns an empty function if the cb doesnt exist', () => {
    const callback = registry.get(1)

    expect(callback).to.be.a('function')
  })

  it('removes a callback to the registry', () => {
    const cb = () => [1, 2, 3, 4, 5]
    const key = registry.add(cb)

    registry.remove(key)
    const afterCB = registry.get(key)

    expect(afterCB).to.be.a('function')
    expect(afterCB.toString()).to.not.equal(cb.toString())
  })
})
