// vitest `setupFiles` entry: runs in the Electron worker before every spec
// file. Installs the mocha-style globals.

import { installMochaGlobals } from './mocha-compat.ts';

installMochaGlobals();
