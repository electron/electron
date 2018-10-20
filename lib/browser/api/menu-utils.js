'use strict'

function splitArray (arr, predicate) {
  const result = arr.reduce((multi, item) => {
    const current = multi[multi.length - 1]
    if (predicate(item)) {
      if (current.length > 0) multi.push([])
    } else {
      current.push(item)
    }
    return multi
  }, [[]])

  if (result[result.length - 1].length === 0) {
    return result.slice(0, result.length - 1)
  }
  return result
}

function joinArrays (arrays, joinIDs) {
  return arrays.reduce((joined, arr, i) => {
    if (i > 0 && arr.length) {
      if (joinIDs.length > 0) {
        joined.push(joinIDs[0])
        joinIDs.splice(0, 1)
      } else {
        joined.push({ type: 'separator' })
      }
    }
    return joined.concat(arr)
  }, [])
}

function pushOntoMultiMap (map, key, value) {
  if (!map.has(key)) {
    map.set(key, [])
  }
  map.get(key).push(value)
}

function indexOfGroupContainingID (groups, id, ignoreGroup) {
  return groups.findIndex(
    candidateGroup =>
      candidateGroup !== ignoreGroup &&
      candidateGroup.some(
        candidateItem => candidateItem.id === id
      )
  )
}

// Sort nodes topologically using a depth-first approach. Encountered cycles
// are broken.
function sortTopologically (originalOrder, edgesById) {
  const sorted = []
  const marked = new Set()

  const visit = (mark) => {
    if (marked.has(mark)) return
    marked.add(mark)
    const edges = edgesById.get(mark)
    if (edges != null) {
      edges.forEach(visit)
    }
    sorted.push(mark)
  }

  originalOrder.forEach(visit)
  return sorted
}

function attemptToMergeAGroup (groups) {
  for (let i = 0; i < groups.length; i++) {
    const group = groups[i]
    for (const item of group) {
      const toIDs = [...(item.before || []), ...(item.after || [])]
      for (const id of toIDs) {
        const index = indexOfGroupContainingID(groups, id, group)
        if (index === -1) continue
        const mergeTarget = groups[index]

        mergeTarget.push(...group)
        groups.splice(i, 1)
        return true
      }
    }
  }
  return false
}

function mergeGroups (groups) {
  let merged = true
  while (merged) {
    merged = attemptToMergeAGroup(groups)
  }
  return groups
}

function sortItemsInGroup (group) {
  const originalOrder = group.map((node, i) => i)
  const edges = new Map()
  const idToIndex = new Map(group.map((item, i) => [item.id, i]))

  group.forEach((item, i) => {
    if (item.before) {
      item.before.forEach(toID => {
        const to = idToIndex.get(toID)
        if (to != null) {
          pushOntoMultiMap(edges, to, i)
        }
      })
    }
    if (item.after) {
      item.after.forEach(toID => {
        const to = idToIndex.get(toID)
        if (to != null) {
          pushOntoMultiMap(edges, i, to)
        }
      })
    }
  })

  const sortedNodes = sortTopologically(originalOrder, edges)
  return sortedNodes.map(i => group[i])
}

function findEdgesInGroup (groups, i, edges) {
  const group = groups[i]
  for (const item of group) {
    if (item.beforeGroupContaining) {
      for (const id of item.beforeGroupContaining) {
        const to = indexOfGroupContainingID(groups, id, group)
        if (to !== -1) {
          pushOntoMultiMap(edges, to, i)
          return
        }
      }
    }
    if (item.afterGroupContaining) {
      for (const id of item.afterGroupContaining) {
        const to = indexOfGroupContainingID(groups, id, group)
        if (to !== -1) {
          pushOntoMultiMap(edges, i, to)
          return
        }
      }
    }
  }
}

function sortGroups (groups) {
  const originalOrder = groups.map((item, i) => i)
  const edges = new Map()

  for (let i = 0; i < groups.length; i++) {
    findEdgesInGroup(groups, i, edges)
  }

  const sortedGroupIndexes = sortTopologically(originalOrder, edges)
  return sortedGroupIndexes.map(i => groups[i])
}

function sortMenuItems (menuItems) {
  const isSeparator = (item) => item.type === 'separator'
  const separators = menuItems.filter(i => i.type === 'separator')

  // Split the items into their implicit groups based upon separators.
  const groups = splitArray(menuItems, isSeparator)
  const mergedGroups = mergeGroups(groups)
  const mergedGroupsWithSortedItems = mergedGroups.map(sortItemsInGroup)
  const sortedGroups = sortGroups(mergedGroupsWithSortedItems)

  const joined = joinArrays(sortedGroups, separators)
  return joined
}

module.exports = { sortMenuItems }
