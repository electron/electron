process.on('message', function () {
  process.send(process.argv);
  process.exit(0);
});
