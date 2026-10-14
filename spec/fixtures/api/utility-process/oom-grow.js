const v8 = require('node:v8');

v8.setHeapSnapshotNearHeapLimit(1);

// Grow the heap towards the --max-old-space-size limit the spec sets in large steps
// (an array of 128K numbers per turn of the event loop),
// so that it gets there in under a hundred turns.
// Growing it by one short string per turn took tens of thousands of turns,
// which on a busy machine overran the test's timeout.
const arr = [];
function runAllocation() {
  arr.push(new Array(128 * 1024).fill(arr.length));
  setImmediate(runAllocation);
}
setImmediate(runAllocation);
