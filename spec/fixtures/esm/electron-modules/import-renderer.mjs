process.on('uncaughtException', (err) => {
  console.error(err);
  process.exit(1);
});

const { net } = await import('electron/renderer');

process.exit(net !== undefined ? 0 : 1);
