import { getCppHeapStatistics } from 'node:v8';

// These helpers measure what the calling isolate keeps alive. Pass this file's
// path into remotely() and require() it there, like heapsnapshot-helpers.js;
// the app must run with --js-flags=--expose-gc.

// Counts the C++ heap objects of each cppgc type, e.g.
// countCppHeapObjects({ menus: 'Electron / Menu' }) returns { menus: 11 }.
// A type's name is what its GetHumanReadableName() returns. The counts include
// garbage that has not been collected yet: right after collectGarbagePrecisely()
// they are the live objects, and they always include every object still in use.
export function countCppHeapObjects(queries) {
  const { type_names: typeNames, space_statistics: spaces } = getCppHeapStatistics('detailed');
  const entries = Object.entries(queries);
  const counts = Object.fromEntries(entries.map(([key]) => [key, 0]));
  for (const space of spaces) {
    for (const page of space.page_stats) {
      page.object_statistics.forEach((entry, index) => {
        for (const [key, typeName] of entries) {
          if (typeNames[index] === typeName) counts[key] += entry.object_count;
        }
      });
    }
  }
  return counts;
}

// Collects garbage from a fresh task without scanning the native stack,
// so a stale pointer cannot keep a released object alive.
// Several rounds also reclaim objects that only become unreachable
// after an earlier round's weak callbacks have run.
export async function collectGarbagePrecisely() {
  for (let i = 0; i < 3; i++) {
    await globalThis.gc({ type: 'major', execution: 'async' });
  }
}

// Counts the WeakRefs in `refs` whose targets have not been collected.
export function countAlive(refs) {
  return refs.filter((ref) => ref.deref() !== undefined).length;
}
