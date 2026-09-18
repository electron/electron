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
      // 'z' restores the upstream RenderWidgetHostViewAura::DidNavigate
      // condition (the behaviour that shipped in 44.4.x); without it the build
      // carries the fix.
      ['upstream-1', 'lz', []],
      ['upstream-2', 'lz', []],
      ['upstream-3', 'lz', []],
      ['upstream-4', 'lz', []],
      ['upstream-5', 'lz', []],
      ['upstream-no-overlay', 'lz', ['--no-overlay']],
      ['upstream-shown', 'lz', ['--show']],
      ['upstream-bgthrottle', 'lz', ['--bg-throttle']],
      ['upstream-retitle', 'lz', ['--retitle']],
      ['upstream-old-wcv-behaviour-1', 'lzo', []],
      ['upstream-old-wcv-behaviour-2', 'lzo', []],
      ['upstream-seed-early-1', 'lzh', []],
      ['upstream-seed-early-2', 'lzh', []],
      ['fixed-1', 'l', []],
      ['fixed-2', 'l', []],
      ['fixed-3', 'l', []],
      ['fixed-4', 'l', []],
      ['fixed-5', 'l', []],
      ['fixed-bgthrottle', 'l', ['--bg-throttle']],
      ['fixed-retitle', 'l', ['--retitle']]
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
