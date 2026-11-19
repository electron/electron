// Renders the JavaScript coverage of lib/ that `script/spec-runner.js
// --coverage` collected (V8 coverage of the internal/electron/js2c bundles,
// see spec/vitest/electron-pool.ts) as a report on lib/*.ts. Needs the
// bundles' source maps, which a build with `electron_js_sourcemaps = true`
// writes next to them in out/<dir>/gen/internal/electron/js2c.
//
//   node script/lib-coverage-report.mjs [--input <raw dir>] [--output <dir>]
//                                       [--gen-dir <out/<dir>/gen>]
//
// writes an HTML report (index.html), lcov.info and coverage-summary.json to
// --output and prints a summary.
//
//   node script/lib-coverage-report.mjs --filter <dir> [--input <raw dir>]
//
// instead copies the raw coverage to <dir> with everything but the bundles
// dropped, which is most of it (Node.js' own built-ins, the specs).

import { CoverageReport } from 'monocart-coverage-reports';

import * as fs from 'node:fs';
import { createRequire } from 'node:module';
import * as path from 'node:path';
import { parseArgs } from 'node:util';

const require = createRequire(import.meta.url);
const { getOutDir, SRC_DIR } = require('./lib/utils.js');

const JS2C_PREFIX = 'node:internal/electron/js2c/';

const { values: args } = parseArgs({
  options: {
    input: { type: 'string' },
    output: { type: 'string' },
    'gen-dir': { type: 'string' },
    filter: { type: 'string' }
  },
  strict: true
});

const outDir = () => path.resolve(SRC_DIR, 'out', getOutDir());
const inputDir = path.resolve(args.input ?? path.join(outDir(), 'lib-coverage', 'raw'));

// The js2c entries of each raw coverage file in inputDir.
function* readCoverage() {
  for (const name of fs.readdirSync(inputDir).sort()) {
    if (!name.endsWith('.json')) continue;
    let result;
    try {
      ({ result } = JSON.parse(fs.readFileSync(path.join(inputDir, name), 'utf8')));
    } catch (error) {
      // A process killed while writing it; nothing to be done.
      console.warn(`Skipping ${name}: ${error.message}`);
      continue;
    }
    yield [name, (result ?? []).filter((entry) => entry.url.startsWith(JS2C_PREFIX))];
  }
}

if (args.filter) {
  const filterDir = path.resolve(args.filter);
  fs.mkdirSync(filterDir, { recursive: true });
  let files = 0;
  for (const [name, result] of readCoverage()) {
    if (result.length === 0) continue;
    fs.writeFileSync(path.join(filterDir, name), JSON.stringify({ result }));
    files++;
  }
  console.log(`Kept the bundle coverage of ${files} files in ${filterDir}`);
  process.exit(0);
}

const genDir = path.resolve(args['gen-dir'] ?? path.join(outDir(), 'gen'));
const bundleDir = path.join(genDir, 'internal', 'electron', 'js2c');
const outputDir = path.resolve(args.output ?? path.join(outDir(), 'lib-coverage', 'report'));

// The bundles as the coverage saw them, with the maps back to lib/.
const bundles = new Map();
for (const name of fs.readdirSync(bundleDir)) {
  if (!name.endsWith('.js')) continue;
  const file = path.join(bundleDir, name);
  if (!fs.existsSync(`${file}.map`)) {
    throw new Error(`${file}.map is missing; build with electron_js_sourcemaps = true`);
  }
  bundles.set(`${JS2C_PREFIX}${path.basename(name, '.js')}`, {
    source: fs.readFileSync(file, 'utf8'),
    sourceMap: JSON.parse(fs.readFileSync(`${file}.map`, 'utf8'))
  });
}

const report = new CoverageReport({
  name: 'Electron lib/ coverage',
  outputDir,
  reports: ['v8', 'lcovonly', 'json-summary', 'console-summary'],
  // The map's sources are already relative to the repository.
  sourcePath: (filePath, info) => (info?.url?.startsWith('lib/') ? info.url : filePath),
  sourceFilter: (sourcePath) => sourcePath.startsWith('lib/'),
  cleanCache: true
});

const withSource = (entry) => {
  const bundle = bundles.get(entry.url);
  if (!bundle) throw new Error(`No bundle in ${bundleDir} for ${entry.url}`);
  return { ...entry, ...bundle };
};

let files = 0;
for (const [, result] of readCoverage()) {
  if (result.length === 0) continue;
  await report.add(result.map(withSource));
  files++;
}
if (files === 0) {
  throw new Error(`No coverage of the bundles in ${inputDir}`);
}

// A bundle that never ran still counts, as uncovered.
await report.add(
  [...bundles.keys()].map((url, index) =>
    withSource({
      url,
      scriptId: String(index),
      functions: [
        {
          functionName: '',
          ranges: [{ startOffset: 0, endOffset: bundles.get(url).source.length, count: 0 }],
          isBlockCoverage: true
        }
      ]
    })
  )
);

await report.generate();
console.log(`Merged ${files} coverage files from ${inputDir} into ${outputDir}`);
