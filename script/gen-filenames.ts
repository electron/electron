import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
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
const collectHeaderSources = (dir: string, extensions: readonly string[]): string[] => {
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
const nodeHeaderSources = Array.from(new Set([
  '../third_party/electron_node/tools/install.py',
  ...collectHeaderSources(
    path.resolve(__dirname, '../../third_party/electron_node/src'),
    ['.h']
  ),
  ...collectHeaderSources(
    path.resolve(__dirname, '../../v8/include'),
    ['.h', '.inc']
  )
])).sort();

const main = async () => {
  const webpackTargets = [
    {
      name: 'sandbox_bundle_deps',
      config: 'webpack.config.sandboxed_renderer.js'
    },
    {
      name: 'isolated_bundle_deps',
      config: 'webpack.config.isolated_renderer.js'
    },
    {
      name: 'browser_bundle_deps',
      config: 'webpack.config.browser.js'
    },
    {
      name: 'renderer_bundle_deps',
      config: 'webpack.config.renderer.js'
    },
    {
      name: 'worker_bundle_deps',
      config: 'webpack.config.worker.js'
    },
    {
      name: 'node_bundle_deps',
      config: 'webpack.config.node.js'
    },
    {
      name: 'utility_bundle_deps',
      config: 'webpack.config.utility.js'
    },
    {
      name: 'preload_realm_bundle_deps',
      config: 'webpack.config.preload_realm.js'
    }
  ];

  const webpackTargetsWithDeps = await Promise.all(
    webpackTargets.map(async (webpackTarget) => {
      const tmpDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'electron-filenames-'));
      const child = cp.spawn(
        'node',
        [
          './node_modules/webpack-cli/bin/cli.js',
          '--config',
          `./build/webpack/${webpackTarget.config}`,
          '--stats',
          'errors-only',
          '--output-path',
          tmpDir,
          '--output-filename',
          `${webpackTarget.name}.measure.js`,
          '--env',
          'PRINT_WEBPACK_GRAPH'
        ],
        {
          cwd: path.resolve(__dirname, '..')
        }
      );
      let output = '';
      child.stdout.on('data', (chunk) => {
        output += chunk.toString();
      });
      child.stderr.on('data', (chunk) => console.error(chunk.toString()));
      await new Promise<void>((resolve, reject) =>
        child.on('exit', (code) => {
          if (code !== 0) {
            console.error(output);
            return reject(new Error(`Failed to list webpack dependencies for entry: ${webpackTarget.name}`));
          }

          resolve();
        })
      );

      const webpackTargetWithDeps = {
        ...webpackTarget,
        dependencies: (JSON.parse(output) as string[])
          // Remove whitespace
          .map((line) => line.trim())
          // Get the relative path
          .map((line) => path.relative(rootPath, line).replace(/\\/g, '/'))
          // Only care about files in //electron
          .filter((line) => !line.startsWith('..'))
          // Only care about our own files
          .filter((line) => !line.startsWith('node_modules'))
          // All webpack builds depend on the tsconfig  and package json files
          .concat(['tsconfig.json', 'tsconfig.electron.json', 'package.json', ...typingFiles])
          // Make the generated list easier to read
          .sort()
      };
      await fs.promises.rm(tmpDir, { force: true, recursive: true });
      return webpackTargetWithDeps;
    })
  );

  fs.writeFileSync(
    gniPath,
    `# THIS FILE IS AUTO-GENERATED, PLEASE DO NOT EDIT BY HAND
auto_filenames = {
  api_docs = [
${allDocs.map((doc) => `    "${doc}",`).join('\n')}
  ]

  node_header_sources = [
${nodeHeaderSources.map((src) => `    "${src}",`).join('\n')}
  ]

${webpackTargetsWithDeps
  .map(
    (target) => `  ${target.name} = [
${target.dependencies.map((dep) => `    "${dep}",`).join('\n')}
  ]`
  )
  .join('\n\n')}
}
`
  );
};

if (require.main === module) {
  main().catch((err) => {
    console.error(err);
    process.exit(1);
  });
}
