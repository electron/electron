import * as electron from 'electron/main';

import * as fs from 'node:fs';
import { Module } from 'node:module';
import * as path from 'node:path';
import * as url from 'node:url';

const { app, dialog } = electron;

type DefaultAppOptions = {
  file: null | string;
  noHelp: boolean;
  version: boolean;
  webdriver: boolean;
  interactive: boolean;
  abi: boolean;
  modules: string[];
}

// Parse command line options.
const argv = process.argv.slice(1);

const option: DefaultAppOptions = {
  file: null,
  noHelp: Boolean(process.env.ELECTRON_NO_HELP),
  version: false,
  webdriver: false,
  interactive: false,
  abi: false,
  modules: []
};

let nextArgIsRequire = false;

for (const arg of argv) {
  if (nextArgIsRequire) {
    option.modules.push(arg);
    nextArgIsRequire = false;
    continue;
  } else if (arg === '--version' || arg === '-v') {
    option.version = true;
    break;
  } else if (arg.match(/^--app=/)) {
    option.file = arg.split('=')[1];
    break;
  } else if (arg === '--interactive' || arg === '-i' || arg === '-repl') {
    option.interactive = true;
  } else if (arg === '--test-type=webdriver') {
    option.webdriver = true;
  } else if (arg === '--require' || arg === '-r') {
    nextArgIsRequire = true;
    continue;
  } else if (arg === '--abi' || arg === '-a') {
    option.abi = true;
    continue;
  } else if (arg === '--no-help') {
    option.noHelp = true;
    continue;
  } else if (arg[0] === '-') {
    continue;
  } else {
    option.file = arg;
    break;
  }
}

if (nextArgIsRequire) {
  console.error('Invalid Usage: --require [file]\n\n"file" is required');
  process.exit(1);
}

// Set up preload modules
if (option.modules.length > 0) {
  (Module as any)._preloadModules(option.modules);
}

async function loadApplicationPackage (packagePath: string) {
  // Add a flag indicating app is started from default app.
  Object.defineProperty(process, 'defaultApp', {
    configurable: false,
    enumerable: true,
    value: true
  });

  try {
    // Override app's package.json data.
    packagePath = path.resolve(packagePath);
    const packageJsonPath = path.join(packagePath, 'package.json');
    let appPath;
    if (fs.existsSync(packageJsonPath)) {
      let packageJson;
      const emitWarning = process.emitWarning;
      try {
        process.emitWarning = () => {};
        packageJson = (await import(url.pathToFileURL(packageJsonPath).toString(), {
          with: { type: 'json' }
        })).default;
      } catch (e) {
        showErrorMessage(`Unable to parse ${packageJsonPath}\n\n${(e as Error).message}`);
        return;
      } finally {
        process.emitWarning = emitWarning;
      }

      if (packageJson.version) {
        app.setVersion(packageJson.version);
      }
      if (packageJson.productName) {
        app.name = packageJson.productName;
      } else if (packageJson.name) {
        app.name = packageJson.name;
      }
      if (packageJson.desktopName) {
        app.setDesktopName(packageJson.desktopName);
      } else {
        app.setDesktopName(`${app.name}.desktop`);
      }
      // Set v8 flags, deliberately lazy load so that apps that do not use this
      // feature do not pay the price
      if (packageJson.v8Flags) {
        (await import('node:v8')).setFlagsFromString(packageJson.v8Flags);
      }
      appPath = packagePath;
    }

    let filePath: string;

    try {
      filePath = (Module as any)._resolveFilename(packagePath, null, true);
      app.setAppPath(appPath || path.dirname(filePath));
    } catch (e) {
      showErrorMessage(`Unable to find Electron app at ${packagePath}\n\n${(e as Error).message}`);
      return;
    }

    // Run the app.
    await import(url.pathToFileURL(filePath).toString());
  } catch (e) {
    console.error('App threw an error during load');
    console.error((e as Error).stack || e);
    throw e;
  }
}

function showErrorMessage (message: string) {
  app.focus();
  dialog.showErrorBox('Error launching app', message);
  process.exit(1);
}

async function loadApplicationByURL (appUrl: string) {
  const { loadURL } = await import('./default_app.js');
  loadURL(appUrl);
}

async function loadApplicationByFile (appPath: string) {
  const { loadFile } = await import('./default_app.js');
  loadFile(appPath);
}

