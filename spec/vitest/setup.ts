// vitest `setupFiles` entry: runs in the Electron worker before every spec
// file. Installs the mocha-style globals and configures chai the way
// spec/index.js did for the mocha runner.

import { afterEach } from 'vitest';

import { runCleanupFunctions } from '../lib/spec-helpers.ts';
import { installMochaGlobals, setCleanupHook } from './mocha-compat.ts';

const chai = await import('chai');
chai.use((await import('chai-as-promised')).default);
chai.use((await import('dirty-chai')).default);
// Show full object diff
// https://github.com/chaijs/chai/issues/469
chai.config.truncateThreshold = 0;

installMochaGlobals();

// `defer()`-ed cleanup runs after every test, innermost suite first (see
// mocha-compat's describe) and once more here for tests declared at file level.
setCleanupHook(runCleanupFunctions);
afterEach(runCleanupFunctions);
