setTimeout(() => {
  let called = 0;
  let result = '';
  process.parentPort.on('message', (e) => {
    result += e.data;
    if (++called === 3) {
      process.parentPort.postMessage(result);
    }
  });
}, 3000);
