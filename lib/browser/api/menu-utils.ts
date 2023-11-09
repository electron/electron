function splitArray<T> (arr: T[], predicate: (x: T) => boolean) {
  const result = arr.reduce((multi, item) => {
    const current = multi[multi.length - 1];
    if (predicate(item)) {
      if (current.length > 0) multi.push([]);
    } else {
      current.push(item);
    }
    return multi;
  }, [[]] as T[][]);

  if (result[result.length - 1].length === 0) {
    return result.slice(0, result.length - 1);
  }
  return result;
}

function joinArrays (arrays: any[][], joinIDs: any[]) {
  return arrays.reduce((joined, arr, i) => {
    if (i > 0 && arr.length) {
      if (joinIDs.length > 0) {
        joined.push(joinIDs[0]);
        joinIDs.splice(0, 1);
      } else {
        joined.push({ type: 'separator' });
      }
    }
    return joined.concat(arr);
  }, []);
}

function pushOntoMultiMap<K, V> (map: Map<K, V[]>, key: K, value: V) {
  if (!map.has(key)) {
    map.set(key, []);
  }
  map.get(key)!.push(value);
}

function indexOfGroupContainingID<T> (groups: {id?: T}[][], id: T, ignoreGroup: {id?: T}[]) {
  return groups.findIndex(
    candidateGroup =>
      candidateGroup !== ignoreGroup &&
      candidateGroup.some(
        candidateItem => candidateItem.id === id
      )
  );
}

// Sort nodes topologically using a depth-first approach. Encountered cycles
// are broken.
function sortTopologically<T> (originalOrder: T[], edgesById: Map<T, T[]>) {
  const sorted = [] as T[];
  const marked = new Set<T>();

  const visit = (mark: T) => {
    if (marked.has(mark)) return;
    marked.add(mark);
    const edges = edgesById.get(mark);
    if (edges != null) {
      for (const edge of edges) {
        visit(edge);
      }
    }
    sorted.push(mark);
  };

  for (const edge of originalOrder) {
    visit(edge);
  }

  return sorted;
}

function attemptToMergeAGroup<T> (groups: {before?: T[], after?: T[], id?: T}[][]) {
  for (let i = 0; i < groups.length; i++) {
    const group = groups[i];
    for (const item of group) {
      const toIDs = [...(item.before || []), ...(item.after || [])];
      for (const id of toIDs) {
        const index = indexOfGroupContainingID(groups, id, group);
        if (index === -1) continue;
        const mergeTarget = groups[index];

        mergeTarget.push(...group);
        groups.splice(i, 1);
        return true;
      }
    }
  }
  return false;
}

function mergeGroups<T> (groups: {before?: T[], after?: T[], id?: T}[][]) {
  let merged = true;
  while (merged) {
    merged = attemptToMergeAGroup(groups);
  }
  return groups;
}

function sortItemsInGroup<T> (group: {before?: T[], after?: T[], id?: T}[]) {
  const originalOrder = group.map((node, i) => i);
  const edges = new Map();
  const idToIndex = new Map(group.map((item, i) => [item.id, i]));

  for (const [i, item] of group.entries()) {
    if (item.before) {
      for (const toID of item.before) {
        const to = idToIndex.get(toID);
        if (to != null) {
          pushOntoMultiMap(edges, to, i);
        }
      }
    }
    if (item.after) {
      for (const toID of item.after) {
        const to = idToIndex.get(toID);
        if (to != null) {
          pushOntoMultiMap(edges, i, to);
        }
      }
    }
  }

  const sortedNodes = sortTopologically(originalOrder, edges);
  return sortedNodes.map(i => group[i]);
}

function findEdgesInGroup<T> (groups: {beforeGroupContaining?: T[], afterGroupContaining?: T[], id?: T}[][], i: number, edges: Map<any, any>) {
  const group = groups[i];
  for (const item of group) {
    if (item.beforeGroupContaining) {
      for (const id of item.beforeGroupContaining) {
        const to = indexOfGroupContainingID(groups, id, group);
        if (to !== -1) {
          pushOntoMultiMap(edges, to, i);
          return;
        }
      }
    }
    if (item.afterGroupContaining) {
      for (const id of item.afterGroupContaining) {
        const to = indexOfGroupContainingID(groups, id, group);
        if (to !== -1) {
          pushOntoMultiMap(edges, i, to);
          return;
        }
      }
    }
  }
}

function sortGroups<T> (groups: {id?: T}[][]) {
  const originalOrder = groups.map((item, i) => i);
  const edges = new Map();

  for (let i = 0; i < groups.length; i++) {
    findEdgesInGroup(groups, i, edges);
  }

  const sortedGroupIndexes = sortTopologically(originalOrder, edges);
  return sortedGroupIndexes.map(i => groups[i]);
}

export function sortMenuItems (menuItems: (Electron.MenuItemConstructorOptions | Electron.MenuItem)[]) {
  const isSeparator = (i: Electron.MenuItemConstructorOptions | Electron.MenuItem) => {
    const opts = i as Electron.MenuItemConstructorOptions;
    return i.type === 'separator' && !opts.before && !opts.after && !opts.beforeGroupContaining && !opts.afterGroupContaining;
  };
  const separators = menuItems.filter(isSeparator);

  // Split the items into their implicit groups based upon separators.
  const groups = splitArray(menuItems, isSeparator);
  const mergedGroups = mergeGroups(groups);
  const mergedGroupsWithSortedItems = mergedGroups.map(sortItemsInGroup);
  const sortedGroups = sortGroups(mergedGroupsWithSortedItems);

  const joined = joinArrays(sortedGroups, separators);
  return joined;
}
