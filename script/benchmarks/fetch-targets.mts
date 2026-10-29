// Resolves, downloads and verifies the builds a benchmark run compares, and
// writes one crossbench browser config per target plus targets.json.
//
// Targets:
//   candidate  Electron build under test (default: the latest nightly)
//   baseline   Electron build it is compared against (default: the nightly
//              before the candidate, or the previous release on the
//              candidate's major line)
//   cft        Chrome for Testing at the candidate's Chromium milestone
//
// Electron versions and their Chromium versions come from @electron/fiddle-core
// (releases.electronjs.org), and the Electron and chromedriver zips are
// downloaded and checked against the release's SHASUMS256.txt by
// @electron/fiddle-core and @electron/get. Builds are for the host platform
// and architecture.
//
// Usage:
//   node script/benchmarks/fetch-targets.mts --out <dir>
//     [--candidate <version>] [--baseline <version>] [--cft auto|none|<version>]
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
import { Readable } from 'node:stream';
import { pipeline } from 'node:stream/promises';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

import type { Target, TargetsFile } from './lib/types.mts';
import type { ReadableStream as NodeReadableStream } from 'node:stream/web';

const CFT_JSON = 'https://googlechromelabs.github.io/chrome-for-testing/known-good-versions-with-downloads.json';
const CFT_BUCKET = 'https://storage.googleapis.com/chrome-for-testing-public';

interface CftPlatform {
  key: string;
  binary: string;
  exe: string;
}

function cftPlatform(platform: string, arch: string): CftPlatform {
  if (platform === 'linux') return { key: 'linux64', binary: 'chrome', exe: '' };
  if (platform === 'darwin') {
    return {
      key: arch === 'arm64' ? 'mac-arm64' : 'mac-x64',
      binary: 'Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing',
      exe: ''
    };
  }
  return { key: arch === 'ia32' ? 'win32' : 'win64', binary: 'chrome.exe', exe: '.exe' };
}

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

// Downloads `url` to `dest`.
async function download(url: string, dest: string) {
  console.log(`[fetch-targets] downloading ${url}`);
  const response = await fetch(url, { signal: AbortSignal.timeout(15 * 60_000) });
  if (!response.ok || !response.body) throw new Error(`GET ${url} failed: ${response.status} ${response.statusText}`);
  await pipeline(Readable.fromWeb(response.body as NodeReadableStream<Uint8Array>), fs.createWriteStream(dest));
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
  return { id, kind: 'electron', electronVersion: version, chromiumVersion, binary, driver };
}

const compareDotted = (a: string, b: string) => {
  const pa = a.split('.').map(Number);
  const pb = b.split('.').map(Number);
  for (let i = 0; i < Math.max(pa.length, pb.length); i++) {
    if ((pa[i] ?? 0) !== (pb[i] ?? 0)) return (pa[i] ?? 0) - (pb[i] ?? 0);
  }
  return 0;
};

interface CftEntry {
  version: string;
  downloads?: Record<string, { platform: string }[] | undefined>;
}

// Picks the Chrome for Testing build for `chromiumVersion`: an exact match if
// one exists, otherwise the newest build with the same major version.
async function resolveCft(chromiumVersion: string, key: string): Promise<string | null> {
  try {
    const response = await fetch(CFT_JSON, { signal: AbortSignal.timeout(60_000) });
    if (!response.ok) throw new Error(`GET ${CFT_JSON} failed: ${response.status} ${response.statusText}`);
    const { versions } = (await response.json()) as { versions: CftEntry[] };
    const major = chromiumVersion.split('.')[0];
    const hasDownload = (entry: CftEntry, name: string) => !!entry.downloads?.[name]?.some((d) => d.platform === key);
    const usable = versions.filter(
      (entry) =>
        entry.version.split('.')[0] === major && hasDownload(entry, 'chrome') && hasDownload(entry, 'chromedriver')
    );
    const match =
      usable.find((entry) => entry.version === chromiumVersion) ??
      usable.sort((a, b) => compareDotted(b.version, a.version))[0];
    if (match) return match.version;
    console.warn(`[fetch-targets] no Chrome for Testing build for milestone ${major}`);
    return null;
  } catch (error) {
    // Fall back to the exact version, which exists for most Chromium builds
    // Electron ships, straight from the bucket.
    console.warn(
      `[fetch-targets] could not read the Chrome for Testing index (${errorMessage(error)}); trying ${chromiumVersion}`
    );
    const response = await fetch(`${CFT_BUCKET}/${chromiumVersion}/${key}/chrome-${key}.zip`, {
      headers: { Range: 'bytes=0-0' },
      signal: AbortSignal.timeout(60_000)
    });
    return response.ok ? chromiumVersion : null;
  }
}

