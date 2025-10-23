process.on('uncaughtException', (err) => {
  console.error(err);
  process.exit(1);
});

const { net } = await import('electron/common');

process.exit(net !== undefined ? 0 : 1);
