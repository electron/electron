import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as path from 'node:path';

const rootPath = path.resolve(__dirname, '..');
const gniPath = path.resolve(__dirname, '../filenames.auto.gni');

const allDocs = fs.readdirSync(path.resolve(__dirname, '../docs/api'))
  .map(doc => `docs/api/${doc}`)
  .concat(
    fs.readdirSync(path.resolve(__dirname, '../docs/api/structures'))
      .map(doc => `docs/api/structures/${doc}`)
  );

const typingFiles = fs.readdirSync(path.resolve(__dirname, '../typings')).map(child => `typings/${child}`);

const main = async () => {
  const bundleTargets = [
    { name: 'sandbox_bundle_deps', config: 'sandboxed_renderer.js' },
    { name: 'isolated_bundle_deps', config: 'isolated_renderer.js' },
    { name: 'browser_bundle_deps', config: 'browser.js' },
    { name: 'renderer_bundle_deps', config: 'renderer.js' },
    { name: 'worker_bundle_deps', config: 'worker.js' },
    { name: 'node_bundle_deps', config: 'node.js' },
    { name: 'utility_bundle_deps', config: 'utility.js' },
    { name: 'preload_realm_bundle_deps', config: 'preload_realm.js' }
  ];

  const targetsWithDeps = await Promise.all(bundleTargets.map(async bundleTarget => {
    const child = cp.spawn('node', [
      './build/esbuild/bundle.js',
      '--config', `./build/esbuild/configs/${bundleTarget.config}`,
      '--print-graph'
    ], {
      cwd: rootPath
    });
    let output = '';
    child.stdout.on('data', chunk => {
      output += chunk.toString();
    });
    child.stderr.on('data', chunk => console.error(chunk.toString()));
    await new Promise<void>((resolve, reject) => child.on('exit', (code) => {
      if (code !== 0) {
        console.error(output);
        return reject(new Error(`Failed to list bundle dependencies for entry: ${bundleTarget.name}`));
      }

      resolve();
    }));

    return {
      ...bundleTarget,
      dependencies: (JSON.parse(output) as string[])
        .map(line => line.trim())
        .map(line => path.relative(rootPath, path.resolve(rootPath, line)).replace(/\\/g, '/'))
        .filter(line => !line.startsWith('..'))
        .filter(line => !line.startsWith('node_modules'))
        .concat(['tsconfig.json', 'tsconfig.electron.json', 'package.json', ...typingFiles])
        .sort()
    };
  }));

  fs.writeFileSync(
    gniPath,
    `# THIS FILE IS AUTO-GENERATED, PLEASE DO NOT EDIT BY HAND
auto_filenames = {
  api_docs = [
${allDocs.map(doc => `    "${doc}",`).join('\n')}
  ]

${targetsWithDeps.map(target => `  ${target.name} = [
${target.dependencies.map(dep => `    "${dep}",`).join('\n')}
  ]`).join('\n\n')}
}
`);
};

if (require.main === module) {
  main().catch((err) => {
    console.error(err);
    process.exit(1);
  });
}
