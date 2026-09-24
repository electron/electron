// vitest `globalSetup`: runs once in the CLI process before any worker starts.
// Plain JavaScript because the CLI imports it as is, and the CLI's Node.js can
// be older than the one that strips types (the specs themselves run in
// Electron's).
//
// Kills Electron processes left behind by an earlier, interrupted run of this
// same build (a spec's fixture app that never quit keeps ports and single
// instance locks busy). The mocha runner did this before every file from
// inside Electron; with several workers sharing one executable that would take
// down the other workers, so it happens once up front instead, and each
// worker's leftovers are reaped by the pool when the worker stops.

import * as childProcess from 'node:child_process';
import * as fs from 'node:fs';

/** @param {string} execPath */
function findProcesses(execPath) {
  if (process.platform === 'win32') {
    const escapedPath = execPath.replace(/\\/g, '\\\\');
    const result = childProcess.spawnSync(
      'wmic',
      ['process', 'where', `ExecutablePath='${escapedPath}'`, 'get', 'ProcessId', '/format:value'],
      { encoding: 'utf8' }
    );
    return [...(result.stdout || '').matchAll(/^ProcessId=(\d+)/gm)].map((m) => Number(m[1]));
  }
  if (process.platform === 'linux') {
    /** @type {number[]} */
    const pids = [];
    for (const entry of fs.readdirSync('/proc')) {
      const pid = parseInt(entry, 10);
      if (isNaN(pid)) continue;
      try {
        if (fs.readlinkSync(`/proc/${pid}/exe`) === execPath) pids.push(pid);
      } catch {
        // no permission or process already exited
      }
    }
    return pids;
  }
  const result = childProcess.spawnSync('pgrep', ['-f', execPath], { encoding: 'utf8' });
  return (result.stdout || '')
    .split('\n')
    .map((s) => parseInt(s, 10))
    .filter((pid) => !isNaN(pid));
}

/** @param {import('vitest/node').TestProject} project */
export function setup(project) {
  project.vitest.logger.log(
    `Running specs in up to ${process.env.ELECTRON_SPEC_WORKERS} Electron processes at a time (ELECTRON_SPEC_WORKERS)`
  );
  const execPath = process.env.ELECTRON_SPEC_ELECTRON_PATH;
  // Opt out when deliberately running two invocations against one build.
  if (!execPath || process.env.ELECTRON_SPEC_KEEP_RUNNING_INSTANCES) return;
  let killed = 0;
  try {
    for (const pid of findProcesses(execPath)) {
      if (pid === process.pid) continue;
      try {
        if (process.platform === 'win32') {
          childProcess.spawnSync('taskkill', ['/F', '/PID', String(pid), '/T']);
        } else {
          process.kill(pid, 'SIGKILL');
        }
        killed++;
      } catch {
        // process may have already exited
      }
    }
  } catch {
    // pgrep / wmic not available or returned an error — ignore
  }
  if (killed > 0) {
    project.vitest.logger.log(
      `Killed ${killed} orphaned Electron process${killed === 1 ? '' : 'es'} from a previous run`
    );
  }
}
