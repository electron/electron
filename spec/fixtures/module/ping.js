process.on('message', function (msg) {
  process.send(msg);
  process.exit(0);
});
