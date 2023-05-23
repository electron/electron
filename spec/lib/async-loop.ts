/**
 * Runs a polling function eagerly in a loop until either it returns a truthy
 * value (by default) or some other condition (specified by the second
 * argument). Once the poll produces a satisfying result, that result is
 * returned asynchronously from this function.
 *
 * This function runs the poll function eagerly, that is before checking the
 * condition function.
 *
 * The duration between polls can be specified in **milliseconds** and defaults
 * to **10ms**.
 *
 * This function will loop until a time limit is hit, then it will throw an
 * error. The time limit can be specified in **milliseconds** and defaults to
 * **10 seconds** (10000ms). This is to promote good test hygiene by putting
 * hard bounds on test execution.
 */
export function pollUntil<T> (
  poll: () => T,
  until?: (t: T) => boolean,
  opts?: { interval?: number; timeLimit?: number }
): Promise<T> {
  const { interval: optInterval, timeLimit: optTimelimit } = opts ?? {};
  const untilFn = until ?? Boolean;
  const interval = optInterval ?? 10; // 10ms
  const timeLimit = optTimelimit ?? 10000; // 10 seconds
  const endTime = Date.now() + timeLimit;

  return new Promise((resolve, reject) => {
    // This holds the `setInterval` id so we can clean it up later
    let intervalId: NodeJS.Timeout | undefined;

    // This function handles clearing the interval and timeouts
    const cleanUp = () => {
      if (intervalId !== undefined) {
        clearInterval(intervalId);
        intervalId = undefined;
      }
    };

    // This function runs the poll check and handles resolving the promise if it
    // returns true. This check function returns true if the polling has
    // finished, either by success or failure.
    const pollCheck = (): boolean => {
      try {
        // Run the poll function
        const result = poll();

        // If the poll result is satisfactory, clean up and resolve with the
        // satisfying poll result
        if (untilFn(result)) {
          cleanUp();
          resolve(result);
          return true;
        }
      } catch (err) {
        // Propogate the error after cleaning up
        cleanUp();
        reject(err);
        return true;
      }

      // Check if we are past the time limit
      if (Date.now() > endTime) {
        cleanUp();
        reject(new Error(`poll loop ran past time limit (${timeLimit}ms)`));
        return true;
      }

      return false;
    };

    // Do an eager poll check just in case we can skip a trip to the event loop
    if (pollCheck()) {
      return;
    }

    // Start the polling interval
    intervalId = setInterval(pollCheck, interval);
  });
}
