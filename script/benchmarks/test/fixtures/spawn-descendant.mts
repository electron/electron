// Stand-in for crossbench in run.test.mts: starts a descendant that ignores
// SIGTERM (like a stuck browser), writes its PID to argv[2], then either exits
// at once ('exit') or hangs ('hang').
import { spawn } from 'node:child_process';
import * as fs from 'node:fs';

const descendant = spawn(process.execPath, ['-e', "process.on('SIGTERM', () => {}); setInterval(() => {}, 1000)"], {
  stdio: 'ignore'
});
fs.writeFileSync(process.argv[2], String(descendant.pid));
if (process.argv[3] === 'hang') {
  process.on('SIGTERM', () => {});
  setInterval(() => {}, 1000);
} else {
  descendant.unref();
}
