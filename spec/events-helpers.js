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
    target.addEventListener(eventName, resolve, { once: true });
  });
};

/**
 * @param {!EventEmitter} emitter
 * @param {string} eventName
 * @return {!Promise<!Array>} With Event as the first item.
 */
const emittedOnce = (emitter, eventName) => {
  return emittedNTimes(emitter, eventName, 1).then(([result]) => result);
};

const emittedNTimes = (emitter, eventName, times) => {
  const events = [];
  return new Promise(resolve => {
    const handler = (...args) => {
      events.push(args);
      if (events.length === times) {
        emitter.removeListener(eventName, handler);
        resolve(events);
      }
    };
    emitter.on(eventName, handler);
  });
};

exports.emittedOnce = emittedOnce;
exports.emittedNTimes = emittedNTimes;
exports.waitForEvent = waitForEvent;