async function fetchCft(version: string, platform: CftPlatform, outDir: string): Promise<Target> {
  const { key } = platform;
  const dir = path.join(outDir, 'cft');
  fs.mkdirSync(dir, { recursive: true });
  // Chrome for Testing publishes no checksums; the bucket is HTTPS-only.
  for (const name of ['chrome', 'chromedriver']) {
    const zip = path.join(dir, `${name}-${key}.zip`);
    await download(`${CFT_BUCKET}/${version}/${key}/${name}-${key}.zip`, zip);
    unzip(zip, path.join(dir, name));
    fs.rmSync(zip);
  }
  return {
    id: 'cft',
    kind: 'chrome',
    chromiumVersion: version,
    binary: path.join(dir, 'chrome', `chrome-${key}`, platform.binary),
    driver: path.join(dir, 'chromedriver', `chromedriver-${key}`, `chromedriver${platform.exe}`)
  };
}

// crossbench cannot detect Electron from its path or parse `electron
// --version`, so the type and Chromium version are given explicitly, the
// driver is Electron's own chromedriver, and the harness app is passed with
// --app. Runs must use --headless (see README.md).
export function browserConfig(target: Target, harnessApp: string) {
  const flags = ['--no-sandbox'];
  if (target.kind === 'electron') flags.push(`--app=${harnessApp}`);
  return {
    browsers: {
      [target.id]: {
        path: target.binary,
        type: target.kind === 'electron' ? 'chromium' : 'chrome',
        version: target.chromiumVersion,
        driver: target.driver,
        flags
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
      cft: { type: 'string', default: 'auto' },
      'harness-app': { type: 'string', default: path.join(import.meta.dirname, 'harness-app') },
      // Scheduled runs skip days without a new nightly.
      'max-candidate-age-hours': { type: 'string' }
    }
  });
  if (!args.out) throw new Error('--out is required');
  for (const value of [args.candidate, args.baseline, args.cft]) {
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

  if (args.cft !== 'none') {
    const cft = cftPlatform(platform, arch);
    const cftVersion = args.cft === 'auto' ? await resolveCft(targets[0].chromiumVersion, cft.key) : args.cft;
    if (cftVersion) {
      targets.push(await fetchCft(cftVersion, cft, outDir));
    } else {
      console.warn('[fetch-targets] skipping Chrome for Testing: no matching build');
    }
  }

  const harnessApp = path.resolve(args['harness-app']);
  for (const target of targets) {
    target.browserConfig = path.join(outDir, `${target.id}.browser.json`);
    fs.writeFileSync(target.browserConfig, JSON.stringify(browserConfig(target, harnessApp), null, 2));
  }
  const targetsFile: TargetsFile = { platform, arch, targets };
  fs.writeFileSync(path.join(outDir, 'targets.json'), JSON.stringify(targetsFile, null, 2));
  for (const target of targets) {
    console.log(
      `[fetch-targets] ${target.id}: ${target.electronVersion ?? 'Chrome for Testing'} (Chromium ${target.chromiumVersion})`
    );
  }
}

if (process.argv[1] && fs.realpathSync(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch((error: unknown) => {
    console.error(`ERROR: ${errorMessage(error)}`);
    process.exit(1);
  });
}
