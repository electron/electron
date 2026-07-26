import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';

const features = process._linkedBinding('electron_common_features');
const fixturesPath = path.resolve(__dirname, '..', 'fixtures');

export const shouldRunCodesignTests = process.platform === 'darwin' && !process.mas && !features.isComponentBuild();

let identity: string | null;

export function getCodesignIdentity() {
  if (identity === undefined) {
    const result = cp.spawnSync(path.resolve(__dirname, '../../script/codesign/get-trusted-identity.sh'));
    if (result.status !== 0 || result.stdout.toString().trim().length === 0) {
      identity = null;
    } else {
      identity = result.stdout.toString().trim();
    }
  }
  return identity;
}

export async function copyMacOSFixtureApp(newDir: string, fixture: string | null = 'initial') {
  const appBundlePath = path.resolve(process.execPath, '../../..');
  const newPath = path.resolve(newDir, 'Electron.app');
  cp.spawnSync('cp', ['-R', appBundlePath, path.dirname(newPath)]);
  if (fixture) {
    const appDir = path.resolve(newPath, 'Contents/Resources/app');
    await fs.promises.mkdir(appDir, { recursive: true });
    await fs.promises.cp(path.resolve(fixturesPath, 'auto-update', fixture), appDir, { recursive: true });
  }
  const plistPath = path.resolve(newPath, 'Contents', 'Info.plist');
  await fs.promises.writeFile(
    plistPath,
    (await fs.promises.readFile(plistPath, 'utf8')).replace(
      '<key>BuildMachineOSBuild</key>',
      `<key>NSAppTransportSecurity</key>
    <dict>
        <key>NSAllowsArbitraryLoads</key>
        <true/>
        <key>NSExceptionDomains</key>
        <dict>
            <key>localhost</key>
            <dict>
                <key>NSExceptionAllowsInsecureHTTPLoads</key>
                <true/>
                <key>NSIncludesSubdomains</key>
                <true/>
            </dict>
        </dict>
    </dict><key>BuildMachineOSBuild</key>`
    )
  );
  return newPath;
}

// A bare trap instruction (__builtin_trap, CFI/UBSan trap mode, ImmediateCrash)
// kills the process without writing anything to stdout or stderr, so a crash
// looks like a bare SIGTRAP with no output. macOS still records the faulting
// backtrace as a crash report, so pull those in to make the failure debuggable.
async function collectCrashReports(since: number) {
  if (process.platform !== 'darwin') return '';

  const dir = path.resolve(os.homedir(), 'Library', 'Logs', 'DiagnosticReports');
  const read = async () => {
    const reports: string[] = [];
    let entries: string[];
    try {
      entries = await fs.promises.readdir(dir);
    } catch {
      return reports;
    }
    for (const entry of entries) {
      if (!entry.endsWith('.ips')) continue;
      const filePath = path.resolve(dir, entry);
      try {
        if ((await fs.promises.stat(filePath)).mtimeMs < since) continue;
        reports.push(`--- ${entry} ---\n${await fs.promises.readFile(filePath, 'utf8')}`);
      } catch {
        // Report vanished or is unreadable - ignore it.
      }
    }
    return reports;
  };

  // ReportCrash writes the report asynchronously, so poll briefly for it.
  for (let attempt = 0; attempt < 10; attempt++) {
    const reports = await read();
    if (reports.length > 0) return `\nCrash reports:\n${reports.join('\n')}`;
    await delay(1000);
  }
  return '\nNo crash report found in ~/Library/Logs/DiagnosticReports.';
}

export function spawn(cmd: string, args: string[], opts: any = {}) {
  let out = '';
  const startedAt = Date.now();
  const child = cp.spawn(cmd, args, opts);
  child.stdout.on('data', (chunk: Buffer) => {
    out += chunk.toString();
  });
  child.stderr.on('data', (chunk: Buffer) => {
    out += chunk.toString();
  });
  return new Promise<{ code: number; out: string }>((resolve, reject) => {
    // Use 'close' rather than 'exit' - 'exit' can fire while the stdio pipes
    // still have buffered data, truncating the output of a crashing subprocess.
    child.on('close', (code, signal) => {
      if (signal !== null) {
        collectCrashReports(startedAt).then((reports) => {
          reject(new Error(`Subprocess exited with signal ${signal}. Output:\n${out}${reports}`));
        }, reject);
        return;
      }
      resolve({
        code: code!,
        out
      });
    });
  });
}

export function signApp(appPath: string, identity: string) {
  return spawn('codesign', ['-s', identity, '--deep', '--force', appPath]);
}

export function unsignApp(appPath: string) {
  return spawn('codesign', ['--remove-signature', '--deep', appPath]);
}
