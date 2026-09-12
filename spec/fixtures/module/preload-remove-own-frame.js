// Removes its own subframe from a task queued while the frame's Node.js
// environment is being created, so the environment is torn down before the
// task that first runs its loop.
if (window !== window.top) {
  setTimeout(() => window.frameElement.remove(), 0);
}
