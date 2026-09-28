// Handles SIGTERM and exits on its own schedule, so kill() must not
// force-kill it. The delay is longer than the 2s grace period the old
// base::EnsureProcessTerminated fallback used.
process.on('SIGTERM', () => {
  setTimeout(() => process.exit(42), 2500);
});
process.parentPort.postMessage('ready');
setInterval(() => {}, 1000);