async function startRepl () {
  if (process.platform === 'win32') {
    console.error('Electron REPL not currently supported on Windows');
    process.exit(1);
  }

  // Prevent quitting.
  app.on('window-all-closed', () => {});

  const GREEN = '32';
  const colorize = (color: string, s: string) => `\x1b[${color}m${s}\x1b[0m`;
  const electronVersion = colorize(GREEN, `v${process.versions.electron}`);
  const nodeVersion = colorize(GREEN, `v${process.versions.node}`);

  console.info(`
    Welcome to the Electron.js REPL \\[._.]/

    You can access all Electron.js modules here as well as Node.js modules.
    Using: Node.js ${nodeVersion} and Electron.js ${electronVersion}
  `);

  const { start } = await import('node:repl');
  const repl = start({
    prompt: '> '
  }).on('exit', () => {
    process.exit(0);
  });

  function defineBuiltin (context: any, name: string, getter: Function) {
    const setReal = (val: any) => {
      // Deleting the property before re-assigning it disables the
      // getter/setter mechanism.
      delete context[name];
      context[name] = val;
    };

    Object.defineProperty(context, name, {
      get: () => {
        const lib = getter();

        delete context[name];
        Object.defineProperty(context, name, {
          get: () => lib,
          set: setReal,
          configurable: true,
          enumerable: false
        });

        return lib;
      },
      set: setReal,
      configurable: true,
      enumerable: false
    });
  }

  defineBuiltin(repl.context, 'electron', () => electron);
  for (const api of Object.keys(electron) as (keyof typeof electron)[]) {
    defineBuiltin(repl.context, api, () => electron[api]);
  }

  // Copied from node/lib/repl.js. For better DX, we don't want to
  // show e.g 'contentTracing' at a higher priority than 'const', so
  // we only trigger custom tab-completion when no common words are
  // potentially matches.
  const commonWords = [
    'async', 'await', 'break', 'case', 'catch', 'const', 'continue',
    'debugger', 'default', 'delete', 'do', 'else', 'export', 'false',
    'finally', 'for', 'function', 'if', 'import', 'in', 'instanceof', 'let',
    'new', 'null', 'return', 'switch', 'this', 'throw', 'true', 'try',
    'typeof', 'var', 'void', 'while', 'with', 'yield'
  ];

  const electronBuiltins = [...Object.keys(electron), 'original-fs', 'electron'];

  const defaultComplete: Function = repl.completer;
  (repl as any).completer = (line: string, callback: Function) => {
    const lastSpace = line.lastIndexOf(' ');
    const currentSymbol = line.substring(lastSpace + 1, repl.cursor);

    const filterFn = (c: string) => c.startsWith(currentSymbol);
    const ignores = commonWords.filter(filterFn);
    const hits = electronBuiltins.filter(filterFn);

    if (!ignores.length && hits.length) {
      callback(null, [hits, currentSymbol]);
    } else {
      defaultComplete.apply(repl, [line, callback]);
    }
  };
}

// Start the specified app if there is one specified in command line, otherwise
// start the default app.
if (option.file && !option.webdriver) {
  const file = option.file;
  // eslint-disable-next-line n/no-deprecated-api
  const protocol = url.parse(file).protocol;
  const extension = path.extname(file);
  if (protocol === 'http:' || protocol === 'https:' || protocol === 'file:' || protocol === 'chrome:') {
    await loadApplicationByURL(file);
  } else if (extension === '.html' || extension === '.htm') {
    await loadApplicationByFile(path.resolve(file));
  } else {
    await loadApplicationPackage(file);
  }
} else if (option.version) {
  console.log('v' + process.versions.electron);
  process.exit(0);
} else if (option.abi) {
  console.log(process.versions.modules);
  process.exit(0);
} else if (option.interactive) {
  await startRepl();
} else {
  if (!option.noHelp) {
    const welcomeMessage = `
Electron ${process.versions.electron} - Build cross platform desktop apps with JavaScript, HTML, and CSS
Usage: electron [options] [path]

A path to an Electron app may be specified. It must be one of the following:
  - index.js file.
  - Folder containing a package.json file.
  - Folder containing an index.js file.
  - .html/.htm file.
  - http://, https://, or file:// URL.

Options:
  -i, --interactive     Open a REPL to the main process.
  -r, --require         Module to preload (option can be repeated).
  -v, --version         Print the version.
  -a, --abi             Print the Node ABI version.`;

    console.log(welcomeMessage);
  }

  await loadApplicationByFile('index.html');
}
