// Resolves, downloads and verifies the builds a benchmark run compares, and
// writes one crossbench browser config per target plus targets.json.
//
// Targets:
//   candidate  Electron build under test (default: the latest nightly)
//   baseline   Electron build it is compared against (default: the nightly
//              before the candidate, or the previous release on the
//              candidate's major line)
//
// Electron versions and their Chromium versions come from @electron/fiddle-core
// (releases.electronjs.org), and the Electron and chromedriver zips are
// downloaded and checked against the release's SHASUMS256.txt by
// @electron/fiddle-core and @electron/get. Builds are for the host platform
// and architecture.
//
// Usage:
//   node script/benchmarks/fetch-targets.mts --out <dir>
//     [--candidate <version>] [--baseline <version>]
//     [--max-candidate-age-hours <n>]
//
// Versions are Electron versions without the leading "v", e.g. 45.1.0,
// 46.0.0-beta.2 or 47.0.0-nightly.20261028.
import {
  BaseVersions,
  compareVersions,
  ElectronVersions,
  Installer,
  SemVer,
  type Versions
} from '@electron/fiddle-core';
import { downloadArtifact } from '@electron/get';

import { execFileSync } from 'node:child_process';
import * as fs from 'node:fs';
import * as path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

import type { Target, TargetsFile } from './lib/types.mts';

const isNightly = (version: string) => version.includes('-nightly.');

const errorMessage = (error: unknown) => (error instanceof Error ? error.message : String(error));

function unzip(zip: string, dir: string) {
  fs.rmSync(dir, { recursive: true, force: true });
  fs.mkdirSync(dir, { recursive: true });
  execFileSync('unzip', ['-q', '-o', zip, '-d', dir], { stdio: 'inherit' });
}

// Nightlies sorted oldest to newest. fiddle-core sorts in branch order, and
// within one major line nightlies come before the line's other prereleases.
const nightliesOf = (versions: Versions) => versions.versions.filter((ver) => ver.prerelease[0] === 'nightly');

export function latestNightly(versions: Versions): string {
  const latest = nightliesOf(versions).at(-1);
  if (!latest) throw new Error('No nightlies found in releases.json');
  return latest.version;
}

// The nightly published before `version`. Also works for a nightly newer than
// the release list.
export function previousNightly(versions: Versions, version: string): string {
  const candidate = new SemVer(version);
  const older = nightliesOf(versions).filter((ver) => compareVersions(ver, candidate) < 0);
  if (!older.length) throw new Error(`Cannot find the nightly before ${version}`);
  return older[older.length - 1].version;
}

// The newest non-nightly release on the candidate's major line that is older
// than it by semver, so both run the same Chromium milestone. A stable X.0.0
// is compared against the last X.0.0 prerelease; the first prerelease of a
// major line has no default baseline.
export function previousRelease(versions: Versions, version: string): string {
  const candidate = new SemVer(version);
  const older = versions
    .inMajor(candidate.major)
    .filter((ver) => !isNightly(ver.version) && compareVersions(ver, candidate) < 0);
  if (!older.length) {
    throw new Error(`No ${candidate.major}.x release before ${version}; pass a baseline version explicitly`);
  }
  return older[older.length - 1].version;
}

// Hours since the start (UTC) of the day `version` was released. Nightlies are
// dated by day only, so a nightly from today is always less than 24 hours old.
export function releaseAgeHours(versions: Versions, version: string, now = Date.now()): number | undefined {
  const date = versions.getReleaseInfo(version)?.date;
  if (!date) return undefined;
  return (now - Date.parse(`${date}T00:00:00Z`)) / 3_600_000;
}

interface ResolveArgs {
  candidate?: string;
  baseline?: string;
  maxCandidateAgeHours?: number;
}

export function resolveVersions(versions: Versions, args: ResolveArgs, now = Date.now()) {
  let { candidate, baseline } = args;
  if (!candidate || candidate === 'latest-nightly') {
    candidate = latestNightly(versions);
    const age = releaseAgeHours(versions, candidate, now);
    console.log(`[fetch-targets] latest nightly: ${candidate} (released ${versions.getReleaseInfo(candidate)?.date})`);
    if (args.maxCandidateAgeHours !== undefined && age !== undefined && age > args.maxCandidateAgeHours) {
      return { skip: `the latest nightly is ${Math.round(age)} hours old` };
    }
  }
  if (!baseline || baseline === 'previous') {
    baseline = isNightly(candidate) ? previousNightly(versions, candidate) : previousRelease(versions, candidate);
  }
  return { candidate, baseline };
}

async function loadVersions(cacheDir: string, needed: boolean): Promise<Versions> {
  try {
    return await ElectronVersions.create({
      paths: { versionsCache: path.join(cacheDir, 'releases.json') },
      ignoreCache: true
    });
  } catch (error) {
    if (needed) throw error;
    // Both versions were given. The Chromium versions can still be read from
    // chromedriver.
    console.warn(`[fetch-targets] could not read releases.json (${errorMessage(error)})`);
    return new BaseVersions([]);
  }
}

