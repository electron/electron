import { defineConfig } from 'vitest/config';

import * as fs from 'node:fs';
import { createRequire } from 'node:module';
import * as os from 'node:os';
import * as path from 'node:path';

import { electronPool } from './vitest/electron-pool.ts';
import { SpecTimingsReporter } from './vitest/timings-reporter.ts';

const require = createRequire(import.meta.url);
const utils = require('../script/lib/utils.js');

const specDir = import.meta.dirname;
const electronPath: string = process.env.ELECTRON_SPEC_ELECTRON_PATH || utils.getAbsoluteElectronExec();
process.env.ELECTRON_SPEC_ELECTRON_PATH = electronPath;
const electronArgs = process.env.ELECTRON_SPEC_WORKER_ARGS
  ? process.env.ELECTRON_SPEC_WORKER_ARGS.split(' ').filter(Boolean)
  : [];

const isCI = !!process.env.CI;

// Suites tagged 'serial' need the machine to themselves (window focus, the
// clipboard, global shortcuts, the screen, ...). Everything else runs first,
// spread over several Electron processes; the serial suites then run one file
// at a time with nothing else going on. A file only takes part in the serial
// phase if it mentions the tag at all, so most files start Electron once.
const SERIAL_TAG = 'serial';
const specFiles = fs.readdirSync(specDir).filter((file) => /\.spec\.[tj]s$/.test(file));
// Only *.spec.ts is picked up; a file still named the old way (*-spec.ts, e.g.
// from a PR opened before the rename) would silently never run.
const misnamed = fs.readdirSync(specDir).filter((file) => /-spec\.[cm]?[jt]sx?$/.test(file));
if (misnamed.length > 0) {
  throw new Error(`Spec files must be named *.spec.ts, rename: ${misnamed.join(', ')}`);
}
const serialTagPattern = new RegExp(`tags:\\s*\\[[^\\]]*['"]${SERIAL_TAG}['"]`);
const serialFiles = specFiles.filter((file) =>
  serialTagPattern.test(fs.readFileSync(path.join(specDir, file), 'utf8'))
);

// How many Electron processes to run at once. Each one keeps roughly three
// processes busy (browser, GPU, renderer), so about a third of the CPUs, but
// two even on the small 3-4 core CI hosts and never more than 8. A container's
// CPU quota counts rather than the host's cores.
function availableCpus() {
  let cpus = os.availableParallelism();
  if (process.platform === 'linux') {
    const quota = (text: string) => {
      const [max, period] = text.trim().split(/\s+/).map(Number);
      return max > 0 && period > 0 ? Math.ceil(max / period) : Infinity;
    };
    try {
      cpus = Math.min(cpus, quota(fs.readFileSync('/sys/fs/cgroup/cpu.max', 'utf8')));
    } catch {}
    try {
      const v1 = ['cpu.cfs_quota_us', 'cpu.cfs_period_us'].map((f) =>
        fs.readFileSync(`/sys/fs/cgroup/cpu/${f}`, 'utf8').trim()
      );
      cpus = Math.min(cpus, quota(v1.join(' ')));
    } catch {}
  }
  return cpus;
}
const cpus = availableCpus();
const maxWorkers =
  Number(process.env.ELECTRON_SPEC_WORKERS) || Math.min(8, Math.max(Math.min(2, cpus), Math.round(cpus / 3)));
process.env.ELECTRON_SPEC_WORKERS = String(maxWorkers);

const reporters: (string | [string, Record<string, unknown>] | SpecTimingsReporter)[] = ['default'];
if (process.env.GITHUB_ACTIONS === 'true') {
  // Annotates failures on the PR; vitest only adds it by itself when no
  // reporters are configured.
  reporters.push('github-actions');
}
if (process.env.MOCHA_FILE) {
  reporters.push(['junit', { outputFile: process.env.MOCHA_FILE, includeConsoleOutput: false }]);
}
// Per-file wall time for script/gen-spec-weights.js; skipped for filtered runs
// so a rerun does not overwrite the full run's numbers.
if (isCI && !process.argv.some((arg) => /^(-t|--testNamePattern)/.test(arg))) {
  reporters.push(new SpecTimingsReporter(specDir));
}

export default defineConfig({
  test: {
    root: specDir,
    dir: specDir,
    // `include` lives on the two projects below (merging would concatenate a
    // root pattern into both).
    // Specs are loaded by Electron's own Node.js (native ESM + type stripping),
    // not transformed by Vite, so they run exactly as an app would load them.
    experimental: {
      viteModuleRunner: false,
      nodeLoader: false
    },
    pool: electronPool({ electronPath, specDir, electronArgs }),
    // A fresh Electron process per spec file.
    isolate: true,
    globals: false,
    runner: './vitest/runner.ts',
    globalSetup: ['./vitest/global-setup.js'],
    setupFiles: ['./vitest/setup.ts'],
    // Show full object diffs in assertion errors (chaijs/chai#469).
    chaiConfig: { truncateThreshold: 0 },
    // mocha-compat implements mocha's resettable timeouts itself.
    testTimeout: 0,
    hookTimeout: 0,
    // Run hooks in the order they were declared, as mocha did; the default
    // ('stack') runs after* hooks in reverse, which would close windows before
    // the defer()-ed cleanup that still needs them.
    sequence: { hooks: 'list' },
    // Electron takes a moment to quit; a slow worker stop is not a failure.
    teardownTimeout: 30_000,
    retry: isCI ? 3 : 0,
    allowOnly: !isCI,
    // A spec file may register no tests at all on some platform (a top level
    // `if (process.platform !== ...) return`); that is not a failure. A
    // --files argument that matches nothing is caught by script/spec-runner.js.
    passWithNoTests: true,
    includeTaskLocation: true,
    tags: [
      { name: SERIAL_TAG, description: 'Needs exclusive use of the machine (focus, clipboard, screen, shortcuts).' }
    ],
    reporters,
    projects: [
      {
        extends: true,
        test: {
          name: 'parallel',
          include: specFiles,
          // Typed as CLI-only, but honoured per project.
          ...({ tagsFilter: [`!${SERIAL_TAG}`] } as object),
          maxWorkers,
          sequence: { hooks: 'list', groupOrder: 0 }
        }
      },
      {
        extends: true,
        test: {
          name: SERIAL_TAG,
          // An empty list would fall back to the root include.
          include: serialFiles.length ? serialFiles : ['.none'],
          ...({ tagsFilter: [SERIAL_TAG] } as object),
          maxWorkers: 1,
          fileParallelism: false,
          sequence: { hooks: 'list', groupOrder: 1 }
        }
      }
    ]
  }
});
