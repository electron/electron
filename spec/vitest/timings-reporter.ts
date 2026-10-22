// Writes spec/artifacts/spec-timings.json, the per-file wall time table that
// script/gen-spec-weights.js turns into script/spec-weights.json to balance CI
// shards. Same shape the mocha runner produced; a file that runs in both the
// parallel and the serial phase has both durations added up.

import * as fs from 'node:fs';
import * as path from 'node:path';

import type { Reporter, TestModule } from 'vitest/node';

export class SpecTimingsReporter implements Reporter {
  readonly #specDir: string;

  // Constructed in vitest.config.ts rather than named by path there: the CLI
  // can be an older Node.js than Electron's and need not load .ts itself.
  constructor(specDir: string) {
    this.#specDir = specDir;
  }

  onTestRunEnd(testModules: ReadonlyArray<TestModule>) {
    const specDir = this.#specDir;
    const baseElectronDir = path.resolve(specDir, '..');
    const files: Record<string, number> = {};
    for (const testModule of testModules) {
      if (!testModule.state() || testModule.state() === 'skipped') continue;
      const file = path.relative(baseElectronDir, testModule.moduleId).split(path.sep).join('/');
      files[file] = (files[file] || 0) + testModule.diagnostic().duration / 1000;
    }
    const artifactsDir = path.join(specDir, 'artifacts');
    fs.mkdirSync(artifactsDir, { recursive: true });
    fs.writeFileSync(
      path.join(artifactsDir, 'spec-timings.json'),
      JSON.stringify(
        {
          platform: process.platform,
          arch: process.arch,
          mas: process.env.ARTIFACT_KEY?.startsWith('mas') ?? false,
          sanitizer: process.env.IS_ASAN === 'true' ? 'asan' : process.env.IS_UBSAN === 'true' ? 'ubsan' : null,
          files
        },
        null,
        2
      )
    );
  }
}
