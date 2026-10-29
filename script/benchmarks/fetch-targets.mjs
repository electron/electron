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
// Usage:
//   node script/benchmarks/fetch-targets.mjs --out <dir>
//     [--candidate <version>] [--baseline <version>] [--cft auto|none|<version>]
//     [--platform linux] [--arch x64] [--max-candidate-age-hours <n>]
//
// Versions are Electron versions without the leading "v", e.g. 45.1.0,
// 46.0.0-beta.2 or 47.0.0-nightly.20261028. Set GITHUB_TOKEN to raise the
// GitHub API rate limit when resolving nightlies.
import { execFileSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import * as fs from 'node:fs';
import * as path from 'node:path';
import { Readable } from 'node:stream';
import { pipeline } from 'node:stream/promises';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

const RELEASES_JSON = 'https://releases.electronjs.org/releases.json';
const NIGHTLIES_API = 'https://api.github.com/repos/electron/nightlies/releases';
const NIGHTLIES_PER_PAGE = 100;
// 20 pages of 100 is several years of nightlies.
const NIGHTLIES_MAX_PAGES = 20;
const CFT_JSON = 'https://googlechromelabs.github.io/chrome-for-testing/known-good-versions-with-downloads.json';
const CFT_BUCKET = 'https://storage.googleapis.com/chrome-for-testing-public';

// Per-platform naming. Only linux is wired into the workflow so far.
const PLATFORMS = {
  linux: { electron: 'linux', electronBinary: 'electron', chromeBinary: 'chrome', exe: '' },
  darwin: {
    electron: 'darwin',
    electronBinary: 'Electron.app/Contents/MacOS/Electron',
    chromeBinary: 'Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing',
    exe: ''
  },
  win32: { electron: 'win32', electronBinary: 'electron.exe', chromeBinary: 'chrome.exe', exe: '.exe' }
};

function cftPlatform(platform, arch) {
  if (platform === 'linux') return 'linux64';
  if (platform === 'darwin') return arch === 'arm64' ? 'mac-arm64' : 'mac-x64';
  return arch === 'ia32' ? 'win32' : 'win64';
}

const isNightly = (version) => version.includes('-nightly.');

async function fetchOk(url, options = {}) {
  const response = await fetch(url, { ...options, signal: AbortSignal.timeout(options.timeout ?? 60_000) });
  if (!response.ok) throw new Error(`GET ${url} failed: ${response.status} ${response.statusText}`);
  return response;
}

async function fetchJson(url, headers = {}) {
  return (await fetchOk(url, { headers })).json();
}

// Downloads `url` to `dest`, returning its sha256.
async function download(url, dest) {
  console.log(`[fetch-targets] downloading ${url}`);
  const response = await fetchOk(url, { timeout: 15 * 60_000 });
  const hash = createHash('sha256');
  await pipeline(
    Readable.fromWeb(response.body),
    async function* (source) {
      for await (const chunk of source) {
        hash.update(chunk);
        yield chunk;
      }
    },
    fs.createWriteStream(dest)
  );
  return hash.digest('hex');
}

function unzip(zip, dir) {
  fs.rmSync(dir, { recursive: true, force: true });
  fs.mkdirSync(dir, { recursive: true });
  execFileSync('unzip', ['-q', '-o', zip, '-d', dir], { stdio: 'inherit' });
}

// Lists published nightlies, newest first. Pages through the releases API
// until `done(list)` returns true, the API runs out of releases, or `maxPages`
// pages have been read.
export async function listNightlies({ done = () => true, maxPages = NIGHTLIES_MAX_PAGES } = {}) {
  const headers = { Accept: 'application/vnd.github+json' };
  if (process.env.GITHUB_TOKEN) headers.Authorization = `Bearer ${process.env.GITHUB_TOKEN}`;
  let list = [];
  for (let page = 1; page <= maxPages; page++) {
    const releases = await fetchJson(`${NIGHTLIES_API}?per_page=${NIGHTLIES_PER_PAGE}&page=${page}`, headers);
    list = list
      .concat(
        releases
          .filter(
            (release) =>
              !release.draft && release.published_at && /^v\d+\.\d+\.\d+-nightly\.\d+$/.test(release.tag_name)
          )
          .map((release) => ({ version: release.tag_name.slice(1), publishedAt: release.published_at }))
      )
      .sort((a, b) => Date.parse(b.publishedAt) - Date.parse(a.publishedAt));
    if (releases.length < NIGHTLIES_PER_PAGE || done(list)) break;
  }
  return list;
}

// Returns the nightly published before `version`, paging back as far as needed.
export async function previousNightly(version) {
  const hasPredecessor = (list) => {
    const index = list.findIndex((nightly) => nightly.version === version);
    return index !== -1 && index + 1 < list.length;
  };
  const list = await listNightlies({ done: hasPredecessor });
  if (!hasPredecessor(list)) throw new Error(`Cannot find the nightly before ${version}`);
  return list[list.findIndex((nightly) => nightly.version === version) + 1].version;
}

function compareVersions(a, b) {
  const pa = a.split('.').map(Number);
  const pb = b.split('.').map(Number);
  for (let i = 0; i < Math.max(pa.length, pb.length); i++) {
    if ((pa[i] ?? 0) !== (pb[i] ?? 0)) return (pa[i] ?? 0) - (pb[i] ?? 0);
  }
  return 0;
}

function parseSemver(version) {
  const match = version.match(/^(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z.-]+))?$/);
  if (!match) return null;
  return { major: Number(match[1]), core: match.slice(1, 4).map(Number), prerelease: match[4]?.split('.') ?? [] };
}

