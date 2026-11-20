// Builds one of Electron's internal JavaScript bundles (see targets.mjs) with
// rolldown. Driven by the `electron_bundle` GN template in bundle.gni:
//
//   node build/bundle/bundle.mjs --target renderer \
//     --output-path out/gen --output-filename renderer_init.js \
//     --buildflags out/gen/electron/buildflags/buildflags.h --mode production
//
// --print-graph prints the bundle's source files as a JSON array instead of
// writing anything; script/gen-filenames.mts uses it to keep filenames.auto.gni
// in sync.
//
// --sourcemap also writes a source map next to the bundle (<bundle>.map) that
// maps it back to lib/, for script/lib-coverage-report.mjs. The bundle itself
// is unchanged: it carries no sourceMappingURL comment.
//
// Type checking is not done here; see typecheck.mjs.

import { rolldown } from 'rolldown';

import * as fs from 'node:fs';
import { builtinModules } from 'node:module';
import * as path from 'node:path';
import { parseArgs } from 'node:util';

import { targets } from './targets.mjs';

const electronRoot = path.resolve(import.meta.dirname, '..', '..');
const libDir = path.resolve(electronRoot, 'lib');

const { values: args } = parseArgs({
  options: {
    target: { type: 'string' },
    'output-path': { type: 'string' },
    'output-filename': { type: 'string' },
    buildflags: { type: 'string' },
    mode: { type: 'string', default: 'development' },
    sourcemap: { type: 'boolean', default: false },
    'print-graph': { type: 'boolean', default: false }
  },
  strict: true
});

const target = targets[args.target];
if (!target) {
  throw new Error(`Unknown --target '${args.target}'; expected one of ${Object.keys(targets).join(', ')}`);
}
if (!['development', 'production'].includes(args.mode)) {
  throw new Error(`Unknown --mode '${args.mode}'; expected development or production`);
}
const printGraph = args['print-graph'];
if (!printGraph && !(args['output-path'] && args['output-filename'])) {
  throw new Error('--output-path and --output-filename are required');
}

const entry = ['init.ts', 'init.js']
  .map((file) => path.resolve(libDir, args.target, file))
  .find((file) => fs.existsSync(file));

const electronAPIFile = path.resolve(
  libDir,
  target.loadElectronFromAlternateTarget ?? args.target,
  'api',
  'exports',
  'electron.ts'
);

