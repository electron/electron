const v8 = require('node:v8');

v8.setHeapSnapshotNearHeapLimit(1);

const arr = [];
function runAllocation () {
  const str = JSON.stringify(process.config).slice(0, 1000);
  arr.push(str);
  setImmediate(runAllocation);
}
setImmediate(runAllocation);
