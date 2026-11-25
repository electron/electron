import { describe, expect, it } from 'vitest';

import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { defer, ifit, spawnAndWait } from './lib/spec-helpers.ts';

const fixture = path.join(import.meta.dirname, 'fixtures', 'apps', 'vitest-pool');
const vitestCli = path.join(import.meta.dirname, 'node_modules', 'vitest', 'vitest.mjs');

// Startup of vitest and of up to two Electron workers, two short test files,
// and quitting. Sanitizer builds raise the spec timeout (ELECTRON_SPEC_TIMEOUT).
const runTimeout = 2 * (Number(process.env.ELECTRON_SPEC_TIMEOUT) || 30_000);
const killTimeout = 5_000;
// A short quit timeout for the fixture's pool: SIGTERM after half of it.
const shortQuitTimeout = (Number(process.env.ELECTRON_SPEC_TIMEOUT) || 30_000) / 30;

// Runs the given files of spec/fixtures/apps/vitest-pool in a vitest run of
// their own with the Electron pool, and returns how that run ended.
async function runFixture(files: string[], env: Record<string, string> = {}) {
  // The Node.js running the spec suite's vitest (set in spec/vitest.config.ts):
  // vitest's native dependencies may not load in Electron as Node.js.
  const nodePath = process.env.ELECTRON_SPEC_NODE_PATH;
  if (!nodePath) throw new Error('ELECTRON_SPEC_NODE_PATH is not set');
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-vitest-pool-spec-'));
  defer(() => fs.rmSync(tmpDir, { recursive: true, force: true }));
  try {
    return await spawnAndWait(
      nodePath,
      [vitestCli, 'run', '--config', path.join(fixture, 'vitest.config.ts'), ...files],
      {
        cwd: fixture,
        env: { ...process.env, ...env, ELECTRON_SPEC_FIXTURE_TMP_DIR: tmpDir },
        timeout: runTimeout,
        killTimeout,
        stripOutput: true
      }
    );
  } catch (error) {
    // The run timed out and its vitest was killed. Kill the workers that have
    // not quit (see the fixture's index.js) as well. Outside Windows this
    // signals the worker's process group, whose id is not reused while any of
    // its processes lives. A worker that died without quitting leaves a record
    // whose pid may have been reused, mostly on Windows; that takes a crash or
    // a hung quit in a run that has already failed by timing out.
    for (const file of fs.readdirSync(tmpDir)) {
      const pid = Number(/^worker-(\d+)\.pid$/.exec(file)?.[1]);
      if (!pid) continue;
      try {
        process.kill(process.platform === 'win32' ? pid : -pid, 'SIGKILL');
      } catch {
        // Already gone.
      }
    }
    throw error;
  }
}

function count(text: string, pattern: RegExp) {
  return text.match(new RegExp(pattern, 'g'))?.length ?? 0;
}

// Behaviour of the vitest pool in spec/vitest/electron-pool.ts.
describe('vitest Electron pool', () => {
  const timeout = runTimeout + killTimeout + 5_000;
  const quitError = /exited with exit code (0x)?1 while quitting after running quit\.fixture\.ts/;

  it('passes a run whose worker quits cleanly', { timeout }, async () => {
    const { code, stdout, stderr } = await runFixture(['quit.fixture.ts']);
    const output = `${stdout}\n${stderr}`;
    expect(output).toMatch(/Tests {2}1 passed/);
    expect(output).not.toMatch(/while quitting/);
    expect(code, output).toBe(0);
  });

  it('fails a run whose last worker exits with an error code while quitting', { timeout }, async () => {
    const { code, stdout, stderr } = await runFixture(['quit.fixture.ts'], {
      ELECTRON_SPEC_FIXTURE_EXIT_WHILE_QUITTING: '1'
    });
    const output = `${stdout}\n${stderr}`;
    expect(output).toMatch(/Tests {2}1 passed/);
    expect(count(output, quitError), output).toBe(1);
    expect(code, output).toBe(1);
  });

  it('lists the error in the summary when another file is still running', { timeout }, async () => {
    const { code, stdout, stderr } = await runFixture(['quit.fixture.ts', 'wait.fixture.ts'], {
      ELECTRON_SPEC_FIXTURE_EXIT_WHILE_QUITTING: '1',
      ELECTRON_SPEC_FIXTURE_WAIT: '1'
    });
    const output = `${stdout}\n${stderr}`;
    expect(output).toMatch(/Tests {2}2 passed/);
    expect(output).toMatch(/Teardown Error/);
    expect(output).toMatch(/originated in "quit\.fixture\.ts"/);
    expect(count(output, quitError), output).toBe(1);
    expect(code, output).toBe(1);
  });

  // On Windows the pool's kill is TerminateProcess, which no worker outlives.
  ifit(process.platform !== 'win32')(
    'fails a run whose worker exits with an error code after the pool sent SIGTERM',
    { timeout },
    async () => {
      const { code, stdout, stderr } = await runFixture(['quit.fixture.ts'], {
        ELECTRON_SPEC_FIXTURE_EXIT_AFTER_SIGTERM: '1',
        ELECTRON_SPEC_FIXTURE_QUIT_TIMEOUT: String(shortQuitTimeout)
      });
      const output = `${stdout}\n${stderr}`;
      expect(output).toMatch(/Tests {2}1 passed/);
      expect(count(output, quitError), output).toBe(1);
      expect(code, output).toBe(1);
    }
  );
});
