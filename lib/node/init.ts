// Initialize ASAR support in fs module.
import { wrapFsWithAsar } from './asar-fs-wrapper';
wrapFsWithAsar(require('fs'));

// Hook child_process.fork.
const cp = require('child_process');
const originalFork = cp.fork;
cp.fork = (modulePath: string, args: any, options: any) => {
  // Parse optional args.
  if (args == null) {
    args = [];
  } else if (typeof args === 'object' && !Array.isArray(args)) {
    options = args;
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
  options.env.ELECTRON_RUN_AS_NODE = 1;
  // On mac the child script runs in helper executable.
  if (!options.execPath && process.platform === 'darwin') {
    options.execPath = process.helperExecPath;
  }
  return originalFork(modulePath, args, options);
};
