// Once told how many messages to post, posts them and exits straight away,
// without waiting for the parent to read them.
process.parentPort.once('message', ({ data: count }) => {
  for (let index = 0; index < count; index++) {
    process.parentPort.postMessage(index);
  }
  process.exit(0);
});
