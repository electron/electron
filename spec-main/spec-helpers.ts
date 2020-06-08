export const ifit = (condition: boolean) => (condition ? it : it.skip);
export const ifdescribe = (condition: boolean) => (condition ? describe : describe.skip);

export const delay = (time: number) => new Promise(resolve => setTimeout(resolve, time));

type CleanupFunction = (() => void) | (() => Promise<void>)
const cleanupFunctions: CleanupFunction[] = [];
export async function runCleanupFunctions () {
  for (const cleanup of cleanupFunctions) {
    const r = cleanup();
    if (r instanceof Promise) { await r; }
  }
  cleanupFunctions.length = 0;
}

afterEach(runCleanupFunctions);

export function defer (f: CleanupFunction) {
  cleanupFunctions.push(f);
}
