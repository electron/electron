const assert = require('assert')
const {deprecations, deprecate, ipcRenderer} = require('electron')

describe.only('deprecations', () => {
  beforeEach(() => {
    deprecations.setHandler(null)
    process.throwDeprecation = true
  })

  // it('allows a deprecation handler function to be specified', () => {
  //   const messages = []
  //
  //   deprecations.setHandler((message) => {
  //     messages.push(message)
  //   })
  //
  //   deprecate.log('this is deprecated')
  //   assert.deepEqual(messages, ['this is deprecated'])
  // })
  //
  // it('returns a deprecation handler after one is set', () => {
  //   const messages = []
  //
  //   deprecations.setHandler((message) => {
  //     messages.push(message)
  //   })
  //
  //   deprecate.log('this is deprecated')
  //   assert(typeof deprecations.getHandler() === 'function')
  // })
  //
  // it('returns a deprecation warning', () => {
  //   const messages = []
  //
  //   deprecations.setHandler((message) => {
  //     messages.push(message)
  //   })
  //
  //   deprecate.warn('old', 'new')
  //   assert.deepEqual(messages, [`'old' is deprecated. Use 'new' instead.`])
  // })

  // it('throws an exception if no deprecation handler is specified', () => {
  //   assert.throws(() => {
  //     deprecate.log('this is deprecated')
  //   }, /this is deprecated/)
  // })

  // it('deprecates a property', () => {
  //   deprecate.property(object, property, method)
  // })
  //
  // it('deprecates an event', () => {
  //   deprecate.event(emitter, oldName, newName, fn)
  // })
  //
  // it('forwards a method to member', () => {
  //   deprecate.member(object, method, member)
  // })

  it('renames a method', () => {
    assert(typeof ipcRenderer.sendSync === 'function')
    assert(typeof ipcRenderer.sendChannelSync === 'undefined')
    deprecate.rename(ipcRenderer, 'sendSync', 'sendChannelSync')
    assert(typeof ipcRenderer.sendChannelSync === 'function')
  })
})