async function fetchElectron(id: string, version: string, versions: Versions, outDir: string): Promise<Target> {
  const dir = path.join(outDir, id);
  fs.rmSync(dir, { recursive: true, force: true });
  // One Installer per target: an Installer keeps a single installed version.
  const installer = new Installer({
    electronDownloads: path.join(outDir, 'downloads'),
    electronInstall: path.join(dir, 'electron')
  });
  const binary = await installer.install(version);
  console.log(`[fetch-targets] installed Electron ${version}`);

  const driverZip = await downloadArtifact({
    version,
    artifactName: 'chromedriver',
    platform: process.platform,
    arch: process.arch
  });
  console.log(`[fetch-targets] verified chromedriver ${version}`);
  unzip(driverZip, path.join(dir, 'chromedriver'));
  const driver = path.join(dir, 'chromedriver', `chromedriver${process.platform === 'win32' ? '.exe' : ''}`);

  let chromiumVersion = versions.getReleaseInfo(version)?.chrome;
  if (!chromiumVersion) {
    // A version that releases.json doesn't list yet. Electron's chromedriver
    // is built from the same Chromium, and reports its version.
    const output = execFileSync(driver, ['--version'], { encoding: 'utf8' });
    chromiumVersion = output.match(/ChromeDriver (\d+\.\d+\.\d+\.\d+)/)?.[1];
    if (!chromiumVersion) throw new Error(`Cannot determine the Chromium version of ${version}: ${output}`);
  }
  return { id, electronVersion: version, chromiumVersion, binary, driver };
}

// crossbench cannot detect Electron from its path or parse `electron
// --version`, so the type and Chromium version are given explicitly, the
// driver is Electron's own chromedriver, and the harness app is passed with
// --app. Runs must use --headless (see README.md).
export function browserConfig(target: Target, harnessApp: string) {
  return {
    browsers: {
      [target.id]: {
        path: target.binary,
        type: 'chromium',
        version: target.chromiumVersion,
        driver: target.driver,
        flags: ['--no-sandbox', `--app=${harnessApp}`]
      }
    }
  };
}

async function main() {
  const { values: args } = parseArgs({
    options: {
      out: { type: 'string' },
      candidate: { type: 'string' },
      baseline: { type: 'string' },
      'harness-app': { type: 'string', default: path.join(import.meta.dirname, 'harness-app') },
      // Scheduled runs skip days without a new nightly.
      'max-candidate-age-hours': { type: 'string' }
    }
  });
  if (!args.out) throw new Error('--out is required');
  for (const value of [args.candidate, args.baseline]) {
    if (value && !/^[\w.-]+$/.test(value)) throw new Error(`Invalid version: ${JSON.stringify(value)}`);
  }

  const outDir = path.resolve(args.out);
  fs.mkdirSync(outDir, { recursive: true });
  const { platform, arch } = process;
  const explicit = (value?: string) => !!value && value !== 'latest-nightly' && value !== 'previous';
  const versions = await loadVersions(outDir, !(explicit(args.candidate) && explicit(args.baseline)));
  const resolved = resolveVersions(versions, {
    candidate: args.candidate,
    baseline: args.baseline,
    maxCandidateAgeHours:
      args['max-candidate-age-hours'] === undefined ? undefined : parseFloat(args['max-candidate-age-hours'])
  });
  if ('skip' in resolved) {
    console.log(`[fetch-targets] nothing to do: ${resolved.skip}`);
    if (process.env.GITHUB_OUTPUT) fs.appendFileSync(process.env.GITHUB_OUTPUT, 'skip=true\n');
    return;
  }
  const { candidate, baseline } = resolved;
  console.log(`[fetch-targets] candidate ${candidate}, baseline ${baseline}`);

  const targets: Target[] = [];
  for (const [id, version] of [
    ['candidate', candidate],
    ['baseline', baseline]
  ]) {
    targets.push(await fetchElectron(id, version, versions, outDir));
  }
  fs.rmSync(path.join(outDir, 'downloads'), { recursive: true, force: true });

  const harnessApp = path.resolve(args['harness-app']);
  for (const target of targets) {
    target.browserConfig = path.join(outDir, `${target.id}.browser.json`);
    fs.writeFileSync(target.browserConfig, JSON.stringify(browserConfig(target, harnessApp), null, 2));
  }
  const targetsFile: TargetsFile = { platform, arch, targets };
  fs.writeFileSync(path.join(outDir, 'targets.json'), JSON.stringify(targetsFile, null, 2));
  for (const target of targets) {
    console.log(`[fetch-targets] ${target.id}: ${target.electronVersion} (Chromium ${target.chromiumVersion})`);
  }
}

if (process.argv[1] && fs.realpathSync(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch((error: unknown) => {
    console.error(`ERROR: ${errorMessage(error)}`);
    process.exit(1);
  });
}
