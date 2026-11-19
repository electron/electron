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
// How the bundles' coverage becomes lib/'s:
// - Every coverage file (one per process, more where coverage was taken more
//   than once, which resets the counts) gives each offset in a bundle the
//   count of the innermost range around it; V8 leaves out ranges whose count
//   is their parent's, and has only the function's own range for functions
//   compiled before coverage started (best-effort coverage), so all of such a
//   function gets its count. The counts are summed over the files.
// - A line of lib/ counts when some code in a bundle maps to it, so comments,
//   type-only lines and closing brackets don't; its count is the highest of
//   that code's. Each line is a statement in the report.
// - The functions are the ones V8 reports, less the bundles' own wrappers.
//   Functions inside code that was never compiled (including bundles that
//   never ran) are not reported, so they are not counted either.
// - There is no branch data (the report shows 0/0): which branches V8 reports
//   depends on the counts.

import * as fs from 'node:fs';
import { createRequire, SourceMap } from 'node:module';
import * as path from 'node:path';
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
const { default: libCoverage } = await import('istanbul-lib-coverage');
const { default: libReport } = await import('istanbul-lib-report');
const { default: reports } = await import('istanbul-reports');

const genDir = path.resolve(args['gen-dir'] ?? path.join(outDir(), 'gen'));
const bundleDir = path.join(genDir, 'internal', 'electron', 'js2c');
const outputDir = path.resolve(args.output ?? path.join(outDir(), 'lib-coverage', 'report'));

const isLibSource = (source) => source.startsWith('lib/') && source.endsWith('.ts');

// The first index in the sorted array for which below() is false.
function partition(array, below) {
  let low = 0;
  let high = array.length;
  while (low < high) {
    const middle = (low + high) >>> 1;
    if (below(array[middle])) low = middle + 1;
    else high = middle;
  }
  return low;
}

// The bundles as the coverage saw them, with the maps back to lib/.
const bundles = new Map();
const sourcesContent = new Map();
for (const name of fs.readdirSync(bundleDir)) {
  if (!name.endsWith('.js')) continue;
  const file = path.join(bundleDir, name);
  if (!fs.existsSync(`${file}.map`)) {
    throw new Error(`${file}.map is missing; build with electron_js_sourcemaps = true`);
  }
  const code = fs.readFileSync(file, 'utf8');
  const sourceMap = new SourceMap(JSON.parse(fs.readFileSync(`${file}.map`, 'utf8')));
  const { sources, sourcesContent: contents = [] } = sourceMap.payload;
  sources.forEach((source, index) => {
    if (contents[index] != null) sourcesContent.set(source, contents[index]);
  });

  const lineStarts = [0];
  for (let index = code.indexOf('\n'); index !== -1; index = code.indexOf('\n', index + 1)) {
    lineStarts.push(index + 1);
  }
  // Every mapping in the bundle, in order. SourceMap has no way to list them,
  // but findEntry() at each position finds the one it falls in.
  const mappings = [];
  for (let line = 0; line < lineStarts.length; line++) {
    const length = (lineStarts[line + 1] ?? code.length + 1) - 1 - lineStarts[line];
    for (let column = 0; column < length; column++) {
      const entry = sourceMap.findEntry(line, column);
      if (entry.generatedLine !== line || entry.generatedColumn !== column || !entry.originalSource) continue;
      // A closing bracket is not code that runs (and V8 counts a function's
      // last brace with the function, after an early return too).
      if ('})]'.includes(code[lineStarts[line] + column])) continue;
      mappings.push({
        offset: lineStarts[line] + column,
        lineStart: lineStarts[line],
        source: entry.originalSource,
        line: entry.originalLine + 1,
        column: entry.originalColumn
      });
    }
  }

  // The calls that define the getters for a module's exports, which map to
  // the exported names.
  const exportGetters = [];
  for (let index = code.indexOf('__exportAll({'); index !== -1; index = code.indexOf('__exportAll({', index + 1)) {
    exportGetters.push([index, code.indexOf('});', index)]);
  }

  bundles.set(`${JS2C_PREFIX}${path.basename(name, '.js')}`, {
    code,
    mappings,
    exportGetters,
    // Summed over the coverage files, per UTF-16 offset as V8 counts.
    counts: new Float64Array(code.length),
    scratch: new Float64Array(code.length),
    // `${startOffset}:${endOffset}` -> { functionName, startOffset, endOffset, count }
    functions: new Map()
  });
}

