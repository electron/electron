import * as path from 'path';

const Module = require('module');

// We do not want to allow use of the VM module in the renderer process as
// it conflicts with Blink's V8::Context internal logic.
if (process.type === 'renderer') {
  const _load = Module._load;
  Module._load = function (request: string) {
    if (request === 'vm') {
      console.warn('The vm module of Node.js is deprecated in the renderer process and will be removed.');
    }
    return _load.apply(this, arguments);
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
Module._resolveFilename = function (request: string, parent: NodeModule, isMain: boolean, options?: { paths: Array<string>}) {
  if (request === 'electron' || request.startsWith('electron/')) {
    return 'electron';
  } else {
    return originalResolveFilename(request, parent, isMain, options);
  }
};
