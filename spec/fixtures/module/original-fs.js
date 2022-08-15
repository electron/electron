process.on('message', function () {
  process.send(typeof require('original-fs'));
});
