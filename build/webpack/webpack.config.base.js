const fs = require('fs');
const path = require('path');
const webpack = require('webpack');
const TerserPlugin = require('terser-webpack-plugin');

const electronRoot = path.resolve(__dirname, '../..');

const onlyPrintingGraph = !!process.env.PRINT_WEBPACK_GRAPH;

class AccessDependenciesPlugin {
  apply (compiler) {
    // Only hook into webpack when we are printing the dependency graph
    if (!onlyPrintingGraph) return;

    compiler.hooks.compilation.tap('AccessDependenciesPlugin', compilation => {
      compilation.hooks.finishModules.tap('AccessDependenciesPlugin', modules => {
        const filePaths = modules.map(m => m.resource).filter(p => p).map(p => path.relative(electronRoot, p));
        console.info(JSON.stringify(filePaths));
      });
    });
  }
}

const defines = {
  BUILDFLAG: onlyPrintingGraph ? '(a => a)' : ''
};

const buildFlagsPrefix = '--buildflags=';
const buildFlagArg = process.argv.find(arg => arg.startsWith(buildFlagsPrefix));

if (buildFlagArg) {
  const buildFlagPath = buildFlagArg.substr(buildFlagsPrefix.length);

  const flagFile = fs.readFileSync(buildFlagPath, 'utf8');
  for (const line of flagFile.split(/(\r\n|\r|\n)/g)) {
    const flagMatch = line.match(/#define BUILDFLAG_INTERNAL_(.+?)\(\) \(([01])\)/);
    if (flagMatch) {
      const [, flagName, flagValue] = flagMatch;
      defines[flagName] = JSON.stringify(Boolean(parseInt(flagValue, 10)));
    }
  }
}

const ignoredModules = [];

if (defines.ENABLE_DESKTOP_CAPTURER === 'false') {
  ignoredModules.push(
    '@electron/internal/browser/desktop-capturer',
    '@electron/internal/browser/api/desktop-capturer',
    '@electron/internal/renderer/api/desktop-capturer'
  );
}

if (defines.ENABLE_REMOTE_MODULE === 'false') {
  ignoredModules.push(
    '@electron/internal/browser/remote/server',
    '@electron/internal/renderer/api/remote'
  );
}

if (defines.ENABLE_VIEWS_API === 'false') {
  ignoredModules.push(
    '@electron/internal/browser/api/views/image-view.js'
  );
}

module.exports = ({
  alwaysHasNode,
  loadElectronFromAlternateTarget,
  targetDeletesNodeGlobals,
  target,
  wrapInitWithProfilingTimeout
}) => {
  let entry = path.resolve(electronRoot, 'lib', target, 'init.ts');
  if (!fs.existsSync(entry)) {
    entry = path.resolve(electronRoot, 'lib', target, 'init.js');
  }

  const electronAPIFile = path.resolve(electronRoot, 'lib', loadElectronFromAlternateTarget || target, 'api', 'exports', 'electron.ts');

  return ({
    mode: 'development',
    devtool: false,
    entry,
    target: alwaysHasNode ? 'node' : 'web',
    output: {
      filename: `${target}.bundle.js`
    },
    wrapInitWithProfilingTimeout,
    resolve: {
      alias: {
        '@electron/internal': path.resolve(electronRoot, 'lib'),
        electron$: electronAPIFile,
        'electron/main$': electronAPIFile,
        'electron/renderer$': electronAPIFile,
        'electron/common$': electronAPIFile,
        // Force timers to resolve to our dependency that doesn't use window.postMessage
        timers: path.resolve(electronRoot, 'node_modules', 'timers-browserify', 'main.js')
      },
      extensions: ['.ts', '.js']
    },
    module: {
      rules: [{
        test: (moduleName) => !onlyPrintingGraph && ignoredModules.includes(moduleName),
        loader: 'null-loader'
      }, {
        test: /\.ts$/,
        loader: 'ts-loader',
        options: {
          configFile: path.resolve(electronRoot, 'tsconfig.electron.json'),
          transpileOnly: onlyPrintingGraph,
          ignoreDiagnostics: [
            // File '{0}' is not under 'rootDir' '{1}'.
            6059
          ]
        }
      }]
    },
    node: {
      __dirname: false,
      __filename: false,
      // We provide our own "timers" import above, any usage of setImmediate inside
      // one of our renderer bundles should import it from the 'timers' package
      setImmediate: false
    },
    optimization: {
      minimize: true,
      minimizer: [
        new TerserPlugin({
          terserOptions: {
            keep_classnames: true,
            keep_fnames: true
          }
        })
      ]
    },
    plugins: [
      new AccessDependenciesPlugin(),
      ...(targetDeletesNodeGlobals ? [
        new webpack.ProvidePlugin({
          process: ['@electron/internal/common/webpack-provider', 'process'],
          global: ['@electron/internal/common/webpack-provider', '_global'],
          Buffer: ['@electron/internal/common/webpack-provider', 'Buffer']
        })
      ] : []),
      new webpack.ProvidePlugin({
        Promise: ['@electron/internal/common/webpack-globals-provider', 'Promise']
      }),
      new webpack.DefinePlugin(defines)
    ]
  });
};
