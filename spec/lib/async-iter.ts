/**
 * Yields values from the provided iterator until a certain condition is met or
 * until the provided iterator finishes, whichever occurs first. The stopping
 * condition is determined by a predicate function that is run after each call
 * to the provided iterator, returning `true` when the iterator should signal
 * that it is done.
 *
 * Note: the resulting iterator's `next` method will still call the provided
 * iterator's `next` method and the predicate function even after it returns
 * `true`. You would need to "fuse" the iterator if you want to prevent this
 * behavior.
 */
export function until<T, TReturn, TNext> (
  iter: AsyncIterator<T, TReturn, TNext>,
  predicate: (t: T) => boolean
): AsyncIterator<T, T | TReturn, TNext> {
  return {
    async next (...args): Promise<IteratorResult<T, T | TReturn>> {
      // Get the next element from the provided iterator
      const result = await iter.next(...args);
      if (result.done) {
        return result;
      }

      // If the condition is met then indicate that this iterator is done.
      const condition = predicate(result.value);
      return condition
        ? {
          done: true,
          value: result.value
        }
        : result;
    },
    return: iter.return,
    throw: iter.throw
  };
}

/**
 * Yields *up to* `n` elements from the provided iterator, stopping either when
 * `n` elements have been yielded or when the provided iterator finishes,
 * whichever happens first.
 */
export function firstN<T, TReturn, TNext> (
  iter: AsyncIterator<T, TReturn, TNext>,
  n: number
): AsyncIterator<T, T | TReturn, TNext> {
  let count = 0;
  return until(iter, () => {
    const isDone = count >= n;
    count += 1;
    return isDone;
  });
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
 * Returns a promise that resolves with the last element of the iterator.
 */
export async function last<TReturn> (
  iter: AsyncIterator<any, TReturn, any>
): Promise<TReturn> {
  while (true) {
    const result = await iter.next();
    if (result.done) {
      return result.value;
    }
  }
}

/**
 * Convenient wrapper to turn an `AsyncIterator` into an `AsyncIterable`, e.g.
 * for use in `for await` loops.
 */
export function iterable<T> (iter: AsyncIterator<T>): AsyncIterableIterator<T> {
  const result: AsyncIterableIterator<T> = {
    ...iter,
    [Symbol.asyncIterator]: () => result
  };
  return result;
}
