/**
 * @fileoverview A set of helper functions to make it easier to work
 * with events in async/await manner.
 */

/**
 * @param {!EventTarget} target
 * @param {string} eventName
 * @return {!Promise<!Event>}
 */
const waitForEvent = (target, eventName) => {
  return new Promise(resolve => {
    target.addEventListener(eventName, resolve, {once: true})
  })
}

/**
 * @param {!EventEmitter} emitter
 * @param {string} eventName
 * @return {!Promise<!Array>} With Event as the first item.
 */
const emittedOnce = (emitter, eventName) => {
  return new Promise(resolve => {
    emitter.once(eventName, (...args) => resolve(args))
  })
}

exports.emittedOnce = emittedOnce
exports.waitForEvent = waitForEvent
