#!/usr/bin/env node
// Driver script that replaces webpack for building Electron's internal
// JS bundles. Each bundle is a single esbuild invocation parameterized by
// the per-target configuration files under build/esbuild/configs.
//
// Invoked by the GN `esbuild_build` template via `npm run bundle -- …`.

'use strict';

const esbuild = require('esbuild');

const fs = require('node:fs');
const path = require('node:path');

const electronRoot = path.resolve(__dirname, '../..');

function parseArgs (argv) {
  const args = {};
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a.startsWith('--')) {
      const key = a.slice(2);
      const next = argv[i + 1];
      if (next === undefined || next.startsWith('--')) {
        args[key] = true;
      } else {
        args[key] = next;
        i++;
      }
    }
  }
  return args;
}

// Parse $target_gen_dir/buildflags/buildflags.h (a C++ header containing
// `#define BUILDFLAG_INTERNAL_NAME() (0|1)` lines) into a map of flag name
// to boolean. Used to seed the `define` table so that `BUILDFLAG(NAME)` call
// sites can be statically folded to `true`/`false` at build time.
function parseBuildflags (buildflagsPath) {
  const flags = {};
  if (!buildflagsPath) return flags;
  const source = fs.readFileSync(buildflagsPath, 'utf8');
  const re = /#define BUILDFLAG_INTERNAL_(.+?)\(\) \(([01])\)/g;
  let match;
  while ((match = re.exec(source)) !== null) {
    const [, name, value] = match;
    flags[name] = value === '1';
  }
  return flags;
}

// Return the list of esbuild `alias` entries used by every bundle. esbuild's
// alias matches the full module specifier (no `$` suffix trickery like
// webpack), so the bare `electron` alias also matches `electron/main`, etc.,
// because esbuild matches the leftmost segment first.
function buildAliases (electronAPIFile, { aliasTimers }) {
  const aliases = {
    electron: electronAPIFile,
    'electron/main': electronAPIFile,
    'electron/renderer': electronAPIFile,
    'electron/common': electronAPIFile,
    'electron/utility': electronAPIFile
  };
  // Only browser-platform bundles (sandboxed_renderer, isolated_renderer,
  // preload_realm) need the timers shim — Node's `timers` builtin is not
  // available there. For node-platform bundles (browser, renderer, worker,
  // utility, node) the alias MUST NOT apply: lib/common/init.ts wraps the
  // real Node timers and then assigns the wrappers onto globalThis. If
  // those bundles saw the shim, the wrappers would recursively call back
  // into globalThis.setTimeout and blow the stack.
  if (aliasTimers) {
    aliases.timers = path.resolve(electronRoot, 'lib', 'common', 'timers-shim.ts');
  }
  return aliases;
}

// esbuild's `alias` does not support wildcard prefixes like `@electron/internal/*`.
// We instead install a tiny resolve plugin that rewrites any import starting
// with that prefix to an absolute path under `lib/`. The plugin must also
// replicate esbuild's extension/index resolution because returning a path
// from onResolve bypasses the default resolver.
function internalAliasPlugin () {
  const candidates = (base) => [
    base,
    `${base}.ts`,
    `${base}.js`,
    path.join(base, 'index.ts'),
    path.join(base, 'index.js')
  ];
  return {
    name: 'electron-internal-alias',
    setup (build) {
      build.onResolve({ filter: /^@electron\/internal(\/|$)/ }, (args) => {
        // Tolerate stray double slashes in import paths (webpack was lenient).
        const rel = args.path.replace(/^@electron\/internal\/?/, '').replace(/^\/+/, '');
        const base = path.resolve(electronRoot, 'lib', rel);
        for (const c of candidates(base)) {
          try {
            if (fs.statSync(c).isFile()) return { path: c };
          } catch { /* keep looking */ }
        }
        return { errors: [{ text: `Cannot resolve @electron/internal path: ${args.path}` }] };
      });
    }
  };
}

