import * as path from 'path';

const Module = require('module') as NodeJS.ModuleInternal;

// We do not want to allow use of the VM module in the renderer process as
// it conflicts with Blink's V8::Context internal logic.
if (process.type === 'renderer') {
  const _load = Module._load;
  Module._load = function (request: string) {
    if (request === 'vm') {
      console.warn('The vm module of Node.js is deprecated in the renderer process and will be removed.');
    }
    return _load.apply(this, arguments as any);
  };
}

// Prevent Node from adding paths outside this app to search paths.
const resourcesPathWithTrailingSlash = process.resourcesPath + path.sep;
const originalNodeModulePaths = Module._nodeModulePaths;
Module._nodeModulePaths = function (from: string) {
  const paths: string[] = originalNodeModulePaths(from);
  const fromPath = path.resolve(from) + path.sep;
  // If "from" is outside the app then we do nothing.
  if (fromPath.startsWith(resourcesPathWithTrailingSlash)) {
    return paths.filter(function (candidate) {
      return candidate.startsWith(resourcesPathWithTrailingSlash);
    });
  } else {
    return paths;
  }
};

// Make a fake Electron module that we will insert into the module cache
const makeElectronModule = (name: string) => {
  const electronModule = new Module('electron', null);
  electronModule.id = 'electron';
  electronModule.loaded = true;
  electronModule.filename = name;
  Object.defineProperty(electronModule, 'exports', {
    get: () => require('electron')
  });
  Module._cache[name] = electronModule;
};

makeElectronModule('electron');
makeElectronModule('electron/common');
if (process.type === 'browser') {
  makeElectronModule('electron/main');
}
if (process.type === 'renderer') {
  makeElectronModule('electron/renderer');
}

const originalResolveFilename = Module._resolveFilename;

// 'electron/main', 'electron/renderer' and 'electron/common' are module aliases
// of the 'electron' module for TypeScript purposes, i.e., the types for
// 'electron/main' consist of only main process modules, etc. It is intentional
// that these can be `require()`-ed from both the main process as well as the
// renderer process regardless of the names, they're superficial for TypeScript
// only.
const electronModuleNames = new Set(['electron', 'electron/main', 'electron/renderer', 'electron/common']);
Module._resolveFilename = function (request, parent, isMain, options) {
  if (electronModuleNames.has(request)) {
    return 'electron';
  } else {
    return originalResolveFilename(request, parent, isMain, options);
  }
};
