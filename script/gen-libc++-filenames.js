const fs = require('node:fs');
const path = require('node:path');

const check = process.argv.includes('--check');

function findAllHeaders (basePath) {
  const allFiles = fs.readdirSync(basePath);
  const toReturn = [];
  for (const file of allFiles) {
    const absPath = path.resolve(basePath, file);
    if (fs.statSync(absPath).isDirectory()) {
      toReturn.push(...findAllHeaders(absPath));
    } else {
      toReturn.push(absPath);
    }
  }
  return toReturn;
}

for (const folder of ['libc++', 'libc++abi']) {
  const prettyName = folder.replace(/\+/g, 'x');

  const libcxxIncludeDir = path.resolve(__dirname, '..', '..', 'third_party', folder, 'src', 'include');
  const gclientPath = `third_party/${folder}/src/include`;

  const headers = findAllHeaders(libcxxIncludeDir).map(absPath => path.relative(path.resolve(__dirname, '../..', gclientPath), absPath));

  const content = `${prettyName}_headers = [
  ${headers.map(f => `"//${path.posix.join(gclientPath, f)}"`).join(',\n  ')},
]

${prettyName}_licenses = [ "//third_party/${folder}/src/LICENSE.TXT" ]
`;

  const filenamesPath = path.resolve(__dirname, '..', `filenames.${prettyName}.gni`);

  if (check) {
    const currentContent = fs.readFileSync(filenamesPath, 'utf8');
    if (currentContent !== content) {
      console.log('currentContent: ', currentContent);
      console.log('content: ', content);
      throw new Error(`${prettyName} filenames need to be regenerated, latest generation does not match current file.  Please run node gen-libc++-filenames.js`);
    }
  } else {
    console.log(filenamesPath);
    fs.writeFileSync(filenamesPath, content);
  }
}
