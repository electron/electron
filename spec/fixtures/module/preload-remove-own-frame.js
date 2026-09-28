// Removes its own subframe while or right after the frame's Node.js
// environment is being created. --remove-own-frame-via picks how; by default
// it's a task queued while the environment is created, so the environment is
// torn down before the task that first runs its loop.
if (window !== window.top) {
  const via = process.argv.find((arg) => arg.startsWith('--remove-own-frame-via='))?.split('=')[1];
  const remove = () => window.frameElement.remove();
  if (via === 'queueMicrotask') {
    queueMicrotask(remove);
  } else if (via === 'nextTick') {
    process.nextTick(remove);
  } else if (via === 'setImmediate') {
    setImmediate(remove);
  } else if (via === 'fs-callback') {
    require('node:fs').readFile(__filename, remove);
  } else {
    setTimeout(remove, 0);
  }
}
