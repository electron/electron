// The type of preload script exports.
interface PreloadExports {
  onBuiltinModulesPatched?: () => {};
}

// Get the preload path.
// Note that the path might be updated later so code should not cache the path.
const v8Util = process._linkedBinding('electron_common_v8_util');
const getPreloadPath = () => {
  return v8Util.getHiddenValue(process, 'preload') as string | undefined;
};

// Run the preload script.
const Module = require('module') as NodeJS.ModuleInternal;
let preload: PreloadExports | undefined;
{
  const preloadPath = getPreloadPath();
  if (preloadPath) {
    try {
      // Load the preload script similiar to how Node implements --require flag.
      const parent = new Module('internal/preload', null);
      preload = Module._load(preloadPath, parent, false) as PreloadExports | undefined;
    } catch (error) {
      console.error(`Error happened when running preload (${preloadPath})`, error);
    }
  }
}

// Initialize ASAR support in fs module.
import { wrapFsWithAsar } from './asar-fs-wrapper'; // eslint-disable-line import/first
wrapFsWithAsar(require('fs'));

// Hook child_process.fork.
import cp = require('child_process'); // eslint-disable-line import/first
const originalFork = cp.fork;
cp.fork = (modulePath, args?, options?: cp.ForkOptions) => {
  // Parse optional args.
  if (args == null) {
    args = [];
  } else if (typeof args === 'object' && !Array.isArray(args)) {
    options = args as cp.ForkOptions;
    args = [];
  }
  // Fallback to original fork to report arg type errors.
  if (typeof modulePath !== 'string' || !Array.isArray(args) ||
      (typeof options !== 'object' && typeof options !== 'undefined')) {
    return originalFork(modulePath, args, options);
  }
  // When forking a child script, we setup a special environment to make
  // the electron binary run like upstream Node.js.
  options = options ?? {};
  options.env = Object.create(options.env || process.env);
  options.env!.ELECTRON_RUN_AS_NODE = '1';
  // If current process has a preload script, pass it to child.
  const preloadPath = getPreloadPath();
  if (preloadPath) {
    args.push(`--node-preload=${preloadPath}`);
    options.env!.ELECTRON_HAS_NODE_PRELOAD = 'true';
  }
  // On mac the child script runs in helper executable.
  if (!options.execPath && process.platform === 'darwin') {
    options.execPath = process.helperExecPath;
  }
  return originalFork(modulePath, args, options);
};

// Prevent Node from adding paths outside this app to search paths.
import path = require('path'); // eslint-disable-line import/first
const resourcesPathWithTrailingSlash = process.resourcesPath + path.sep;
const originalNodeModulePaths = Module._nodeModulePaths;
Module._nodeModulePaths = function (from) {
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

if (typeof preload?.onBuiltinModulesPatched === 'function') {
  preload.onBuiltinModulesPatched();
}
