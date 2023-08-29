const fs = require('node:fs');
const path = require('node:path');

const check = process.argv.includes('--check');

const dictsPath = path.resolve(__dirname, '..', '..', 'third_party', 'hunspell_dictionaries');
const gclientPath = 'third_party/hunspell_dictionaries';

const allFiles = fs.readdirSync(dictsPath);

const dictionaries = allFiles
  .filter(file => path.extname(file) === '.bdic');

const licenses = allFiles
  .filter(file => file.startsWith('LICENSE') || file.startsWith('COPYING'));

const content = `hunspell_dictionaries = [
  ${dictionaries.map(f => `"//${path.posix.join(gclientPath, f)}"`).join(',\n  ')},
]

hunspell_licenses = [
  ${licenses.map(f => `"//${path.posix.join(gclientPath, f)}"`).join(',\n  ')},
]
`;

const filenamesPath = path.resolve(__dirname, '..', 'filenames.hunspell.gni');

if (check) {
  const currentContent = fs.readFileSync(filenamesPath, 'utf8');
  if (currentContent !== content) {
    throw new Error('hunspell filenames need to be regenerated, latest generation does not match current file.  Please run node gen-hunspell-filenames.js');
  }
} else {
  fs.writeFileSync(filenamesPath, content);
}
