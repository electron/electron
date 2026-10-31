import { getHeapSnapshot } from 'node:v8';

// These helpers read V8's flat heap snapshot arrays directly. Building a JS
// object for every node and edge, as test/common/heap's recordState() does,
// takes longer than taking the snapshot itself.

// Parses a heap snapshot (its JSON text, as a Buffer or string) and indexes
// each node's first outgoing edge.
export function parseHeapSnapshot(data) {
  const {
    snapshot: { meta },
    nodes,
    edges,
    strings
  } = JSON.parse(data.toString());
  const nodeFieldCount = meta.node_fields.length;
  const edgeFieldCount = meta.edge_fields.length;
  const edgeCountOffset = meta.node_fields.indexOf('edge_count');
  const nodeCount = nodes.length / nodeFieldCount;
  // Index into `edges` of each node's first outgoing edge.
  const firstEdge = new Uint32Array(nodeCount + 1);
  for (let node = 0; node < nodeCount; node++) {
    firstEdge[node + 1] = firstEdge[node] + nodes[node * nodeFieldCount + edgeCountOffset] * edgeFieldCount;
  }
  return { meta, nodes, edges, strings, nodeCount, firstEdge };
}

// Takes a heap snapshot of the calling isolate. Use it in the main process or
// a utility process; for a renderer, use webContents.takeHeapSnapshot(file)
// and parse the file, so the snapshot does not run script in the renderer
// whose heap it measures.
export function takeHeapSnapshot() {
  const stream = getHeapSnapshot();
  stream.pause();
  // Since Node.js 26, read() returns one buffered chunk at a time; read the
  // whole snapshot in one call, as test/common/heap.js does.
  stream.read(0);
  return parseHeapSnapshot(stream.read(stream.readableLength));
}

const toParsedSnapshot = (snapshot) => (snapshot.firstEdge ? snapshot : parseHeapSnapshot(snapshot));

// Counts the nodes that match each query, e.g.
// countHeapSnapshotNodes(snapshot, { wrappers: { name: 'Electron / WebContents' } }).
// String nodes never match. `snapshot` is from takeHeapSnapshot() or
// parseHeapSnapshot(), or the JSON text of a snapshot file.
export function countHeapSnapshotNodes(snapshot, queries) {
  const { meta, nodes, strings } = toParsedSnapshot(snapshot);
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

const detachednessValues = { unknown: 0, attached: 1, detached: 2 };

// Returns the distinct nodes at the end of `retainingPath`. Each step is a node
// name, or an object that can also match:
// - type: the node's type; otherwise any type except string.
// - detachedness: 'unknown', 'attached' or 'detached'.
// - edgeType: the type of the edge from the previous node; otherwise any
//   type except weak, since a weak edge does not retain its target.
// - edgeName: the edge's property name, or its index for element and hidden
//   edges.
function findRetainingPathNodes(snapshot, retainingPath) {
  const { meta, nodes, edges, strings, nodeCount, firstEdge } = toParsedSnapshot(snapshot);
  const nodeFieldCount = meta.node_fields.length;
  const edgeFieldCount = meta.edge_fields.length;
  const nameOffset = meta.node_fields.indexOf('name');
  const typeOffset = meta.node_fields.indexOf('type');
  const detachednessOffset = meta.node_fields.indexOf('detachedness');
  const nodeTypes = meta.node_types[typeOffset];
  const edgeTypeOffset = meta.edge_fields.indexOf('type');
  const edgeNameOffset = meta.edge_fields.indexOf('name_or_index');
  const toNodeOffset = meta.edge_fields.indexOf('to_node');
  const edgeTypes = meta.edge_types[edgeTypeOffset];
  const steps = retainingPath.map((step) => (typeof step === 'string' ? { name: step } : step));

  const nodeMatches = (node, step) => {
    const base = node * nodeFieldCount;
    const type = nodeTypes[nodes[base + typeOffset]];
    if (step.type === undefined ? type === 'string' : type !== step.type) return false;
    if (strings[nodes[base + nameOffset]] !== step.name) return false;
    if (step.detachedness !== undefined) {
      if (!(step.detachedness in detachednessValues)) throw new Error(`Unknown detachedness ${step.detachedness}`);
      if (nodes[base + detachednessOffset] !== detachednessValues[step.detachedness]) return false;
    }
    return true;
  };
  const edgeMatches = (edge, step) => {
    const type = edgeTypes[edges[edge + edgeTypeOffset]];
    if (step.edgeType === undefined ? type === 'weak' : type !== step.edgeType) return false;
    if (step.edgeName !== undefined) {
      const nameOrIndex = edges[edge + edgeNameOffset];
      const name = type === 'element' || type === 'hidden' ? nameOrIndex : strings[nameOrIndex];
      if (name !== step.edgeName) return false;
    }
    return true;
  };

  let found = [];
  for (let node = 0; node < nodeCount; node++) {
    if (nodeMatches(node, steps[0])) found.push(node);
  }
  for (let i = 1; i < steps.length && found.length; i++) {
    const next = new Set();
    for (const node of found) {
      for (let edge = firstEdge[node]; edge < firstEdge[node + 1]; edge += edgeFieldCount) {
        const child = edges[edge + toNodeOffset] / nodeFieldCount;
        if (edgeMatches(edge, steps[i]) && nodeMatches(child, steps[i])) next.add(child);
      }
    }
    found = [...next];
  }
  if (!found.length) {
    console.log(`No retaining path found for ${JSON.stringify(retainingPath)}`);
  }
  return found;
}

// Whether any node is reached by `retainingPath` (see findRetainingPathNodes()).
export function snapshotContainsRetainingPath(snapshot, retainingPath) {
  return findRetainingPathNodes(snapshot, retainingPath).length > 0;
}

// The number of distinct nodes reached by `retainingPath`, e.g. how many
// WebFrameMain objects are persistent roots. Each node counts once, however
// many paths lead to it.
export function countRetainingPathNodes(snapshot, retainingPath) {
  return findRetainingPathNodes(snapshot, retainingPath).length;
}
