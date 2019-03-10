import { EventEmitter } from "electron";

/**
 * @fileoverview A set of helper functions to make it easier to work
 * with events in async/await manner.
 */

/**
 * @param {!EventTarget} target
 * @param {string} eventName
 * @return {!Promise<!Event>}
 */
export const waitForEvent = (target: EventTarget, eventName: string) => {
  return new Promise(resolve => {
    target.addEventListener(eventName, resolve, { once: true })
  })
}

/**
 * @param {!EventEmitter} emitter
 * @param {string} eventName
 * @return {!Promise<!Array>} With Event as the first item.
 */
export const emittedOnce = (emitter: EventEmitter, eventName: string) => {
  return emittedNTimes(emitter, eventName, 1).then(([result]) => result)
}

export const emittedNTimes = (emitter: EventEmitter, eventName: string, times: number) => {
  const events: any[][] = []
  return new Promise<any[][]>(resolve => {
    const handler = (...args: any[]) => {
      events.push(args)
      if (events.length === times) {
        emitter.removeListener(eventName, handler)
        resolve(events)
      }
    }
    emitter.on(eventName, handler)
  })
}
