const chalk = require('chalk');

const fs = require('node:fs');
const path = require('node:path');

const check = process.argv.includes('--check');

function findAllHeaders (basePath) {
  const allFiles = fs.readdirSync(basePath);
  allFiles.sort();
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

const diff = (array1, array2) => {
  const set1 = new Set(array1);
  const set2 = new Set(array2);

  const added = array1.filter(item => !set2.has(item));
  const removed = array2.filter(item => !set1.has(item));

  console.log(chalk.white.bgGreen.bold('Files Added:'));
  added.forEach(item => console.log(chalk.green.bold(`+ ${item}`)));

  console.log(chalk.white.bgRed.bold('Files Removed:'));
  removed.forEach(item => console.log(chalk.red.bold(`- ${item}`)));
};

const parseHeaders = (name, content) => {
  const pattern = new RegExp(`${name}_headers\\s*=\\s*\\[(.*?)\\]`, 's');
  const headers = content.match(pattern);
  if (!headers) return [];
  return headers[1].split(',')
    .map(item => item.trim().replace(/"/g, ''))
    .filter(item => item.length > 0);
};

for (const folder of ['libc++', 'libc++abi']) {
  const prettyName = folder.replace(/\+/g, 'x');

  const libcxxIncludeDir = path.resolve(__dirname, '..', '..', 'third_party', folder, 'src', 'include');
  const gclientPath = `third_party/${folder}/src/include`;

  const headers = findAllHeaders(libcxxIncludeDir).map(absPath => path.relative(path.resolve(__dirname, '../..', gclientPath), absPath).replaceAll('\\', '/'));

  const newHeaders = headers.map(f => `//${path.posix.join(gclientPath, f)}`);
  const content = `${prettyName}_headers = [
  ${newHeaders.map(h => `"${h}"`).join(',\n  ')},
]

${prettyName}_licenses = [ "//third_party/${folder}/src/LICENSE.TXT" ]
`;

  const file = `filenames.${prettyName}.gni`;
  const filenamesPath = path.resolve(__dirname, '..', file);

  if (check) {
    const currentContent = fs.readFileSync(filenamesPath, 'utf8');
    if (currentContent !== content) {
      const currentHeaders = parseHeaders(prettyName, currentContent);

      console.error(chalk.bold(`${file} contents are not up to date:\n`));
      diff(currentHeaders, newHeaders);
      console.error(chalk.bold(`\nRun node script/gen-libc++-filenames.js to regenerate ${file}`));
      process.exit(1);
    }
  } else {
    console.log(filenamesPath);
    fs.writeFileSync(filenamesPath, content);
  }
}
