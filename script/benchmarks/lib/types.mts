// Shapes of the files fetch-targets.mts, run.mts and compare.mts hand to each
// other.

// Features the harness app reports for one Electron version (see
// harness-app/main.js --cb-capabilities).
export type Capabilities = Partial<Record<string, boolean | string>>;

// One build to benchmark, as written to targets.json.
export interface Target {
  id: string;
  electronVersion: string;
  chromiumVersion: string;
  binary: string;
  driver: string;
  browserConfig?: string;
}

export interface TargetsFile {
  platform: string;
  arch: string;
  targets: Target[];
}

// The part of a target recorded in metadata.json.
export interface TargetInfo {
  id: string;
  electronVersion: string;
  chromiumVersion: string;
  capabilities?: Capabilities;
}

// One crossbench run of one suite against one target.
export interface Invocation {
  round: number;
  suite: string;
  target: string;
  exitCode: number;
  // Values each measure should have. Older metadata only has `repeat`.
  repetitions?: number;
  seconds?: number;
  dir: string;
}

// metadata.json, written by run.mts.
export interface RunMetadata {
  startedAt: string;
  finishedAt?: string;
  stoppedBy?: string;
  platform: string;
  arch: string;
  cpuModel: string;
  cpuCount: number;
  memoryGiB?: number;
  kernel?: string;
  runnerName: string;
  vmSize?: string;
  crossbenchRevision: string;
  rounds: number;
  repeat: number;
  suites?: string[];
  fileCount?: number;
  targets: TargetInfo[];
  invocations: Invocation[];
}
