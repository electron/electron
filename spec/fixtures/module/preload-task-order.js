window.taskOrder = ['preload'];
Promise.resolve().then(() => window.taskOrder.push('microtask'));
process.nextTick(() => window.taskOrder.push('nextTick'));
