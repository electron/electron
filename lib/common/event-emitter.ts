/**
 * In this file we effectively clone the EventEmitter class but in a way
 * such that prototype modifications on the original EventEmitter do
 * not propogate to Electron's internal instances.  This means that user
 * code can't pollute Electron logic.
 */
import { EventEmitter as NodeEventEmitter } from 'events'

export const EventEmitter = function EventEmitter (this: NodeEventEmitter) {
  NodeEventEmitter.call(this)
} as any as typeof NodeEventEmitter

Object.assign(EventEmitter.prototype, NodeEventEmitter.prototype)
