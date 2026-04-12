type CleanupFunction = (() => void) | (() => Promise<void>);
const cleanupFunctions: CleanupFunction[] = [];

export async function runCleanupFunctions() {
  // Drain before running so a throwing cleanup can't leave stale entries to
  // be re-run on the next test.
  const pending = cleanupFunctions.splice(0, cleanupFunctions.length);
  const errors: unknown[] = [];
  for (const cleanup of pending) {
    try {
      const r = cleanup();
      if (r instanceof Promise) {
        await r;
      }
    } catch (err) {
      errors.push(err);
    }
  }
  if (errors.length === 1) throw errors[0];
  if (errors.length > 1) throw new AggregateError(errors, `${errors.length} defer() cleanups failed`);
}

export function defer(f: CleanupFunction) {
  cleanupFunctions.unshift(f);
}
