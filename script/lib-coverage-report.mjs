// Renders the JavaScript coverage of lib/ that `script/spec-runner.js
// --coverage` collected (V8 coverage of the internal/electron/js2c bundles,
// see spec/vitest/electron-pool.ts) as a report on lib/*.ts. Needs the
// bundles' source maps, which a build with `electron_js_sourcemaps = true`
// writes next to them in out/<dir>/gen/internal/electron/js2c.
//
//   node script/lib-coverage-report.mjs [--input <raw dir>] [--output <dir>]
//                                       [--gen-dir <out/<dir>/gen>]
//
// writes an HTML report (index.html), lcov.info, coverage-summary.json and a
// per-file table (summary.txt) to --output and prints a summary.
//
//   node script/lib-coverage-report.mjs --filter <dir> [--input <raw dir>]
//
// instead copies the raw coverage to <dir> with everything but the bundles
// dropped, which is most of it (Node.js' own built-ins, the specs).
//
// The conversion is Vitest's (@vitest/coverage-v8): ast-v8-to-istanbul maps
// each coverage file's V8 ranges for a bundle onto the bundle's syntax tree
// (from rolldown's parser) and through its source map to lib/, and the
// results are merged with istanbul-lib-coverage.

import * as fs from 'node:fs';
import { createRequire } from 'node:module';
import * as path from 'node:path';
import { pathToFileURL } from 'node:url';
import { parseArgs } from 'node:util';

const require = createRequire(import.meta.url);
const { getOutDir, ELECTRON_DIR, SRC_DIR } = require('./lib/utils.js');

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

// Only the report needs these, so --filter runs without node_modules.
const { default: astV8ToIstanbul } = await import('ast-v8-to-istanbul');
const { default: libCoverage } = await import('istanbul-lib-coverage');
const { default: libReport } = await import('istanbul-lib-report');
const { default: reports } = await import('istanbul-reports');
const { parseAst } = await import('rolldown/parseAst');

const genDir = path.resolve(args['gen-dir'] ?? path.join(outDir(), 'gen'));
const bundleDir = path.join(genDir, 'internal', 'electron', 'js2c');
const outputDir = path.resolve(args.output ?? path.join(outDir(), 'lib-coverage', 'report'));

// Each bundle's coverage, one functions list per coverage file that has it.
const bundleCoverage = new Map();
let files = 0;
for (const [, result] of readCoverage()) {
  if (result.length === 0) continue;
  for (const { url, functions } of result) {
    if (!bundleCoverage.has(url)) bundleCoverage.set(url, []);
    bundleCoverage.get(url).push(functions);
  }
  files++;
}
if (files === 0) {
  throw new Error(`No coverage of the bundles in ${inputDir}`);
}

// Like @vitest/coverage-v8 does for Vite's, leaves out the statements
// rolldown adds that map to lib/: the init_*() calls that run an imported
// module where the import was, and the __decorate() (decorators) and __name()
// (keepNames) calls. The rest of what it adds (its runtime helpers, the module
// wrappers, export getters and import bindings) and the wrappers bundle.mjs
// puts around the bundle map to nothing, so the conversion leaves them out by
// itself.
const ignoreNode = (node, type) =>
  type === 'statement' &&
  node.type === 'ExpressionStatement' &&
  node.expression.type === 'CallExpression' &&
  /^(init_|__decorate$|__name$)/.test(node.expression.callee.name);

const coverageMap = libCoverage.createCoverageMap({});
const sourcesContent = new Map();
for (const name of fs.readdirSync(bundleDir)) {
  if (!name.endsWith('.js')) continue;
  const file = path.join(bundleDir, name);
  if (!fs.existsSync(`${file}.map`)) {
    throw new Error(`${file}.map is missing; build with electron_js_sourcemaps = true`);
  }
  // The bundle is compiled as is (as a function body, without a wrapper), so
  // V8's offsets are offsets in this file.
  const code = fs.readFileSync(file, 'utf8');
  const sourceMap = JSON.parse(fs.readFileSync(`${file}.map`, 'utf8'));
  sourceMap.sources.forEach((source, index) => sourcesContent.set(source, sourceMap.sourcesContent?.[index]));

  const url = `${JS2C_PREFIX}${path.basename(name, '.js')}`;
  const coverages = bundleCoverage.get(url) ?? [];
  bundleCoverage.delete(url);
  // A bundle that never ran still counts, as uncovered.
  if (coverages.length === 0) coverages.push([]);
  for (const functions of coverages) {
    const data = await astV8ToIstanbul({
      code,
      sourceMap,
      // The conversion changes the tree, so each needs its own.
      ast: parseAst(code),
      // The map names its sources relative to the repository (lib/...).
      coverage: { url: pathToFileURL(path.join(ELECTRON_DIR, name)).href, functions },
      ignoreNode
    });
    for (const [filePath, fileCoverage] of Object.entries(data)) {
      const source = path.relative(ELECTRON_DIR, filePath).replaceAll(path.sep, '/');
      if (!source.startsWith('lib/') || !source.endsWith('.ts')) continue;
      coverageMap.merge({ [source]: { ...fileCoverage, path: source } });
    }
  }
}
if (bundleCoverage.size > 0) {
  throw new Error(`No bundle in ${bundleDir} for ${[...bundleCoverage.keys()].join(', ')}`);
}

const context = libReport.createContext({
  dir: outputDir,
  coverageMap,
  defaultSummarizer: 'nested',
  // The paths are relative to the repository, and the maps carry the sources.
  sourceFinder: (filePath) =>
    sourcesContent.get(filePath) ?? fs.readFileSync(path.resolve(ELECTRON_DIR, filePath), 'utf8')
});
fs.rmSync(outputDir, { recursive: true, force: true });
for (const [name, options] of [
  ['html', {}],
  ['lcovonly', { file: 'lcov.info' }],
  ['json-summary', {}],
  ['text', { file: 'summary.txt' }],
  ['text-summary', {}]
]) {
  reports.create(name, options).execute(context);
}
console.log(`Merged ${files} coverage files from ${inputDir} into ${outputDir}`);
