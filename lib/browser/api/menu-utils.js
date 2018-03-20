function splitArray (arr, predicate) {
  let lastArr = []
  const multiArr = [lastArr]
  arr.forEach(item => {
    if (predicate(item)) {
      if (lastArr.length > 0) {
        lastArr = []
        multiArr.push(lastArr)
      }
    } else {
      lastArr.push(item)
    }
  })
  return multiArr
}

function joinArrays (arrays, joiner) {
  const joinedArr = []
  arrays.forEach((arr, i) => {
    if (i > 0 && arr.length > 0) {
      joinedArr.push(joiner)
    }
    joinedArr.push(...arr)
  })
  return joinedArr
}

function pushOntoMultiMap (map, key, value) {
  if (!map.has(key)) {
    map.set(key, [])
  }
  map.get(key).push(value)
}

function indexOfGroupContainingLabel (groups, label, ignoreGroup) {
  return groups.findIndex(
    candidateGroup =>
      candidateGroup !== ignoreGroup &&
      candidateGroup.some(
        candidateItem => candidateItem.label === label
      )
  )
}

// Sort nodes topologically using a depth-first approach. Encountered cycles
// are broken.
function sortTopologically (originalOrder, edgesById) {
  const sorted = []
  const marked = new Set()

  const visit = (id) => {
    if (marked.has(id)) return
    marked.add(id)
    const edges = edgesById.get(id)
    if (edges != null) {
      edges.forEach(visit)
    }
    sorted.push(id)
  }

  originalOrder.forEach(visit)
  return sorted
}

function attemptToMergeAGroup (groups) {
  for (let i = 0; i < groups.length; i++) {
    const group = groups[i]
    for (const item of group) {
      const toLabels = [...(item.before || []), ...(item.after || [])]
      for (const label of toLabels) {
        const index = indexOfGroupContainingLabel(groups, label, group)
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
  const labelToIndex = new Map(group.map((item, i) => [item.label, i]))

  group.forEach((item, i) => {
    if (item.before) {
      item.before.forEach(toLabel => {
        const to = labelToIndex.get(toLabel)
        if (to != null) {
          pushOntoMultiMap(edges, to, i)
        }
      })
    }
    if (item.after) {
      item.after.forEach(toLabel => {
        const to = labelToIndex.get(toLabel)
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
      for (const label of item.beforeGroupContaining) {
        const to = indexOfGroupContainingLabel(groups, label, group)
        if (to !== -1) {
          pushOntoMultiMap(edges, to, i)
          return
        }
      }
    }
    if (item.afterGroupContaining) {
      for (const label of item.afterGroupContaining) {
        const to = indexOfGroupContainingLabel(groups, label, group)
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

  // Split the items into their implicit groups based upon separators.
  const groups = splitArray(menuItems, isSeparator)
  const mergedGroups = mergeGroups(groups)
  const mergedGroupsWithSortedItems = mergedGroups.map(sortItemsInGroup)
  const sortedGroups = sortGroups(mergedGroupsWithSortedItems)

  const joined = joinArrays(sortedGroups, { type: 'separator' })
  console.log(joined)
  return joined
}

module.exports = {sortMenuItems}
