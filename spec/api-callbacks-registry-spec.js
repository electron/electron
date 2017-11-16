const assert = require('assert')
const {CallbacksRegistry} = require('electron')

describe('CallbacksRegistry module', () => {
  let registry = null

  beforeEach(() => {
    registry = new CallbacksRegistry()
  })

  it('adds a callback to the registry', () => {
    const cb = () => [1, 2, 3, 4, 5]
    const id = registry.add(cb)
    assert.equal(id, 1)
  })

  it('returns a specified callback if it is in the registry', () => {
    const cb = () => [1, 2, 3, 4, 5]
    registry.add(cb)

    const callback = registry.get(1)
    assert.equal(callback.toString(), cb.toString())
  })

  it('returns an empty function if the cb doesnt exist', () => {
    const callback = registry.get(1)
    assert.equal(callback.toString(), 'function () {}')
  })

  it('removes a callback to the registry', () => {
    const cb = () => [1, 2, 3, 4, 5]
    const id = registry.add(cb)
    assert.equal(id, 1)

    const beforeCB = registry.get(1)
    assert.equal(beforeCB.toString(), cb.toString())

    registry.remove(1)
    const afterCB = registry.get(1)
    assert.equal(afterCB.toString(), 'function () {}')
  })
})
