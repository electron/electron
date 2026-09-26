// vitest `setupFiles` entry: runs in the Electron worker before every spec
// file. Installs the mocha-style globals.

import { afterEach } from 'vitest';

import { runCleanupFunctions } from '../lib/spec-helpers.ts';
import { installMochaGlobals, setCleanupHook } from './mocha-compat.ts';

installMochaGlobals();

// `defer()`-ed cleanup runs after every test, innermost suite first (see
// mocha-compat's describe) and once more here for tests declared at file level.
setCleanupHook(runCleanupFunctions);
afterEach(runCleanupFunctions);
