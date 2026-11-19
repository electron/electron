// Resolves the versions of a version-ladder (history) run: the latest stable
// release of every requested Electron major, and the anchor version every
// major is compared against (see README.md). Used by the Benchmarks History
// workflow to build its job matrix.
//
// Writes `matrix` (a JSON list of { major, version }) and `anchor` to
// $GITHUB_OUTPUT when set, and prints them.
//
// Usage:
//   node script/benchmarks/resolve-history.mts --majors 22-latest
//     [--anchor <version>|latest-stable]
//
// --anchor defaults to DEFAULT_ANCHOR, pinned so that history runs made at
// different times are compared against the same version.
//
// --majors is a comma-separated list of majors and ranges, e.g. "30-45",
// "22,28,33-latest". "latest" is the newest major with a stable release.
// Majors without a stable release are left out with a warning.
import { ElectronVersions, type Versions } from '@electron/fiddle-core';

import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

export interface HistoryJob {
  major: number;
  version: string;
}

// The newest stable release (no prerelease tag) of one major, if any.
export function latestStableInMajor(versions: Versions, major: number): string | undefined {
  return versions
    .inMajor(major)
    .filter((ver) => ver.prerelease.length === 0)
    .at(-1)?.version;
}

export function latestStableMajor(versions: Versions): number {
  const latest = versions.latestStable;
  if (!latest) throw new Error('No stable release found in releases.json');
  return latest.major;
}

// Expands "22-25,30,33-latest" into a sorted list of unique majors.
export function parseMajors(spec: string, latestMajor: number): number[] {
  const majors = new Set<number>();
  const parse = (value: string) => {
    const trimmed = value.trim();
    if (trimmed === 'latest') return latestMajor;
    if (!/^\d+$/.test(trimmed)) throw new Error(`Invalid major ${JSON.stringify(value)} in ${JSON.stringify(spec)}`);
    return Number(trimmed);
  };
  for (const part of spec.split(',')) {
    if (!part.trim()) continue;
    const [from, to, ...rest] = part.split('-');
    if (rest.length) throw new Error(`Invalid range ${JSON.stringify(part)}`);
    const start = parse(from);
    const end = to === undefined ? start : parse(to);
    if (end < start) throw new Error(`Invalid range ${JSON.stringify(part)}: ${end} is before ${start}`);
    for (let major = start; major <= end; major++) majors.add(major);
  }
  if (majors.size === 0) throw new Error(`No majors in ${JSON.stringify(spec)}`);
  return [...majors].sort((a, b) => a - b);
}

// The anchor every history run uses unless told otherwise. Don't change it:
// ratios are only comparable between runs with the same anchor.
export const DEFAULT_ANCHOR = '45.3.0';

export function resolveHistory(
  versions: Versions,
  { majors: spec, anchor = DEFAULT_ANCHOR }: { majors: string; anchor?: string }
): { anchor: string; jobs: HistoryJob[]; missing: number[] } {
  const latestMajor = latestStableMajor(versions);
  // An empty anchor (a blanked workflow input) means the default.
  let anchorVersion = anchor || DEFAULT_ANCHOR;
  if (anchorVersion === 'latest-stable') {
    anchorVersion = versions.latestStable!.version;
  } else if (!versions.isVersion(anchorVersion)) {
    throw new Error(`Unknown anchor version ${anchorVersion}`);
  }
  const jobs: HistoryJob[] = [];
  const missing: number[] = [];
  for (const major of parseMajors(spec, latestMajor)) {
    const version = latestStableInMajor(versions, major);
    if (version) jobs.push({ major, version });
    else missing.push(major);
  }
  if (jobs.length === 0) throw new Error(`None of the majors in ${JSON.stringify(spec)} has a stable release`);
  return { anchor: anchorVersion, jobs, missing };
}

async function main() {
  const { values: args } = parseArgs({
    options: {
      majors: { type: 'string' },
      anchor: { type: 'string', default: DEFAULT_ANCHOR }
    }
  });
  if (!args.majors) throw new Error('--majors is required');
  if (!/^[\w.-]*$/.test(args.anchor)) throw new Error(`Invalid anchor: ${JSON.stringify(args.anchor)}`);
  const cacheDir = fs.mkdtempSync(path.join(os.tmpdir(), 'benchmarks-history-'));
  const versions = await ElectronVersions.create({
    paths: { versionsCache: path.join(cacheDir, 'releases.json') },
    ignoreCache: true
  });
  const { anchor, jobs, missing } = resolveHistory(versions, { majors: args.majors, anchor: args.anchor });
  if (missing.length) {
    const prefix = process.env.GITHUB_ACTIONS ? '::warning title=Benchmarks history::' : 'WARNING: ';
    console.log(`${prefix}no stable release for Electron ${missing.join(', ')}; leaving them out`);
  }
  console.log(`[resolve-history] anchor: ${anchor}`);
  for (const { major, version } of jobs) console.log(`[resolve-history] ${major}: ${version}`);
  if (process.env.GITHUB_OUTPUT) {
    fs.appendFileSync(process.env.GITHUB_OUTPUT, `anchor=${anchor}\nmatrix=${JSON.stringify(jobs)}\n`);
  }
}

if (process.argv[1] && fs.realpathSync(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch((error: unknown) => {
    console.error(`ERROR: ${error instanceof Error ? error.message : String(error)}`);
    process.exit(1);
  });
}