function addCoverage({ url, functions }) {
  const bundle = bundles.get(url);
  if (!bundle) throw new Error(`No bundle in ${bundleDir} for ${url}`);

  // The ranges nest, so painting them outermost first leaves each offset with
  // its innermost range's count.
  const ranges = functions
    .flatMap((fn) => fn.ranges)
    .sort((a, b) => a.startOffset - b.startOffset || b.endOffset - a.endOffset);
  const { counts, scratch } = bundle;
  scratch.fill(0);
  for (const { startOffset, endOffset, count } of ranges) {
    scratch.fill(count, startOffset, endOffset);
  }
  for (let offset = 0; offset < counts.length; offset++) {
    counts[offset] += scratch[offset];
  }

  for (const {
    functionName,
    ranges: [{ startOffset, endOffset, count }]
  } of functions) {
    const key = `${startOffset}:${endOffset}`;
    const fn = bundle.functions.get(key);
    if (fn) {
      fn.count += count;
    } else {
      bundle.functions.set(key, { functionName, startOffset, endOffset, count });
    }
  }
}

let files = 0;
for (const [, result] of readCoverage()) {
  if (result.length === 0) continue;
  result.forEach(addCoverage);
  files++;
}
if (files === 0) {
  throw new Error(`No coverage of the bundles in ${inputDir}`);
}

// lib/ file -> its lines (line -> count) and functions (`line:column` -> ...).
// A bundle that never ran still counts, as uncovered.
const libFiles = new Map();
const libFile = (source) => {
  if (!libFiles.has(source)) libFiles.set(source, { lines: new Map(), functions: new Map() });
  return libFiles.get(source);
};

for (const bundle of bundles.values()) {
  const { code, counts, mappings, exportGetters } = bundle;
  const countAt = (offset) => counts[offset] ?? 0;

  // A source line can be several mappings in one bundle (take the highest
  // count) and appear in several bundles (add them up).
  const bundleLines = new Map();
  for (const { offset, source, line, column } of mappings) {
    if (!isLibSource(source)) continue;
    const key = `${source}:${line}`;
    const known = bundleLines.get(key);
    if (known) {
      known.count = Math.max(known.count, countAt(offset));
      known.column = Math.min(known.column, column);
    } else {
      bundleLines.set(key, { source, line, column, count: countAt(offset) });
    }
  }
  for (const { source, line, column, count } of bundleLines.values()) {
    const { lines } = libFile(source);
    const known = lines.get(line);
    lines.set(line, {
      count: (known?.count ?? 0) + count,
      column: Math.min(known?.column ?? column, column)
    });
  }

  for (const { functionName, startOffset, endOffset, count } of bundle.functions.values()) {
    // The bundle's own function, around everything, and the ones V8 makes up
    // (<instance_members_initializer> for class fields).
    if ((startOffset === 0 && endOffset >= code.length - 1) || functionName.startsWith('<')) continue;
    // Where the function starts, unless that is code the bundler added (the
    // wrappers it puts around each module), which has no mapping of its own
    // on that line.
    const start = mappings[partition(mappings, (mapping) => mapping.offset <= startOffset) - 1];
    const lineStart = code.lastIndexOf('\n', startOffset - 1) + 1;
    if (!start || start.lineStart !== lineStart || !isLibSource(start.source)) continue;
    // Nor the getters it defines for a module's exports.
    if (exportGetters.some(([from, to]) => from <= startOffset && startOffset < to)) continue;
    const last = mappings[partition(mappings, (mapping) => mapping.offset < endOffset) - 1];
    const end = last?.source === start.source && last.line >= start.line ? last : start;

    const key = `${start.line}:${start.column}`;
    const { functions } = libFile(start.source);
    const known = functions.get(key);
    if (known) {
      known.count += count;
    } else {
      functions.set(key, { name: functionName, start, end, count });
    }
  }
}

// As istanbul's file coverage: a statement per line, the functions, no
// branches.
const lineLength = (source, line) => sourcesContent.get(source)?.split('\n')[line - 1]?.length ?? 0;
const coverageMap = libCoverage.createCoverageMap({});
for (const [source, { lines, functions }] of [...libFiles].sort(([a], [b]) => a.localeCompare(b))) {
  const data = { path: source, statementMap: {}, s: {}, fnMap: {}, f: {}, branchMap: {}, b: {} };
  [...lines]
    .sort(([a], [b]) => a - b)
    .forEach(([line, { count, column }], index) => {
      data.statementMap[index] = {
        start: { line, column },
        end: { line, column: Math.max(column + 1, lineLength(source, line)) }
      };
      data.s[index] = count;
    });
  const names = new Map();
  [...functions.values()]
    .sort((a, b) => a.start.line - b.start.line || a.start.column - b.start.column)
    .forEach(({ name, start, end, count }, index) => {
      // lcov tells functions apart by name.
      const base = name || '(anonymous)';
      const seen = names.get(base) ?? 0;
      names.set(base, seen + 1);
      const decl = {
        start: { line: start.line, column: start.column },
        end: { line: start.line, column: Math.max(start.column + 1, lineLength(source, start.line)) }
      };
      data.fnMap[index] = {
        name: seen ? `${base}_${seen}` : base,
        decl,
        loc: { start: decl.start, end: { line: end.line, column: end.column } },
        line: start.line
      };
      data.f[index] = count;
    });
  coverageMap.addFileCoverage(data);
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