// Semver precedence: 45.0.0-alpha.1 < 45.0.0-beta.2 < 45.0.0-beta.10 < 45.0.0.
export function compareSemver(a, b) {
  const pa = parseSemver(a);
  const pb = parseSemver(b);
  if (!pa || !pb) throw new Error(`Cannot compare ${a} and ${b}`);
  for (let i = 0; i < 3; i++) {
    if (pa.core[i] !== pb.core[i]) return pa.core[i] - pb.core[i];
  }
  // A release ranks above its prereleases.
  if (!pa.prerelease.length || !pb.prerelease.length) return pb.prerelease.length - pa.prerelease.length;
  for (let i = 0; i < Math.max(pa.prerelease.length, pb.prerelease.length); i++) {
    const x = pa.prerelease[i];
    const y = pb.prerelease[i];
    if (x === undefined || y === undefined) return x === undefined ? -1 : 1;
    if (x === y) continue;
    const xNumeric = /^\d+$/.test(x);
    const yNumeric = /^\d+$/.test(y);
    if (xNumeric && yNumeric) return Number(x) - Number(y);
    if (xNumeric !== yNumeric) return xNumeric ? -1 : 1;
    return x < y ? -1 : 1;
  }
  return 0;
}

// The newest non-nightly release on the candidate's major line that is older
// than it by semver, so both run the same Chromium milestone. A stable
// X.0.0 is compared against the last X.0.0 prerelease; the first prerelease
// of a major line has no default baseline. releases.json is ordered by date,
// and several lines ship in the same week, so its order cannot be used.
export function previousRelease(releases, version) {
  const candidate = parseSemver(version);
  if (!candidate) throw new Error(`Cannot parse version ${version}`);
  const older = releases
    .map((release) => release.version)
    .filter((v) => !isNightly(v) && parseSemver(v)?.major === candidate.major && compareSemver(v, version) < 0)
    .sort(compareSemver);
  if (!older.length) {
    throw new Error(
      `No ${candidate.major}.x release before ${version} in releases.json; pass a baseline version explicitly`
    );
  }
  return older[older.length - 1];
}

