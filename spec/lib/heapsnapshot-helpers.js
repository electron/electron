import { getHeapSnapshot } from 'node:v8';

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

// Takes a heap snapshot of the calling isolate and keeps V8's flat arrays.
// test/common/heap's recordState() also builds a JS object for every node and
// edge, plus the embedder graph, which takes longer than taking the snapshot
// itself; tests that only check retaining paths can use this instead.
export function takeHeapSnapshot() {
  const stream = getHeapSnapshot();
  stream.pause();
  const { snapshot, nodes, edges, strings } = JSON.parse(stream.read());
  const { meta } = snapshot;
  const nodeFieldCount = meta.node_fields.length;
  const edgeCountOffset = meta.node_fields.indexOf('edge_count');
  // Index into `edges` of each node's first outgoing edge.
  const firstEdge = new Uint32Array(nodes.length / nodeFieldCount + 1);
  for (let node = 0; node < firstEdge.length - 1; node++) {
    firstEdge[node + 1] = firstEdge[node] + nodes[node * nodeFieldCount + edgeCountOffset] * meta.edge_fields.length;
  }
  return { meta, nodes, edges, strings, firstEdge };
}

// containsRetainingPath() for a snapshot from takeHeapSnapshot().
export function snapshotContainsRetainingPath(snapshot, retainingPath, options) {
  const { meta, nodes, edges, strings, firstEdge } = snapshot;
  const nodeFieldCount = meta.node_fields.length;
  const nameOffset = meta.node_fields.indexOf('name');
  const typeOffset = meta.node_fields.indexOf('type');
  const stringType = meta.node_types[typeOffset].indexOf('string');
  const toNodeOffset = meta.edge_fields.indexOf('to_node');
  const isNamed = (node, name) =>
    nodes[node * nodeFieldCount + typeOffset] !== stringType &&
    strings[nodes[node * nodeFieldCount + nameOffset]] === name;

  let root = [];
  for (let node = 0; node < firstEdge.length - 1; node++) {
    if (isNamed(node, retainingPath[0])) root.push(node);
  }
  for (let i = 1; i < retainingPath.length; i++) {
    const needle = retainingPath[i];
    const newRoot = [];
    for (const node of root) {
      for (let edge = firstEdge[node]; edge < firstEdge[node + 1]; edge += meta.edge_fields.length) {
        const child = edges[edge + toNodeOffset] / nodeFieldCount;
        if (isNamed(child, needle)) newRoot.push(child);
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