// Parses the BUILDFLAG(...) values out of the generated buildflags.h so that
// `BUILDFLAG(NAME)` in lib/ folds to a boolean literal, the same way it does
// in C++.
const buildflags = new Map();
if (args.buildflags) {
  const flagFile = fs.readFileSync(args.buildflags, 'utf8');
  for (const [, name, value] of flagFile.matchAll(/#define BUILDFLAG_INTERNAL_(.+?)\(\) \(([01])\)/g)) {
    buildflags.set(name, value === '1');
  }
}

const buildflagPlugin = {
  name: 'electron-buildflag',
  transform: {
    filter: { code: 'BUILDFLAG(' },
    handler(code, id) {
      // Without a buildflags.h (--print-graph, or a plain compile check) the
      // call is left for the BUILDFLAG declaration in typings/ to satisfy.
      if (!args.buildflags) return null;
      return code.replace(/\bBUILDFLAG\(\s*([A-Za-z0-9_]+)\s*\)/g, (_, name) => {
        if (!buildflags.has(name)) {
          this.error(`${path.relative(electronRoot, id)}: BUILDFLAG(${name}) is not defined in ${args.buildflags}`);
        }
        return `(${buildflags.get(name)})`;
      });
    }
  }
};

// `electron` and its process-specific entry points all resolve to this
// target's API module list; bundles without Node.js get a native EventEmitter
// for `events`.
const exactAliases = new Map(
  ['electron', 'electron/main', 'electron/renderer', 'electron/common', 'electron/utility'].map((id) => [
    id,
    electronAPIFile
  ])
);
if (!target.alwaysHasNode) {
  exactAliases.set('events', path.resolve(libDir, 'common', 'node-events.ts'));
}
const aliasPlugin = {
  name: 'electron-alias',
  resolveId: {
    filter: { id: new RegExp(`^(${[...exactAliases.keys()].join('|')})$`) },
    handler(id) {
      return exactAliases.get(id) ?? null;
    }
  }
};

// The source files that ended up in the module graph, for --print-graph.
const graphFiles = new Set();
const graphPlugin = {
  name: 'electron-graph',
  buildEnd() {
    for (const id of this.getModuleIds()) {
      if (path.isAbsolute(id)) graphFiles.add(path.relative(electronRoot, id).replaceAll(path.sep, '/'));
    }
  }
};

// Free variables that are rewritten to imports of an internal module, like
// webpack's ProvidePlugin. Every bundle captures the original Promise so that
// user land replacing the global does not affect Electron.
const inject = {
  Promise: ['@electron/internal/common/primordials', 'Promise']
};
if (target.targetDeletesNodeGlobals) {
  // See the Module.wrapper override in lib/renderer/init.ts.
  Object.assign(inject, {
    Buffer: ['@electron/internal/common/node-globals', 'Buffer'],
    global: ['@electron/internal/common/node-globals', '_global'],
    process: ['@electron/internal/common/node-globals', 'process']
  });
}
if (!target.alwaysHasNode) {
  inject.process = ['@electron/internal/webview/process', 'default'];
}

// There is no Node.js `global` in a sandboxed renderer, but code shared with
// the Node.js bundles may still refer to it.
const define = target.alwaysHasNode ? {} : { global: 'globalThis' };

// In a Node.js environment the bundle is called with Node's internal
// `require`, which serves both public built-ins and lib/internal/* modules.
// A plain external `import` would be hoisted to a require() at the very top of
// the bundle; routing it through a one-line CommonJS shim instead keeps the
// require() lazy, so a built-in is only loaded once the (lazily evaluated)
// module importing it actually runs, as it was with webpack.
const nodeModules = new Set(builtinModules);
const nodeShimPrefix = '\0electron-node-external:';
const nodeExternalsPlugin = {
  name: 'electron-node-externals',
  resolveId: {
    filter: { id: /^[a-z0-9_:/]+$/ },
    handler(source, importer, { kind }) {
      // That require knows nothing of the node: scheme.
      const id = source.replace(/^node:/, '');
      if (!nodeModules.has(id) && !id.startsWith('internal/')) return null;
      if (!target.alwaysHasNode) {
        this.error(`'${id}' imported by ${path.relative(electronRoot, importer)} is not available without Node.js`);
      }
      if (kind === 'require-call') return { id, external: true };
      return { id: `${nodeShimPrefix}${id}`, moduleSideEffects: false };
    }
  },
  load: {
    filter: { id: /^\0electron-node-external:/ },
    handler(id) {
      return `module.exports = require(${JSON.stringify(id.slice(nodeShimPrefix.length))});`;
    }
  }
};

const bundle = await rolldown({
  input: entry,
  cwd: electronRoot,
  platform: target.alwaysHasNode ? 'node' : 'browser',
  tsconfig: path.resolve(electronRoot, 'tsconfig.electron.json'),
  resolve: {
    alias: { '@electron/internal': libDir },
    extensions: ['.ts', '.js']
  },
  transform: { define, inject },
  plugins: [aliasPlugin, nodeExternalsPlugin, buildflagPlugin, graphPlugin],
  // Anything rolldown feels the need to warn about (an import it could not
  // resolve, a missing export, ...) would be a broken bundle at runtime.
  onLog(level, log, defaultHandler) {
    if (level === 'warn') level = 'error';
    defaultHandler(level, log);
  }
});

const outputFilename = args['output-filename'] ?? `${args.target}.bundle.js`;

// GN passes --mode=production for official builds; that only decides whether
// the output is minified so testing builds run the same module graph as
// releases.
const minify =
  args.mode === 'production'
    ? {
        compress: { keepNames: { function: true, class: true } },
        mangle: { toplevel: true, keepNames: true }
      }
    : false;

const { output } = await bundle.generate({
  // The bundles are compiled as function bodies (see util::CompileBundle), the
  // Node.js ones with a `require` parameter that external imports are left as
  // calls to. Nothing provides `module` or `exports`.
  format: 'cjs',
  exports: 'none',
  codeSplitting: false,
  // Function and class names show up in stack traces and in the API
  // (e.g. BrowserWindow.name), keep them through scope hoisting renames.
  keepNames: true,
  minify,
  // Added below, outside the wrappers.
  strict: false,
  // 'hidden' leaves the code exactly as without a map (no sourceMappingURL).
  sourcemap: args.sourcemap ? 'hidden' : false,
  // Name the sources relative to the repository (lib/...), whatever the
  // output directory.
  sourcemapPathTransform: (source, sourcemapPath) =>
    path.relative(electronRoot, path.resolve(path.dirname(sourcemapPath), source)).replaceAll(path.sep, '/')
});
await bundle.close();

// With --sourcemap the map also comes back as an asset; chunk.map has it.
const [chunk, ...rest] = output.filter((file) => !(file.type === 'asset' && file.fileName.endsWith('.map')));

if (rest.length) {
  throw new Error(`Expected a single output chunk, got ${1 + rest.length}`);
}
if (!target.alwaysHasNode && chunk.imports.length + chunk.dynamicImports.length) {
  throw new Error(
    `The ${args.target} bundle has no require() at runtime but imports ${[...chunk.imports, ...chunk.dynamicImports].join(', ')}`
  );
}

if (printGraph) {
  console.log(JSON.stringify([...graphFiles].sort()));
} else {
  // The bundle is compiled as a function body (by Electron with the parameters
  // in shell/common/js2c_bundle_ids.h, and by node_mksnapshot with Node's
  // built-in module parameters: exports, require, module, process,
  // internalBinding, primordials), so keep the hoisted top-level declarations
  // of lib/ in their own scope rather than let one of them redeclare a
  // parameter.
  let code = `(() => {
${chunk.code}
})();
`;
  // Where chunk.code starts in the final bundle, for the source map.
  let wrapperLines = 1;

  if (target.wrapInitWithProfilingTimeout) {
    wrapperLines += 1;
    code = `function ___electron_init__() {
${code}
};
if ((globalThis.process || binding.process).argv.includes("--profile-electron-init")) {
  setTimeout(___electron_init__, 0);
} else {
  ___electron_init__();
}
`;
  }

  if (target.wrapInitWithTryCatch) {
    wrapperLines += 1;
    code = `try {
${code}
} catch (err) {
  console.error('Electron ${outputFilename} script failed to run');
  console.error(err);
}
`;
  }

  code = `'use strict';\n${code}`;
  wrapperLines += 1;

  // Leave an unchanged bundle's mtime alone so that ninja (GN actions are
  // restat) does not regenerate and recompile electron_natives.cc for it.
  const writeIfChanged = (file, contents) => {
    if (!fs.existsSync(file) || fs.readFileSync(file, 'utf8') !== contents) {
      fs.mkdirSync(path.dirname(file), { recursive: true });
      fs.writeFileSync(file, contents);
    }
  };
  const outputFile = path.resolve(args['output-path'], outputFilename);
  writeIfChanged(outputFile, code);

  if (args.sourcemap) {
    const map = chunk.map;
    // Shift the mappings down past the wrapper lines added above.
    map.mappings = ';'.repeat(wrapperLines) + map.mappings;
    map.file = outputFilename;
    writeIfChanged(`${outputFile}.map`, map.toString());
  }
}
