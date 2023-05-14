import { on } from 'events';
import { arrayFromAsync, find, firstN, iterable } from './async-iter';

// TODO: unfortunately, TypeScript doesn't have amazing handling of overloads
// when it comes to type functions. That means we can't easily get the event
// names of, say, `BrowserWindow` without writing enough code to become its own
// library.
//
// For the sake of the future, these functions' interfaces should be augmented
// to take a type `E` for the `eventName` parameter such that `E` is the union
// of all event names for the emitter type `T`:
// `T extends { on: (eventName: E, (...args: EventArgs<T, E>) => any) => any }`

/**
 * Similar to the `on` helper from `node:events`, however this function bounds
 * the lifetime of the event listener to the execution of the passed closure.
 * That is, the event listener will be cleaned up after the `closure` returns.
 *
 * The built-in `on` helper from `node:events` has similar functionality when
 * used with `for await` loops, but is less ergonomic when the resulting
 * iterator is used directly. In comparison, this helper function choses to
 * use the provided closure for more ergonomic cleanup.
 */
export async function scopedOn<R> (
  emitter: NodeJS.EventEmitter,
  eventName: string,
  closure: (events: AsyncIterableIterator<any[]>) => Promise<R>
): Promise<R> {
  // Create the controller for the `AbortSignal` that will clean up the event
  // listener
  const controller = new AbortController();

  // Call the closure on an iterable event listener
  const events = on(emitter, eventName, { signal: controller.signal });
  const result = await closure(events);

  // Cancel the event listener, letting it clean up
  controller.abort();

  // Return the results collected earlier.
  return result;
}

/**
 * Returns a promise that resolves when the event emitter has emitted the
 * '`eventName`' event `n` times.
 *
 * This function only listens to the event during the execution of this fuction.
 */
export async function emittedN (
  emitter: NodeJS.EventEmitter,
  eventName: string,
  n: number
): Promise<any[][]> {
  return scopedOn(emitter, eventName, (events) =>
    // 1. Continue iterating for the first `n` events
    // 2. Collect the event arguments into an array
    arrayFromAsync(iterable(firstN(events, n)))
  );
}

/**
 * Returns a promise that resolves when the predicate function passes. The
 * predicate is passed the event parameters each time the event is emitted.
 * The promise returned from this function resolves with the event parameters
 * from the passing emit.
 *
 * This function only listens to the event during the execution of this fuction.
 */
export async function findEmit (
  emitter: NodeJS.EventEmitter,
  eventName: string,
  predicate: (...args: any[]) => boolean
): Promise<any[]> {
  const result = await scopedOn(emitter, eventName, (events) =>
    find(events, (args) => predicate(...args))
  );

  if (result === undefined) {
    throw new Error(`No emit of '${eventName}' was found`);
  }

  return result;
}
