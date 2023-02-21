/**
 * @fileoverview A set of helper functions to make it easier to work
 * with events in async/await manner.
 */

export const emittedNTimes = async (emitter: NodeJS.EventEmitter, eventName: string, times: number, trigger?: () => void) => {
  const events: any[][] = [];
  const p = new Promise<any[][]>(resolve => {
    const handler = (...args: any[]) => {
      events.push(args);
      if (events.length === times) {
        emitter.removeListener(eventName, handler);
        resolve(events);
      }
    };
    emitter.on(eventName, handler);
  });
  if (trigger) {
    await Promise.resolve(trigger());
  }
  return p;
};

export const emittedUntil = async (emitter: NodeJS.EventEmitter, eventName: string, untilFn: Function) => {
  const p = new Promise<any[]>(resolve => {
    const handler = (...args: any[]) => {
      if (untilFn(...args)) {
        emitter.removeListener(eventName, handler);
        resolve(args);
      }
    };
    emitter.on(eventName, handler);
  });
  return p;
};
