export function countHeapSnapshotNodes(snapshot, queries) {
  const {
    snapshot: { meta },
    nodes,
    strings
  } = JSON.parse(snapshot.toString());
  const nameOffset = meta.node_fields.indexOf('name');
  const typeOffset = meta.node_fields.indexOf('type');
  const types = meta.node_types[typeOffset];
  const entries = Object.entries(queries);
  const counts = Object.fromEntries(entries.map(([key]) => [key, 0]));

  for (let i = 0; i < nodes.length; i += meta.node_fields.length) {
    const name = strings[nodes[i + nameOffset]];
    const type = types[nodes[i + typeOffset]];
    if (type === 'string') continue;
    for (const [key, query] of entries) {
      if (name === query.name && (query.type === undefined || type === query.type)) {
        counts[key]++;
      }
    }
  }
  return counts;
}

export function containsRetainingPath(snapshot, retainingPath, options) {
  let root = snapshot.filter((node) => node.name === retainingPath[0] && node.type !== 'string');
  for (let i = 1; i < retainingPath.length; i++) {
    const needle = retainingPath[i];
    const newRoot = [];
    for (const node of root) {
      for (let j = 0; j < node.outgoingEdges.length; j++) {
        const child = node.outgoingEdges[j].to;
        if (child.type === 'string') continue;
        if (child.name === needle) {
          newRoot.push(child);
        }
      }
    }
    if (!newRoot.length) {
      console.log(`No retaining path found for ${needle}`);
      return false;
    }
    root = newRoot;
  }
  return options?.occurrences ? root.length === options.occurrences : true;
}