async function resolveVersions(args) {
  let releases = [];
  try {
    releases = await fetchJson(RELEASES_JSON);
  } catch (error) {
    console.warn(`[fetch-targets] could not read releases.json (${error.message})`);
  }

  let candidate = args.candidate;
  let baseline = args.baseline;
  if (!candidate || candidate === 'latest-nightly') {
    const [latest] = await listNightlies();
    if (!latest) throw new Error('No published nightlies found');
    candidate = latest.version;
    console.log(`[fetch-targets] latest nightly: ${candidate} (published ${latest.publishedAt})`);
    const ageHours = (Date.now() - Date.parse(latest.publishedAt)) / 3_600_000;
    if (args['max-candidate-age-hours'] && ageHours > parseFloat(args['max-candidate-age-hours'])) {
      return { skip: `the latest nightly is ${Math.round(ageHours)} hours old` };
    }
  }
  if (!baseline || baseline === 'previous') {
    if (isNightly(candidate)) {
      baseline = await previousNightly(candidate);
    } else {
      baseline = previousRelease(releases, candidate);
    }
  }
  return { candidate, baseline, releases };
}

async function fetchElectron({ id, version, releases, platform, arch, outDir }) {
  const names = PLATFORMS[platform];
  const repo = isNightly(version) ? 'electron/nightlies' : 'electron/electron';
  const base = `https://github.com/${repo}/releases/download/v${version}`;
  const dir = path.join(outDir, id);
  fs.mkdirSync(dir, { recursive: true });

  const shasumsPath = path.join(dir, 'SHASUMS256.txt');
  await download(`${base}/SHASUMS256.txt`, shasumsPath);
  const expected = new Map(
    fs
      .readFileSync(shasumsPath, 'utf8')
      .split('\n')
      .map((line) => line.trim().match(/^([0-9a-f]{64})\s+\*?(.+)$/))
      .filter(Boolean)
      .map(([, sha, name]) => [name, sha])
  );

  const unpacked = {};
  for (const name of ['electron', 'chromedriver']) {
    const file = `${name}-v${version}-${names.electron}-${arch}.zip`;
    const zip = path.join(dir, file);
    const actual = await download(`${base}/${file}`, zip);
    if (!expected.has(file)) throw new Error(`${file} is not listed in SHASUMS256.txt`);
    if (expected.get(file) !== actual) {
      throw new Error(`Checksum mismatch for ${file}: ${actual} != ${expected.get(file)}`);
    }
    console.log(`[fetch-targets] verified ${file}`);
    unpacked[name] = path.join(dir, name);
    unzip(zip, unpacked[name]);
    fs.rmSync(zip);
  }

  const binary = path.join(unpacked.electron, names.electronBinary);
  const driver = path.join(unpacked.chromedriver, `chromedriver${names.exe}`);
  let chromiumVersion = releases.find((release) => release.version === version)?.chrome;
  if (!chromiumVersion) {
    // New nightlies can be missing from releases.json for a while. Electron's
    // chromedriver is built from the same Chromium, and reports its version.
    const output = execFileSync(driver, ['--version'], { encoding: 'utf8' });
    chromiumVersion = output.match(/ChromeDriver (\d+\.\d+\.\d+\.\d+)/)?.[1];
    if (!chromiumVersion) throw new Error(`Cannot determine the Chromium version of ${version}: ${output}`);
  }
  return { id, kind: 'electron', electronVersion: version, chromiumVersion, binary, driver };
}

// Picks the Chrome for Testing build for `chromiumVersion`: an exact match if
// one exists, otherwise the newest build with the same major version.
async function resolveCft(chromiumVersion, platformKey) {
  try {
    const { versions } = await fetchJson(CFT_JSON);
    const major = chromiumVersion.split('.')[0];
    const usable = versions.filter(
      (entry) =>
        entry.version.split('.')[0] === major &&
        entry.downloads?.chrome?.some((d) => d.platform === platformKey) &&
        entry.downloads?.chromedriver?.some((d) => d.platform === platformKey)
    );
    const match =
      usable.find((entry) => entry.version === chromiumVersion) ??
      usable.sort((a, b) => compareVersions(b.version, a.version))[0];
    if (match) return match.version;
    console.warn(`[fetch-targets] no Chrome for Testing build for milestone ${major}`);
    return null;
  } catch (error) {
    // Fall back to the exact version, which exists for most Chromium builds
    // Electron ships, straight from the bucket.
    console.warn(
      `[fetch-targets] could not read the Chrome for Testing index (${error.message}); trying ${chromiumVersion}`
    );
    const response = await fetch(`${CFT_BUCKET}/${chromiumVersion}/${platformKey}/chrome-${platformKey}.zip`, {
      headers: { Range: 'bytes=0-0' },
      signal: AbortSignal.timeout(60_000)
    });
    return response.ok ? chromiumVersion : null;
  }
}

