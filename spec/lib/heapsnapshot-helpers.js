export function containsRetainingPath (snapshot, retainingPath, options) {
  let root = snapshot.filter(
    (node) => node.name === retainingPath[0] && node.type !== 'string');
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
  return options?.occurrances
    ? root.length === options.occurrances
    : true;
}