// Rewrites `BUILDFLAG(NAME)` call-sites to `(true)` or `(false)` at load
// time, equivalent to the combination of webpack's DefinePlugin substitution
// (BUILDFLAG -> "" and NAME -> "true"/"false") that the old config used.
// Doing it in a single regex pass keeps the semantics identical and avoids
// fighting with esbuild's AST-level `define` quoting rules.
function buildflagPlugin (buildflags, { allowUnknown = false } = {}) {
  return {
    name: 'electron-buildflag',
    setup (build) {
      build.onLoad({ filter: /\.(ts|js)$/ }, async (args) => {
        const source = await fs.promises.readFile(args.path, 'utf8');
        if (!source.includes('BUILDFLAG(')) {
          return { contents: source, loader: args.path.endsWith('.ts') ? 'ts' : 'js' };
        }
        const rewritten = source.replace(/BUILDFLAG\(([A-Z0-9_]+)\)/g, (_, name) => {
          if (!Object.prototype.hasOwnProperty.call(buildflags, name)) {
            if (allowUnknown) return '(false)';
            throw new Error(`Unknown BUILDFLAG: ${name} (in ${args.path})`);
          }
          return `(${buildflags[name]})`;
        });
        return { contents: rewritten, loader: args.path.endsWith('.ts') ? 'ts' : 'js' };
      });
    }
  };
}

// TODO(MarshallOfSound): drop this patch once evanw/esbuild#4441 lands and
// we bump esbuild — that PR adds a `__toCommonJSCached` helper for the
// inline-require path so identity is preserved upstream. Tracked at
// https://github.com/evanw/esbuild/issues/4440.
//
// esbuild's runtime emits `__toCommonJS = (mod) => __copyProps(__defProp({},
// "__esModule", { value: true }), mod)`, which allocates a fresh wrapper
// object every time `require()` resolves to a bundled ESM module. That
// breaks identity expectations our code relies on (e.g. sandboxed preloads
// expecting `require('timers') === require('node:timers')`, and the
// defineProperties getters in lib/common/define-properties.ts expecting
// stable namespaces). A cached WeakMap-backed version of __toCommonJS
// existed in older esbuild releases (see evanw/esbuild#2126) but was
// removed in evanw/esbuild@f4ff26d3 (0.14.27). Substitute a memoized
// variant in post-processing so every call site returns the same wrapper
// for the same underlying namespace, matching webpack's
// `__webpack_require__` cache semantics.
const ESBUILD_TO_COMMONJS_PATTERN =
  /var __toCommonJS = \(mod\) => __copyProps\(__defProp\(\{\}, "__esModule", \{ value: true \}\), mod\);/;
const ESBUILD_TO_COMMONJS_REPLACEMENT =
  'var __toCommonJS = /* @__PURE__ */ ((cache) => (mod) => {\n' +
  '    var cached = cache.get(mod);\n' +
  '    if (cached) return cached;\n' +
  '    var result = __copyProps(__defProp({}, "__esModule", { value: true }), mod);\n' +
  '    cache.set(mod, result);\n' +
  '    return result;\n' +
  '  })(new WeakMap());';

function patchToCommonJS (source) {
  // Once evanw/esbuild#4441 lands, esbuild will emit `__toCommonJSCached`
  // for inline require() — when we see that helper in the output, the
  // upstream fix is active and this whole patch is a no-op (and should be
  // deleted on the next esbuild bump).
  if (source.includes('__toCommonJSCached')) {
    return source;
  }
  if (!ESBUILD_TO_COMMONJS_PATTERN.test(source)) {
    // Some bundles may not contain any ESM-shaped modules, in which case
    // esbuild omits the helper entirely and there is nothing to patch.
    if (source.includes('__toCommonJS')) {
      throw new Error(
        'esbuild bundle contains __toCommonJS but did not match the ' +
        'expected pattern; the runtime helper has likely changed upstream. ' +
        'Update ESBUILD_TO_COMMONJS_PATTERN in build/esbuild/bundle.js, or ' +
        'delete patchToCommonJS entirely if evanw/esbuild#4441 has landed.'
      );
    }
    return source;
  }
  return source.replace(ESBUILD_TO_COMMONJS_PATTERN, ESBUILD_TO_COMMONJS_REPLACEMENT);
}

// Wrap bundle source text in the same header/footer pairs webpack's
// wrapper-webpack-plugin used. The try/catch wrapper is load-bearing:
// shell/common/node_util.cc's CompileAndCall relies on it to prevent
// exceptions from tearing down bootstrap.
function applyWrappers (source, opts, outputFilename) {
  let wrapped = patchToCommonJS(source);
  if (opts.wrapInitWithProfilingTimeout) {
    const header = 'function ___electron_webpack_init__() {';
    const footer = '\n};\nif ((globalThis.process || binding.process).argv.includes("--profile-electron-init")) {\n  setTimeout(___electron_webpack_init__, 0);\n} else {\n  ___electron_webpack_init__();\n}';
    wrapped = header + wrapped + footer;
  }
  if (opts.wrapInitWithTryCatch) {
    const header = 'try {';
    const footer = `\n} catch (err) {\n  console.error('Electron ${outputFilename} script failed to run');\n  console.error(err);\n}`;
    wrapped = header + wrapped + footer;
  }
  return wrapped;
}