async function fetchCft({ version, platform, arch, outDir }) {
  const key = cftPlatform(platform, arch);
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
    binary: path.join(dir, 'chrome', `chrome-${key}`, PLATFORMS[platform].chromeBinary),
    driver: path.join(dir, 'chromedriver', `chromedriver-${key}`, `chromedriver${PLATFORMS[platform].exe}`)
  };
}

// crossbench cannot detect Electron from its path or parse `electron
// --version`, so the type and Chromium version are given explicitly, the
// driver is Electron's own chromedriver, and the harness app is passed with
// --app. Runs must use --headless (see README.md).
export function browserConfig(target, harnessApp) {
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
      platform: { type: 'string', default: 'linux' },
      arch: { type: 'string', default: 'x64' },
      'harness-app': { type: 'string', default: path.join(__dirname, 'harness-app') },
      // Scheduled runs skip days without a new nightly.
      'max-candidate-age-hours': { type: 'string' }
    }
  });
  if (!args.out) throw new Error('--out is required');
  if (!PLATFORMS[args.platform]) throw new Error(`Unsupported platform ${args.platform}`);
  for (const value of [args.candidate, args.baseline, args.cft]) {
    if (value && !/^[\w.-]+$/.test(value)) throw new Error(`Invalid version: ${JSON.stringify(value)}`);
  }

  const outDir = path.resolve(args.out);
  fs.mkdirSync(outDir, { recursive: true });
  const { platform, arch } = args;
  const { candidate, baseline, releases, skip } = await resolveVersions(args);
  if (skip) {
    console.log(`[fetch-targets] nothing to do: ${skip}`);
    if (process.env.GITHUB_OUTPUT) fs.appendFileSync(process.env.GITHUB_OUTPUT, 'skip=true\n');
    return;
  }
  console.log(`[fetch-targets] candidate ${candidate}, baseline ${baseline}`);

  const targets = [];
  for (const [id, version] of [
    ['candidate', candidate],
    ['baseline', baseline]
  ]) {
    targets.push(await fetchElectron({ id, version, releases, platform, arch, outDir }));
  }

  if (args.cft !== 'none') {
    const cftVersion =
      args.cft === 'auto' ? await resolveCft(targets[0].chromiumVersion, cftPlatform(platform, arch)) : args.cft;
    if (cftVersion) {
      targets.push(await fetchCft({ version: cftVersion, platform, arch, outDir }));
    } else {
      console.warn('[fetch-targets] skipping Chrome for Testing: no matching build');
    }
  }

  const harnessApp = path.resolve(args['harness-app']);
  for (const target of targets) {
    target.browserConfig = path.join(outDir, `${target.id}.browser.json`);
    fs.writeFileSync(target.browserConfig, JSON.stringify(browserConfig(target, harnessApp), null, 2));
  }
  fs.writeFileSync(path.join(outDir, 'targets.json'), JSON.stringify({ platform, arch, targets }, null, 2));
  for (const target of targets) {
    console.log(
      `[fetch-targets] ${target.id}: ${target.electronVersion ?? 'Chrome for Testing'} (Chromium ${target.chromiumVersion})`
    );
  }
}

if (process.argv[1] && fs.realpathSync(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch((error) => {
    console.error(`ERROR: ${error.message}`);
    process.exit(1);
  });
}
