const TerserPlugin = require('terser-webpack-plugin');
const webpack = require('webpack');

const fs = require('node:fs');
const path = require('node:path');

const electronRoot = path.resolve(__dirname, '../..');

// Wraps the emitted bundle in `header` / `footer`. BannerPlugin instances run
// in registration order, so wrapping twice nests the later wrapper outside the
// earlier one.
const wrapBundle = (header, footer) => [
  new webpack.BannerPlugin({ banner: header, raw: true }),
  new webpack.BannerPlugin({ banner: footer, raw: true, footer: true })
];

class AccessDependenciesPlugin {
  apply(compiler) {
    compiler.hooks.compilation.tap('AccessDependenciesPlugin', (compilation) => {
      compilation.hooks.finishModules.tap('AccessDependenciesPlugin', (modules) => {
        const filePaths = modules
          .map((m) => m.resource)
          .filter((p) => p)
          .map((p) => path.relative(electronRoot, p));
        console.info(JSON.stringify(filePaths));
      });
    });
  }
}

module.exports = ({
  alwaysHasNode,
  loadElectronFromAlternateTarget,
  targetDeletesNodeGlobals,
  target,
  wrapInitWithProfilingTimeout,
  wrapInitWithTryCatch
}) => {
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

  return (env = {}, argv = {}) => {
    const onlyPrintingGraph = !!env.PRINT_WEBPACK_GRAPH;
    const outputFilename = argv['output-filename'] || `${target}.bundle.js`;

    const defines = {
      BUILDFLAG: onlyPrintingGraph ? '(a => a)' : ''
    };

    if (env.buildflags) {
      const flagFile = fs.readFileSync(env.buildflags, 'utf8');
      for (const line of flagFile.split(/(\r\n|\r|\n)/g)) {
        const flagMatch = line.match(/#define BUILDFLAG_INTERNAL_(.+?)\(\) \(([01])\)/);
        if (flagMatch) {
          const [, flagName, flagValue] = flagMatch;
          defines[flagName] = JSON.stringify(Boolean(parseInt(flagValue, 10)));
        }
      }
    }

    const plugins = [];

    if (onlyPrintingGraph) {
      plugins.push(new AccessDependenciesPlugin());
    }

    if (targetDeletesNodeGlobals) {
      plugins.push(
        new webpack.ProvidePlugin({
          Buffer: ['@electron/internal/common/webpack-provider', 'Buffer'],
          global: ['@electron/internal/common/webpack-provider', '_global'],
          process: ['@electron/internal/common/webpack-provider', 'process']
        })
      );
    }

    const alias = {};

    // Webpack 5 no longer polyfills process.
    if (!alwaysHasNode) {
      plugins.push(
        new webpack.ProvidePlugin({
          process: 'process/browser'
        })
      );
      // No Node.js `events` in these bundles; EventEmitter is implemented
      // natively instead.
      alias.events$ = path.resolve(electronRoot, 'lib', 'common', 'node-events.ts');
    }

    plugins.push(
      new webpack.ProvidePlugin({
        Promise: ['@electron/internal/common/webpack-globals-provider', 'Promise']
      })
    );

    plugins.push(new webpack.DefinePlugin(defines));

    if (wrapInitWithProfilingTimeout) {
      plugins.push(
        ...wrapBundle(
          'function ___electron_webpack_init__() {',
          `
};
if ((globalThis.process || binding.process).argv.includes("--profile-electron-init")) {
  setTimeout(___electron_webpack_init__, 0);
} else {
  ___electron_webpack_init__();
}`
        )
      );
    }

    if (wrapInitWithTryCatch) {
      plugins.push(
        ...wrapBundle(
          'try {',
          `
} catch (err) {
  console.error('Electron ${outputFilename} script failed to run');
  console.error(err);
}`
        )
      );
    }

    return {
      mode: 'development',
      devtool: false,
      entry,
      target: alwaysHasNode ? 'node' : 'web',
      output: {
        filename: outputFilename
      },
      resolve: {
        alias: {
          ...alias,
          '@electron/internal': path.resolve(electronRoot, 'lib'),
          electron$: electronAPIFile,
          'electron/main$': electronAPIFile,
          'electron/renderer$': electronAPIFile,
          'electron/common$': electronAPIFile,
          'electron/utility$': electronAPIFile
        },
        extensions: ['.ts', '.js']
      },
      module: {
        rules: [
          {
            test: /\.ts$/,
            loader: 'ts-loader',
            options: {
              configFile: path.resolve(electronRoot, 'tsconfig.electron.json'),
              transpileOnly: onlyPrintingGraph,
              ignoreDiagnostics: [
                // File '{0}' is not under 'rootDir' '{1}'.
                6059,
                // Private field '{0}' must be declared in an enclosing class.
                1111
              ]
            }
          }
        ]
      },
      node: {
        __dirname: false,
        __filename: false
      },
      optimization: {
        minimize: env.mode === 'production',
        minimizer: [
          new TerserPlugin({
            terserOptions: {
              keep_classnames: true,
              keep_fnames: true
            }
          })
        ]
      },
      plugins
    };
  };
};
