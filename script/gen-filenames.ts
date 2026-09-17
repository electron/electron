import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as path from 'node:path';

const rootPath = path.resolve(__dirname, '..');
const gniPath = path.resolve(__dirname, '../filenames.auto.gni');

const allDocs = fs
  .readdirSync(path.resolve(__dirname, '../docs/api'))
  .map((doc) => `docs/api/${doc}`)
  .concat(fs.readdirSync(path.resolve(__dirname, '../docs/api/structures')).map((doc) => `docs/api/structures/${doc}`));

const typingFiles = fs.readdirSync(path.resolve(__dirname, '../typings')).map((child) => `typings/${child}`);

// Recursively collect files under `dir` matching any of the provided
// extensions. Paths are returned relative to `rootPath` using forward slashes
// so they are consumable from BUILD.gn.
const collectSources = (dir: string, extensions: readonly string[]): string[] => {
  if (!fs.existsSync(dir)) return [];
  const results: string[] = [];
  const walk = (current: string) => {
    for (const entry of fs.readdirSync(current, { withFileTypes: true })) {
      const full = path.join(current, entry.name);
      if (entry.isDirectory()) {
        walk(full);
      } else if (entry.isFile() && extensions.some((ext) => entry.name.endsWith(ext))) {
        results.push(path.relative(rootPath, full).replace(/\\/g, '/'));
      }
    }
  };
  walk(dir);
  return results.sort();
};

// Inputs for the `generate_node_headers` action in BUILD.gn. Any change to
// these files must invalidate the generated node_headers directory, otherwise
// stale copies are left behind after a Node.js or V8 bump (see electron#51091
// fallout). install.py is the source of truth for which headers are copied,
// so it is included explicitly; the rest are enumerated by recursive scan to
// cover headers that install.py may pick up dynamically.
const nodeHeaderSources = Array.from(
  new Set([
    '../third_party/electron_node/tools/install.py',
    ...collectSources(path.resolve(__dirname, '../../third_party/electron_node/src'), ['.h']),
    ...collectSources(path.resolve(__dirname, '../../v8/include'), ['.h', '.inc'])
  ])
).sort();

// Every file the lib/ type check (tsconfig.electron.json) reads from this repo.
const libTypecheckSources = [
  ...collectSources(path.resolve(rootPath, 'lib'), ['.ts', '.js']),
  ...typingFiles,
  'build/bundle/typecheck.mjs',
  'package.json',
  'tsconfig.electron.json',
  'tsconfig.json'
].sort();

const bundleTargets = [
  { name: 'webview_bundle_deps', target: 'webview' },
  { name: 'isolated_bundle_deps', target: 'isolated_renderer' },
  { name: 'browser_bundle_deps', target: 'browser' },
  { name: 'renderer_bundle_deps', target: 'renderer' },
  { name: 'worker_bundle_deps', target: 'worker' },
  { name: 'node_bundle_deps', target: 'node' },
  { name: 'utility_bundle_deps', target: 'utility' }
];

const main = async () => {
  const bundleTargetsWithDeps = await Promise.all(
    bundleTargets.map(async (bundleTarget) => {
      const child = cp.spawn('node', ['./build/bundle/bundle.mjs', '--target', bundleTarget.target, '--print-graph'], {
        cwd: rootPath
      });
      let output = '';
      child.stdout.on('data', (chunk) => {
        output += chunk.toString();
      });
      child.stderr.on('data', (chunk) => console.error(chunk.toString()));
      await new Promise<void>((resolve, reject) =>
        child.on('exit', (code) => {
          if (code !== 0) {
            console.error(output);
            return reject(new Error(`Failed to list bundle dependencies for entry: ${bundleTarget.name}`));
          }

          resolve();
        })
      );

      return {
        ...bundleTarget,
        dependencies: (JSON.parse(output) as string[])
          // Only care about files in //electron
          .filter((line) => !line.startsWith('..'))
          // Only care about our own files
          .filter((line) => !line.startsWith('node_modules'))
          // All bundles depend on the tsconfig and package json files
          .concat(['tsconfig.json', 'tsconfig.electron.json', 'package.json'])
          // Make the generated list easier to read
          .sort()
      };
    })
  );

  const generated = `# THIS FILE IS AUTO-GENERATED, PLEASE DO NOT EDIT BY HAND
auto_filenames = {
  api_docs = [
${allDocs.map((doc) => `    "${doc}",`).join('\n')}
  ]

  node_header_sources = [
${nodeHeaderSources.map((src) => `    "${src}",`).join('\n')}
  ]

  lib_typecheck_sources = [
${libTypecheckSources.map((src) => `    "${src}",`).join('\n')}
  ]

${bundleTargetsWithDeps
  .map(
    (target) => `  ${target.name} = [
${target.dependencies.map((dep) => `    "${dep}",`).join('\n')}
  ]`
  )
  .join('\n\n')}
}
`;

  if (process.argv.includes('--check')) {
    const existing = fs.existsSync(gniPath) ? fs.readFileSync(gniPath, 'utf8') : '';
    if (existing !== generated) {
      console.error(
        `${path.relative(rootPath, gniPath)} is out of date. Run 'node script/gen-filenames.ts' to regenerate.`
      );
      process.exit(1);
    }
  } else {
    fs.writeFileSync(gniPath, generated);
  }
};

if (require.main === module) {
  main().catch((err) => {
    console.error(err);
    process.exit(1);
  });
}
