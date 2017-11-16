const {assert} = require('chai')
const {CallbacksRegistry} = require('electron')

describe('CallbacksRegistry module', () => {
  let registry = null

  beforeEach(() => {
    registry = new CallbacksRegistry()
  })

  it('adds a callback to the registry', () => {
    const cb = () => [1, 2, 3, 4, 5]
    const key = registry.add(cb)

    assert.exists(key)
  })

  it('returns a specified callback if it is in the registry', () => {
    const cb = () => [1, 2, 3, 4, 5]
    const key = registry.add(cb)
    const callback = registry.get(key)

    assert.equal(callback.toString(), cb.toString())
  })

  it('returns an empty function if the cb doesnt exist', () => {
    const callback = registry.get(1)

    assert.isFunction(callback)
  })

  it('removes a callback to the registry', () => {
    const cb = () => [1, 2, 3, 4, 5]
    const key = registry.add(cb)

    assert.exists(key)

    const beforeCB = registry.get(key)

    assert.equal(beforeCB.toString(), cb.toString())

    registry.remove(key)
    const afterCB = registry.get(key)

    assert.isFunction(afterCB)
    assert.notEqual(afterCB.toString(), cb.toString())
  })
})
