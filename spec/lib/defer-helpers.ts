type CleanupFunction = (() => void) | (() => Promise<void>);
const cleanupFunctions: CleanupFunction[] = [];

export async function runCleanupFunctions() {
  for (const cleanup of cleanupFunctions) {
    const r = cleanup();
    if (r instanceof Promise) {
      await r;
    }
  }
  cleanupFunctions.length = 0;
}

export function defer(f: CleanupFunction) {
  cleanupFunctions.unshift(f);
}
