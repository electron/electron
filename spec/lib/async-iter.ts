/**
 * Yields *up to* `n` elements from the provided iterator, stopping either when
 * `n` elements have been yielded or when the provided iterator finishes,
 * whichever happens first.
 */
export function firstN<T, TReturn, TNext> (
  iter: AsyncIterator<T, TReturn, TNext>,
  n: number
): AsyncIterator<T, TReturn | undefined, TNext> {
  let i = 0;
  return {
    ...iter,
    async next (
      ...args: [] | [TNext]
    ): Promise<IteratorResult<T, TReturn | undefined>> {
      if (i >= n) {
        return { done: true, value: undefined };
      } else {
        const result = await iter.next(...args);
        if (result.done) {
          return result;
        } else {
          i += 1;
          return result;
        }
      }
    }
  };
}

/**
 * Finds the first element in the provided iterator that passes the provided
 * predicate function. Returns `undefined` if no element passes the predicate
 * before the iterator finishes or if the iterator is empty.
 */
export async function find<T> (
  iter: AsyncIterator<T, any, any>,
  predicate: (t: T) => boolean
): Promise<T | undefined> {
  for (let elem = await iter.next(); !elem.done; elem = await iter.next()) {
    if (predicate(elem.value)) {
      return elem.value;
    }
  }
  return undefined;
}

// TODO: the following fn can be replaced by `Array.fromAsync` once the proposal
// for that function gets implemented. As of writing, that function is still in
// stage 2.
/**
 * Collects elements yielded from an async iterator into an array until that
 * iterator finishes.
 */
export async function arrayFromAsync<T> (
  iterable: AsyncIterable<T>
): Promise<T[]> {
  const result = [];
  for await (const elem of iterable) {
    result.push(elem);
  }
  return result;
}

/**
 * Convenient wrapper to turn an `AsyncIterator` into an `AsyncIterable`, e.g.
 * for use in `for await` loops.
 */
export function iterable<T> (iter: AsyncIterator<T>): AsyncIterableIterator<T> {
  if (Symbol.asyncIterator in iter) {
    return iter as AsyncIterableIterator<T>;
  }

  const result: AsyncIterableIterator<T> = {
    ...iter,
    [Symbol.asyncIterator]: () => result
  };
  return result;
}
