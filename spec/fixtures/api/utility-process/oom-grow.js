const v8 = require('node:v8');

v8.setHeapSnapshotNearHeapLimit(1);

const arr = [];
function runAllocation() {
  arr.push('x'.repeat(1024 * 1024));
  setImmediate(runAllocation);
}
setImmediate(runAllocation);
