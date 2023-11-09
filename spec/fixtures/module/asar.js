const fs = require('node:fs');
process.on('message', function (file) {
  process.send(fs.readFileSync(file).toString());
});
