import * as path from 'path';

const Module = require('module');

// Clear Node's global search paths.
Module.globalPaths.length = 0;

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
const electronModule = new Module('electron', null);
electronModule.id = 'electron';
electronModule.loaded = true;
electronModule.filename = 'electron';
Object.defineProperty(electronModule, 'exports', {
  get: () => require('electron')
});

Module._cache['electron'] = electronModule;

const originalResolveFilename = Module._resolveFilename;
Module._resolveFilename = function (request: string, parent: NodeModule, isMain: boolean) {
  if (request === 'electron') {
    return 'electron';
  } else {
    return originalResolveFilename(request, parent, isMain);
  }
};
