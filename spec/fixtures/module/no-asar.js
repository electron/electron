const fs = require('fs');
const path = require('path');

const stats = fs.statSync(path.join(__dirname, '..', 'test.asar', 'a.asar'));

const details = {
  isFile: stats.isFile(),
  size: stats.size
};

if (process.send != null) {
  process.send(details);
} else {
  console.log(JSON.stringify(details));
}