async function buildBundle (opts, cliArgs) {
  const {
    target,
    alwaysHasNode,
    loadElectronFromAlternateTarget,
    wrapInitWithProfilingTimeout,
    wrapInitWithTryCatch
  } = opts;

  const outputFilename = cliArgs['output-filename'] || `${target}.bundle.js`;
  const outputPath = cliArgs['output-path'] || path.resolve(electronRoot, 'out');
  const mode = cliArgs.mode || 'development';
  const minify = mode === 'production';
  const printGraph = !!cliArgs['print-graph'];

  let entry = path.resolve(electronRoot, 'lib', target, 'init.ts');
  if (!fs.existsSync(entry)) {
    entry = path.resolve(electronRoot, 'lib', target, 'init.js');
  }

  const electronAPIFile = path.resolve(
    electronRoot,
    'lib',
    loadElectronFromAlternateTarget || target,
    'api',
    'exports',
    'electron.ts'
  );

  const buildflags = parseBuildflags(cliArgs.buildflags);

  // Shims that stand in for webpack ProvidePlugin. Each target gets the
  // minimum set of globals it needs; the capture files mirror the originals
  // under lib/common so the behavior (grab globals before user code can
  // delete them) is preserved exactly.
  const inject = [];
  if (opts.targetDeletesNodeGlobals) {
    inject.push(path.resolve(__dirname, 'shims', 'node-globals-shim.js'));
  }
  if (!alwaysHasNode) {
    inject.push(path.resolve(__dirname, 'shims', 'browser-globals-shim.js'));
  }
  inject.push(path.resolve(__dirname, 'shims', 'promise-shim.js'));

  const result = await esbuild.build({
    entryPoints: [entry],
    bundle: true,
    format: 'iife',
    platform: alwaysHasNode ? 'node' : 'browser',
    target: 'es2022',
    minify,
    // Preserve class/function names in both development and production so
    // gin_helper-surfaced constructor names and stack traces stay readable.
    // (Under webpack this only mattered when terser ran in is_official_build;
    // esbuild applies the same rename pressure in dev too, so keep it on
    // unconditionally for consistency.)
    keepNames: true,
    sourcemap: false,
    logLevel: 'warning',
    metafile: true,
    write: false,
    resolveExtensions: ['.ts', '.js'],
    alias: buildAliases(electronAPIFile, { aliasTimers: !alwaysHasNode }),
    inject,
    define: {
      __non_webpack_require__: 'require'
    },
    // Node internal modules we pull through __non_webpack_require__ at runtime.
    // These must not be bundled — esbuild should leave the literal require()
    // call alone so the outer Node scope resolves them.
    external: [
      'internal/modules/helpers',
      'internal/modules/run_main',
      'internal/fs/utils',
      'internal/util',
      'internal/validators',
      'internal/url'
    ],
    plugins: [
      internalAliasPlugin(),
      buildflagPlugin(buildflags, { allowUnknown: printGraph })
    ]
  });

  if (printGraph) {
    const inputs = Object.keys(result.metafile.inputs)
      .filter((p) => !p.includes('node_modules') && !p.startsWith('..'))
      .map((p) => path.relative(electronRoot, path.resolve(electronRoot, p)));
    process.stdout.write(JSON.stringify(inputs) + '\n');
    return;
  }

  if (result.outputFiles.length !== 1) {
    throw new Error(`Expected exactly one output file, got ${result.outputFiles.length}`);
  }

  const wrapped = applyWrappers(
    result.outputFiles[0].text,
    { wrapInitWithProfilingTimeout, wrapInitWithTryCatch },
    outputFilename
  );

  await fs.promises.mkdir(outputPath, { recursive: true });
  await fs.promises.writeFile(path.join(outputPath, outputFilename), wrapped);
}

async function main () {
  const cliArgs = parseArgs(process.argv.slice(2));
  if (!cliArgs.config) {
    console.error('Usage: bundle.js --config <path> [--output-filename X] [--output-path Y] [--mode development|production] [--buildflags path/to/buildflags.h] [--print-graph]');
    process.exit(1);
  }
  const configPath = path.resolve(cliArgs.config);
  const opts = require(configPath);
  await buildBundle(opts, cliArgs);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
