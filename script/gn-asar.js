const asar = require('@electron/asar');
const assert = require('node:assert');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const getArgGroup = (name) => {
  const group = [];
  let inGroup = false;
  for (const arg of process.argv) {
    // At the next flag we stop being in the current group
    if (arg.startsWith('--')) inGroup = false;
    // Push all args in the group
    if (inGroup) group.push(arg);
    // If we find the start flag, start pushing
    if (arg === `--${name}`) inGroup = true;
  }

  return group;
};

const base = getArgGroup('base');
const files = getArgGroup('files');
const out = getArgGroup('out');

assert(base.length === 1, 'should have a single base dir');
assert(files.length >= 1, 'should have at least one input file');
assert(out.length === 1, 'should have a single out path');

// Ensure all files are inside the base dir
for (const file of files) {
  if (!file.startsWith(base[0])) {
    console.error(`Expected all files to be inside the base dir but "${file}" was not in "${base[0]}"`);
    process.exit(1);
  }
}

const tmpPath = fs.mkdtempSync(path.resolve(os.tmpdir(), 'electron-gn-asar-'));

try {
  // Copy all files to a tmp dir to avoid including scrap files in the ASAR
  for (const file of files) {
    const newLocation = path.resolve(tmpPath, path.relative(base[0], file));
    fs.mkdirSync(path.dirname(newLocation), { force: true, recursive: true });
    fs.writeFileSync(newLocation, fs.readFileSync(file));
  }

  // Create the ASAR archive
  asar.createPackageWithOptions(tmpPath, out[0], {});
} catch (err) {
  console.error('Unexpected error while generating ASAR', err);
  process.exitCode = 1;
} finally {
  fs.rmSync(tmpPath, { force: true, recursive: true });
}
