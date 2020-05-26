async function getFiles (directoryPath, { filter = null } = {}) {
  const files = [];
  const walker = require('walkdir').walk(directoryPath, {
    no_recurse: true
  });
  walker.on('file', (file) => {
    if (!filter || filter(file)) {
      files.push(file);
    }
  });
  await new Promise((resolve) => walker.on('end', resolve));
  return files;
}

module.exports = getFiles;
