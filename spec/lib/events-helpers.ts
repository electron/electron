/**
 * @fileoverview A set of helper functions to make it easier to work
 * with events in async/await manner.
 */

import { on } from 'node:events';

export const emittedNTimes = async (emitter: NodeJS.EventEmitter, eventName: string, times: number, trigger?: () => void) => {
  const events: any[][] = [];
  const iter = on(emitter, eventName);
  if (trigger) await Promise.resolve(trigger());
  for await (const args of iter) {
    events.push(args);
    if (events.length === times) { break; }
  }
  return events;
};

export const emittedUntil = async (emitter: NodeJS.EventEmitter, eventName: string, untilFn: Function) => {
  for await (const args of on(emitter, eventName)) {
    if (untilFn(...args)) { return args; }
  }
};
