// DIAGNOSTIC ONLY (https://github.com/electron/electron/issues/54025).
// Drives spec/fixtures/diag-54025 under each ELECTRON_DIAG_WCO variant and
// prints what happened. Not intended for merge.
import { expect } from 'chai';

import * as cp from 'node:child_process';
import * as path from 'node:path';

const fixture = path.resolve(__dirname, 'fixtures', 'diag-54025');

type Run = { tag: string; env: string; args: string[]; output: string; result: any };

function runProbe(tag: string, diagEnv: string, args: string[]): Promise<Run> {
  return new Promise((resolve) => {
    const child = cp.spawn(process.execPath, [fixture, `--tag=${tag}`, ...args], {
      env: { ...process.env, ELECTRON_DIAG_WCO: diagEnv, ELECTRON_ENABLE_LOGGING: '1' },
      stdio: ['ignore', 'pipe', 'pipe']
    });
    let output = '';
    child.stdout.on('data', (d) => {
      output += d;
    });
    child.stderr.on('data', (d) => {
      output += d;
    });
    const timer = setTimeout(() => {
      output += '\n[spec] killing after 40s\n';
      child.kill();
    }, 40000);
    child.on('exit', () => {
      clearTimeout(timer);
      const line = output
        .split('\n')
        .reverse()
        .find((l) => l.startsWith('RESULT '));
      let result: any = null;
      try {
        result = line ? JSON.parse(line.slice(7)) : null;
      } catch {
        result = line;
      }
      resolve({ tag, env: diagEnv, args, output, result });
    });
  });
}

describe('diag 54025: ready-to-show with titleBarOverlay on a hidden window', function () {
  this.timeout(20 * 60 * 1000);

  it('probes each variant', async () => {
    const variants: [string, string, string[]][] = [
      ['current-1', 'l', []],
      ['current-2', 'l', []],
      ['current-3', 'l', []],
      ['current-no-overlay', 'l', ['--no-overlay']],
      ['current-shown', 'l', ['--show']],
      ['current-default-titlebar', 'l', ['--no-hidden-style']],
      ['current-bgthrottle', 'l', ['--bg-throttle']],
      ['no-bounds-push', 'lb', []],
      ['sync-notify', 'ls', []],
      ['no-attach-push', 'la', []],
      ['unclipped-rect', 'lu', []],
      ['ignore-live-gate', 'lh', []],
      ['no-bounds-push+sync-notify', 'lbs', []],
      ['no-bounds-push+no-attach-push', 'lba', []],
      ['old-behaviour-1', 'lo', []],
      ['old-behaviour-2', 'lo', []],
      ['old-behaviour-no-overlay', 'lo', ['--no-overlay']]
    ];
    const runs: Run[] = [];
    for (const [tag, env, args] of variants) {
      const r = await runProbe(tag, env, args);
      runs.push(r);
      console.log(`\n===== DIAG54025 RUN ${tag} (ELECTRON_DIAG_WCO=${env} ${args.join(' ')}) =====\n${r.output}\n`);
    }
    console.log('\n===== DIAG54025 SUMMARY =====');
    for (const r of runs) {
      const res = r.result || {};
      const ph = res.probeHidden || {};
      const ps = res.probeShown || {};
      console.log(
        `DIAG54025 ${r.tag.padEnd(30)} env=${r.env.padEnd(4)} args=${(r.args.join(' ') || '-').padEnd(18)} ` +
          `rtsHidden=${res.readyToShowWhileHidden} rtsAfterShow=${res.readyToShowAfterShow} rts=${res.readyToShow} ` +
          `rafHidden=${ph.raf} rafShown=${ps.raf} wcoHidden=${JSON.stringify(ph.wcoRect)} wcoShown=${JSON.stringify(ps.wcoRect)} ` +
          `t(rts)=${res.events && res.events['ready-to-show']}`
      );
    }
    expect(runs.length).to.equal(variants.length);
  });
});
