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
export const emittedOnce = (emitter: EventEmitter, eventName: string, trigger?: () => void) => {
  return emittedNTimes(emitter, eventName, 1, trigger).then(([result]) => result)
}

export const emittedNTimes = async (emitter: EventEmitter, eventName: string, times: number, trigger?: () => void) => {
  const events: any[][] = []
  const p = new Promise<any[][]>(resolve => {
    const handler = (...args: any[]) => {
      events.push(args)
      if (events.length === times) {
        emitter.removeListener(eventName, handler)
        resolve(events)
      }
    }
    emitter.on(eventName, handler)
  })
  if (trigger) {
    await Promise.resolve(trigger())
  }
  return p
}
