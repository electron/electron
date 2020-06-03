const cp = require('child_process');
const fs = require('fs-extra');
const os = require('os');
const path = require('path');

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
    }
  ];

  await Promise.all(webpackTargets.map(async webpackTarget => {
    const tmpDir = await fs.mkdtemp(path.resolve(os.tmpdir(), 'electron-filenames-'));
    const child = cp.spawn('node', [
      'build/webpack/get-outputs.js',
      `./${webpackTarget.config}`,
      path.resolve(tmpDir, `${webpackTarget.name}.measure.js`)
    ], {
      cwd: path.resolve(__dirname, '..')
    });
    let output = '';
    child.stdout.on('data', chunk => {
      output += chunk.toString();
    });
    child.stderr.on('data', chunk => console.error(chunk.toString()));
    await new Promise((resolve, reject) => child.on('exit', (code) => {
      if (code !== 0) {
        console.error(output);
        return reject(new Error(`Failed to list webpack dependencies for entry: ${webpackTarget.name}`));
      }

      resolve();
    }));

    webpackTarget.dependencies = JSON.parse(output)
      // Remove whitespace
      .map(line => line.trim())
      // Get the relative path
      .map(line => path.relative(rootPath, line).replace(/\\/g, '/'))
      // Only care about files in //electron
      .filter(line => !line.startsWith('..'))
      // Only care about our own files
      .filter(line => !line.startsWith('node_modules'))
      // All webpack builds depend on the tsconfig  and package json files
      .concat(['tsconfig.json', 'tsconfig.electron.json', 'package.json', ...typingFiles])
      // Make the generated list easier to read
      .sort();
    await fs.remove(tmpDir);
  }));

  fs.writeFileSync(
    gniPath,
    `# THIS FILE IS AUTO-GENERATED, PLEASE DO NOT EDIT BY HAND
auto_filenames = {
  api_docs = [
${allDocs.map(doc => `    "${doc}",`).join('\n')}
  ]

${webpackTargets.map(target => `  ${target.name} = [
${target.dependencies.map(dep => `    "${dep}",`).join('\n')}
  ]`).join('\n\n')}
}
`);
};

if (process.mainModule === module) {
  main().catch((err) => {
    console.error(err);
    process.exit(1);
  });
}
